/*
🤖 NABI ROBOT - Node.js WebSocket Gateway
Real-time communication between ESP32 and Python AI Backend
*/

require('dotenv').config();
const WebSocket = require('ws');
const axios = require('axios');
const express = require('express');
const path = require('path');

// ============ CONFIG ============
const PORT = process.env.PORT || 8080;
const PYTHON_SERVICE_URL = process.env.PYTHON_SERVICE_URL || 'http://localhost:5000';
const DEBUG = process.env.DEBUG || 1;

// ============ EXPRESS SETUP ============
const app = express();
app.use(express.json());

// Serve web-dashboard
app.use(express.static(path.join(__dirname, '../web-dashboard')));

// ============ HTTP ENDPOINTS ============

// Health check
app.get('/health', (req, res) => {
    res.json({ status: 'ok', message: 'Gateway running' });
});

// Forward sensor data to Python
app.post('/api/sensor', async (req, res) => {
    try {
        const response = await axios.post(`${PYTHON_SERVICE_URL}/api/sensor`, req.body);
        res.json(response.data);
    } catch (error) {
        console.error('❌ Sensor forward error:', error.message);
        res.status(500).json({ error: error.message });
    }
});

// Forward AI processing to Python
app.post('/api/ai/process', async (req, res) => {
    try {
        const response = await axios.post(`${PYTHON_SERVICE_URL}/api/ai/process`, req.body);
        res.json(response.data);
    } catch (error) {
        console.error('❌ AI process error:', error.message);
        res.status(500).json({ error: error.message });
    }
});

// Get status
app.get('/api/status', async (req, res) => {
    try {
        const response = await axios.get(`${PYTHON_SERVICE_URL}/api/status`);
        res.json(response.data);
    } catch (error) {
        res.json({ status: 'error', message: error.message });
    }
});

// ============ WEBSOCKET SERVER ============

const wss = new WebSocket.Server({ 
    noServer: true,
    path: '/ws'
});

// Track connected robots
const connectedRobots = new Map();

// Utility function
const formatTime = () => new Date().toISOString();

// ============ WEBSOCKET CONNECTION ============

wss.on('connection', (ws, req) => {
    const clientIp = req.socket.remoteAddress || 'unknown';
    const robotId = `robot_${Date.now()}`;
    
    connectedRobots.set(ws, {
        id: robotId,
        ip: clientIp,
        connectedAt: new Date()
    });
    
    log(`🔌 Robot Connected from IP: ${clientIp}`, 'connect');
    log(`📊 Total robots: ${connectedRobots.size}`, 'info');
    
    // Welcome message
    ws.send(JSON.stringify({
        event: 'welcome',
        message: 'Connected to Nabi Gateway!',
        robotId: robotId,
        timestamp: formatTime()
    }));
    
    // ============ MESSAGE HANDLER ============
    ws.on('message', async (message) => {
        try {
            const data = JSON.parse(message);
            log(`📥 Message from ${robotId}: ${data.event}`, 'receive');
            
            if (DEBUG) {
                console.log('   Payload:', JSON.stringify(data).substring(0, 100));
            }
            
            // Event 1: Voice-to-Text + AI Response
            if (data.event === 'voice_text') {
                await handleVoiceText(ws, data, robotId);
            }
            
            // Event 2: Sensor Telemetry
            else if (data.event === 'sensor_telemetry') {
                await handleSensorTelemetry(ws, data, robotId);
            }
            
            // Event 3: Status Update
            else if (data.event === 'status_update') {
                await handleStatusUpdate(ws, data, robotId);
            }
            
            // Event 4: Heartbeat/Ping
            else if (data.event === 'ping') {
                ws.send(JSON.stringify({
                    event: 'pong',
                    timestamp: formatTime()
                }));
            }
            
            else {
                log(`⚠️ Unknown event: ${data.event}`, 'warning');
            }
            
        } catch (error) {
            log(`❌ Error processing message: ${error.message}`, 'error');
            ws.send(JSON.stringify({
                event: 'error',
                message: error.message,
                timestamp: formatTime()
            }));
        }
    });
    
    // ============ CONNECTION CLOSE ============
    ws.on('close', () => {
        connectedRobots.delete(ws);
        log(`❌ Robot Disconnected (${robotId})`, 'disconnect');
        log(`📊 Total robots: ${connectedRobots.size}`, 'info');
    });
    
    // ============ ERROR HANDLER ============
    ws.on('error', (error) => {
        log(`⚠️ WebSocket error: ${error.message}`, 'error');
    });
});

