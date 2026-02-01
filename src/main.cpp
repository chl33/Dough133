// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <Arduino.h>
#include <LittleFS.h>
#include <og3/blink_led.h>
#include <og3/constants.h>
#include <og3/din.h>
#include <og3/ha_app.h>
#include <og3/html_table.h>
#include <og3/kernel_filter.h>
#include <og3/oled_display_ring.h>
#include <og3/oled_wifi_info.h>
#include <og3/pid.h>
#include <og3/pwm.h>
#include <og3/relay.h>
#include <og3/shtc3.h>
#include <og3/units.h>
#include <og3/variable.h>
#include <og3/web.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>

#define VERSION "0.9.95"

// TODO(chrishl):
//  - Get board temperature working.
namespace og3 {

static const char kManufacturer[] = "Chris Lee";
static const char kModel[] = "Doughl33";
static const char kSoftware[] = "Doughl33 v" VERSION;

// Hardware config
// -- Pin assignments --
constexpr uint8_t kButtonPin = 34;
constexpr uint8_t kRelayHeaterPin = 32;
constexpr uint8_t kSafetyPWMPin = 19;
constexpr uint8_t kRelayFanPin = 33;
constexpr uint8_t kPowerLEDPin = 4;
constexpr uint8_t kSda2 = 23;
constexpr uint8_t kScl2 = 25;

// State machine config.
constexpr int kUpdateOnMsec = 1 * kMsecInSec;
constexpr int kUpdateOffMsec = 10 * kMsecInSec;
constexpr int kHeaterCooldownMsec = 90 * kMsecInSec;
constexpr double kHeaterPwmFrequency = 100;  // Pwm::kAnalogWriteFreqMin;  // 100Hz.
constexpr double kSafetyPwmFrequency = 200;
// Time to turn on relay from web button press.
constexpr int kFanOnMsec = 60 * kMsecInSec;

constexpr float kDefaultTargetTemp = 27.0f;
constexpr float kDefaultMinValidTemp = 10.0f;
constexpr float kDefaultMaxValidTemp = 40.0f;
constexpr float kDefaultCtlP = 0.25f;
constexpr float kDefaultCtlI = 0.001f;
constexpr float kDefaultCtlD = 5.0f;
constexpr float kDefaultCtlIMin = -0.15f;
constexpr float kDefaultCtlIMax = 0.15f;
constexpr float kDefaultCtlFFPerDeltaC = 0.01f;
constexpr float kDefaultRampRate = 0.05f;  // °C/sec
constexpr float kDefaultFFPerRate = 0.0f;  // pwm / (°C/sec)
constexpr float kTargetTempMax = 35.0f;
constexpr float kTargetTempMin = 15.0f;

constexpr uint8_t kPwmChannel = 0;
constexpr uint8_t kSafetyPwmChannel = 1;
constexpr uint8_t kPwmResolution = 16;

// Delay between updates of the OLED.
constexpr unsigned kOledSwitchMsec = 5000;

static const char kEnclosureTemperature[] = "enclosure_temp";
static const char kRoomTemperature[] = "room_temp";
static const char kFilteredTemperature[] = "filt_temp";
static const char kFilteredDTemperature[] = "filt_d_temp";
static const char kTargetTemperature[] = "target_temp";
static const char kEnclosureHumidity[] = "enclosure_humidity";
static const char kRoomHumidity[] = "room_humidity";
static const char kHeater[] = "heater";
static const char kFan[] = "fan";
static const char kHeaterState[] = "heater_state";
static const char kHeaterError[] = "heater_error";
static const char kSafetyPWM[] = "safety_pwm";
static const char kCommandP[] = "cmd_p";
static const char kCommandD[] = "cmd_d";
static const char kCommandI[] = "cmd_i";
static const char kCommandFF[] = "cmd_ff";

static const char kHtrMode[] = "htr_mode";
static const char kFanMode[] = "fan_mode";
static const char kOff[] = "off";
static const char kHeat[] = "heat";
static const char kHigh[] = "high";

#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
constexpr App::LogType kLogType = App::LogType::kUdp;
#else
constexpr App::LogType kLogType = App::LogType::kSerial;  // kSerial
#endif

HAApp s_app(HAApp::Options(kManufacturer, kModel,
                           WifiApp::Options()
                               .withSoftwareName(kSoftware)
                               .withDefaultDeviceName("doughl33")
#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
                               .withUdpLogHost(IPAddress(LOG_UDP_ADDRESS))

#endif
                               .withOta(OtaManager::Options(OTA_PASSWORD))
                               .withApp(App::Options().withLogType(kLogType))));

// Have oled display IP address or AP status.
OledWifiInfo wifi_infof(&s_app.tasks());

VariableGroup s_vg("dough");
VariableGroup s_cvg("dough_cfg");
VariableGroup s_cmdvg("dough_cmd");

DIn s_button_reader("power_button", &s_app.module_system(), kButtonPin, "power button", s_vg);

// void onConfigLoad();

const float kCommandMax = 1.0f;
const float kCommandMin = 0.0f;
const float kIMax = kDefaultCtlIMax;
const float kFeedforward = 0.0f;
const float kIMin = kDefaultCtlIMin;

PID s_pid(PID::Gains(kDefaultCtlP, kDefaultCtlI, kDefaultCtlD)
              .withCommandMax(kCommandMax)
              .withCommandMin(kCommandMin)
              .withIMax(kDefaultCtlIMax)
              .withIMin(kDefaultCtlIMin),
          s_vg, s_cvg, s_cmdvg);

Shtc3 s_shtc3_enclosure(kEnclosureTemperature, kEnclosureHumidity, &s_app.module_system(),
                        "enclosure temperature", s_vg, true, true);
Shtc3 s_shtc3_room(kRoomTemperature, kRoomHumidity, &s_app.module_system(), "room temperature",
                   s_vg, true, true, &Wire1);

Relay s_relay_fan(kFan, &s_app.tasks(), kRelayFanPin, "fan", true, s_vg);

// PWM to regulate heater power
Pwm s_pwm_heater(kHeater, kRelayHeaterPin, kPwmChannel, kPwmResolution, &s_app.module_system(),
                 kHeaterPwmFrequency);
// PWM to enable the heater.  If this stops, the safety circuit will stop the heater signal.
Pwm s_pwm_safety(kSafetyPWM, kSafetyPWMPin, kSafetyPwmChannel, kPwmResolution,
                 &s_app.module_system(), kSafetyPwmFrequency);

// Control of the power/mode LED.
BlinkLed s_blink("power", kPowerLEDPin, &s_app, 500, false /*on-low*/);

void heaterOn(float duty) {
  s_pwm_heater.setDutyF(duty);  // Set the heater power level via PWM ratio.
  s_pwm_safety.setDutyF(0.5);   // This PWM signal allows heater power to pass to the MOSFET.
}

void heaterOff() {
  s_pwm_heater.setDutyF(0.0f);  // Turn off the heater power.
  s_pwm_safety.setDutyF(0.0f);  // Disable the safety PWM signal.
}

KernelFilter s_temp_filter(
    {
        .name = kFilteredTemperature,
        .units = units::kCelsius,
        .description = "filtered enclosure temperature",
        .var_flags = 0,
        .sigma = 20.0,
        .decimals = 2,
        .size = 20,
    },
    &s_app.module_system(), s_vg);
KernelFilter s_d_temp_filter(
    {
        .name = kFilteredDTemperature,
        .units = "°C/sec",
        .description = "filtered enclosure temperature change",
        .var_flags = 0,
        .sigma = 15.0,
        .decimals = 2,
        .size = 15,
    },
    &s_app.module_system(), s_vg);

OledDisplayRing s_oled(&s_app.module_system(), "DoughL33", kOledSwitchMsec, Oled::kTenPt);

class TempControl : public Module {
 public:
  enum State {
    kStateDisabled,  // no heating
    kStateEnabled,   // heater control is enabled
    kStateCooldown,  // run fan after heating to cooldown
    kStateError,     // a problem was detected.
  };

