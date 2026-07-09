// ============ CONFIG ============
const SERVER_URL = "https://nabirob-production.up.railway.app";


// ============ STATE ============
let sensorHistory = [];
let lastUpdate = null;

// ============ INITIALIZATION ============
document.addEventListener('DOMContentLoaded', function() {
    console.log("Dashboard loaded");
    addLog("🟢 Dashboard initialized", 'success');
    testConnection();
    updateTime();
    setInterval(updateTime, 1000);
    setInterval(autoRefresh, SENSOR_INTERVAL);
});

// ============ UPDATE TIME ============
const SENSOR_INTERVAL = 10000; // 10 seconds

function updateTime() {
    const now = new Date();
    const timeStr = now.toLocaleTimeString();
    document.getElementById('lastUpdate').textContent = timeStr;
}

// ============ CONNECTION TEST ============
function testConnection() {
    addLog("🔍 Testing server connection...", 'info');
    
    fetch(SERVER_URL + '/health')
        .then(response => {
            if (response.ok) {
                setServerStatus(true);
                addLog("✅ Server connected", 'success');
                getLatestSensor();
            } else {
                setServerStatus(false);
                addLog("❌ Server error: " + response.status, 'error');
            }
        })
        .catch(error => {
            setServerStatus(false);
            addLog("❌ Connection failed: " + error, 'error');
        });
}

function setServerStatus(online) {
    const badge = document.getElementById('serverStatus');
    if (online) {
        badge.textContent = 'Connected';
        badge.className = 'status-badge online';
    } else {
        badge.textContent = 'Disconnected';
        badge.className = 'status-badge offline';
    }
}

function setRobotStatus(online) {
    const badge = document.getElementById('robotStatus');
    if (online) {
        badge.textContent = 'Online';
        badge.className = 'status-badge online';
    } else {
        badge.textContent = 'Offline';
        badge.className = 'status-badge offline';
    }
}

// ============ SENSOR DATA ============
function getLatestSensor() {
    fetch(SERVER_URL + '/api/sensor/latest')
        .then(response => response.json())
        .then(data => {
            if (data.timestamp) {
                updateSensorDisplay(data.data);
                addSensorToHistory(data);
                setRobotStatus(true);
                addLog("📊 Sensor data updated", 'success');
            }
        })
        .catch(error => {
            setRobotStatus(false);
            addLog("❌ Sensor fetch failed: " + error, 'error');
        });
}

function updateSensorDisplay(data) {
    document.getElementById('tempValue').textContent = 
        data.temperature ? data.temperature.toFixed(1) : '--';
    document.getElementById('humidityValue').textContent = 
        data.humidity || '--';
    document.getElementById('motionValue').textContent = 
        data.motion ? 'Yes' : 'No';
    document.getElementById('batteryValue').textContent = 
        data.battery || '--';
}

function addSensorToHistory(sensorData) {
    sensorHistory.unshift({
        timestamp: new Date(sensorData.timestamp),
        data: sensorData.data
    });
    
    // Keep only last 20 entries
    if (sensorHistory.length > 20) {
        sensorHistory.pop();
    }
    
    updateHistoryDisplay();
}

function updateHistoryDisplay() {
    const historyDiv = document.getElementById('sensorHistory');
    
    if (sensorHistory.length === 0) {
        historyDiv.innerHTML = '<p class="placeholder">No data yet...</p>';
        return;
    }
    
    historyDiv.innerHTML = sensorHistory.map(entry => `
        <div class="history-item">
            <div class="history-timestamp">${entry.timestamp.toLocaleTimeString()}</div>
            <div class="history-data">
                🌡️ ${entry.data.temperature?.toFixed(1) || '--'}°C | 
                💧 ${entry.data.humidity || '--'}% | 
                🔋 ${entry.data.battery || '--'}%
            </div>
        </div>
    `).join('');
}

// ============ MOTOR CONTROL ============
function moveForward() {
    const speed = parseInt(document.getElementById('speedSlider').value);
    const duration = parseInt(document.getElementById('durationSlider').value);
    sendCommand('move_forward', duration, speed);
}

function moveBackward() {
    const speed = parseInt(document.getElementById('speedSlider').value);
    const duration = parseInt(document.getElementById('durationSlider').value);
    sendCommand('move_backward', duration, speed);
}

function turnLeft() {
    const speed = parseInt(document.getElementById('speedSlider').value);
    const duration = parseInt(document.getElementById('durationSlider').value);
    sendCommand('turn_left', duration, speed);
}

function turnRight() {
    const speed = parseInt(document.getElementById('speedSlider').value);
    const duration = parseInt(document.getElementById('durationSlider').value);
    sendCommand('turn_right', duration, speed);
}

function stopRobot() {
    sendCommand('stop', 0, 0);
}

function ledOn() {
    sendCommand('led_on', 0, 0);
}

function ledOff() {
    sendCommand('led_off', 0, 0);
}

function sendCommand(action, duration, speed) {
    addLog(`⚡ Sending command: ${action}`, 'info');
    
    const commandData = {
        action: action,
        duration: duration,
        speed: speed
    };
    
    fetch(SERVER_URL + '/api/command/send', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(commandData)
    })
        .then(response => response.json())
        .then(data => {
            if (data.status === 'success') {
                addLog(`✅ Command sent: ${action}`, 'success');
            } else {
                addLog(`❌ Command failed: ${data.message}`, 'error');
            }
        })
        .catch(error => {
            addLog(`❌ Send failed: ${error}`, 'error');
        });
}

// ============ SPEED & DURATION CONTROLS ============
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('speedSlider').addEventListener('input', function() {
        document.getElementById('speedValue').textContent = this.value;
    });
    
    document.getElementById('durationSlider').addEventListener('input', function() {
        document.getElementById('durationValue').textContent = this.value;
    });
});

// ============ UTILITY FUNCTIONS ============
function refreshData() {
    addLog("🔄 Refreshing data...", 'info');
    testConnection();
    getLatestSensor();
}

function autoRefresh() {
    getLatestSensor();
}

function clearHistory() {
    sensorHistory = [];
    updateHistoryDisplay();
    addLog("🗑️ History cleared", 'info');
}

function addLog(message, type = 'info') {
    const logsDiv = document.getElementById('logs');
    const logEntry = document.createElement('p');
    logEntry.className = `log-entry ${type}`;
    
    const timestamp = new Date().toLocaleTimeString();
    logEntry.textContent = `[${timestamp}] ${message}`;
    
    logsDiv.insertBefore(logEntry, logsDiv.firstChild);
    
    // Keep only last 50 logs
    while (logsDiv.children.length > 50) {
        logsDiv.removeChild(logsDiv.lastChild);
    }
}