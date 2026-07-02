"""
🤖 NABI ROBOT - Backend Server
Deploy on Railway - Miễn phí
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from datetime import datetime
import json
import os

app = Flask(__name__)
CORS(app)

# Database giả (In-Memory)
# Trong production có thể dùng Database thực
database = {
    "robot_status": "online",
    "last_update": None,
    "sensor_history": [],
    "commands": []
}

# ============ UTILITY FUNCTIONS ============

def log_request(endpoint, method, data=None):
    """Log request để debug"""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {method} {endpoint} - Data: {data}")

def save_to_database(key, value):
    """Lưu dữ liệu vào database giả"""
    database[key] = value

# ============ HEALTH CHECK ============

@app.route('/', methods=['GET'])
def home():
    """Trang chủ - Kiểm tra server hoạt động"""
    return jsonify({
        "status": "online",
        "message": "🤖 Nabi Robot Backend Server",
        "version": "1.0.0",
        "timestamp": datetime.now().isoformat()
    })

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint (AWS/Railway dùng để monitor)"""
    log_request('/health', 'GET')
    
    return jsonify({
        "status": "ok",
        "message": "Server is running",
        "timestamp": datetime.now().isoformat()
    }), 200

# ============ SENSOR DATA ENDPOINTS ============

@app.route('/api/sensor', methods=['POST'])
def receive_sensor_data():
    """
    ESP32 gửi dữ liệu cảm biến lên server
    
    Request JSON:
    {
        "temperature": 28.5,
        "humidity": 60,
        "motion": 1,
        "battery": 85,
        "timestamp": "2024-01-15T10:30:00"
    }
    """
    try:
        data = request.get_json()
        
        log_request('/api/sensor', 'POST', data)
        
        # Lưu vào database
        sensor_entry = {
            "timestamp": datetime.now().isoformat(),
            "data": data
        }
        database["sensor_history"].append(sensor_entry)
        
        # Giới hạn lịch sử (giữ 100 entry cuối)
        if len(database["sensor_history"]) > 100:
            database["sensor_history"].pop(0)
        
        database["last_update"] = datetime.now().isoformat()
        
        # Response
        return jsonify({
            "status": "success",
            "message": "Sensor data received",
            "data_id": len(database["sensor_history"]),
            "server_time": datetime.now().isoformat()
        }), 200
        
    except Exception as e:
        print(f"❌ Error in /api/sensor: {e}")
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 400

@app.route('/api/sensor/latest', methods=['GET'])
def get_latest_sensor():
    """Lấy dữ liệu cảm biến mới nhất"""
    log_request('/api/sensor/latest', 'GET')
    
    if database["sensor_history"]:
        latest = database["sensor_history"][-1]
        return jsonify(latest), 200
    else:
        return jsonify({
            "status": "no_data",
            "message": "Chưa có dữ liệu cảm biến"
        }), 404

@app.route('/api/sensor/history', methods=['GET'])
def get_sensor_history():
    """Lấy tất cả lịch sử cảm biến"""
    log_request('/api/sensor/history', 'GET')
    
    limit = request.args.get('limit', 10, type=int)
    history = database["sensor_history"][-limit:]
    
    return jsonify({
        "count": len(history),
        "data": history
    }), 200

# ============ COMMAND ENDPOINTS ============

@app.route('/api/command', methods=['GET'])
def get_command():
    """
    ESP32 lấy lệnh từ server
    
    Response JSON:
    {
        "action": "move_forward",
        "duration": 1000,
        "speed": 200,
        "timestamp": "2024-01-15T10:30:00"
    }
    """
    log_request('/api/command', 'GET')
    
    # Lệnh mặc định (idle)
    command = {
        "action": "idle",
        "duration": 0,
        "speed": 0,
        "timestamp": datetime.now().isoformat()
    }
    
    # Nếu có lệnh trong database, gửi lệnh đó
    if database["commands"]:
        command = database["commands"].pop(0)
        print(f"📤 Sending command: {command}")
    
    return jsonify(command), 200

