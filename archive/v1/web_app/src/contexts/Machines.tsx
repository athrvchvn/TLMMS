import { db } from "@/lib/firebase";
import { addDoc, collection, onSnapshot } from "firebase/firestore";
import { createContext, useContext, useEffect, useMemo, useState } from "react";

export interface UsageLog {
  id: string;
  user_name: string;
  roll_number: string;
  machine: string;
  action: 'access_granted' | 'access_denied' | 'session_start' | 'session_end';
  timestamp: Date;
  duration?: number; // in minutes
  status: 'success' | 'failed' | 'warning';
}

export interface Machine {
  name: string;
  current: string;
  previous: string;
  past_users: number;
  total_hours: number;
  reserved_by?: string;
  reserved_at?: any;
}

type Context = {
  logs: UsageLog[],
  machines: Machine[],
}

const machinesContext = createContext<Context>({} as any);

export function useMachines() {
  return useContext(machinesContext);
}

export default function MachinesProvider({ children }: { children: React.ReactNode }) {
  const [logs, setLogs] = useState<UsageLog[]>([]);
  const [machines, setMachines] = useState<Machine[]>([]);
  const logsCollection = useMemo(() => collection(db, "logs"), []);
  const machinesCollection = useMemo(() => collection(db, "machines"), []);

  useEffect(() => {
    const unsubscribe = onSnapshot(logsCollection, (querySnapshot) => {
      const data = querySnapshot.docs.map(doc => doc.data()).map(doc => ({ ...doc, timestamp: doc?.timestamp?.toDate(), }));
      setLogs(data.sort((a: any, b: any) => {
        const dateDiff = b.timestamp.getTime() - a.timestamp.getTime();
        if (dateDiff !== 0) return dateDiff;

        return a.user_name.localeCompare(b.user_name);
      }) as any);
    });

    return () => unsubscribe();
  }, []);

  useEffect(() => {
    const unsubscribe = onSnapshot(machinesCollection, (querySnapshot) => {
      const data = querySnapshot.docs.map(doc => doc.data()).map(doc => ({
        ...doc,
        current: doc.current.toString(),
        previous: doc.previous.toString(),
      }));

      setMachines(data as any);
    });

    return () => unsubscribe();
  }, []);

  const values = {
    logs,
    machines,
  };

  return (
    <machinesContext.Provider value={values}>
      {children}
    </machinesContext.Provider>
  )
}
