
import { initializeApp } from 'firebase/app';
import { getFirestore, collection, doc, getDocs, updateDoc, addDoc, query, orderBy } from 'firebase/firestore';
import { getAuth } from "firebase/auth";

const firebaseConfig = {
  apiKey: "AIzaSyAGcNXJeJRhdWvKhTjUG3d5rpnT9xkXTHM",
  authDomain: "lab-access-f569b.firebaseapp.com",
  projectId: "lab-access-f569b",
  storageBucket: "lab-access-f569b.firebasestorage.app",
  messagingSenderId: "911923971560",
  appId: "1:911923971560:web:13168ac1a680934948d05d",
  measurementId: "G-SW9L9LWR7G"
};

const app = initializeApp(firebaseConfig);
export const db = getFirestore(app);
export const auth = getAuth(app);

export interface User {
  roll_number: string;
  name: string;
  branch: string;
  year: string;
  pin_hash: string;
  accessible_machines: string[];
  card_id: string | null;
  created_at: any;
  last_updated: any;
}

export const hashPin = async (pin: string): Promise<string> => {
  const encoder = new TextEncoder();
  const data = encoder.encode(pin);
  const hash = await crypto.subtle.digest('SHA-256', data);
  return Array.from(new Uint8Array(hash))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
};

export const getUsers = async (): Promise<User[]> => {
  try {
    const usersRef = collection(db, 'users');
    const q = query(usersRef, orderBy('created_at', 'desc'));
    const querySnapshot = await getDocs(q);
    return querySnapshot.docs.map(doc => ({
      ...doc.data(),
      roll_number: doc.id
    })) as User[];
  } catch (error) {
    console.error('Error fetching users:', error);
    return [];
  }
};

export const updateUserAccess = async (rollNumber: string, accessibleMachines: string[]): Promise<void> => {
  try {
    const userRef = doc(db, 'users', rollNumber);
    await updateDoc(userRef, {
      accessible_machines: accessibleMachines,
      last_updated: new Date()
    });
  } catch (error) {
    console.error('Error updating user access:', error);
    throw error;
  }
};

export const addUser = async (userData: Omit<User, 'created_at' | 'last_updated'>): Promise<void> => {
  try {
    const userRef = doc(db, 'users', userData.roll_number);
    await addDoc(collection(db, 'users'), {
      ...userData,
      created_at: new Date(),
      last_updated: new Date()
    });
  } catch (error) {
    console.error('Error adding user:', error);
    throw error;
  }
};

export const mockUsers: User[] = [
  {
    roll_number: '240003021',
    name: 'Atharva Chavan',
    branch: 'Mechanical Engineering',
    year: '1st Year',
    pin_hash: 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
    accessible_machines: ['3D Printer', 'CNC Machine', 'Laser Cutter'],
    card_id: 'CARD001',
    created_at: new Date('2024-01-15'),
    last_updated: new Date('2024-06-20')
  },
  {
    roll_number: '240002018',
    name: 'Dhruv Bharadwaj',
    branch: 'Electrical Engineering',
    year: '1st Year',
    pin_hash: 'a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3',
    accessible_machines: ['Oscilloscope', 'Function Generator', 'Soldering Station'],
    card_id: 'CARD002',
    created_at: new Date('2024-01-20'),
    last_updated: new Date('2024-06-18')
  },
  {
    roll_number: '240001015',
    name: 'Priya Sharma',
    branch: 'Computer Science',
    year: '1st Year',
    pin_hash: 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f',
    accessible_machines: ['3D Printer', 'Laser Cutter'],
    card_id: 'CARD003',
    created_at: new Date('2024-02-10'),
    last_updated: new Date('2024-06-15')
  },
  {
    roll_number: '240001032',
    name: 'Arjun Patel',
    branch: 'Computer Science',
    year: '1st Year',
    pin_hash: 'c6ee9e33cf5c6715a1d148fd73f7318884b41adcb916021e2bc0e800a5c5dd97',
    accessible_machines: ['Laser Cutter', '3D Printer', 'Soldering Station'],
    card_id: null,
    created_at: new Date('2024-01-25'),
    last_updated: new Date('2024-06-22')
  },
  {
    roll_number: '240004007',
    name: 'Meera Gupta',
    branch: 'Civil Engineering',
    year: '1st Year',
    pin_hash: 'b03ddf3ca2e714a6548e7495e2a03f5e824eaac9837cd7c3c7e1ac2e7e7e6a28',
    accessible_machines: ['CNC Machine'],
    card_id: 'CARD005',
    created_at: new Date('2024-02-05'),
    last_updated: new Date('2024-06-12')
  },
  {
    roll_number: '240005003',
    name: 'Karthik Reddy',
    branch: 'Chemical Engineering',
    year: '1st Year',
    pin_hash: 'f7c3bc1d808e04732adf679965ccc34ca7ae3441f59d4b1bd7e8b4a2b8f9b7a8',
    accessible_machines: ['Heat Gun', 'Glue Gun'],
    card_id: 'CARD006',
    created_at: new Date('2024-02-12'),
    last_updated: new Date('2024-06-08')
  }
];
