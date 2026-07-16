from flask import Flask, render_template, request, jsonify
from flask_cors import CORS
from flask_socketio import SocketIO, emit, join_room, leave_room
from datetime import datetime
import json
import os

app = Flask(__name__)
CORS(app)

# SocketIO Setup
socketio = SocketIO(app, cors_allowed_origins="*")

# Database giả (In-Memory)
database = {
    "robot_status": "online",
    "last_update": None,
    "sensor_history": [],
    "commands": [],
    "connected_clients": 0
}

# Connected clients tracking
connected_robots = {}

# ============ UTILITY FUNCTIONS ============

def log_message(message, msg_type="info"):
    """Log messages with timestamp"""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    emoji = {
        "info": "ℹ️",
        "success": "✅",
        "error": "❌",
        "websocket": "🔌",
        "sensor": "📊",
        "command": "⚡"
    }
    print(f"[{timestamp}] {emoji.get(msg_type, '•')} {message}")

# ============ HTTP ENDPOINTS ============

@app.route('/', methods=['GET'])
def home():
    """Trang chủ"""
    return jsonify({
        "status": "online",
        "message": "🤖 Nabi Robot Backend Server (WebSocket)",
        "version": "2.0.0",
        "timestamp": datetime.now().isoformat(),
        "websocket": "Enabled"
    })

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    log_message("Health check requested", "info")
    
    return jsonify({
        "status": "ok",
        "message": "Server is running",
        "websocket_connected": len(connected_robots),
        "timestamp": datetime.now().isoformat()
    }), 200

# ============ SENSOR DATA ENDPOINTS ============

@app.route('/api/sensor', methods=['POST'])
def receive_sensor_data():
    """HTTP endpoint để nhận sensor data (fallback)"""
    try:
        data = request.get_json()
        log_message(f"Sensor data received via HTTP: {data}", "sensor")
        
        sensor_entry = {
            "timestamp": datetime.now().isoformat(),
            "data": data
        }
        database["sensor_history"].append(sensor_entry)
        
        if len(database["sensor_history"]) > 100:
            database["sensor_history"].pop(0)
        
        database["last_update"] = datetime.now().isoformat()
        
        # Broadcast to all connected clients
        socketio.emit('sensor_update', sensor_entry, broadcast=True)
        
        return jsonify({
            "status": "success",
            "message": "Sensor data received",
            "data_id": len(database["sensor_history"])
        }), 200
        
    except Exception as e:
        log_message(f"Error in sensor endpoint: {e}", "error")
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/api/sensor/latest', methods=['GET'])
def get_latest_sensor():
    """Lấy sensor data mới nhất"""
    if database["sensor_history"]:
        return jsonify(database["sensor_history"][-1]), 200
    else:
        return jsonify({"status": "no_data"}), 404

@app.route('/api/sensor/history', methods=['GET'])
def get_sensor_history():
    """Lấy sensor history"""
    limit = request.args.get('limit', 10, type=int)
    history = database["sensor_history"][-limit:]
    
    return jsonify({
        "count": len(history),
        "data": history
    }), 200

# ============ COMMAND ENDPOINTS ============

@app.route('/api/command', methods=['GET'])
def get_command():
    """HTTP endpoint để robot polling commands (fallback)"""
    command = {
        "action": "idle",
        "duration": 0,
        "speed": 0,
        "timestamp": datetime.now().isoformat()
    }
    
    if database["commands"]:
        command = database["commands"].pop(0)
        log_message(f"Command sent via HTTP: {command}", "command")
    
    return jsonify(command), 200

@app.route('/api/command/send', methods=['POST'])
def send_command():
    """Gửi command"""
    try:
        data = request.get_json()
        log_message(f"Command received: {data}", "command")
        
        command = {
            "action": data.get("action", "idle"),
            "duration": data.get("duration", 0),
            "speed": data.get("speed", 0),
            "timestamp": datetime.now().isoformat()
        }
        
        database["commands"].append(command)
        
        # Broadcast command to all connected robots
        socketio.emit('robot_command', command, broadcast=True)
        
        return jsonify({
            "status": "success",
            "message": "Command queued",
            "command": command
        }), 200
        
    except Exception as e:
        log_message(f"Error in command endpoint: {e}", "error")
        return jsonify({"status": "error", "message": str(e)}), 400

# ============ STATUS ENDPOINTS ============

@app.route('/api/status', methods=['GET'])
def get_status():
    """Lấy robot status"""
    return jsonify({
        "robot_status": database["robot_status"],
        "last_update": database["last_update"],
        "sensor_count": len(database["sensor_history"]),
        "pending_commands": len(database["commands"]),
        "websocket_clients": len(connected_robots),
        "server_time": datetime.now().isoformat()
    }), 200

