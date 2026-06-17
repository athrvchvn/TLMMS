import React, { useEffect, useState } from 'react';
import { Users, Monitor, Activity, LogIn, Clock, Menu, X, Calendar, EthernetPort } from 'lucide-react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { useNavigate } from 'react-router-dom';
import { Button } from './ui/button';
import { db, User } from '@/lib/firebase';
import { collection, doc, getDocs, onSnapshot, query, Timestamp, updateDoc, where } from 'firebase/firestore';
import { useMachines } from '@/contexts/Machines';
import { Input } from './ui/input';
import { toast } from '@/hooks/use-toast';

const PublicDashboard: React.FC = () => {
  const [users, setUsers] = useState<User[]>([]);
  const [activeTab, setActiveTab] = useState('overview');
  const [isSidebarOpen, setIsSidebarOpen] = useState(false);
  const [openMachine, setOpenMachine] = useState<string | null>(null);
  const [selectedDate, setSelectedDate] = useState("");
  const [selectedTime, setSelectedTime] = useState("");
  const [roll, setRoll] = useState("");
  const [pin, setPin] = useState("");

  const navigate = useNavigate();
  const { machines } = useMachines();

  const getCurrentUser = (roll: string) => {
    const list = users.filter(user => user.roll_number === roll);
    return list;
  }

  useEffect(() => {
    const q = collection(db, "users");

    const unsubscribe = onSnapshot(q, (querySnapshot) => {
      const data = querySnapshot.docs.map(doc => doc.data()).map(doc => ({ ...doc, created_at: doc.created_at.toDate(), last_updated: doc?.last_updated?.toDate() }));
      setUsers(data as any[]);
    });

    return () => unsubscribe();
  }, []);

  useEffect(() => {
    const interval = setInterval(() => {
      machines.forEach(async (m) => {
        if (m.reserved_at && m.reserved_at.toDate() < new Date()) {
          const doc = await getDocs(query(collection(db, "machines"), where("name", "==", m.name)));
          if (doc.empty) return;
          await updateDoc(doc.docs[0].ref, {
            reserved_at: null,
            reserved_by: null,
          })
        }
      });
    }, 60000); // check every 1 min

    return () => clearInterval(interval);
  }, [machines]);

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'Available': return 'bg-green-100 text-green-800';
      case 'In Use': return 'bg-blue-100 text-blue-800';
      case 'Maintenance': return 'bg-red-100 text-red-800';
      default: return 'bg-gray-100 text-gray-800';
    }
  };

  const getBranchColor = (branch: string) => {
    const colors = {
      'Computer Science': 'bg-blue-100 text-blue-800',
      'Electrical Engineering': 'bg-yellow-100 text-yellow-800',
      'Mechanical Engineering': 'bg-green-100 text-green-800',
      'Civil Engineering': 'bg-orange-100 text-orange-800',
      'Chemical Engineering': 'bg-purple-100 text-purple-800',
    };
    return colors[branch as keyof typeof colors] || 'bg-gray-100 text-gray-800';
  };

  const renderOverview = () => (
    <div className="space-y-6">
      {/* Stats Cards */}
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
              <div className="text-2xl font-bold">{machines.filter(m => m.current === '-1').length}</div>
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
              <div className="text-2xl font-bold">{machines.filter(m => m.current !== '-1').length}</div>
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

      {/* Current Activity */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4 md:gap-6">
        <Card>
          <CardHeader>
            <CardTitle>Machines Currently in Use</CardTitle>
            <CardDescription>Live status of active machines</CardDescription>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              {machines.filter(m => m.current !== '-1').map((machine, idx) => (
                <div key={idx} className="flex items-center justify-between p-3 bg-blue-50 rounded-lg">
                  <div className="truncate">
                    <p className="font-medium truncate">{machine.name}</p>
                    <p className="text-sm text-gray-500 truncate">{machine.name} • Used by {getCurrentUser(machine.current)[0]?.name}</p>
                  </div>
                  <Badge className="bg-blue-500 text-white whitespace-nowrap">In Use</Badge>
                </div>
              ))}
              {machines.filter(m => m.current === '-1').length === 0 && (
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
                      <p className="text-sm text-gray-500 truncate">{user.roll_number} • {user.branch}</p>
                    </div>
                  </div>
                  <Badge className={getBranchColor(user.branch) + " whitespace-nowrap flex-shrink-0"}>
                    {user.year}
                  </Badge>
                </div>
              ))}
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );

  const renderMachines = () => {
    const handleBook = async (machineName: string) => {
      const usersCollection = collection(db, "users");
      const machinesCollection = collection(db, "machines");

      const users = await getDocs(query(usersCollection, where("roll_number", "==", roll)));
      if (users.empty) {
        toast({
          title: "User not found",
          description: "User with the provided roll number not found",
        });
        return handleClose();
      }

      const userDoc = users.docs[0];
      const userData = userDoc.data();
      if (userData.pin_hash !== pin) {
        toast({
          title: "Invalid PIN",
          description: "The PIN provided was incorrect",
        });
        return handleClose();
      }

      const machines = await getDocs(query(machinesCollection, where("name", "==", openMachine)));
      if (machines.empty) {
        return handleClose();
      }

      const machineDoc = machines.docs[0];

      await updateDoc(machineDoc.ref, {
        reserved_by: userData.roll_number,
        reserved_at: Timestamp.fromDate(new Date(`${selectedDate}T${selectedTime}`)),
      });

      setOpenMachine(null);
    };

    const handleClose = () => {
      setOpenMachine(null);
      setSelectedDate("");
      setSelectedTime("");
    }

    return (
      <div className="">
        <div>
          <h2 className="text-xl md:text-2xl font-bold text-gray-900">Machine Status</h2>
          <p className="text-gray-500">Real-time status of all lab equipment</p>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
          {machines.map((machine, idx) => (
            <Card key={idx} className="hover:shadow-lg transition-all duration-200">
              <CardHeader className="pb-3">
                <div className="flex items-center justify-between">
                  <div className="truncate">
                    <CardTitle className="text-lg truncate">{machine.name}</CardTitle>
                  </div>
                  <Badge className={getStatusColor(machine.current === "-1" ? "Available" : "In Use") + " whitespace-nowrap flex-shrink-0"}>
                    {machine.current === "-1" ? "Available" : "In Use"}
                  </Badge>
                </div>
              </CardHeader>
              <CardContent className="space-y-3">
                {machine.current !== "-1" && (
                  <div className="flex items-center justify-between">
                    <span className="text-sm text-gray-600">Current User:</span>
                    <span className="text-sm font-medium truncate max-w-[120px]">{getCurrentUser(machine.current)[0]?.name}</span>
                  </div>
                )}

                <div className="flex items-center justify-between">
                  <span className="text-sm text-gray-600">Total Hours:</span>
                  <span className="text-sm font-medium">{machine.total_hours}h</span>
                </div>

                {machine.reserved_by && machine.reserved_at && (
                  <div className="p-3 rounded-lg bg-green-50 border border-green-200">
                    <p className="text-sm text-green-700 font-medium">
                      Reserved by: {getCurrentUser(machine.reserved_by)[0]?.name}
                    </p>
                    <p className="text-xs text-green-600">
                      At: {machine.reserved_at.toDate().toLocaleString()}
                    </p>
                  </div>
                )}

                <Button
                  className="w-full mt-2 bg-gradient-to-r from-purple-500 to-blue-500 hover:from-purple-600 hover:to-blue-600"
                  onClick={() => setOpenMachine(machine.name)}
                >
                  Book Slot
                </Button>
              </CardContent>
            </Card>
          ))}
        </div>

        {/* Custom Centered Modal */}
        {/* Custom Centered Modal */}
        {openMachine && (
          <div onClick={handleClose} className="fixed inset-0 bg-black bg-opacity-50 z-50 flex items-center justify-center">
            <div onClick={e => e.stopPropagation()} className="relative bg-white w-full max-w-md rounded-2xl shadow-lg p-6 md:p-8">
              {/* Close Button */}
              <button
                onClick={handleClose}
                className="absolute top-4 right-4 text-gray-600 hover:text-gray-900"
              >
                <X className="w-6 h-6" />
              </button>

              {/* Title */}
              <h2 className="text-xl font-bold text-gray-900 mb-6">
                Book Slot for {openMachine}
              </h2>

              {/* Form Content */}
              <div className="space-y-4">
                {/* Roll Number */}
                <div>
                  <label className="block text-sm font-medium text-gray-700">Roll Number</label>
                  <Input
                    type="text"
                    placeholder="Enter your roll number"
                    className="w-full mt-1"
                    value={roll}
                    onChange={e => setRoll(e.target.value)}
                  />
                </div>

                {/* PIN */}
                <div>
                  <label className="block text-sm font-medium text-gray-700">PIN</label>
                  <Input
                    type="password"
                    placeholder="Enter 4-digit PIN"
                    maxLength={4}
                    className="w-full mt-1"
                    value={pin}
                    onChange={e => setPin(e.target.value)}
                  />
                </div>

                {/* Date */}
                <div>
                  <label className="block text-sm font-medium text-gray-700">Date</label>
                  <div className="relative mt-1">
                    <Input
                      type="date"
                      value={selectedDate}
                      onChange={(e) => setSelectedDate(e.target.value)}
                      className="w-full pl-10"
                    />
                    <Calendar className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400 w-5 h-5" />
                  </div>
                </div>

                {/* Time */}
                <div>
                  <label className="block text-sm font-medium text-gray-700">Time</label>
                  <div className="relative mt-1">
                    <Input
                      type="time"
                      value={selectedTime}
                      onChange={(e) => setSelectedTime(e.target.value)}
                      className="w-full pl-10"
                    />
                    <Clock className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400 w-5 h-5" />
                  </div>
                </div>
              </div>

              {/* Footer */}
              <div className="flex justify-end gap-3 border-t pt-4 mt-6">
                <Button
                  variant="outline"
                  className="w-24"
                  onClick={() => setOpenMachine(null)}
                >
                  Cancel
                </Button>
                <Button
                  className="w-24 bg-gradient-to-r from-purple-500 to-blue-500 hover:from-purple-600 hover:to-blue-600"
                  onClick={() => handleBook(openMachine!)}
                >
                  Confirm
                </Button>
              </div>
            </div>
          </div>
        )}


      </div>
    );
  };

  const renderUsers = () => (
    <div className="space-y-6">
      <div>
        <h2 className="text-xl md:text-2xl font-bold text-gray-900">Lab Members</h2>
        <p className="text-gray-500">Registered users and their access permissions</p>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
        {users.map((user) => (
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
                <Badge className={getBranchColor(user.branch) + " whitespace-nowrap"}>
                  {user.branch}
                </Badge>
                <Badge variant="outline" className="whitespace-nowrap">
                  {user.year}
                </Badge>
              </div>

              <div>
                <p className="text-sm font-medium text-gray-700 mb-2">Machine Access:</p>
                <div className="flex flex-wrap gap-1">
                  {user.accessible_machines.length > 0 ? (
                    user.accessible_machines.map((machine) => (
                      <Badge key={machine} variant="secondary" className="text-xs whitespace-nowrap">
                        {machine}
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
                  <span className="whitespace-nowrap">Joined {user.created_at.toLocaleDateString()}</span>
                </div>
                <div className="flex items-center gap-1">
                  <div className={`w-2 h-2 rounded-full ${user.card_id ? 'bg-green-500' : 'bg-gray-300'}`}></div>
                  <span className="text-xs text-gray-500 whitespace-nowrap">
                    {user.card_id ? 'Card Linked' : 'No Card'}
                  </span>
                </div>
              </div>
            </CardContent>
          </Card>
        ))}
      </div>
    </div>
  );

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-50 via-blue-50 to-indigo-100">
      {/* Header */}
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
          <div className="flex items-center gap-4">
            <Button
              className="bg-gradient-to-r from-purple-500 to-blue-500 hover:from-purple-600 hover:to-blue-600 text-sm md:text-base"
              onClick={() => navigate("/login")}
            >
              <LogIn className="w-4 h-4 mr-2" />
              <span className="hidden sm:inline">Admin Login</span>
              <span className="sm:hidden">Login</span>
            </Button>
          </div>
        </div>
      </header>

      <div className="flex relative">
        {/* Sidebar */}
        <aside className={`
          w-64 bg-white/60 backdrop-blur-sm border-r border-white/20 min-h-screen p-6 fixed md:static inset-y-0 left-0 z-40 transform transition-transform duration-300 ease-in-out
          ${isSidebarOpen ? 'translate-x-0 z-40' : '-translate-x-full md:translate-x-0'}
        `}>
          <nav className="space-y-2">
            {[
              { id: 'overview', label: 'Overview', icon: Activity },
              { id: 'machines', label: 'Machines', icon: Monitor },
              { id: 'users', label: 'Users', icon: Users },
            ].map((item) => (
              <button
                key={item.id}
                onClick={() => {
                  setActiveTab(item.id);
                  setIsSidebarOpen(false);
                }}
                className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 ${activeTab === item.id
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

        {/* Overlay for mobile sidebar */}
        {isSidebarOpen && (
          <div
            className="fixed inset-0 bg-black bg-opacity-50 z-20 md:hidden"
            onClick={() => setIsSidebarOpen(false)}
          />
        )}

        {/* Main Content */}
        <main className="flex-1 p-4 md:p-6">
          {activeTab === 'overview' && renderOverview()}
          {activeTab === 'machines' && renderMachines()}
          {activeTab === 'users' && renderUsers()}
        </main>
      </div>
    </div>
  );
};

export default PublicDashboard;
