import React, { useState } from 'react';
import {
  Monitor, Download, Clock, User, Play, Square,
  AlertCircle, Search, StopCircle, Wifi, WifiOff,
  Settings, Shield
} from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Badge } from './ui/badge';
import {
  Dialog, DialogContent, DialogDescription,
  DialogHeader, DialogTitle, DialogTrigger
} from '@/components/ui/dialog';
import { Label } from '@/components/ui/label';
import { useMachines } from '@/contexts/Machines';
import { useFirebaseData } from '@/contexts/Firebase';
import {
  issueRemoteStop, updateMachineConfig, revokeCard,
  type Machine as MachineConfig, type NodeState, type User as UserType
} from '@/lib/firebase';
import { toast } from '@/hooks/use-toast';

type Props = {
  users: UserType[];
};

const STATE_COLORS: Record<string, string> = {
  active:   'bg-green-100 text-green-800 border-green-200',
  idle:     'bg-gray-100 text-gray-600 border-gray-200',
  offline:  'bg-red-100 text-red-800 border-red-200',
  disabled: 'bg-purple-100 text-purple-800 border-purple-200',
  error:    'bg-orange-100 text-orange-800 border-orange-200',
  ota:      'bg-blue-100 text-blue-800 border-blue-200',
};

function formatDuration(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h}h ${m}m`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

function SessionTimer({ startTs }: { startTs: number }) {
  const [elapsed, setElapsed] = React.useState(Math.floor(Date.now() / 1000) - startTs);
  React.useEffect(() => {
    const id = setInterval(() => setElapsed(Math.floor(Date.now() / 1000) - startTs), 1000);
    return () => clearInterval(id);
  }, [startTs]);
  return <span>{formatDuration(elapsed)}</span>;
}

// ---------------------------------------------------------------------------
// Machine card with real-time RTDB state
// ---------------------------------------------------------------------------
function MachineCard({
  machine, state, users, adminUid
}: {
  machine: MachineConfig;
  state?: NodeState;
  users: UserType[];
  adminUid: string;
}) {
  const [stopping,   setStopping]   = useState(false);
  const [editOpen,   setEditOpen]   = useState(false);
  const [limitInput, setLimitInput] = useState(String(machine.session_limit_minutes));
  const [activeInput, setActiveInput] = useState(machine.machine_active);

  const st = state?.state ?? 'offline';
  const currentRoll = state?.roll_number ?? null;
  const currentUser = currentRoll ? users.find(u => u.roll_number === currentRoll) : null;

  const handleRemoteStop = async () => {
    setStopping(true);
    try {
      await issueRemoteStop(machine.id, adminUid);
      toast({ title: `Stop command sent to ${machine.name}` });
    } catch {
      toast({ title: "Failed to send stop command", variant: "destructive" });
    } finally {
      setStopping(false);
    }
  };

  const handleSaveConfig = async () => {
    try {
      await updateMachineConfig(machine.id, {
        session_limit_minutes: parseInt(limitInput) || machine.session_limit_minutes,
        machine_active: activeInput,
      });
      setEditOpen(false);
      toast({ title: `${machine.name} config updated` });
    } catch {
      toast({ title: "Config update failed", variant: "destructive" });
    }
  };

  return (
    <Card className="relative overflow-hidden">
      {/* Online/offline indicator strip */}
      <div className={`absolute top-0 left-0 right-0 h-1 ${
        st === 'offline' ? 'bg-red-500' :
        st === 'active'  ? 'bg-green-500' :
        st === 'idle'    ? 'bg-gray-400' : 'bg-orange-400'
      }`} />

      <CardHeader className="pt-4 pb-2">
        <div className="flex items-start justify-between gap-2">
          <div className="flex items-center gap-2">
            {st === 'offline' ? <WifiOff className="w-4 h-4 text-red-500" /> :
                                <Wifi className="w-4 h-4 text-green-500" />}
            <CardTitle className="text-base">{machine.name}</CardTitle>
          </div>
          <Badge className={STATE_COLORS[st] ?? STATE_COLORS.offline}>
            {st}
          </Badge>
        </div>
        <p className="text-xs text-muted-foreground">
          Machine ID: {machine.id}
          {state?.firmware_version ? ` · v${state.firmware_version}` : ''}
          {state?.ip_address ? ` · ${state.ip_address}` : ''}
        </p>
      </CardHeader>

      <CardContent className="space-y-3">
        {/* Active session info */}
        {st === 'active' && currentRoll && (
          <div className="bg-green-50 dark:bg-green-950 rounded-md p-3 space-y-1">
            <div className="flex items-center gap-2 text-sm font-medium text-green-800 dark:text-green-200">
              <User className="w-4 h-4" />
              <span>{currentUser?.name ?? currentRoll}</span>
            </div>
            <div className="flex items-center gap-2 text-xs text-green-600 dark:text-green-400">
              <Clock className="w-3 h-3" />
              {state?.session_start
                ? <SessionTimer startTs={state.session_start} />
                : <span>Session active</span>
              }
              <span>· Limit: {machine.session_limit_minutes}m</span>
            </div>
          </div>
        )}

        {/* Offline state */}
        {st === 'offline' && (
          <p className="text-xs text-muted-foreground">
            {state?.last_seen
              ? `Last seen: ${new Date(state.last_seen * 1000).toLocaleTimeString()}`
              : 'Node not connected'}
          </p>
        )}

        {/* Actions */}
        <div className="flex gap-2">
          {(st === 'active') && (
            <Button
              variant="destructive" size="sm"
              onClick={handleRemoteStop}
              disabled={stopping}
              className="flex-1"
            >
              <StopCircle className="w-4 h-4 mr-1" />
              {stopping ? 'Stopping...' : 'Remote Stop'}
            </Button>
          )}

          <Dialog open={editOpen} onOpenChange={setEditOpen}>
            <DialogTrigger asChild>
              <Button variant="outline" size="sm" className={st !== 'active' ? 'flex-1' : ''}>
                <Settings className="w-4 h-4 mr-1" />
                Config
              </Button>
            </DialogTrigger>
            <DialogContent>
              <DialogHeader>
                <DialogTitle>Configure {machine.name}</DialogTitle>
                <DialogDescription>
                  Changes are pushed to the node immediately via MQTT.
                </DialogDescription>
              </DialogHeader>
              <div className="space-y-4 pt-2">
                <div className="space-y-2">
                  <Label>Session Limit (minutes)</Label>
                  <Input
                    type="number" min={1} max={480}
                    value={limitInput}
                    onChange={e => setLimitInput(e.target.value)}
                  />
                </div>
                <div className="flex items-center gap-3">
                  <input
                    type="checkbox"
                    id={`active_${machine.id}`}
                    checked={activeInput}
                    onChange={e => setActiveInput(e.target.checked)}
                    className="w-4 h-4"
                  />
                  <Label htmlFor={`active_${machine.id}`}>Machine active (uncheck to disable remotely)</Label>
                </div>
                <Button onClick={handleSaveConfig} className="w-full">Save Changes</Button>
              </div>
            </DialogContent>
          </Dialog>
        </div>
      </CardContent>
    </Card>
  );
}

// ---------------------------------------------------------------------------
// Main component
// ---------------------------------------------------------------------------
const MachineManagement: React.FC<Props> = ({ users }) => {
  const [search, setSearch] = useState('');
  const { machines, nodeStates } = useMachines();
  const { email } = useFirebaseData();

  const filtered = machines.filter(m =>
    m.name.toLowerCase().includes(search.toLowerCase())
  );

  const exportCsv = () => {
    const rows = [
      ['ID', 'Name', 'State', 'Session Limit (min)', 'Active', 'Firmware'],
      ...filtered.map(m => {
        const ns = nodeStates[m.id];
        return [
          m.id, m.name,
          ns?.state ?? 'offline',
          String(m.session_limit_minutes),
          String(m.machine_active),
          ns?.firmware_version ?? '',
        ];
      }),
    ];
    const csv = rows.map(r => r.join(',')).join('\n');
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([csv], { type: 'text/csv' }));
    a.download = 'machines.csv';
    a.click();
  };

  const activeCount  = Object.values(nodeStates).filter(s => s.state === 'active').length;
  const offlineCount = machines.length - Object.values(nodeStates).filter(s => s.state !== 'offline').length;

  return (
    <div className="space-y-4">
      {/* Summary row */}
      <div className="flex gap-3 flex-wrap">
        <Badge className="bg-green-100 text-green-800">{activeCount} active</Badge>
        <Badge className="bg-gray-100 text-gray-700">
          {Object.values(nodeStates).filter(s => s.state === 'idle').length} idle
        </Badge>
        <Badge className="bg-red-100 text-red-700">{offlineCount} offline</Badge>
      </div>

      {/* Search + export */}
      <div className="flex gap-2">
        <div className="relative flex-1">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
          <Input
            className="pl-9"
            placeholder="Search machines..."
            value={search}
            onChange={e => setSearch(e.target.value)}
          />
        </div>
        <Button variant="outline" onClick={exportCsv}>
          <Download className="w-4 h-4 mr-2" />
          Export
        </Button>
      </div>

      {/* Machine grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {filtered.map(m => (
          <MachineCard
            key={m.id}
            machine={m}
            state={nodeStates[m.id]}
            users={users}
            adminUid={email ?? ''}
          />
        ))}
        {filtered.length === 0 && (
          <div className="col-span-3 text-center text-muted-foreground py-8">
            No machines found.
          </div>
        )}
      </div>
    </div>
  );
};

export default MachineManagement;