  static const char* state_names[];

  static constexpr float kUninitializedTemp = -100.0f;
  static constexpr unsigned kCfgFlag = (VariableBase::kSettable | VariableBase::kConfig);
  static constexpr unsigned kNoFlag = 0;

  TempControl()
      : Module("temp_ctl", &s_app.module_system()),
        m_scheduler(&s_app.tasks()),
        m_state("state", kStateDisabled, "heater state", kStateError, state_names, kNoFlag, s_vg),
        m_temp_min_ok("temp_min_ok", kDefaultMinValidTemp, units::kCelsius, "Min valid temperature",
                      kCfgFlag, 1, s_cvg),
        m_temp_max_ok("temp_max_ok", kDefaultMaxValidTemp, units::kCelsius, "Max valid temperature",
                      kCfgFlag, 1, s_cvg),
        m_ctl_ff_per_delta_c("ctl_ff_per_delta_c", kDefaultCtlFFPerDeltaC, "pwm/deltaC",
                             "FF per deltaC", kCfgFlag, 3, s_cvg),
        m_set_temp("set_temp", kDefaultTargetTemp, units::kCelsius, "Target Temperature", kCfgFlag,
                   1, s_cmdvg),
        m_ramp_rate("ramp_rate", kDefaultRampRate, "°C/s", "Ramp Rate", kCfgFlag, 3, s_cvg),
        m_ff_per_rate("ff_per_rate", kDefaultFFPerRate, "pwm/(°C/s)", "FF per Rate", kCfgFlag, 3,
                      s_cvg),
        m_heat_mode(kHtrMode, kOff, "", "heater mode", kNoFlag, s_vg),
        m_fan_mode(kFanMode, kOff, "", "fan mode", kNoFlag, s_vg) {
    add_init_fn([this]() {
      s_oled.addDisplayFn([this]() { show_state(); });
      auto* had = &s_app.ha_discovery();
      had->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) -> bool {
        return this->haDiscovery(had, json);
      });
      had->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addEnum(json, m_state, ha::device_type::kSensor, nullptr);
      });
    });  // end of init-fn
  }

  void setTargetTemp(float target) { m_set_temp = target; }

  bool enabled() const { return m_state.value() == kStateEnabled; }

  void setEnable() {
    // Enable control from current state.
    switch (m_state.value()) {
      case kStateEnabled:
        break;
      case kStateDisabled:
      case kStateCooldown:
      case kStateError:
        // Make sure feedforward temperature will be recomputed if control is re-enabled.
        m_initial_temp = kUninitializedTemp;
        s_pid.feedforward() = 0.0f;
        // Start ramping from current temperature
        if (s_shtc3_enclosure.read()) {
          s_pid.target() = s_shtc3_enclosure.temperature();
          s_pid.d_target() = 0.0f;
        } else {
          s_pid.target() = m_set_temp.value();
          s_pid.d_target() = 0.0f;
        }
        setState(kStateEnabled, 100);
        break;
    }
  }

  void setDisable() {
    // Disable control from current state.
    switch (m_state.value()) {
      case kStateEnabled:
        setState(kStateCooldown, 100);
        break;
      case kStateDisabled:
      case kStateCooldown:
        break;
      case kStateError:
        setState(kStateDisabled, 100);
        break;
    }
  }

  void toggleEnable() {
    if (enabled()) {
      s_app.log().log("Disabling temperature control.");
      setDisable();
    } else {
      s_app.log().log("Enabling temperature control.");
      setEnable();
    }
    show_state();  // show on OLED
  }

  void delaySetEnable(bool enable) {
    if (!enable && enabled()) {
      m_scheduler.runIn(1, [this]() { setDisable(); });
    } else if (enable && !enabled()) {
      m_scheduler.runIn(1, [this]() { setEnable(); });
    }
  }

  long msecInState() const { return millis() - m_last_state_change_msec; }
  float initialTemp() const { return m_initial_temp; }

  void turnFanOff() {
    s_relay_fan.turnOff();
    m_fan_mode = kOff;
  }
  void turnFanOn() {
    s_relay_fan.turnOn();
    m_fan_mode = kHigh;
  }

  void show_state() {
    char display[80];
    const State state = m_state.value();
    s_shtc3_enclosure.read();
    if (state == kStateEnabled) {
      snprintf(display, sizeof(display), "%s\n%.1f -> %.1f", state_names[m_state.value()],
               s_shtc3_enclosure.temperature(), s_pid.target().value());
      s_oled.setFontSize(Oled::kTenPt);
    } else {
      snprintf(display, sizeof(display), "%s %.1f C", state_names[m_state.value()],
               s_shtc3_enclosure.temperature());
      s_oled.setFontSize(Oled::kSixteenPt);
    }
    s_oled.display(display);
  }

  void update() {
    if (!s_shtc3_enclosure.read() && m_state.value() != kStateDisabled) {
      s_app.log().logf("Failed to read SHTC3 enclosure sensor");
      setState(kStateError, 10 * kMsecInSec);
    }
    if (!s_shtc3_room.read()) {
      s_app.log().logf("Failed to read SHTC3 room sensor");
    }
    const long now_msec = millis();
    const float temp = s_shtc3_enclosure.temperature();
    const bool temp_ok = temp >= m_temp_min_ok.value() && temp <= m_temp_max_ok.value();
    const float now_sec = now_msec * 1e-3;

    if (!temp_ok) {
      s_app.log().logf("Temperature %.1f outside valid range %.1f-%.1f", temp,
                       m_temp_min_ok.value(), m_temp_min_ok.value());
      setState(kStateError, 10 * kMsecInSec);
    }

    // Store the enclosur temperature when control is first enabled.
    if (m_state.value() == kStateEnabled && m_initial_temp == kUninitializedTemp) {
      m_initial_temp = temp;
    }

    // Return a target d_temp of -ramp_rate or ramp_rate, unless within a degree of them
    //  target, in which case scale ramp linearly to zero when error is zero.
    auto compute_target_d_temp = [this](float goal, float current_target) {
      const float error = goal - current_target;
      const float scale = (error > 1.0    ? 1.0      // increase at full-ramp rate.
                           : error < -1.0 ? -1.0     // decrease at full-ramp rate.
                                          : error);  // increase/decrease scaled by error.
      return scale * m_ramp_rate.value();
    };

    // Ramping and Feedforward Logic
    if (m_state.value() == kStateEnabled && m_last_msec > 0) {
      const float dt = (now_msec - m_last_msec) * 1.0e-3;
      if (dt > 0.0f && dt < 2.0f) {  // Sanity check on dt
        const float current_target = s_pid.target().value();
        const float target_d_temp = compute_target_d_temp(m_set_temp.value(), current_target);
        const float next_target = current_target + target_d_temp * dt;
        s_pid.target() = next_target;
        s_pid.d_target() = target_d_temp;

        // Calculate Feedforward
        // 1. Dynamic FF: Power required to change temperature (Heat Capacity)
        const float dynamic_ff = target_d_temp * m_ff_per_rate.value();

        // 2. Static FF: Power required to maintain delta T (Insulation Loss)
        const float static_ff = (next_target - m_initial_temp) * m_ctl_ff_per_delta_c.value();

        s_pid.feedforward() = static_ff + dynamic_ff;
      }
    }

    // Track filtered temperature and temperature derivatives.
    if (temp_ok) {
      s_temp_filter.addSample(now_sec, temp);
      float filt_d_temp = 0.0f;
      if (m_last_temp != 0.0f) {
        const float delta_temp = temp - m_last_temp;
        const float delta_time = (now_msec - m_last_msec) * 1.0e-3;
        const float dtemp = delta_temp / delta_time;
        filt_d_temp = s_d_temp_filter.addSample(now_sec, dtemp);
      }
      m_last_temp = temp;
      m_last_msec = now_msec;
    }

    switch (m_state.value()) {
      case kStateDisabled:
        heaterOff();
        turnFanOff();
        sameState(kUpdateOffMsec);
        break;
      case kStateCooldown:
      case kStateError:
        heaterOff();
        if (msecInState() < kHeaterCooldownMsec) {
          // If error or cooldown, keep fan on while cooling.
          turnFanOn();
          sameState(kUpdateOffMsec);
        } else {
          turnFanOff();
          if (m_state.value() == kStateCooldown) {
            setState(kStateDisabled, kUpdateOffMsec);
          } else {
            sameState(kUpdateOffMsec);
          }
        }
        break;
      case kStateEnabled: {
        const float cmd = s_pid.command(temp, filt_d_temp, now_msec);
        heaterOn(cmd);
        turnFanOn();
        sameState(kUpdateOnMsec);
        break;
      }
    }

    s_app.mqttSend(s_vg);
    // Send config variables unless marked kNoPublish.
    s_app.mqttSend(s_cvg, VariableBase::kNoPublish | VariableBase::kConfig);
    s_app.mqttSend(s_cmdvg, VariableBase::kConfig);
  }

  void writeHtmlStatusTable(String* out) {
    html::writeTableStart(out, "Status");
    html::writeRowInto(out, s_pid.target());
    html::writeRowInto(out, m_heat_mode);
    html::writeRowInto(out, m_fan_mode);
    html::writeRowInto(out, s_shtc3_enclosure.temperatureVar());
    html::writeRowInto(out, s_shtc3_enclosure.humidityVar());
    html::writeRowInto(out, s_shtc3_room.temperatureVar());
    html::writeRowInto(out, s_shtc3_room.humidityVar());
    html::writeTableEnd(out);
  }

 protected:
  void setState(State state, unsigned msec) {
    if (m_state.value() != state) {
      s_app.log().logf("state %u -> %u.", static_cast<unsigned>(m_state.value()),
                       static_cast<unsigned>(state));
      m_state = state;
      m_last_state_change_msec = millis();
      s_pid.initialize();
      m_heat_mode = enabled() ? kHeat : kOff;
    }
    m_scheduler.runIn(1, [this]() { update(); });
    // Internal LED follows enable/disable state.
    if (enabled()) {
      s_blink.on();
    } else {
      s_blink.off();
    }
  }
  void sameState(unsigned msec) {
    m_scheduler.runIn(msec, [this]() { update(); });
  }

  void mqttSetMode(const char* topic, const char* payload, size_t len) {
    if (0 == strncmp(payload, kOff, len)) {
      delaySetEnable(false);
    } else if (0 == strncmp(payload, kHeat, len)) {
      delaySetEnable(true);
    } else {
      s_app.log().logf("setMode('%s', (%d)'%s') unknown mode", topic, static_cast<int>(len),
                       payload);
    }
  }
  void mqttSetFanMode(const char* topic, const char* payload, size_t len) {
    if (0 == strncmp(payload, kOff, len)) {
      turnFanOff();
    } else if (0 == strncmp(payload, kHigh, len)) {
      turnFanOn();
    } else {
      s_app.log().logf("setMode('%s', (%d)'%s') unknown mode", topic, static_cast<int>(len),
                       payload);
    }
  }
  void mqttSetTargetTemp(const char* topic, const char* payload, size_t len) {
    float temp = 0.0f;
    if (1 != sscanf(payload, "%f", &temp)) {
      s_app.log().logf("setTargetTemp('%s', (%d)'%s') failed to parse payload temp", topic,
                       static_cast<int>(len), payload);
    } else if (temp > kTargetTempMax) {
      s_app.log().logf("setTargetTemp('%s', %g) target too high", topic, temp);
    } else if (temp < kTargetTempMin) {
      s_app.log().logf("setTargetTemp('%s', %g) target too low", topic, temp);
    } else {
      setTargetTemp(temp);
    }
  }
  bool haDiscovery(HADiscovery* had, JsonDocument* json) {
    json->clear();

    {
      // The variable is not used for addRoot() -- this just sets device informaton.
      HADiscovery::Entry entry(m_temp_min_ok, ha::device_type::kClimate, nullptr);
      had->addRoot(json, entry);
    }

    String name = "thermostat";
    auto& js = *json;
    js["name"] = name;
    js["mode_cmd_t"] = "~/mode/set";
    js["mode_stat_t"] = "~/dough";
    js["mode_stat_tpl"] = "{{value_json.htr_mode}}";  // Set state to "off", "heat";
    js["temp_cmd_t"] = "~/set_temp/set";
    js["temp_stat_t"] = "~/dough_cmd";
    js["temp_stat_tpl"] = "{{value_json.set_temp}}";
    js["temperature_unit"] = "C";
    js["fan_mode_cmd_t"] = "~/fan_mode/set";
    js["fan_mode_stat_t"] = "~/dough";
    js["fan_mode_stat_tpl"] = "{{value_json.fan_mode}}";  // Set state to "off", "high";
    js["curr_temp_t"] = "~/dough";
    js["curr_temp_tpl"] = "{{value_json.filt_temp}}";
    js["min_temp"] = kTargetTempMin;
    js["max_temp"] = kTargetTempMax;
    js["temp_step"] = 0.5;
    js["modes"][0] = kOff;
    js["modes"][1] = kHeat;
    js["fan_modes"][0] = kOff;
    js["fan_modes"][1] = kHigh;

    char value[128];
    snprintf(value, sizeof(value), "%s_%s", had->deviceId(), name.c_str());
    js["uniq_id"] = value;

    had->mqttSubscribe("mode/set", [this](const char* topic, const char* payload, size_t len) {
      this->mqttSetMode(topic, payload, len);
    });
    had->mqttSubscribe("fan_mode/set", [this](const char* topic, const char* payload, size_t len) {
      this->mqttSetFanMode(topic, payload, len);
    });
    had->mqttSubscribe("set_temp/set", [this](const char* topic, const char* payload, size_t len) {
      this->mqttSetTargetTemp(topic, payload, len);
    });

    return had->mqttSendConfig(name.c_str(), ha::device_type::kClimate, json);
  }

 private:
  TaskIdScheduler m_scheduler;
  EnumStrVariable<State> m_state;
  float m_initial_temp = kUninitializedTemp;
  float m_last_temp = 0.0f;
  unsigned long m_last_msec = 0;
  unsigned long m_last_state_change_msec = 0;

  FloatVariable m_temp_min_ok;
  FloatVariable m_temp_max_ok;
  FloatVariable m_ctl_ff_per_delta_c;
  FloatVariable m_set_temp;
  FloatVariable m_ramp_rate;
  FloatVariable m_ff_per_rate;
  Variable<String> m_heat_mode;  // heater mode for HA thermostat ('off' / 'on').
  Variable<String> m_fan_mode;   // fan mode for HA thermostat ('off' / 'high').
};

