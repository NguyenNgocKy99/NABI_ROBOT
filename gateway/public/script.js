// ============ CONFIG ============
const SERVER_URL = "https://nabirob-production-916d.up.railway.app";

// ============ STATE ============
let socket = null;
let sensorHistory = [];
let isConnected = false;

// ============ INITIALIZATION ============
document.addEventListener('DOMContentLoaded', function() {
    console.log("Dashboard loaded");
    addLog("🟢 Dashboard initialized", 'success');
    
    // Connect to WebSocket
    connectToServer();
    
    updateTime();
    setInterval(updateTime, 1000);
    
    // Setup slider listeners
    setupSliders();
});

// ============ WEBSOCKET CONNECTION ============

function connectToServer() {
    addLog("🔌 Connecting to WebSocket server...", 'info');
    
    socket = io(SERVER_URL, {
        reconnection: true,
        reconnectionDelay: 1000,
        reconnectionDelayMax: 5000,
        reconnectionAttempts: Infinity,
        transports: ['websocket', 'polling']
    });
    
    // Connection events
    socket.on('connect', function() {
        isConnected = true;
        console.log("✅ Connected to server");
        setServerStatus(true);
        addLog("✅ WebSocket connected!", 'success');
        
        // Identify as dashboard
        socket.emit('identify', { type: 'dashboard' });
        
        // Request current status
        socket.emit('request_status');
    });
    
    socket.on('disconnect', function() {
        isConnected = false;
        console.log("❌ Disconnected from server");
        setServerStatus(false);
        addLog("❌ WebSocket disconnected", 'error');
    });
    
    // Receive events
    socket.on('welcome', function(data) {
        console.log("Welcome:", data);
        addLog("🎉 " + data.message, 'success');
    });
    
    socket.on('sensor_update', function(data) {
        console.log("Sensor update:", data);
        if (data.data) {
            updateSensorDisplay(data.data);
            addSensorToHistory(data);
            setRobotStatus(true);
            addLog("📊 Sensor data received", 'success');
        }
    });
    
    socket.on('status_response', function(data) {
        console.log("Status response:", data);
        addLog(`📊 Clients connected: ${data.connected_clients}`, 'info');
        document.getElementById('clientCount').textContent = data.connected_clients;
    });
    
    socket.on('ack', function(data) {
        console.log("Acknowledgement:", data);
        addLog("✓ " + data.message, 'success');
    });
    
    socket.on('heartbeat_ack', function(data) {
        console.log("Heartbeat acknowledged");
    });
    
    socket.on('client_count', function(data) {
        document.getElementById('clientCount').textContent = data.count;
    });
    
    socket.on('error', function(error) {
        console.error("Socket error:", error);
        addLog("❌ Error: " + error, 'error');
    });
}

// ============ UPDATE TIME ============

function updateTime() {
    const now = new Date();
    const timeStr = now.toLocaleTimeString();
    document.getElementById('lastUpdate').textContent = timeStr;
}

// ============ CONNECTION STATUS ============

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

// ============ TEST CONNECTION ============

function testConnection() {
    addLog("🔍 Testing connection...", 'info');
    
    if (!isConnected) {
        addLog("❌ Not connected to server", 'error');
        return;
    }
    
    socket.emit('request_status');
    addLog("✅ Connection test sent", 'success');
}

// ============ SENSOR DATA ============

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
    if (!isConnected) {
        addLog("❌ Not connected to server", 'error');
        return;
    }
    
    addLog(`⚡ Sending command: ${action}`, 'info');
    
    const commandData = {
        action: action,
        duration: duration,
        speed: speed
    };
    
    // Emit via WebSocket
    socket.emit('send_command', commandData);
}

// ============ REQUEST STATUS ============

function requestStatus() {
    if (!isConnected) {
        addLog("❌ Not connected to server", 'error');
        return;
    }
    
    addLog("📊 Requesting status...", 'info');
    socket.emit('request_status');
}

// ============ SLIDER SETUP ============

function setupSliders() {
    document.getElementById('speedSlider').addEventListener('input', function() {
        document.getElementById('speedValue').textContent = this.value;
    });
    
    document.getElementById('durationSlider').addEventListener('input', function() {
        document.getElementById('durationValue').textContent = this.value;
    });
}

// ============ UTILITY FUNCTIONS ============

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