@app.route('/api/status/update', methods=['POST'])
def update_status():
    """Cập nhật robot status"""
    try:
        data = request.get_json()
        database["robot_status"] = data.get("status", "unknown")
        database["last_update"] = datetime.now().isoformat()
        
        # Broadcast status update
        socketio.emit('status_update', {
            "status": database["robot_status"],
            "timestamp": database["last_update"]
        }, broadcast=True)
        
        return jsonify({"status": "success"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 400

# ============ DEBUG ENDPOINTS ============

@app.route('/api/debug', methods=['GET'])
def debug_info():
    """Debug info"""
    return jsonify({
        "database": database,
        "connected_robots": len(connected_robots),
        "server_time": datetime.now().isoformat()
    }), 200

@app.route('/api/debug/reset', methods=['POST'])
def debug_reset():
    """Reset database"""
    global database
    database = {
        "robot_status": "online",
        "last_update": None,
        "sensor_history": [],
        "commands": [],
        "connected_clients": 0
    }
    log_message("Database reset!", "success")
    return jsonify({"status": "ok"}), 200

# ============ WEBSOCKET EVENTS ============

@socketio.on('connect')
def handle_connect():
    """Robot/Client connects"""
    client_id = request.sid
    connected_robots[client_id] = {
        "connected_at": datetime.now().isoformat(),
        "type": "unknown"
    }
    
    log_message(f"Client connected: {client_id} (Total: {len(connected_robots)})", "websocket")
    
    # Send welcome message
    emit('welcome', {
        "message": "Connected to Nabi Backend",
        "client_id": client_id,
        "timestamp": datetime.now().isoformat()
    })
    
    # Broadcast connection count
    socketio.emit('client_count', {"count": len(connected_robots)}, broadcast=True)

@socketio.on('disconnect')
def handle_disconnect():
    """Client disconnects"""
    client_id = request.sid
    if client_id in connected_robots:
        del connected_robots[client_id]
    
    log_message(f"Client disconnected: {client_id} (Total: {len(connected_robots)})", "websocket")
    
    # Broadcast updated count
    socketio.emit('client_count', {"count": len(connected_robots)}, broadcast=True)

@socketio.on('sensor_data')
def handle_sensor_data(data):
    """Receive sensor data from robot"""
    client_id = request.sid
    connected_robots[client_id]["type"] = "robot"
    
    log_message(f"Sensor data from robot: {data}", "sensor")
    
    sensor_entry = {
        "timestamp": datetime.now().isoformat(),
        "data": data,
        "robot_id": client_id
    }
    
    database["sensor_history"].append(sensor_entry)
    if len(database["sensor_history"]) > 100:
        database["sensor_history"].pop(0)
    
    database["last_update"] = datetime.now().isoformat()
    
    # Broadcast sensor data to all clients (web dashboard)
    emit('sensor_update', sensor_entry, broadcast=True, include_self=False)
    
    # Acknowledge to sender
    emit('ack', {"message": "Sensor data received"})

@socketio.on('send_command')
def handle_send_command(data):
    """Send command to robot"""
    client_id = request.sid
    
    log_message(f"Command request: {data}", "command")
    
    command = {
        "action": data.get("action", "idle"),
        "duration": data.get("duration", 0),
        "speed": data.get("speed", 0),
        "timestamp": datetime.now().isoformat()
    }
    
    # Broadcast command to all robots
    emit('robot_command', command, broadcast=True, include_self=False)
    
    # Acknowledge to sender
    emit('ack', {"message": "Command sent"})

@socketio.on('request_status')
def handle_request_status():
    """Client requests current status"""
    status = {
        "robot_status": database["robot_status"],
        "last_update": database["last_update"],
        "sensor_count": len(database["sensor_history"]),
        "pending_commands": len(database["commands"]),
        "connected_clients": len(connected_robots),
        "timestamp": datetime.now().isoformat()
    }
    
    emit('status_response', status)

@socketio.on('heartbeat')
def handle_heartbeat():
    """Heartbeat/ping from client"""
    emit('heartbeat_ack', {"timestamp": datetime.now().isoformat()})

@socketio.on('identify')
def handle_identify(data):
    """Client identifies itself (robot or dashboard)"""
    client_id = request.sid
    client_type = data.get("type", "unknown")
    
    if client_id in connected_robots:
        connected_robots[client_id]["type"] = client_type
    
    log_message(f"Client identified as {client_type}: {client_id}", "websocket")
    
    emit('identification_ack', {"type": client_type})

# ============ ERROR HANDLERS ============

@app.errorhandler(404)
def not_found(error):
    return jsonify({"status": "error", "message": "Endpoint not found"}), 404

@app.errorhandler(500)
def server_error(error):
    return jsonify({"status": "error", "message": "Internal server error"}), 500

# ============ MAIN ============

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    
    print("\n" + "="*50)
    print("🤖 NABI ROBOT BACKEND SERVER v2.0")
    print("="*50)
    print(f"✓ Running on port {port}")
    print(f"✓ WebSocket: Enabled")
    print(f"✓ Start time: {datetime.now().isoformat()}")
    print("="*50 + "\n")
    
    socketio.run(
        app,
        host='0.0.0.0',
        port=port,
        debug=False,
        allow_unsafe_werkzeug=True
    )