// ============ EVENT HANDLERS ============

async function handleVoiceText(ws, data, robotId) {
    try {
        log(`🎤 Processing voice text: "${data.text}"`, 'voice');
        
        // Forward to Python AI service
        const aiResponse = await axios.post(`${PYTHON_SERVICE_URL}/api/ai/process`, {
            message: data.text
        });
        
        log(`🧠 AI Response generated`, 'success');
        
        // Send back to ESP32
        ws.send(JSON.stringify({
            event: 'ai_response',
            reply: aiResponse.data.reply || 'No response',
            command: aiResponse.data.command || 'idle',
            timestamp: formatTime()
        }));
        
        log(`📤 AI response sent to robot`, 'send');
        
    } catch (error) {
        log(`❌ Voice processing error: ${error.message}`, 'error');
        ws.send(JSON.stringify({
            event: 'error',
            message: 'AI processing failed',
            timestamp: formatTime()
        }));
    }
}

async function handleSensorTelemetry(ws, data, robotId) {
    try {
        log(`📊 Sensor data received: temp=${data.data?.temperature}°C`, 'sensor');
        
        // Forward to Python storage
        await axios.post(`${PYTHON_SERVICE_URL}/api/sensor`, data.data);
        
        log(`✅ Sensor data stored`, 'success');
        
        // Send acknowledgement
        ws.send(JSON.stringify({
            event: 'ack',
            message: 'Sensor data received',
            timestamp: formatTime()
        }));
        
    } catch (error) {
        log(`❌ Sensor storage error: ${error.message}`, 'error');
    }
}

async function handleStatusUpdate(ws, data, robotId) {
    try {
        log(`📍 Status update: ${data.status}`, 'status');
        
        // Forward to Python backend
        await axios.post(`${PYTHON_SERVICE_URL}/api/status/update`, {
            status: data.status
        });
        
        log(`✅ Status updated`, 'success');
        
    } catch (error) {
        log(`❌ Status update error: ${error.message}`, 'error');
    }
}

// ============ LOGGING UTILITY ============

function log(message, type = 'info') {
    const colors = {
        'info': '🔵',
        'success': '🟢',
        'warning': '🟡',
        'error': '🔴',
        'receive': '📥',
        'send': '📤',
        'connect': '🔌',
        'disconnect': '❌',
        'voice': '🎤',
        'sensor': '📊',
        'status': '📍'
    };
    
    const color = colors[type] || '⚪';
    console.log(`[${formatTime()}] ${color} ${message}`);
}

// ============ UPGRADE HTTP TO WEBSOCKET ============

const server = app.listen(PORT, () => {
    console.log('\n╔════════════════════════════════════╗');
    console.log('║   🤖 NABI GATEWAY SERVER v1.0      ║');
    console.log('║   Node.js WebSocket Gateway        ║');
    console.log('╚════════════════════════════════════╝\n');
    
    log(`✓ HTTP Server running on port ${PORT}`, 'success');
    log(`✓ Python backend: ${PYTHON_SERVICE_URL}`, 'info');
    log(`✓ WebSocket path: ws://localhost:${PORT}/ws`, 'info');
    log(`✓ Web dashboard: http://localhost:${PORT}`, 'info');
    console.log('');
});

server.on('upgrade', (request, socket, head) => {
    if (request.url === '/ws') {
        wss.handleUpgrade(request, socket, head, (ws) => {
            wss.emit('connection', ws, request);
        });
    } else {
        socket.destroy();
    }
});

// ============ GRACEFUL SHUTDOWN ============

process.on('SIGINT', () => {
    log('Shutting down gracefully...', 'warning');
    wss.close(() => {
        log('WebSocket server closed', 'info');
    });
    server.close(() => {
        log('HTTP server closed', 'info');
        process.exit(0);
    });
});

// ============ EXPORT ============

module.exports = { app, wss, server };