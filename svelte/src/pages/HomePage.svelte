<script>
  import { Thermometer, Droplets, Fan, Zap, Activity } from 'lucide-svelte';

  export let systemStatus;
  export let config;

  $: status = $systemStatus;
  $: conf = $config;

  async function toggleEnable() {
    // Only call 'disable' if we are currently in the 'Running' state (idx 1)
    const action = status.state_idx === 1 ? 'disable' : 'enable';
    try {
      await fetch(`/api/${action}`, { method: 'POST' });
    } catch (err) {
      console.error(`Error ${action}ing:`, err);
    }
  }

  async function toggleFan() {
    const isOff = status.fanMode === 'off';
    const action = isOff ? 'on' : 'off';
    try {
      await fetch(`/api/fan/${action}`, { method: 'POST' });
    } catch (err) {
      console.error(`Error toggling fan ${action}:`, err);
    }
  }

  async function setTargetTemp(e) {
    const newTarget = parseFloat(e.target.value);
    if (isNaN(newTarget)) return;
    try {
      await fetch('/api/target', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ setTemp: newTarget })
      });
    } catch (err) {
      console.error('Error setting target:', err);
    }
  }

  function getStatusColor(stateIdx) {
    switch (stateIdx) {
      case 0: return 'text-gray'; // Off
      case 1: return 'text-green'; // Running
      case 2: return 'text-blue'; // Cooling
      case 3: return 'text-red'; // Error
      case 4: return 'text-orange'; // Test
      default: return 'text-gray';
    }
  }
</script>

<div class="dashboard">
  <header class="header">
    <h1>Dough133 Overview</h1>
    <div class="status-badge" class:active={status.state_idx === 1}>
      <span class="status-dot {getStatusColor(status.state_idx)}"></span>
      {status.state}
    </div>
  </header>

  <div class="grid">
    <!-- Enclosure Status -->
    <section class="card">
      <div class="card-header">
        <Thermometer size={20} />
        <h2>Enclosure</h2>
      </div>
      <div class="card-content">
        <div class="stat">
          <span class="label">Temperature</span>
          <span class="value">{status.tempEnclosure.toFixed(1)}°C</span>
        </div>
        <div class="stat">
          <span class="label">Humidity</span>
          <span class="value">{status.humEnclosure.toFixed(1)}%</span>
        </div>
        <div class="stat">
          <span class="label">Filtered</span>
          <span class="value">{status.tempFilt.toFixed(2)}°C</span>
        </div>
        <div class="stat">
          <span class="label">Rate</span>
          <span class="value">{status.tempDFilt.toFixed(3)}°C/s</span>
        </div>
      </div>
    </section>

    <!-- Room Status -->
    <section class="card">
      <div class="card-header">
        <Thermometer size={20} class="text-blue" />
        <h2>Room</h2>
      </div>
      <div class="card-content">
        <div class="stat">
          <span class="label">Temperature</span>
          <span class="value">{status.tempRoom.toFixed(1)}°C</span>
        </div>
        <div class="stat">
          <span class="label">Humidity</span>
          <span class="value">{status.humRoom.toFixed(1)}%</span>
        </div>
      </div>
    </section>

    <!-- Control Status -->
    <section class="card">
      <div class="card-header">
        <Zap size={20} class="text-orange" />
        <h2>Heating Control</h2>
      </div>
      <div class="card-content">
        <div class="control-group">
          <label for="target-temp">Target Temperature (°C)</label>
          <div class="input-with-action">
            <input
              id="target-temp"
              type="number"
              step="0.5"
              value={status.setTemp}
              on:change={setTargetTemp}
            />
            <button class="btn" class:btn-danger={status.state_idx === 1} on:click={toggleEnable}>
              {status.state_idx === 1 ? 'Disable' : 'Enable'}
            </button>
          </div>
        </div>
        <div class="stat mt-4">
          <span class="label">Current Target (Ramp)</span>
          <span class="value">{status.target.toFixed(2)}°C</span>
        </div>
        <div class="stat">
          <span class="label">Heater Power (PWM)</span>
          <span class="value">{(status.heater * 100).toFixed(1)}%</span>
        </div>
        <div class="stat">
          <span class="label">Heater Mode</span>
          <span class="value">{status.heatMode}</span>
        </div>
        <div class="control-group mt-2">
          <label>Fan Control</label>
          <div class="input-with-action">
            <div class="stat flex-1">
              <span class="label">State</span>
              <span class="value" class:text-green={status.fan}>{status.fan ? 'ON' : 'OFF'} ({status.fanMode})</span>
            </div>
            <button class="btn btn-secondary" on:click={toggleFan}>
              <Fan size={16} class="mr-1" />
              {status.fanMode === 'off' ? 'Fan On' : 'Fan Off'}
            </button>
          </div>
        </div>
      </div>
    </section>

    <!-- PID Status -->
    <section class="card">
      <div class="card-header">
        <Activity size={20} />
        <h2>PID Terms</h2>
      </div>
      <div class="card-content">
        <div class="stat">
          <span class="label">P Term</span>
          <span class="value">{status.cmdP.toFixed(3)}</span>
        </div>
        <div class="stat">
          <span class="label">I Term</span>
          <span class="value">{status.cmdI.toFixed(3)}</span>
        </div>
        <div class="stat">
          <span class="label">D Term</span>
          <span class="value">{status.cmdD.toFixed(3)}</span>
        </div>
        <div class="stat">
          <span class="label">FF Term</span>
          <span class="value">{status.cmdFF.toFixed(3)}</span>
        </div>
        <div class="stat total">
          <span class="label">Total Command</span>
          <span class="value">{(status.heater).toFixed(3)}</span>
        </div>
      </div>
    </section>
  </div>