const char* TempControl::state_names[] = {
    "Off",
    "Running",
    "Cooling...",
    "Error!",
};

TempControl s_temp_control;

#define CONFIG_URL "/configure"
const char* s_config_url = CONFIG_URL;

void handleEnable(AsyncWebServerRequest* request) {
  s_app.log().logf("http -> enable");
  s_temp_control.delaySetEnable(true);
  request->redirect("/");
}
void handleDisable(AsyncWebServerRequest* request) {
  s_app.log().logf("http -> disable");
  s_temp_control.delaySetEnable(false);
  request->redirect("/");
}

void handleFanRelay(AsyncWebServerRequest* request) {
  s_blink.blink(2);
  s_app.log().logf("turning on fan for %u msec.", kFanOnMsec);
  s_temp_control.turnFanOn();
  s_relay_fan.turnOn(kFanOnMsec);  // turn on for 1000msec
  request->redirect(s_config_url);
}

void handleHeaterRelay(AsyncWebServerRequest* request) {
  // static Ticker s_heater_off_ticker;
  heaterOn(0.2);  // turn on for 1000msec
  s_blink.blink(3);
  s_app.log().logf("turning on heater for %u msec.", 1000);
  s_app.tasks().runIn(10000, []() { heaterOff(); });
  request->redirect(s_config_url);
}

