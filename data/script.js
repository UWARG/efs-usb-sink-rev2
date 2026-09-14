const voltSelect = document.getElementById('voltage');
const currSlider = document.getElementById('current');
const currValStr = document.getElementById('current-val');
const toggleBtn = document.getElementById('toggle-btn');
const sysLog = document.getElementById('log');

const measV = document.getElementById('meas-v');
const measI = document.getElementById('meas-i');
const measP = document.getElementById('meas-p');

let isEnabled = false;

currSlider.addEventListener('input', (e) => {
    currValStr.textContent = parseFloat(e.target.value).toFixed(1);
});

function logMsg(msg, isError = false) {
    const time = new Date().toLocaleTimeString('en-US', { hour12: false });
    sysLog.textContent = `[${time}] ${msg}`;
    if (isError) {
        sysLog.classList.add('error');
    } else {
        sysLog.classList.remove('error');
    }
}

async function fetchTelemetry() {
    try {
        const res = await fetch('/api/status');
        if (res.ok) {
            const data = await res.json();
            measV.textContent = data.voltage.toFixed(2);
            measI.textContent = data.current.toFixed(2);
            measP.textContent = (data.voltage * data.current).toFixed(2);
        }
    } catch (err) {
        // Silently fail telemetry on network drop to prevent log spam
    }
}

setInterval(fetchTelemetry, 1000);

async function sendConfig() {
    const payload = {
        voltage: parseFloat(voltSelect.value),
        current: parseFloat(currSlider.value),
        enable: isEnabled
    };

    try {
        const response = await fetch('/api/set', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        
        if (response.ok) {
            logMsg(`ACK: V=${payload.voltage}V, I=${payload.current}A, EN=${payload.enable}`);
        } else {
            const data = await response.json().catch(() => null);
            const errMsg = data && data.message ? data.message : 'NACK FROM MCU';
            logMsg(`FAIL: ${errMsg}`, true);
            
            // Force toggle state back to off to protect system
            if (isEnabled) {
                isEnabled = false;
                toggleBtn.textContent = 'SYS_OFF';
                toggleBtn.className = 'off';
            }
        }
    } catch (err) {
        logMsg(`ERR: COMM FAILURE`, true);
    }
}

voltSelect.addEventListener('change', sendConfig);
currSlider.addEventListener('change', sendConfig);

toggleBtn.addEventListener('click', () => {
    isEnabled = !isEnabled;
    toggleBtn.textContent = isEnabled ? 'SYS_ON' : 'SYS_OFF';
    toggleBtn.className = isEnabled ? 'on' : 'off';
    sendConfig();
});
