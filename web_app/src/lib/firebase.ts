import { initializeApp } from 'firebase/app';
import {
  getFirestore, collection, doc, getDocs, getDoc,
  updateDoc, setDoc, addDoc, deleteDoc,
  query, orderBy, serverTimestamp
} from 'firebase/firestore';
import { getAuth } from 'firebase/auth';
import { getDatabase, ref, set, onValue, off } from 'firebase/database';

const firebaseConfig = {
  apiKey: "AIzaSyAGcNXJeJRhdWvKhTjUG3d5rpnT9xkXTHM",
  authDomain: "lab-access-f569b.firebaseapp.com",
  databaseURL: "https://lab-access-f569b-default-rtdb.firebaseio.com",
  projectId: "lab-access-f569b",
  storageBucket: "lab-access-f569b.firebasestorage.app",
  messagingSenderId: "911923971560",
  appId: "1:911923971560:web:13168ac1a680934948d05d",
  measurementId: "G-SW9L9LWR7G"
};

const app = initializeApp(firebaseConfig);
export const db  = getFirestore(app);
export const auth = getAuth(app);
export const rtdb = getDatabase(app);

// ---------------------------------------------------------------------------
// V2 Data interfaces
// ---------------------------------------------------------------------------

export interface User {
  roll_number: string;
  name: string;
  email: string;
  branch?: string;
  year?: string;
  pin_hash: string;
  card_uid: string | null;
  machine_permissions: number;  // uint32 bitmask
  issued_at: any;
  issued_by?: string;
  is_active: boolean;
  last_seen?: any;
}

export interface Machine {
  id: string;
  name: string;
  session_limit_minutes: number;
  relay_active_high: boolean;
  machine_active: boolean;
  location?: string;
}

export interface NodeState {
  state: 'offline' | 'idle' | 'active' | 'denied' | 'disabled' | 'error' | 'ota';
  roll_number: string | null;
  session_start: number | null;
  firmware_version: string;
  ip_address: string;
  last_seen: number;
  rssi: number;
}

export interface Session {
  id: string;
  machine_id: string;
  machine_name: string;
  roll_number: string;
  user_name: string;
  start_time: any;
  end_time: any | null;
  duration_s: number | null;
  end_reason: string;
  status: 'active' | 'completed' | 'error';
}

// ---------------------------------------------------------------------------
// PIN hashing (web app side — SHA-256 for Firestore storage)
// Note: card PIN hash uses HMAC-SHA256 (done by kiosk); this is for web display only
// ---------------------------------------------------------------------------
export const hashPin = async (pin: string): Promise<string> => {
  const encoder = new TextEncoder();
  const data = encoder.encode(pin);
  const hash = await crypto.subtle.digest('SHA-256', data);
  return Array.from(new Uint8Array(hash))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
};

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------
export const getUsers = async (): Promise<User[]> => {
  const q = query(collection(db, 'users'), orderBy('issued_at', 'desc'));
  const snap = await getDocs(q);
  return snap.docs.map(d => ({ ...d.data(), roll_number: d.id }) as User);
};

export const addUser = async (userData: {
  roll_number: string;
  name: string;
  email: string;
  branch?: string;
  year?: string;
}): Promise<void> => {
  // FIX: use setDoc with roll_number as document ID (was addDoc — wrong)
  await setDoc(doc(db, 'users', userData.roll_number), {
    ...userData,
    pin_hash: '',
    card_uid: null,
    machine_permissions: 0,
    is_active: true,
    issued_at: serverTimestamp(),
  }, { merge: true });
};

export const updateUserPermissions = async (rollNumber: string, permMask: number): Promise<void> => {
  await updateDoc(doc(db, 'users', rollNumber), {
    machine_permissions: permMask,
    last_updated: serverTimestamp(),
  });
};

// ---------------------------------------------------------------------------
// Machines
// ---------------------------------------------------------------------------
export const getMachines = async (): Promise<Machine[]> => {
  const snap = await getDocs(collection(db, 'machines'));
  const machines = snap.docs.map(d => ({ ...d.data(), id: d.id }) as Machine);
  return machines.sort((a, b) => parseInt(a.id) - parseInt(b.id));
};

export const updateMachineConfig = async (machineId: string, updates: Partial<Machine>): Promise<void> => {
  const { id: _, ...data } = updates as any;
  await updateDoc(doc(db, 'machines', machineId), data);
};

// ---------------------------------------------------------------------------
// Remote stop command (writes to RTDB — bridge.py forwards to ESP32 via MQTT)
// ---------------------------------------------------------------------------
export const issueRemoteStop = async (machineId: string, adminUid: string): Promise<void> => {
  const cmdRef = ref(rtdb, `mms/commands/${machineId}`);
  await set(cmdRef, {
    command:      'stop',
    issued_by:    adminUid,
    timestamp:    Math.floor(Date.now() / 1000),
    acknowledged: false,
  });
};

// ---------------------------------------------------------------------------
// OTA trigger (super admin only)
// ---------------------------------------------------------------------------
export const triggerOta = async (targetVersion: string, firmwareUrl: string): Promise<void> => {
  const otaRef = ref(rtdb, 'mms/ota');
  await set(otaRef, {
    target_version: targetVersion,
    firmware_url:   firmwareUrl,
    released_at:    Math.floor(Date.now() / 1000),
  });
};

// ---------------------------------------------------------------------------
// Card revocation
// ---------------------------------------------------------------------------
export const revokeCard = async (cardUid: string, rollNumber: string,
                                  revokedBy: string, reason: string): Promise<void> => {
  await setDoc(doc(db, 'revoked', cardUid), {
    uid:        cardUid,
    roll_number: rollNumber,
    revoked_at:  serverTimestamp(),
    revoked_by:  revokedBy,
    reason,
  });
  // Also mark card_uid null on user to indicate card is invalid
  await updateDoc(doc(db, 'users', rollNumber), { card_uid: null });
};

export const unrevokeCard = async (cardUid: string): Promise<void> => {
  await deleteDoc(doc(db, 'revoked', cardUid));
};

// ---------------------------------------------------------------------------
// RTDB: real-time node state subscription
// ---------------------------------------------------------------------------
export const subscribeNodeStates = (
  callback: (states: Record<string, NodeState>) => void
): (() => void) => {
  const nodesRef = ref(rtdb, 'mms/nodes');
  const listener = onValue(nodesRef, snap => {
    callback((snap.val() ?? {}) as Record<string, NodeState>);
  });
  return () => off(nodesRef, 'value', listener);
};

// ---------------------------------------------------------------------------
// Machine permission bitmask helpers
// ---------------------------------------------------------------------------
export const permMaskToIds = (mask: number): number[] => {
  const ids: number[] = [];
  for (let i = 0; i < 32; i++) {
    if ((mask >> i) & 1) ids.push(i + 1);
  }
  return ids;
};

export const idsToPermMask = (ids: number[]): number => {
  return ids.reduce((mask, id) => mask | (1 << (id - 1)), 0);
};