// The send of the web page happens asynchronously, so we need to make
//  sure the storage for the page remains after they are rendered to html.
// That is why html is stored in this static variable.
static String s_html;

void handleUpdateTarget(AsyncWebServerRequest* request) {
#ifndef NATIVE
  s_html.clear();
  ::og3::read(*request, s_cmdvg);
  html::writeFormTableInto(&s_html, s_cmdvg);
  s_html += HTML_BUTTON("/", "Back");
  sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_html.c_str());
  s_app.config().write_config(s_cmdvg);
#endif
}

void handleUpdateConfig(AsyncWebServerRequest* request) {
#ifndef NATIVE
  s_html.clear();
  ::og3::read(*request, s_cvg);
  html::writeFormTableInto(&s_html, s_cvg);
  s_html += HTML_BUTTON(CONFIG_URL, "Back");
  sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_html.c_str());
  s_app.config().write_config(s_cvg);
#endif
}

og3::WebButton s_button_wifi_config = s_app.createWifiConfigButton();
og3::WebButton s_button_mqtt_config = s_app.createMqttConfigButton();
og3::WebButton s_button_app_status = s_app.createAppStatusButton();
og3::WebButton s_button_restart = s_app.createRestartButton();

void handleConfigure(AsyncWebServerRequest* request);
og3::WebButton s_button_config(&s_app.web_server(), "Configuration", s_config_url, handleConfigure);

