import firebase_admin
from firebase_admin import credentials, firestore

# Download service account key from Firebase Console
cred = credentials.Certificate('./service-account.json')
firebase_admin.initialize_app(cred)

db = firestore.client()

def get_user_by_roll(roll_number):
    """Get user data from Firestore using document ID"""
    try:
        # Since document ID is the roll_number, access directly
        user_ref = db.collection('users').where("roll_number", "==", str(roll_number)).limit(1).stream()
        doc = next(user_ref, None)
        
        if doc:
            return doc.to_dict()
        else:
            print(f"DEBUG: No document found for roll number: {roll_number}")
            return None
            
    except Exception as e:
        print(f"Error getting user by roll: {e}")
        return None

def save_user_pin(roll_number, pin_hash):
    """Save/update user PIN"""
    user_ref = db.collection('users').document(roll_number)
    user_ref.update({'pin_hash': pin_hash})

def create_user(user_data):
    """Create new user in database"""
    user_ref = db.collection('users').document(user_data['roll_number'])
    user_ref.set(user_data)