</div>

<style>
  .dashboard {
    display: flex;
    flex-direction: column;
    gap: 2rem;
  }

  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  h1 {
    font-size: 1.5rem;
    font-weight: 700;
    color: #111827;
  }

  .status-badge {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.5rem 1rem;
    background: white;
    border-radius: 9999px;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
    font-size: 0.875rem;
    font-weight: 600;
  }

  .status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #6b7280;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
  }

  .card {
    background: white;
    border-radius: 0.75rem;
    padding: 1.5rem;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  }

  .card-header {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    margin-bottom: 1.25rem;
    border-bottom: 1px solid #f3f4f6;
    padding-bottom: 0.75rem;
  }

  .card-header h2 {
    font-size: 1.125rem;
    font-weight: 600;
    color: #374151;
  }

  .stat {
    display: flex;
    justify-content: space-between;
    padding: 0.5rem 0;
  }

  .flex-1 { flex: 1; }

  .stat .label {
    color: #6b7280;
    font-size: 0.875rem;
  }

  .stat .value {
    font-weight: 600;
    color: #111827;
  }

  .stat.total {
    margin-top: 0.5rem;
    padding-top: 0.75rem;
    border-top: 2px solid #f3f4f6;
  }

  .control-group {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .control-group label {
    font-size: 0.875rem;
    color: #6b7280;
  }

  .input-with-action {
    display: flex;
    gap: 0.5rem;
    align-items: center;
  }

  input {
    flex: 1;
    padding: 0.5rem;
    border: 1px solid #d1d5db;
    border-radius: 0.375rem;
  }

  .btn {
    display: flex;
    align-items: center;
    padding: 0.5rem 1rem;
    background: #059669;
    color: white;
    border: none;
    border-radius: 0.375rem;
    cursor: pointer;
    font-weight: 600;
    white-space: nowrap;
  }

  .btn:hover { background: #047857; }
  .btn-danger { background: #dc2626; }
  .btn-danger:hover { background: #b91c1c; }

  .btn-secondary {
    background: #f3f4f6;
    color: #374151;
    border: 1px solid #d1d5db;
  }
  .btn-secondary:hover { background: #e5e7eb; }

  .mt-2 { margin-top: 0.5rem; }
  .mt-4 { margin-top: 1rem; }
  :global(.mr-1) { margin-right: 0.25rem; }

  :global(.text-green) { color: #10b981; }
  :global(.text-blue) { color: #3b82f6; }
  :global(.text-red) { color: #ef4444; }
  :global(.text-gray) { color: #6b7280; }
  :global(.text-orange) { color: #f59e0b; }
</style>