og3::WebButton s_button_enable(&s_app.web_server(), "Turn on", "/doughlee/enable", handleEnable);
og3::WebButton s_button_disable(&s_app.web_server(), "Turn off", "/doughlee/disable",
                                handleDisable);
og3::WebButton s_button_doughl33_target(&s_app.web_server(), "Set target temp", "/doughlee/target",
                                        handleUpdateTarget);
og3::WebButton s_button_doughl33_config(&s_app.web_server(), "Temperature control",
                                        "/doughlee/update", handleUpdateConfig);
og3::WebButton s_button_test_fan(&s_app.web_server(), "Test fan", "/relay/fan", handleFanRelay);
og3::WebButton s_button_test_heater(&s_app.web_server(), "Test heater", "/relay/heater",
                                    handleHeaterRelay);

void handleWebRoot(AsyncWebServerRequest* request) {
  s_html.clear();
  s_temp_control.writeHtmlStatusTable(&s_html);

  html::writeTableStart(&s_html, "Connection");
  html::writeRowInto(&s_html, s_app.wifi_manager().ipAddressVariable(), "IP address");
  html::writeRowInto(&s_html, s_app.mqtt_manager().connectionStatusVariable(), "MQTT connection");
  html::writeTableEnd(&s_html);

  if (s_temp_control.enabled()) {
    s_button_disable.add_button(&s_html);
  } else {
    s_button_enable.add_button(&s_html);
  }
  s_button_doughl33_target.add_button(&s_html);
  s_button_config.add_button(&s_html);
  s_button_restart.add_button(&s_html);
  sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_html.c_str());
}

