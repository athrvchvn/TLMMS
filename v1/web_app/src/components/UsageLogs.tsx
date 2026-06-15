import React, { useState } from 'react';
import { Activity, Filter, Download, Calendar, Clock, User, Monitor } from 'lucide-react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { MACHINES } from '@/lib/machines';
import { useMachines } from '@/contexts/Machines';

const UsageLogs: React.FC = () => {
    const [searchTerm, setSearchTerm] = useState('');
    const [filterMachine, setFilterMachine] = useState('all');
    const [filterAction, setFilterAction] = useState('all');
    const [filterDate, setFilterDate] = useState('all');
    const { logs } = useMachines();

    const filteredLogs = logs.filter(log => {
        const matchesSearch = log.user_name.toLowerCase().includes(searchTerm.toLowerCase()) ||
            log.roll_number.toLowerCase().includes(searchTerm.toLowerCase()) ||
            log.machine.toLowerCase().includes(searchTerm.toLowerCase());

        const matchesMachine = filterMachine === 'all' || log.machine === filterMachine;
        const matchesAction = filterAction === 'all' || log.action === filterAction;

        let matchesDate = true;
        if (filterDate !== 'all') {
            const today = new Date();
            const logDate = log.timestamp;

            switch (filterDate) {
                case 'today':
                    matchesDate = logDate.toDateString() === today.toDateString();
                    break;
                case 'week':
                    const weekAgo = new Date(today.getTime() - 7 * 24 * 60 * 60 * 1000);
                    matchesDate = logDate >= weekAgo;
                    break;
                case 'month':
                    const monthAgo = new Date(today.getTime() - 30 * 24 * 60 * 60 * 1000);
                    matchesDate = logDate >= monthAgo;
                    break;
            }
        }

        return matchesSearch && matchesMachine && matchesAction && matchesDate;
    });

    const getActionColor = (action: string) => {
        const colors = {
            'access_granted': 'bg-green-100 text-green-800',
            'access_denied': 'bg-red-100 text-red-800',
            'session_start': 'bg-blue-100 text-blue-800',
            'session_end': 'bg-gray-100 text-gray-800',
        };
        return colors[action as keyof typeof colors] || 'bg-gray-100 text-gray-800';
    };

    const getStatusColor = (status: string) => {
        const colors = {
            'success': 'bg-green-500',
            'failed': 'bg-red-500',
            'warning': 'bg-yellow-500',
        };
        return colors[status as keyof typeof colors] || 'bg-gray-500';
    };

    const formatAction = (action: string) => {
        return action.split('_').map(word =>
            word.charAt(0).toUpperCase() + word.slice(1)
        ).join(' ');
    };

    const exportLogs = () => {
        const csvContent = [
            ['Timestamp', 'User', 'Roll Number', 'Machine', 'Action', 'Duration (min)', 'Status'],
            ...filteredLogs.map(log => [
                log.timestamp.toLocaleString(),
                log.user_name,
                log.roll_number,
                log.machine,
                formatAction(log.action),
                log.duration?.toString() || '',
                log.status
            ])
        ].map(row => row.join(',')).join('\n');

        const blob = new Blob([csvContent], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
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
                    <span className="hidden xs:inline">Export CSV</span>
                    <span className="xs:hidden">Export</span>
                </Button>
            </div>

            {/* Filters */}
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
                                    {MACHINES.map((machine, idx) => <SelectItem value={machine} key={idx}>{machine}</SelectItem>)}
                                </SelectContent>
                            </Select>
                        </div>
                        <div className="space-y-2">
                            <label className="text-sm font-medium">Action</label>
                            <Select value={filterAction} onValueChange={setFilterAction}>
                                <SelectTrigger className="text-sm">
                                    <SelectValue placeholder="Select action" />
                                </SelectTrigger>
                                <SelectContent>
                                    <SelectItem value="all">All Actions</SelectItem>
                                    <SelectItem value="access_granted">Access Granted</SelectItem>
                                    <SelectItem value="access_denied">Access Denied</SelectItem>
                                    <SelectItem value="session_start">Session Start</SelectItem>
                                    <SelectItem value="session_end">Session End</SelectItem>
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

            {/* Logs Table */}
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
                        {filteredLogs.map((log, idx) => (
                            <div
                                key={idx}
                                className="flex flex-col sm:flex-row sm:items-center justify-between p-3 md:p-4 border rounded-lg hover:bg-gray-50 transition-colors gap-3 md:gap-4"
                            >
                                <div className="flex items-center gap-3 md:gap-4">
                                    <div className={`w-2 h-2 md:w-3 md:h-3 rounded-full ${getStatusColor(log.status)} flex-shrink-0`}></div>
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
                                        <span className="font-medium text-sm md:text-base truncate max-w-[100px] md:max-w-none">{log.machine}</span>
                                    </div>
                                </div>

                                <div className="flex items-center gap-2 md:gap-4 ml-5 sm:ml-0">
                                    <Badge className={getActionColor(log.action) + " text-xs whitespace-nowrap"}>
                                        {formatAction(log.action)}
                                    </Badge>

                                    <div className="flex items-center gap-1 text-xs md:text-sm text-gray-500">
                                        <Calendar className="w-3 h-3 md:w-4 md:h-4 flex-shrink-0" />
                                        <span className="whitespace-nowrap">{log.timestamp.toLocaleDateString()}</span>
                                        <span className="hidden md:inline">{log.timestamp.toLocaleTimeString()}</span>
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
                                {searchTerm || filterMachine !== 'all' || filterAction !== 'all' || filterDate !== 'all'
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