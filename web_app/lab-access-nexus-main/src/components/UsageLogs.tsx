import React, { useState } from 'react';
import { Activity, Filter, Download, Calendar, Clock, Monitor } from 'lucide-react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { useMachines } from '@/contexts/Machines';

function formatDuration(seconds: number | null): string {
    if (seconds === null) return '—';
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    if (h > 0) return `${h}h ${m}m`;
    if (m > 0) return `${m}m ${s}s`;
    return `${s}s`;
}

const END_REASON_COLORS: Record<string, string> = {
    card_removed:     'bg-gray-100 text-gray-800',
    timeout:          'bg-orange-100 text-orange-800',
    remote_stop:      'bg-red-100 text-red-800',
    watchdog:         'bg-purple-100 text-purple-800',
    machine_disabled: 'bg-yellow-100 text-yellow-800',
    power_loss:       'bg-rose-100 text-rose-800',
};

const STATUS_DOT: Record<string, string> = {
    active:    'bg-green-500',
    completed: 'bg-blue-500',
    error:     'bg-red-500',
};

function formatLabel(raw: string): string {
    return raw.split('_').map(w => w.charAt(0).toUpperCase() + w.slice(1)).join(' ');
}

const UsageLogs: React.FC = () => {
    const [searchTerm,    setSearchTerm]    = useState('');
    const [filterMachine, setFilterMachine] = useState('all');
    const [filterReason,  setFilterReason]  = useState('all');
    const [filterDate,    setFilterDate]    = useState('all');
    const { logs, machines } = useMachines();

    const filteredLogs = logs.filter(log => {
        const matchesSearch =
            log.user_name.toLowerCase().includes(searchTerm.toLowerCase()) ||
            log.roll_number.toLowerCase().includes(searchTerm.toLowerCase()) ||
            log.machine_name.toLowerCase().includes(searchTerm.toLowerCase());

        const matchesMachine = filterMachine === 'all' || log.machine_name === filterMachine;
        const matchesReason  = filterReason  === 'all' || log.end_reason   === filterReason;

        let matchesDate = true;
        if (filterDate !== 'all') {
            const today   = new Date();
            const logDate = log.start_time;
            switch (filterDate) {
                case 'today': matchesDate = logDate.toDateString() === today.toDateString(); break;
                case 'week':  matchesDate = logDate >= new Date(today.getTime() - 7  * 86400000); break;
                case 'month': matchesDate = logDate >= new Date(today.getTime() - 30 * 86400000); break;
            }
        }

        return matchesSearch && matchesMachine && matchesReason && matchesDate;
    });

    const exportLogs = () => {
        const rows = [
            ['Start Time', 'End Time', 'User', 'Roll Number', 'Machine', 'Duration', 'End Reason', 'Status'],
            ...filteredLogs.map(log => [
                log.start_time.toLocaleString(),
                log.end_time?.toLocaleString() ?? '',
                log.user_name,
                log.roll_number,
                log.machine_name,
                formatDuration(log.duration_s),
                log.end_reason,
                log.status,
            ]),
        ];
        const csv  = rows.map(r => r.join(',')).join('\n');
        const blob = new Blob([csv], { type: 'text/csv' });
        const url  = window.URL.createObjectURL(blob);
        const a    = document.createElement('a');
        a.setAttribute('hidden', '');
        a.setAttribute('href', url);
        a.setAttribute('download', `usage_logs_${new Date().toISOString().split('T')[0]}.csv`);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    };

    return (
        <div className="space-y-4 md:space-y-6">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
                <div>
                    <h2 className="text-xl md:text-2xl font-bold text-gray-900">Usage Logs</h2>
                    <p className="text-gray-500 text-sm md:text-base">Monitor user access and machine usage patterns</p>
                </div>
                <Button onClick={exportLogs} className="bg-gradient-to-r from-purple-500 to-blue-500 w-full sm:w-auto">
                    <Download className="w-4 h-4 mr-2" />
                    Export CSV
                </Button>
            </div>

            <Card>
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-lg">
                        <Filter className="w-4 h-4 md:w-5 md:h-5" />
                        Filters
                    </CardTitle>
                </CardHeader>
                <CardContent>
                    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3 md:gap-4">
                        <div className="space-y-2">
                            <label className="text-sm font-medium">Search</label>
                            <Input
                                placeholder="Search users, machines..."
                                value={searchTerm}
                                onChange={(e) => setSearchTerm(e.target.value)}
                                className="text-sm"
                            />
                        </div>
                        <div className="space-y-2">
                            <label className="text-sm font-medium">Machine</label>
                            <Select value={filterMachine} onValueChange={setFilterMachine}>
                                <SelectTrigger className="text-sm">
                                    <SelectValue placeholder="Select machine" />
                                </SelectTrigger>
                                <SelectContent>
                                    <SelectItem value="all">All Machines</SelectItem>
                                    {machines.map(m => (
                                        <SelectItem key={m.id} value={m.name}>{m.name}</SelectItem>
                                    ))}
                                </SelectContent>
                            </Select>
                        </div>
                        <div className="space-y-2">
                            <label className="text-sm font-medium">End Reason</label>
                            <Select value={filterReason} onValueChange={setFilterReason}>
                                <SelectTrigger className="text-sm">
                                    <SelectValue placeholder="Select reason" />
                                </SelectTrigger>
                                <SelectContent>
                                    <SelectItem value="all">All Reasons</SelectItem>
                                    <SelectItem value="card_removed">Card Removed</SelectItem>
                                    <SelectItem value="timeout">Timeout</SelectItem>
                                    <SelectItem value="remote_stop">Remote Stop</SelectItem>
                                    <SelectItem value="watchdog">Watchdog Reset</SelectItem>
                                    <SelectItem value="machine_disabled">Machine Disabled</SelectItem>
                                    <SelectItem value="power_loss">Power Loss</SelectItem>
                                </SelectContent>
                            </Select>
                        </div>
                        <div className="space-y-2">
                            <label className="text-sm font-medium">Date Range</label>
                            <Select value={filterDate} onValueChange={setFilterDate}>
                                <SelectTrigger className="text-sm">
                                    <SelectValue placeholder="Select date range" />
                                </SelectTrigger>
                                <SelectContent>
                                    <SelectItem value="all">All Time</SelectItem>
                                    <SelectItem value="today">Today</SelectItem>
                                    <SelectItem value="week">This Week</SelectItem>
                                    <SelectItem value="month">This Month</SelectItem>
                                </SelectContent>
                            </Select>
                        </div>
                    </div>
                </CardContent>
            </Card>

            <Card>
                <CardHeader>
                    <CardTitle className="flex items-center gap-2 text-lg md:text-xl">
                        <Activity className="w-4 h-4 md:w-5 md:h-5" />
                        Recent Activity ({filteredLogs.length} entries)
                    </CardTitle>
                    <CardDescription className="text-sm md:text-base">
                        Real-time logs of user access and machine usage
                    </CardDescription>
                </CardHeader>
                <CardContent>
                    <div className="space-y-3 md:space-y-4">
                        {filteredLogs.map((log) => (
                            <div
                                key={log.id}
                                className="flex flex-col sm:flex-row sm:items-center justify-between p-3 md:p-4 border rounded-lg hover:bg-gray-50 transition-colors gap-3 md:gap-4"
                            >
                                <div className="flex items-center gap-3 md:gap-4">
                                    <div className={`w-2 h-2 md:w-3 md:h-3 rounded-full ${STATUS_DOT[log.status] ?? 'bg-gray-400'} flex-shrink-0`} />
                                    <div className="flex items-center gap-2 md:gap-3">
                                        <div className="w-8 h-8 md:w-10 md:h-10 bg-gradient-to-br from-purple-500 to-blue-500 rounded-full flex items-center justify-center text-white font-bold text-xs md:text-sm flex-shrink-0">
                                            {log.user_name.charAt(0)}
                                        </div>
                                        <div className="min-w-0">
                                            <p className="font-medium text-sm md:text-base truncate">{log.user_name}</p>
                                            <p className="text-xs md:text-sm text-gray-500 truncate">{log.roll_number}</p>
                                        </div>
                                    </div>
                                    <div className="flex items-center gap-1 md:gap-2">
                                        <Monitor className="w-3 h-3 md:w-4 md:h-4 text-gray-400 flex-shrink-0" />
                                        <span className="font-medium text-sm md:text-base truncate max-w-[100px] md:max-w-none">
                                            {log.machine_name}
                                        </span>
                                    </div>
                                </div>

                                <div className="flex items-center gap-2 md:gap-4 ml-5 sm:ml-0 flex-wrap">
                                    <Badge className={(END_REASON_COLORS[log.end_reason] ?? 'bg-gray-100 text-gray-800') + ' text-xs whitespace-nowrap'}>
                                        {formatLabel(log.end_reason || log.status)}
                                    </Badge>

                                    {log.duration_s !== null && (
                                        <div className="flex items-center gap-1 text-xs text-gray-500">
                                            <Clock className="w-3 h-3 flex-shrink-0" />
                                            <span>{formatDuration(log.duration_s)}</span>
                                        </div>
                                    )}

                                    <div className="flex items-center gap-1 text-xs md:text-sm text-gray-500">
                                        <Calendar className="w-3 h-3 md:w-4 md:h-4 flex-shrink-0" />
                                        <span className="whitespace-nowrap">{log.start_time.toLocaleDateString()}</span>
                                        <span className="hidden md:inline">{log.start_time.toLocaleTimeString()}</span>
                                    </div>
                                </div>
                            </div>
                        ))}
                    </div>

                    {filteredLogs.length === 0 && (
                        <div className="text-center py-8 md:py-12">
                            <Activity className="w-8 h-8 md:w-12 md:h-12 text-gray-400 mx-auto mb-3 md:mb-4" />
                            <h3 className="text-base md:text-lg font-medium text-gray-900 mb-1 md:mb-2">No logs found</h3>
                            <p className="text-gray-500 text-sm md:text-base">
                                {searchTerm || filterMachine !== 'all' || filterReason !== 'all' || filterDate !== 'all'
                                    ? 'No logs match your current filters'
                                    : 'No usage logs available yet'}
                            </p>
                        </div>
                    )}
                </CardContent>
            </Card>
        </div>
    );
};

export default UsageLogs;