void handleConfigure(AsyncWebServerRequest* request) {
  s_html.clear();
  html::writeTableInto(&s_html, s_vg, "Control status");
  html::writeTableInto(&s_html, s_app.wifi_manager().variables());
  html::writeTableInto(&s_html, s_app.mqtt_manager().variables());

  s_button_wifi_config.add_button(&s_html);
  s_button_mqtt_config.add_button(&s_html);
  s_button_app_status.add_button(&s_html);

  s_button_doughl33_config.add_button(&s_html);
  s_button_test_fan.add_button(&s_html);
  s_button_test_heater.add_button(&s_html);
  s_html += HTML_BUTTON("/", "Back");
  sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_html.c_str());
}

}  // namespace og3

void setup() {
  Wire1.setPins(og3::kSda2, og3::kScl2);  // The room temp sensor uses this second i2c bus.
  og3::s_app.web_server().on("/", og3::handleWebRoot);

  og3::s_oled.setup();
  og3::s_oled.addDisplayFn([]() {
    og3::s_oled.setFontSize(og3::Oled::kSixteenPt);
    og3::s_oled.display(og3::s_app.board_cname());
  });

  og3::s_app.setup();
  og3::s_app.config().read_config(og3::s_cvg);
  og3::s_app.config().read_config(og3::s_cmdvg);
  og3::s_button_reader.read();  // read state of the button on startup.
  og3::heaterOff();
}

void loop() {
  og3::s_app.loop();

  // Detect button-down transition.
  static unsigned s_button_count = 0;
  const bool button_was_high = og3::s_button_reader.isHigh();
  og3::s_button_reader.read();
  if (!button_was_high && og3::s_button_reader.isHigh()) {
    og3::s_temp_control.toggleEnable();
    s_button_count += 1;
    og3::s_app.log().logf("button -> high");
  } else if (button_was_high && !og3::s_button_reader.isHigh()) {
    og3::s_app.log().logf("button -> low");
  }
}
