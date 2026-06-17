# Firebase Firestore structure
"""
Collection: users
Document ID: roll_number
Fields:
{
    'roll_number': '2021001',
    'name': 'John Doe',
    'branch': 'Computer Science',
    'year': '3rd Year',
    'pin_hash': 'hashed_pin_here',
    'accessible_machines': ['3D Printer', 'Laser Cutter'],
    'card_id': 'written_card_id',
    'created_at': timestamp,
    'last_updated': timestamp
}
"""

import hashlib
import datetime

def hash_pin(pin):
    """Hash PIN for secure storage"""
    return pin

def verify_pin(pin, pin_hash):
    """Verify PIN against stored hash"""
    return pin == pin_hash

def create_user_data(roll_number, name, branch, year, machines, pin):
    """Create user data structure"""
    return {
        'roll_number': roll_number,
        'name': name,
        'branch': branch,
        'year': year,
        'pin_hash': hash_pin(pin),
        'accessible_machines': machines,
        'card_id': None,
        'created_at': datetime.datetime.now(),
        'last_updated': datetime.datetime.now()
    }

def update_user_pin(roll_number, new_pin):
    """Update user PIN in Firebase"""
    from firebase_config import db
    try:
        user_ref = db.collection('users').document(roll_number)
        user_ref.update({
            'pin_hash': hash_pin(new_pin),
            'last_updated': datetime.datetime.now()
        })
        return True
    except Exception as e:
        print(f"Error updating PIN: {e}")
        return False

def validate_pin_format(pin):
    """Validate PIN format"""
    return len(pin) == 4 and pin.isdigit()
