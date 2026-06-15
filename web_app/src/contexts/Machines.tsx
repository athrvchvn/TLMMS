import { db, rtdb, subscribeNodeStates } from "@/lib/firebase";
import type { NodeState, Machine as MachineConfig } from "@/lib/firebase";
import { collection, onSnapshot, query, orderBy } from "firebase/firestore";
import { createContext, useContext, useEffect, useMemo, useState } from "react";

export interface UsageLog {
  id: string;
  user_name: string;
  roll_number: string;
  machine_id: string;
  machine_name: string;
  start_time: Date;
  end_time: Date | null;
  duration_s: number | null;
  end_reason: string;
  status: 'active' | 'completed' | 'error';
}

type Context = {
  logs:        UsageLog[];
  machines:    MachineConfig[];
  nodeStates:  Record<string, NodeState>;
};

const machinesContext = createContext<Context>({} as any);

export function useMachines() {
  return useContext(machinesContext);
}

export default function MachinesProvider({ children }: { children: React.ReactNode }) {
  const [logs,       setLogs]       = useState<UsageLog[]>([]);
  const [machines,   setMachines]   = useState<MachineConfig[]>([]);
  const [nodeStates, setNodeStates] = useState<Record<string, NodeState>>({});

  // Firestore: session logs (sessions collection)
  const sessionsCol = useMemo(() => collection(db, "sessions"), []);
  useEffect(() => {
    const q = query(sessionsCol, orderBy("start_time", "desc"));
    const unsub = onSnapshot(q, snap => {
      const data: UsageLog[] = snap.docs.map(d => {
        const raw = d.data();
        return {
          id:          d.id,
          user_name:   raw.user_name   ?? "",
          roll_number: raw.roll_number ?? "",
          machine_id:  raw.machine_id  ?? "",
          machine_name: raw.machine_name ?? "",
          start_time:  raw.start_time?.toDate() ?? new Date(),
          end_time:    raw.end_time?.toDate()   ?? null,
          duration_s:  raw.duration_s ?? null,
          end_reason:  raw.end_reason ?? "",
          status:      raw.status     ?? "completed",
        };
      });
      setLogs(data);
    });
    return () => unsub();
  }, []);

  // Firestore: machine config list
  const machinesCol = useMemo(() => collection(db, "machines"), []);
  useEffect(() => {
    const unsub = onSnapshot(machinesCol, snap => {
      const data: MachineConfig[] = snap.docs
        .map(d => ({ ...d.data(), id: d.id }) as MachineConfig)
        .sort((a, b) => parseInt(a.id) - parseInt(b.id));
      setMachines(data);
    });
    return () => unsub();
  }, []);

  // RTDB: real-time node states
  useEffect(() => {
    const unsub = subscribeNodeStates(states => setNodeStates(states));
    return () => unsub();
  }, []);

  return (
    <machinesContext.Provider value={{ logs, machines, nodeStates }}>
      {children}
    </machinesContext.Provider>
  );
}
