import React, { useState } from 'react';
import { Monitor, Download, Clock, User, Play, Square, AlertCircle, Search } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { useMachines } from '@/contexts/Machines';
import { Badge } from './ui/badge';
import { User as UserType } from '@/lib/firebase';

type Props = {
    users: UserType[],
}

const MachineManagement: React.FC<Props> = ({ users }) => {
    const [searchTerm, setSearchTerm] = useState('');
    const { machines } = useMachines();

    const filteredMachines = machines.filter(machine =>
        machine.name.toLowerCase().includes(searchTerm.toLowerCase())
    );

    const getStatusColor = (status: string) => {
        const colors = {
            'active': 'bg-green-100 text-green-800 border-green-200',
            'idle': 'bg-gray-100 text-gray-800 border-gray-200',
            'maintenance': 'bg-red-100 text-red-800 border-red-200',
        };
        return colors[status as keyof typeof colors] || 'bg-gray-100 text-gray-800';
    };

    const getStatusIcon = (status: string) => {
        switch (status) {
            case 'active':
                return <Play className="w-3 h-3" />;
            case 'idle':
                return <Square className="w-3 h-3" />;
            case 'maintenance':
                return <AlertCircle className="w-3 h-3" />;
            default:
                return <Square className="w-3 h-3" />;
        }
    };

    const exportMachineData = () => {
        const csvContent = [
            ['Name', 'Status', 'Total Hours', 'Current User'],
            ...filteredMachines.map(machine => [
                machine.name,
                machine.current === "-1" ? "idle" : "active",
                machine.total_hours.toString(),
                machine.current !== "-1" ? machine.current : "None",
            ])
        ].map(row => row.join(',')).join('\n');
        console.log(csvContent);

        const blob = new Blob([csvContent], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.setAttribute('hidden', '');
        a.setAttribute('href', url);
        a.setAttribute('download', `machine_data_${new Date().toISOString().split('T')[0]}.csv`);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    };

    const getCurrentUser = (roll: string) => {
        const list = users.filter(user => user.roll_number === roll);
        return list;
    }

    return (
        <div className="space-y-4 md:space-y-6">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
                <div>
                    <h2 className="text-xl md:text-2xl font-bold text-gray-900">Machine Management</h2>
                    <p className="text-gray-500 text-sm md:text-base">Monitor all lab machines, usage, and maintenance schedules</p>
                </div>
                <div className="flex flex-col xs:flex-row items-stretch xs:items-center gap-3 w-full sm:w-auto">
                    <div className="relative flex-1">
                        <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 text-gray-400 w-4 h-4" />
                        <Input
                            placeholder="Search machines..."
                            value={searchTerm}
                            onChange={(e) => setSearchTerm(e.target.value)}
                            className="pl-9 w-full"
                        />
                    </div>
                    <Button
                        onClick={exportMachineData}
                        className="bg-gradient-to-r from-purple-500 to-blue-500 whitespace-nowrap"
                        size="sm"
                    >
                        <Download className="w-4 h-4 mr-1 md:mr-2" />
                        <span className="hidden xs:inline">Export CSV</span>
                        <span className="xs:hidden">Export</span>
                    </Button>
                </div>
            </div>

            {/* Machines Grid */}
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
                {filteredMachines.map((machine, idx) => (
                    <Card key={idx} className="group hover:shadow-lg transition-all duration-200 border-2 hover:border-purple-200">
                        <CardHeader className="pb-3">
                            <div className="flex items-start justify-between gap-2">
                                <div className="min-w-0 flex-1">
                                    <CardTitle className="text-base md:text-lg flex items-center gap-2 truncate">
                                        <Monitor className="w-4 h-4 md:w-5 md:h-5 text-purple-500 flex-shrink-0" />
                                        <span className="truncate">{machine.name}</span>
                                    </CardTitle>
                                </div>
                                <Badge className={`${getStatusColor(machine.current !== "-1" ? "active" : "idle")} flex items-center gap-1`}>
                                    {getStatusIcon(machine.current !== "-1" ? "active" : "idle")}
                                    {(machine.current !== "-1" ? "active" : "idle")[0].toUpperCase() + (machine.current !== "-1" ? "active" : "idle").slice(1)}
                                </Badge>
                            </div>
                        </CardHeader>
                        <CardContent className="space-y-3 md:space-y-4">
                            {/* Current Usage */}
                            {machine.current !== '-1' && (
                                <div className="p-2 md:p-3 bg-green-50 rounded-lg border border-green-200">
                                    <div className="flex items-center gap-2 mb-1 md:mb-2">
                                        <User className="w-3 h-3 md:w-4 md:h-4 text-green-600 flex-shrink-0" />
                                        <span className="font-medium text-green-800 text-sm md:text-base">Currently in use</span>
                                    </div>
                                    <p className="text-sm text-green-700 truncate">{getCurrentUser(machine.current)[0]?.name}</p>
                                    <p className="text-xs text-green-600 truncate">{machine.current}</p>
                                </div>
                            )}

                            {/* Usage Stats */}
                            <div className="grid grid-cols-2 gap-3 md:gap-4">
                                <div className="text-center p-2 bg-gray-50 rounded">
                                    <Clock className="w-3 h-3 md:w-4 md:h-4 mx-auto text-gray-600 mb-1" />
                                    <p className="text-xs text-gray-500">Total Hours</p>
                                    <p className="font-bold text-gray-900 text-sm md:text-base">{machine.total_hours}h</p>
                                </div>
                                <div className="text-center p-2 bg-gray-50 rounded">
                                    <User className="w-3 h-3 md:w-4 md:h-4 mx-auto text-gray-600 mb-1" />
                                    <p className="text-xs text-gray-500">Past Users</p>
                                    <p className="font-bold text-gray-900 text-sm md:text-base">{machine.past_users}</p>
                                </div>
                            </div>

                            {/* Last User */}
                            {machine.previous !== "-1" && (
                                <div className="pt-2 border-t">
                                    <p className="text-xs text-gray-500 mb-1">Last User:</p>
                                    <p className="text-sm font-medium truncate">{getCurrentUser(machine.previous)[0]?.name}</p>
                                    <p className="text-xs text-gray-400 truncate">{machine.previous}</p>
                                </div>
                            )}
                        </CardContent>
                    </Card>
                ))}
            </div>

            {filteredMachines.length === 0 && (
                <Card>
                    <CardContent className="flex flex-col items-center justify-center py-8 md:py-12">
                        <Monitor className="w-8 h-8 md:w-12 md:h-12 text-gray-400 mb-3 md:mb-4" />
                        <h3 className="text-base md:text-lg font-medium text-gray-900 mb-1 md:mb-2">No machines found</h3>
                        <p className="text-gray-500 text-center text-sm md:text-base">
                            {searchTerm ? `No machines match "${searchTerm}"` : 'No machines available'}
                        </p>
                    </CardContent>
                </Card>
            )}
        </div>
    );
};

export default MachineManagement;