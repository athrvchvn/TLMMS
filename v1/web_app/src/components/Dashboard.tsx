import React, { useState, useEffect } from 'react';
import { Users, Monitor, Activity, LogOut, UserPlus, BarChart3, Calendar, Menu, X } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import UserManagement from '@/components/UserManagement';
import MachineManagement from '@/components/MachineManagement';
import UsageLogs from '@/components/UsageLogs';
import AddUserForm from '@/components/AddUserForm';
import { auth, db, User } from '@/lib/firebase';
import { signOut } from 'firebase/auth';
import { useNavigate } from 'react-router-dom';
import { collection, onSnapshot } from 'firebase/firestore';
import { MACHINES } from '@/lib/machines';
import { UsageLog, useMachines } from '@/contexts/Machines';

const Dashboard: React.FC = () => {
    const [activeTab, setActiveTab] = useState('overview');
    const [users, setUsers] = useState<User[]>([]);
    const [isSidebarOpen, setIsSidebarOpen] = useState(false);
    const navigate = useNavigate()
    const { machines, logs } = useMachines();

    useEffect(() => {
        const q = collection(db, "users");

        const unsubscribe = onSnapshot(q, (querySnapshot) => {
            const data = querySnapshot.docs.map(doc => doc.data()).map(doc => ({ ...doc, created_at: doc.created_at.toDate(), last_updated: doc?.last_updated?.toDate() }));
            setUsers(data as any[]);
        });

        return () => unsubscribe();
    }, []);

    const handleLogout = async () => {
        await signOut(auth);
        navigate("/login");
    }

    const sidebarItems = [
        { id: 'overview', label: 'Dashboard', icon: BarChart3 },
        { id: 'users', label: 'User Management', icon: Users },
        { id: 'machines', label: 'Machine Management', icon: Monitor },
        { id: 'logs', label: 'Usage Logs', icon: Activity },
        { id: 'add-user', label: 'Add User', icon: UserPlus },
    ];

    const renderContent = () => {
        switch (activeTab) {
            case 'users':
                return <UserManagement users={users} setUsers={setUsers} availableMachines={MACHINES} />;
            case 'machines':
                return <MachineManagement users={users} />;
            case 'logs':
                return <UsageLogs />;
            case 'add-user':
                return <AddUserForm />;
            default:
                return (
                    <div className="space-y-4 md:space-y-6">
                        <div>
                            <h2 className="text-xl md:text-2xl font-bold text-gray-900">Dashboard Overview</h2>
                            <p className="text-gray-500">Welcome to Tinkerer's Lab Management System</p>
                        </div>

                        {/* Stats Cards */}
                        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 md:gap-6">
                            <Card>
                                <CardContent className="p-4 md:p-6">
                                    <div className="flex items-center justify-between">
                                        <div>
                                            <p className="text-sm text-gray-500">Total Users</p>
                                            <p className="text-xl md:text-2xl font-bold text-gray-900">{users.length}</p>
                                        </div>
                                        <Users className="w-6 h-6 md:w-8 md:h-8 text-purple-500" />
                                    </div>
                                </CardContent>
                            </Card>

                            <Card>
                                <CardContent className="p-4 md:p-6">
                                    <div className="flex items-center justify-between">
                                        <div>
                                            <p className="text-sm text-gray-500">Active Machines</p>
                                            <p className="text-xl md:text-2xl font-bold text-gray-900">{machines.reduce((prev, machine) => prev + (machine.current !== "-1" ? 1 : 0), 0)}</p>
                                        </div>
                                        <Monitor className="w-6 h-6 md:w-8 md:h-8 text-green-500" />
                                    </div>
                                </CardContent>
                            </Card>

                            <Card>
                                <CardContent className="p-4 md:p-6">
                                    <div className="flex items-center justify-between">
                                        <div>
                                            <p className="text-sm text-gray-500">Total Machines</p>
                                            <p className="text-xl md:text-2xl font-bold text-gray-900">{machines.length}</p>
                                        </div>
                                        <Monitor className="w-6 h-6 md:w-8 md:h-8 text-blue-500" />
                                    </div>
                                </CardContent>
                            </Card>

                            <Card>
                                <CardContent className="p-4 md:p-6">
                                    <div className="flex items-center justify-between">
                                        <div>
                                            <p className="text-sm text-gray-500">Lab Status</p>
                                            <p className="text-xl md:text-2xl font-bold text-gray-900">Active</p>
                                        </div>
                                        <div className="w-3 h-3 bg-green-400 rounded-full animate-pulse"></div>
                                    </div>
                                </CardContent>
                            </Card>
                        </div>

                        {/* Recent Activity */}
                        <Card>
                            <CardHeader>
                                <CardTitle>Recent Activity</CardTitle>
                                <CardDescription>Latest machine usage and user activities</CardDescription>
                            </CardHeader>
                            <CardContent>
                                <div className="space-y-4">
                                    {logs.slice(0, 5).map((log, idx) => <RecentLog log={log} key={idx} />)}
                                </div>
                            </CardContent>
                        </Card>
                    </div>
                );
        }
    };

    return (
        <div className="min-h-screen bg-gray-50">
            {/* Header */}
            <header className="bg-white shadow-sm border-b">
                <div className="flex items-center justify-between px-4 py-3 md:px-6 md:py-4">
                    <div className="flex items-center gap-3">
                        <button
                            className="md:hidden p-2 rounded-md text-gray-700"
                            onClick={() => setIsSidebarOpen(!isSidebarOpen)}
                        >
                            {isSidebarOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
                        </button>
                        <img
                            src="/lovable-uploads/4eecbf7d-95ff-44fb-b13d-35d974c4d878.png"
                            alt="Tinkerer's Lab Logo"
                            className="w-6 h-6 md:w-8 md:h-8"
                        />
                        <div>
                            <h1 className="text-lg md:text-xl font-bold text-gray-900">Tinkerer's Lab</h1>
                            <p className="text-xs md:text-sm text-gray-500">Admin Dashboard</p>
                        </div>
                    </div>
                    <Button variant="outline" onClick={handleLogout} size="sm" className="text-sm">
                        <LogOut className="w-4 h-4 mr-1 md:mr-2" />
                        <span className="hidden sm:inline">Logout</span>
                    </Button>
                </div>
            </header>

            <div className="flex relative">
                {/* Sidebar */}
                <aside className={`
          w-64 bg-white shadow-sm min-h-screen fixed md:static inset-y-0 left-0 z-30 transform transition-transform duration-300 ease-in-out
          ${isSidebarOpen ? 'translate-x-0' : '-translate-x-full md:translate-x-0'}
        `}>
                    <nav className="p-4 space-y-2">
                        {sidebarItems.map((item) => (
                            <button
                                key={item.id}
                                onClick={() => {
                                    setActiveTab(item.id);
                                    setIsSidebarOpen(false);
                                }}
                                className={`w-full flex items-center gap-3 px-3 py-2 rounded-lg text-left transition-colors ${activeTab === item.id
                                    ? 'bg-purple-100 text-purple-700 font-medium'
                                    : 'text-gray-600 hover:bg-gray-100'
                                }`}
                            >
                                <item.icon className="w-5 h-5" />
                                {item.label}
                            </button>
                        ))}
                    </nav>
                </aside>

                {/* Overlay for mobile sidebar */}
                {isSidebarOpen && (
                    <div
                        className="fixed inset-0 bg-black bg-opacity-50 z-20 md:hidden"
                        onClick={() => setIsSidebarOpen(false)}
                    />
                )}

                {/* Main Content */}
                <main className="flex-1 p-4 md:p-6">
                    {renderContent()}
                </main>
            </div>
        </div>
    );
};

type Props = {
    log: UsageLog,
}

function RecentLog({ log }: Props) {
    return (
        <div className="flex flex-col sm:flex-row sm:items-center justify-between p-3 bg-gray-50 rounded-lg gap-2">
            <div className="flex items-center gap-3">
                <div className="w-2 h-2 bg-green-500 rounded-full flex-shrink-0"></div>
                <span className="font-medium text-sm sm:text-base">{log.user_name} {log.action === "access_granted" ? "started" : "finished"} using {log.machine}</span>
            </div>
            <span className="text-xs sm:text-sm text-gray-500 flex items-center gap-2 ml-5 sm:ml-0">
        <Calendar className="w-3 h-3 sm:w-4 sm:h-4 flex-shrink-0" />
                {log.timestamp.toLocaleString()}
      </span>
        </div>
    );
}

export default Dashboard;