@app.route('/api/command/send', methods=['POST'])
def send_command():
    """
    Gửi lệnh từ web dashboard / mobile
    
    Request JSON:
    {
        "action": "move_forward",
        "duration": 2000,
        "speed": 255
    }
    """
    try:
        data = request.get_json()
        
        log_request('/api/command/send', 'POST', data)
        
        command = {
            "action": data.get("action", "idle"),
            "duration": data.get("duration", 0),
            "speed": data.get("speed", 0),
            "timestamp": datetime.now().isoformat()
        }
        
        # Thêm vào queue
        database["commands"].append(command)
        
        return jsonify({
            "status": "success",
            "message": "Command queued",
            "command": command
        }), 200
        
    except Exception as e:
        print(f"❌ Error in /api/command/send: {e}")
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 400

# ============ STATUS ENDPOINTS ============

@app.route('/api/status', methods=['GET'])
def get_status():
    """Lấy trạng thái toàn bộ robot"""
    log_request('/api/status', 'GET')
    
    return jsonify({
        "robot_status": database["robot_status"],
        "last_update": database["last_update"],
        "sensor_count": len(database["sensor_history"]),
        "pending_commands": len(database["commands"]),
        "server_time": datetime.now().isoformat()
    }), 200

@app.route('/api/status/update', methods=['POST'])
def update_status():
    """Robot gửi cập nhật trạng thái"""
    try:
        data = request.get_json()
        
        log_request('/api/status/update', 'POST', data)
        
        database["robot_status"] = data.get("status", "unknown")
        database["last_update"] = datetime.now().isoformat()
        
        return jsonify({
            "status": "success",
            "message": "Status updated"
        }), 200
        
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 400

# ============ MESSAGE ENDPOINTS ============

@app.route('/api/message', methods=['POST'])
def receive_message():
    """Robot gửi tin nhắn"""
    try:
        data = request.get_json()
        msg = data.get("message", "")
        
        log_request('/api/message', 'POST', {"message": msg})
        
        print(f"💬 Message from Nabi: {msg}")
        
        return jsonify({
            "status": "ok",
            "message": "Message received"
        }), 200
        
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": str(e)
        }), 400

# ============ DEBUG / ADMIN ENDPOINTS ============

@app.route('/api/debug', methods=['GET'])
def debug_info():
    """Thông tin debug (chỉ dùng trong dev)"""
    log_request('/api/debug', 'GET')
    
    return jsonify({
        "database": database,
        "server_time": datetime.now().isoformat()
    }), 200

@app.route('/api/debug/reset', methods=['POST'])
def debug_reset():
    """Reset database (chỉ dùng trong dev)"""
    global database
    
    database = {
        "robot_status": "online",
        "last_update": None,
        "sensor_history": [],
        "commands": []
    }
    
    print("🔄 Database reset!")
    
    return jsonify({
        "status": "ok",
        "message": "Database reset"
    }), 200

# ============ ERROR HANDLERS ============

@app.errorhandler(404)
def not_found(error):
    """404 Not Found"""
    return jsonify({
        "status": "error",
        "message": "Endpoint not found",
        "path": request.path
    }), 404

@app.errorhandler(500)
def server_error(error):
    """500 Server Error"""
    return jsonify({
        "status": "error",
        "message": "Internal server error"
    }), 500

# ============ MAIN ============

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    
    print("\n" + "="*50)
    print("🤖 NABI ROBOT BACKEND SERVER")
    print("="*50)
    print(f"✓ Running on port {port}")
    print(f"✓ Flask environment: {os.environ.get('FLASK_ENV', 'production')}")
    print(f"✓ Start time: {datetime.now().isoformat()}")
    print("="*50 + "\n")
    
    app.run(
        host='0.0.0.0',
        port=port,
        debug=False,
        threaded=True
    )