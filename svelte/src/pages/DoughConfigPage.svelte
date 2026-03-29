<script>
  import { Settings, Save, RefreshCcw } from 'lucide-svelte';

  export let config;

  $: conf = $config;

  let localConfig = { ...conf };

  $: {
    if (JSON.stringify(localConfig) !== JSON.stringify(conf)) {
      localConfig = { ...conf };
    }
  }

  async function saveConfig() {
    try {
      const response = await fetch('/api/config', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(localConfig)
      });
      if (response.ok) {
        alert('Configuration saved!');
        config.set({ ...localConfig });
      } else {
        alert('Failed to save configuration');
      }
    } catch (err) {
      console.error('Error saving config:', err);
      alert('Error saving configuration');
    }
  }

  async function resetConfig() {
    localConfig = { ...conf };
  }
</script>

<div class="config-page">
  <header class="header">
    <div class="title-with-icon">
      <Settings size={24} />
      <h1>Dough Control Configuration</h1>
    </div>
    <div class="actions">
      <button class="btn btn-secondary" on:click={resetConfig}>
        <RefreshCcw size={18} />
        Reset
      </button>
      <button class="btn btn-primary" on:click={saveConfig}>
        <Save size={18} />
        Save Configuration
      </button>
    </div>
  </header>

  <div class="config-grid">
    <!-- Target & Test Settings -->
    <section class="card">
      <h2>Target & Test Settings</h2>
      <div class="form-group">
        <label for="setTemp">Target Temperature (°C)</label>
        <input id="setTemp" type="number" step="0.1" bind:value={localConfig.setTemp} />
      </div>
      <div class="form-group">
        <label for="testCommand">Test PWM Command (0-1)</label>
        <input id="testCommand" type="number" step="0.01" min="0" max="1" bind:value={localConfig.testCommand} />
      </div>
      <div class="form-group">
        <label for="testCommandSec">Test Duration (sec)</label>
        <input id="testCommandSec" type="number" step="1" bind:value={localConfig.testCommandSec} />
      </div>
    </section>

    <!-- Safety Limits -->
    <section class="card">
      <h2>Safety Limits</h2>
      <div class="form-group">
        <label for="tempMinOk">Min Valid Temp (°C)</label>
        <input id="tempMinOk" type="number" step="1" bind:value={localConfig.tempMinOk} />
        <p class="help">Heater will shut off if enclosure temp is below this.</p>
      </div>
      <div class="form-group">
        <label for="tempMaxOk">Max Valid Temp (°C)</label>
        <input id="tempMaxOk" type="number" step="1" bind:value={localConfig.tempMaxOk} />
        <p class="help">Heater will shut off if enclosure temp is above this.</p>
      </div>
    </section>

    <!-- Ramping & Feedforward -->
    <section class="card">
      <h2>Ramping & Feedforward</h2>
      <div class="form-group">
        <label for="rampRate">Ramp Rate (°C/s)</label>
        <input id="rampRate" type="number" step="0.001" bind:value={localConfig.rampRate} />
      </div>
      <div class="form-group">
        <label for="ctlFeedforwardPerDeltaC">Static FF (PWM / Δ°C)</label>
        <input id="ctlFeedforwardPerDeltaC" type="number" step="0.001" bind:value={localConfig.ctlFeedforwardPerDeltaC} />
        <p class="help">Feedforward power per degree above initial temp.</p>
      </div>
      <div class="form-group">
        <label for="feedforwardPerRate">Dynamic FF (PWM / (°C/s))</label>
        <input id="feedforwardPerRate" type="number" step="0.001" bind:value={localConfig.feedforwardPerRate} />
        <p class="help">Feedforward power proportional to ramp rate.</p>
      </div>
      <div class="form-group">
        <label for="feedforward">PID Constant FF (0-1)</label>
        <input id="feedforward" type="number" step="0.01" min="0" max="1" bind:value={localConfig.feedforward} />
      </div>
    </section>

    <!-- PID Gains -->
    <section class="card">
      <h2>PID Gains</h2>
      <div class="form-group">
        <label for="kP">Proportional (kP)</label>
        <input id="kP" type="number" step="0.01" bind:value={localConfig.kP} />
      </div>
      <div class="form-group">
        <label for="kI">Integral (kI)</label>
        <input id="kI" type="number" step="0.0001" bind:value={localConfig.kI} />
      </div>
      <div class="form-group">
        <label for="kD">Derivative (kD)</label>
        <input id="kD" type="number" step="0.1" bind:value={localConfig.kD} />
      </div>
      <div class="form-group">
        <label for="iMax">Integral Max (PWM)</label>
        <input id="iMax" type="number" step="0.01" bind:value={localConfig.iMax} />
      </div>
      <div class="form-group">
        <label for="iMin">Integral Min (PWM)</label>
        <input id="iMin" type="number" step="0.01" bind:value={localConfig.iMin} />
      </div>
    </section>

    <!-- Output Clamping -->
    <section class="card">
      <h2>Output Clamping</h2>
      <div class="form-group">
        <label for="commandMin">Min PWM Command (0-1)</label>
        <input id="commandMin" type="number" step="0.01" min="0" max="1" bind:value={localConfig.commandMin} />
      </div>
      <div class="form-group">
        <label for="commandMax">Max PWM Command (0-1)</label>
        <input id="commandMax" type="number" step="0.01" min="0" max="1" bind:value={localConfig.commandMax} />
      </div>
    </section>
  </div>
</div>

<style>
  .config-page {
    display: flex;
    flex-direction: column;
    gap: 2rem;
  }

  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .title-with-icon {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  h1 {
    font-size: 1.5rem;
    font-weight: 700;
    color: #111827;
  }

  .actions {
    display: flex;
    gap: 1rem;
  }

  .config-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
    gap: 1.5rem;
  }

  .card {
    background: white;
    border-radius: 0.75rem;
    padding: 1.5rem;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  }

  h2 {
    font-size: 1.125rem;
    font-weight: 600;
    color: #374151;
    margin-bottom: 1.25rem;
    border-bottom: 1px solid #f3f4f6;
    padding-bottom: 0.75rem;
  }

  .form-group {
    margin-bottom: 1.25rem;
  }

  label {
    display: block;
    font-size: 0.875rem;
    font-weight: 500;
    color: #4b5563;
    margin-bottom: 0.375rem;
  }

  input {
    width: 100%;
    padding: 0.5rem;
    border: 1px solid #d1d5db;
    border-radius: 0.375rem;
    font-size: 0.875rem;
  }

  .help {
    font-size: 0.75rem;
    color: #6b7280;
    margin-top: 0.25rem;
  }

  .btn {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.5rem 1rem;
    border: none;
    border-radius: 0.375rem;
    font-weight: 600;
    cursor: pointer;
    font-size: 0.875rem;
  }

  .btn-primary {
    background: #059669;
    color: white;
  }

  .btn-primary:hover { background: #047857; }

  .btn-secondary {
    background: #f3f4f6;
    color: #374151;
  }

  .btn-secondary:hover { background: #e5e7eb; }
</style>
