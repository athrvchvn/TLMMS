from flask import Flask, render_template, request, jsonify
from flask_cors import CORS
import traceback
from firebase_config import get_user_by_roll
import requests
from models import verify_pin, hash_pin
import time
import serial
import json


app = Flask(__name__)
CORS(app)

try:
    esp32_serial = serial.Serial('/dev/ttyUSB0', 115200, timeout=10)
    print("ESP32 serial connection established")
except Exception as e:
    print(f"Failed to connect to ESP32: {e}")
    esp32_serial = None


def format_card_id_for_display(uid_string):
    """Formats the UID string for display, e.g., '12345678' -> '12:34:56:78'"""
    if uid_string and isinstance(uid_string, str) and len(uid_string) == 8:
        return ":".join([uid_string[i:i+2] for i in range(0, 8, 2)])
    return uid_string # Return as is if not a 8-char hex string

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/begin_issuing', methods=['POST'])
def begin_issuing():
    try:
        print("Beginning card issuing process...")
        
        return jsonify({
            'success': True,
            'message': 'Card issuing process started. Please enter your roll number.'
        })
            
    except Exception as e:
        print(f"Begin issuing error: {e}")
        traceback.print_exc()
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/write_card1', methods=['POST'])
def write_card1():
    try:
        data = request.json
        roll_number = data.get('roll_number')

        print(f"Write card request for roll: {roll_number}")

        if not roll_number:
            return jsonify({'success': False, 'error': 'Roll number required'})

        # Get user data
        user = get_user_by_roll(roll_number)
        if not user:
            return jsonify({'success': False, 'error': 'User not found'})

        # Prepare machine flags (for 5 machines)
        accessible_machines = user.get('accessible_machines', [])
        machine_flags = '0' * 5

        machine_id_map = {
            'Creality 3D Printer': 1,
            'Fracktal Laser Cutter': 2,
            'Soldering Station 1': 3,
            'Soldering Station 2': 4,
            'Bosche Grinder': 5,
        }

        print(f"Accessible machines: {accessible_machines}")

        for machine in accessible_machines:
            try:
                if isinstance(machine, str):
                    machine_id = machine_id_map.get(machine)
                else:
                    machine_id = int(machine)

                if machine_id and 1 <= machine_id <= 5:
                    bit_position = machine_id - 1
                    machine_flags = machine_flags[:bit_position] + '1' + machine_flags[bit_position+1:]
                    print(f"Set bit {bit_position} for machine {machine}")

            except (ValueError, TypeError) as e:
                print(f"Error processing machine {machine}: {e}")
                continue

        print(f"Final machine flags: {machine_flags}")

        raw_serial_string = f"{roll_number}{machine_flags}\n"
        esp32_serial.write(raw_serial_string.encode("ascii"))
        esp32_serial.flush()
        print(f"Sent to ESP32: {raw_serial_string.strip()}")

        # Read ESP32 responses for 10 seconds
        for i in range(100):  # 10 seconds worth of checks
            if esp32_serial.in_waiting > 0:
                response = esp32_serial.readline().decode('ascii').strip()
                print(f"ESP32 says: {response}")
            time.sleep(0.1)

        time.sleep(5)  # Wait for write and dispense

        # Update database with card info
        try:
            from firebase_config import db
            user_ref = db.collection('users').document(roll_number)
            user_ref.update({
                'card_id': roll_number,
                'card_written_at': time.time()
            })
            print(f"Database updated for user {roll_number}")
        except Exception as db_error:
            print(f"Database update error: {db_error}")

        return jsonify({
            'success': True,
            'message': 'Card written and dispensed successfully',
            'card_id': roll_number
        })

    except Exception as e:
        print(f"Write card error: {e}")
        traceback.print_exc()
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/check_user', methods=['POST'])
def check_user():
    """Check if user exists in database"""
    try:
        data = request.json
        roll_number = data.get('roll_number')

        if not roll_number:
            return jsonify({'exists': False, 'error': 'Roll number required'})

        user = get_user_by_roll(roll_number)
        if user:
            return jsonify({
                'exists': True,
                'has_pin': bool(user.get('pin_hash')),
                'user_data': {
                    'name': user.get('name', ''),
                    'branch': user.get('branch', ''),
                    'year': user.get('year', '')
                }
            })
        else:
            return jsonify({'exists': False})

    except Exception as e:
        print(f"Check user error: {e}")
        traceback.print_exc()
        return jsonify({'exists': False, 'error': str(e)})

@app.route('/api/create_pin', methods=['POST'])
def create_pin():
    """Create new PIN for user with null PIN"""
    try:
        data = request.json
        roll_number = data.get('roll_number')
        new_pin = data.get('pin')
        confirm_pin = data.get('confirm_pin')

        if not roll_number or not new_pin or not confirm_pin:
            return jsonify({'success': False, 'error': 'All fields required'})

        if new_pin != confirm_pin:
            return jsonify({'success': False, 'error': 'PINs do not match'})

        if len(new_pin) != 4 or not new_pin.isdigit():
            return jsonify({'success': False, 'error': 'PIN must be 4 digits'})

        user = get_user_by_roll(roll_number)
        if not user:
            return jsonify({'success': False, 'error': 'User not found'})

        if user.get('pin_hash'):
            return jsonify({'success': False, 'error': 'User already has a PIN'})

        # Update user with new PIN
        from firebase_config import db
        user_ref = db.collection('users').document(roll_number)
        user_ref.update({
            'pin_hash': hash_pin(new_pin),
            'last_updated': time.time()
        })

        return jsonify({
            'success': True,
            'message': 'PIN created successfully',
            'user_data': {
                'name': user.get('name', ''),
                'branch': user.get('branch', ''),
                'year': user.get('year', ''),
                'accessible_machines': user.get('accessible_machines', [])
            }
        })

    except Exception as e:
        print(f"Create PIN error: {e}")
        traceback.print_exc()
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/verify_pin', methods=['POST'])
def verify_pin_route():
    """Verify user PIN"""
    try:
        data = request.json
        roll_number = data.get('roll_number')
        pin = data.get('pin')

        print(f"DEBUG: Verifying PIN for roll: {roll_number}, PIN: {pin}")  # Add this

        if not roll_number or not pin:
            return jsonify({'valid': False, 'error': 'Roll number and PIN required'})

        user = get_user_by_roll(roll_number)
        if user:
            stored_hash = user.get('pin_hash')
            print(f"DEBUG: Stored hash: {stored_hash}")  # Add this
            print(f"DEBUG: New hash: {hash_pin(pin)}")    # Add this
            
            if stored_hash and verify_pin(pin, stored_hash):
                return jsonify({
                    'valid': True,
                    'user_data': {
                        'name': user.get('name', ''),
                        'branch': user.get('branch', ''),
                        'year': user.get('year', ''),
                        'accessible_machines': user.get('accessible_machines', [])
                    }
                })
            else:
                print("DEBUG: PIN verification failed")  # Add this
                return jsonify({'valid': False, 'error': 'Invalid PIN'})
        else:
            print("DEBUG: User not found")  # Add this
            return jsonify({'valid': False, 'error': 'User not found'})

    except Exception as e:
        print(f"Verify PIN error: {e}")
        traceback.print_exc()
        return jsonify({'valid': False, 'error': str(e)})

if __name__ == '__main__':
    try:
        print("RFID Card Station starting...")
        print("Access the interface at: http://localhost:5000")
        
        app.run(host='0.0.0.0', port=5000, debug=True)
        
    except KeyboardInterrupt:
        print("Shutting down...")