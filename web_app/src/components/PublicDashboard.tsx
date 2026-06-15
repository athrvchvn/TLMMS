import React, { useEffect, useState } from 'react';
import { Users, Monitor, Activity, LogIn, Clock, Menu, X } from 'lucide-react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { useNavigate } from 'react-router-dom';
import { Button } from './ui/button';
import { db, User, permMaskToIds, hashPin } from '@/lib/firebase';
import { collection, onSnapshot } from 'firebase/firestore';
import { useMachines } from '@/contexts/Machines';

const PublicDashboard: React.FC = () => {
    const [users, setUsers] = useState<User[]>([]);
    const [activeTab, setActiveTab] = useState('overview');
    const [isSidebarOpen, setIsSidebarOpen] = useState(false);

    const navigate = useNavigate();
    const { machines, nodeStates } = useMachines();

    useEffect(() => {
        const unsub = onSnapshot(collection(db, 'users'), snap => {
            setUsers(snap.docs.map(d => ({ ...d.data(), roll_number: d.id }) as User));
        });
        return () => unsub();
    }, []);

    const activeCount   = Object.values(nodeStates).filter(s => s.state === 'active').length;
    const idleCount     = machines.length - Object.values(nodeStates).filter(s => s.state !== 'offline').length;
    const getUserByRoll = (roll: string | null) => roll ? users.find(u => u.roll_number === roll) : null;

    const getMachineNames = (permMask: number): string[] =>
        permMaskToIds(permMask).map(id => machines.find(m => m.id === String(id))?.name ?? `Machine ${id}`);

    const getBranchColor = (branch: string = '') => {
        const colors: Record<string, string> = {
            'Computer Science':      'bg-blue-100 text-blue-800',
            'Electrical Engineering':'bg-yellow-100 text-yellow-800',
            'Mechanical Engineering':'bg-green-100 text-green-800',
            'Civil Engineering':     'bg-orange-100 text-orange-800',
            'Chemical Engineering':  'bg-purple-100 text-purple-800',
        };
        return colors[branch] || 'bg-gray-100 text-gray-800';
    };

    const getStateBadge = (machineId: string) => {
        const st = nodeStates[machineId]?.state ?? 'offline';
        const labels: Record<string, { label: string; cls: string }> = {
            active:   { label: 'In Use',   cls: 'bg-blue-500 text-white'  },
            idle:     { label: 'Available',cls: 'bg-green-500 text-white' },
            offline:  { label: 'Offline',  cls: 'bg-gray-400 text-white'  },
            disabled: { label: 'Disabled', cls: 'bg-purple-500 text-white'},
            error:    { label: 'Error',    cls: 'bg-orange-500 text-white'},
            ota:      { label: 'Updating', cls: 'bg-blue-300 text-white'  },
        };
        return labels[st] ?? labels.offline;
    };

    // ---------------------------------------------------------------------------
    const renderOverview = () => (
        <div className="space-y-6">
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 md:gap-6">
                <Card className="bg-gradient-to-br from-blue-500 to-blue-600 text-white border-0">
                    <CardHeader className="pb-2">
                        <CardTitle className="text-sm font-medium text-blue-100">Total Users</CardTitle>
                    </CardHeader>
                    <CardContent>
                        <div className="flex items-center justify-between">
                            <div className="text-2xl font-bold">{users.length}</div>
                            <Users className="w-6 h-6 md:w-8 md:h-8 text-blue-200" />
                        </div>
                        <p className="text-xs text-blue-200 mt-1">Registered members</p>
                    </CardContent>
                </Card>

                <Card className="bg-gradient-to-br from-green-500 to-green-600 text-white border-0">
                    <CardHeader className="pb-2">
                        <CardTitle className="text-sm font-medium text-green-100">Available Machines</CardTitle>
                    </CardHeader>
                    <CardContent>
                        <div className="flex items-center justify-between">
                            <div className="text-2xl font-bold">{idleCount}</div>
                            <Monitor className="w-6 h-6 md:w-8 md:h-8 text-green-200" />
                        </div>
                        <p className="text-xs text-green-200 mt-1">Ready to use</p>
                    </CardContent>
                </Card>

                <Card className="bg-gradient-to-br from-orange-500 to-orange-600 text-white border-0">
                    <CardHeader className="pb-2">
                        <CardTitle className="text-sm font-medium text-orange-100">Machines in Use</CardTitle>
                    </CardHeader>
                    <CardContent>
                        <div className="flex items-center justify-between">
                            <div className="text-2xl font-bold">{activeCount}</div>
                            <Activity className="w-6 h-6 md:w-8 md:h-8 text-orange-200" />
                        </div>
                        <p className="text-xs text-orange-200 mt-1">Currently active</p>
                    </CardContent>
                </Card>

                <Card className="bg-gradient-to-br from-purple-500 to-purple-600 text-white border-0">
                    <CardHeader className="pb-2">
                        <CardTitle className="text-sm font-medium text-purple-100">Total Machines</CardTitle>
                    </CardHeader>
                    <CardContent>
                        <div className="flex items-center justify-between">
                            <div className="text-2xl font-bold">{machines.length}</div>
                            <Monitor className="w-6 h-6 md:w-8 md:h-8 text-purple-200" />
                        </div>
                        <p className="text-xs text-purple-200 mt-1">Lab equipment</p>
                    </CardContent>
                </Card>
            </div>

            <div className="grid grid-cols-1 lg:grid-cols-2 gap-4 md:gap-6">
                <Card>
                    <CardHeader>
                        <CardTitle>Machines Currently in Use</CardTitle>
                        <CardDescription>Live status of active machines</CardDescription>
                    </CardHeader>
                    <CardContent>
                        <div className="space-y-4">
                            {machines
                                .filter(m => nodeStates[m.id]?.state === 'active')
                                .map(machine => {
                                    const ns   = nodeStates[machine.id];
                                    const user = getUserByRoll(ns?.roll_number ?? null);
                                    return (
                                        <div key={machine.id} className="flex items-center justify-between p-3 bg-blue-50 rounded-lg">
                                            <div className="truncate">
                                                <p className="font-medium truncate">{machine.name}</p>
                                                {user && (
                                                    <p className="text-sm text-gray-500 truncate">Used by {user.name}</p>
                                                )}
                                            </div>
                                            <Badge className="bg-blue-500 text-white whitespace-nowrap">In Use</Badge>
                                        </div>
                                    );
                                })
                            }
                            {activeCount === 0 && (
                                <p className="text-gray-500 text-center py-4">No machines currently in use</p>
                            )}
                        </div>
                    </CardContent>
                </Card>

                <Card>
                    <CardHeader>
                        <CardTitle>Recent Lab Members</CardTitle>
                        <CardDescription>Newest registered users</CardDescription>
                    </CardHeader>
                    <CardContent>
                        <div className="space-y-4">
                            {users.slice(0, 5).map((user) => (
                                <div key={user.roll_number} className="flex items-center justify-between p-3 bg-gray-50 rounded-lg">
                                    <div className="flex items-center gap-3 truncate">
                                        <div className="w-8 h-8 md:w-10 md:h-10 bg-gradient-to-br from-purple-500 to-blue-500 rounded-full flex items-center justify-center text-white font-bold text-sm flex-shrink-0">
                                            {user.name.charAt(0)}
                                        </div>
                                        <div className="truncate">
                                            <p className="font-medium truncate">{user.name}</p>
                                            <p className="text-sm text-gray-500 truncate">{user.roll_number} · {user.branch ?? '—'}</p>
                                        </div>
                                    </div>
                                    <Badge className={getBranchColor(user.branch) + ' whitespace-nowrap flex-shrink-0'}>
                                        {user.year ?? '—'}
                                    </Badge>
                                </div>
                            ))}
                            {users.length === 0 && (
                                <p className="text-gray-500 text-center py-4">No members yet</p>
                            )}
                        </div>
                    </CardContent>
                </Card>
            </div>
        </div>
    );

    // ---------------------------------------------------------------------------
    const renderMachines = () => (
        <div className="space-y-4">
            <div>
                <h2 className="text-xl md:text-2xl font-bold text-gray-900">Machine Status</h2>
                <p className="text-gray-500">Real-time status of all lab equipment</p>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
                {machines.map((machine) => {
                    const ns      = nodeStates[machine.id];
                    const badge   = getStateBadge(machine.id);
                    const curUser = getUserByRoll(ns?.roll_number ?? null);
                    return (
                        <Card key={machine.id} className="hover:shadow-lg transition-all duration-200">
                            <CardHeader className="pb-3">
                                <div className="flex items-center justify-between">
                                    <CardTitle className="text-lg truncate">{machine.name}</CardTitle>
                                    <Badge className={badge.cls + ' whitespace-nowrap flex-shrink-0'}>{badge.label}</Badge>
                                </div>
                                {ns?.ip_address && (
                                    <p className="text-xs text-muted-foreground">{ns.ip_address} · v{ns.firmware_version}</p>
                                )}
                            </CardHeader>
                            <CardContent className="space-y-3">
                                {ns?.state === 'active' && curUser && (
                                    <div className="flex items-center justify-between">
                                        <span className="text-sm text-gray-600">Current User:</span>
                                        <span className="text-sm font-medium truncate max-w-[140px]">{curUser.name}</span>
                                    </div>
                                )}
                                <div className="flex items-center justify-between">
                                    <span className="text-sm text-gray-600">Session Limit:</span>
                                    <span className="text-sm font-medium">{machine.session_limit_minutes} min</span>
                                </div>
                                {ns?.last_seen && (
                                    <div className="flex items-center justify-between">
                                        <span className="text-sm text-gray-600">Last Seen:</span>
                                        <span className="text-sm text-muted-foreground">
                                            {new Date(ns.last_seen * 1000).toLocaleTimeString()}
                                        </span>
                                    </div>
                                )}
                            </CardContent>
                        </Card>
                    );
                })}
            </div>
        </div>
    );

    // ---------------------------------------------------------------------------
    const renderUsers = () => (
        <div className="space-y-6">
            <div>
                <h2 className="text-xl md:text-2xl font-bold text-gray-900">Lab Members</h2>
                <p className="text-gray-500">Registered users and their access permissions</p>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
                {users.map((user) => {
                    const machineNames = getMachineNames(user.machine_permissions);
                    return (
                        <Card key={user.roll_number} className="hover:shadow-lg transition-all duration-200">
                            <CardHeader className="pb-3">
                                <div className="flex items-center gap-3">
                                    <div className="w-10 h-10 md:w-12 md:h-12 bg-gradient-to-br from-purple-500 to-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">
                                        {user.name.charAt(0)}
                                    </div>
                                    <div className="truncate">
                                        <CardTitle className="text-lg truncate">{user.name}</CardTitle>
                                        <p className="text-sm text-gray-500 truncate">{user.roll_number}</p>
                                    </div>
                                </div>
                            </CardHeader>
                            <CardContent className="space-y-4">
                                <div className="flex items-center justify-between flex-wrap gap-2">
                                    <Badge className={getBranchColor(user.branch) + ' whitespace-nowrap'}>
                                        {user.branch ?? '—'}
                                    </Badge>
                                    <Badge variant="outline" className="whitespace-nowrap">
                                        {user.year ?? '—'}
                                    </Badge>
                                </div>

                                <div>
                                    <p className="text-sm font-medium text-gray-700 mb-2">Machine Access:</p>
                                    <div className="flex flex-wrap gap-1">
                                        {machineNames.length > 0 ? (
                                            machineNames.map(name => (
                                                <Badge key={name} variant="secondary" className="text-xs whitespace-nowrap">
                                                    {name}
                                                </Badge>
                                            ))
                                        ) : (
                                            <p className="text-xs text-gray-400">No access granted</p>
                                        )}
                                    </div>
                                </div>

                                <div className="flex items-center justify-between pt-2 border-t flex-wrap gap-2">
                                    <div className="flex items-center gap-2 text-xs text-gray-500">
                                        <Clock className="w-3 h-3" />
                                        <span className="whitespace-nowrap">
                                            {(user.issued_at as any)?.toDate?.()?.toLocaleDateString() ?? 'N/A'}
                                        </span>
                                    </div>
                                    <div className="flex items-center gap-1">
                                        <div className={`w-2 h-2 rounded-full ${user.card_uid ? 'bg-green-500' : 'bg-gray-300'}`} />
                                        <span className="text-xs text-gray-500 whitespace-nowrap">
                                            {user.card_uid ? 'Card Linked' : 'No Card'}
                                        </span>
                                    </div>
                                </div>
                            </CardContent>
                        </Card>
                    );
                })}
            </div>
        </div>
    );

    // ---------------------------------------------------------------------------
    return (
        <div className="min-h-screen bg-gradient-to-br from-purple-50 via-blue-50 to-indigo-100">
            <header className="bg-white/80 backdrop-blur-sm border-b border-white/20 sticky top-0 z-40">
                <div className="flex items-center justify-between px-4 py-3 md:px-6 md:py-4">
                    <div className="flex items-center gap-3">
                        <button
                            className="md:hidden p-2 rounded-md text-gray-700"
                            onClick={() => setIsSidebarOpen(!isSidebarOpen)}
                        >
                            {isSidebarOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
                        </button>
                        <div className="p-2 bg-gradient-to-br from-purple-500 to-blue-500 rounded-lg hidden sm:block">
                            <img
                                src="/lovable-uploads/4eecbf7d-95ff-44fb-b13d-35d974c4d878.png"
                                alt="Tinkerer's Lab Logo"
                                className="w-6 h-6"
                            />
                        </div>
                        <div>
                            <h1 className="text-lg md:text-xl font-bold text-gray-900">Tinkerer's Lab</h1>
                            <p className="text-xs md:text-sm text-gray-500">Machine Management System</p>
                        </div>
                    </div>
                    <Button
                        className="bg-gradient-to-r from-purple-500 to-blue-500 hover:from-purple-600 hover:to-blue-600 text-sm"
                        onClick={() => navigate('/login')}
                    >
                        <LogIn className="w-4 h-4 mr-2" />
                        <span className="hidden sm:inline">Admin Login</span>
                        <span className="sm:hidden">Login</span>
                    </Button>
                </div>
            </header>

            <div className="flex relative">
                <aside className={`
                    w-64 bg-white/60 backdrop-blur-sm border-r border-white/20 min-h-screen p-6
                    fixed md:static inset-y-0 left-0 z-40
                    transform transition-transform duration-300 ease-in-out
                    ${isSidebarOpen ? 'translate-x-0' : '-translate-x-full md:translate-x-0'}
                `}>
                    <nav className="space-y-2">
                        {[
                            { id: 'overview',  label: 'Overview',  icon: Activity },
                            { id: 'machines',  label: 'Machines',  icon: Monitor  },
                            { id: 'users',     label: 'Users',     icon: Users    },
                        ].map(item => (
                            <button
                                key={item.id}
                                onClick={() => { setActiveTab(item.id); setIsSidebarOpen(false); }}
                                className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 ${
                                    activeTab === item.id
                                        ? 'bg-gradient-to-r from-purple-500 to-blue-500 text-white shadow-lg'
                                        : 'text-gray-600 hover:bg-white/50'
                                }`}
                            >
                                <item.icon className="w-5 h-5" />
                                {item.label}
                            </button>
                        ))}
                    </nav>
                </aside>

                {isSidebarOpen && (
                    <div
                        className="fixed inset-0 bg-black bg-opacity-50 z-20 md:hidden"
                        onClick={() => setIsSidebarOpen(false)}
                    />
                )}

                <main className="flex-1 p-4 md:p-6">
                    {activeTab === 'overview' && renderOverview()}
                    {activeTab === 'machines' && renderMachines()}
                    {activeTab === 'users'    && renderUsers()}
                </main>
            </div>
        </div>
    );
};

export default PublicDashboard;
