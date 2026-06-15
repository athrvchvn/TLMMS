
import React, { useState } from 'react';
import { Shield, UserPlus, LogOut, Users, Eye, EyeOff } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { toast } from '@/hooks/use-toast';

interface SuperAdminPanelProps {
  onLogout: () => void;
}

interface Admin {
  id: string;
  username: string;
  email: string;
  roll_number?: string;
  created_at: Date;
  last_login?: Date;
  status: 'active' | 'inactive';
}

const SuperAdminPanel: React.FC<SuperAdminPanelProps> = ({ onLogout }) => {
  const [newAdmin, setNewAdmin] = useState({
    username: '',
    email: '',
    roll_number: '',
    password: '',
    confirmPassword: ''
  });
  const [showPassword, setShowPassword] = useState(false);
  const [admins, setAdmins] = useState<Admin[]>([
    {
      id: '1',
      username: 'admin',
      email: 'labmanagerTL@iiti.ac.in',
      roll_number: '240001001',
      created_at: new Date('2024-01-15'),
      last_login: new Date('2024-06-29'),
      status: 'active'
    },
    {
      id: '2',
      username: 'lab_manager',
      email: 'juniorsenatormech@iiti.ac.in',
      created_at: new Date('2024-02-10'),
      last_login: new Date('2024-06-28'),
      status: 'active'
    }
  ]);

  const handleCreateAdmin = async (e: React.FormEvent) => {
    e.preventDefault();

    if (!newAdmin.username || !newAdmin.email || !newAdmin.password) {
      toast({
        title: "Error",
        description: "Please fill in all required fields",
        variant: "destructive",
      });
      return;
    }

    if (newAdmin.password !== newAdmin.confirmPassword) {
      toast({
        title: "Error",
        description: "Passwords do not match",
        variant: "destructive",
      });
      return;
    }

    if (newAdmin.password.length < 6) {
      toast({
        title: "Error",
        description: "Password must be at least 6 characters long",
        variant: "destructive",
      });
      return;
    }

    // Check if username already exists
    if (admins.some(admin => admin.username === newAdmin.username)) {
      toast({
        title: "Error",
        description: "Username already exists",
        variant: "destructive",
      });
      return;
    }

    // Validate email format for IITI
    if (newAdmin.email && !newAdmin.email.includes('@iiti.ac.in')) {
      toast({
        title: "Error",
        description: "Email must be an IITI email address (@iiti.ac.in)",
        variant: "destructive",
      });
      return;
    }

    // Validate roll number format if provided
    if (newAdmin.roll_number && !/^\d{9}$/.test(newAdmin.roll_number)) {
      toast({
        title: "Error",
        description: "Roll number must be 9 digits (e.g., 240001001)",
        variant: "destructive",
      });
      return;
    }

    try {
      const newAdminData: Admin = {
        id: Date.now().toString(),
        username: newAdmin.username,
        email: newAdmin.email,
        roll_number: newAdmin.roll_number || undefined,
        created_at: new Date(),
        status: 'active'
      };

      setAdmins([...admins, newAdminData]);
      setNewAdmin({
        username: '',
        email: '',
        roll_number: '',
        password: '',
        confirmPassword: ''
      });

      toast({
        title: "Admin Created",
        description: `Admin account for ${newAdmin.username} has been created successfully`,
      });
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to create admin account. Please try again.",
        variant: "destructive",
      });
    }
  };

  const toggleAdminStatus = (adminId: string) => {
    setAdmins(admins.map(admin => 
      admin.id === adminId 
        ? { ...admin, status: admin.status === 'active' ? 'inactive' : 'active' }
        : admin
    ));
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-600 via-blue-600 to-indigo-800">
      {/* Animated Background */}
      <div className="absolute inset-0 overflow-hidden">
        <div className="absolute -top-40 -right-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse"></div>
        <div className="absolute -bottom-40 -left-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse delay-1000"></div>
      </div>

      {/* Header */}
      <header className="relative z-10 bg-white/10 backdrop-blur-sm border-b border-white/20">
        <div className="flex items-center justify-between px-6 py-4">
          <div className="flex items-center gap-3">
            <div className="p-2 bg-white/20 rounded-lg backdrop-blur-sm">
              <img 
                src="/lovable-uploads/4eecbf7d-95ff-44fb-b13d-35d974c4d878.png" 
                alt="Tinkerer's Lab Logo" 
                className="w-6 h-6"
              />
            </div>
            <div>
              <h1 className="text-xl font-bold text-white">Tinkerer's Lab - Super Admin</h1>
              <p className="text-sm text-white/70">Administrator Management System</p>
            </div>
          </div>
          <Button variant="outline" onClick={onLogout} className="bg-white/10 border-white/20 text-white hover:bg-white/20">
            <LogOut className="w-4 h-4 mr-2" />
            Logout
          </Button>
        </div>
      </header>

      <div className="relative z-10 max-w-6xl mx-auto p-6">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
          {/* Create Admin Form */}
          <Card className="bg-white/10 backdrop-blur-lg border-white/20">
            <CardHeader>
              <CardTitle className="text-white flex items-center gap-2">
                <UserPlus className="w-5 h-5" />
                Create New Administrator
              </CardTitle>
              <CardDescription className="text-white/70">
                Add a new administrator account to the system
              </CardDescription>
            </CardHeader>
            <CardContent>
              <form onSubmit={handleCreateAdmin} className="space-y-4">
                <div className="space-y-2">
                  <Label htmlFor="username" className="text-white">Username</Label>
                  <Input
                    id="username"
                    placeholder="Enter username"
                    value={newAdmin.username}
                    onChange={(e) => setNewAdmin({ ...newAdmin, username: e.target.value })}
                    className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                    required
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="email" className="text-white">Email Address</Label>
                  <Input
                    id="email"
                    type="email"
                    placeholder="labmanagerTL@iiti.ac.in"
                    value={newAdmin.email}
                    onChange={(e) => setNewAdmin({ ...newAdmin, email: e.target.value })}
                    className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                    required
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="roll_number" className="text-white">Roll Number (Optional)</Label>
                  <Input
                    id="roll_number"
                    placeholder="240001001"
                    value={newAdmin.roll_number}
                    onChange={(e) => setNewAdmin({ ...newAdmin, roll_number: e.target.value.replace(/\D/g, '').slice(0, 9) })}
                    className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="password" className="text-white">Password</Label>
                  <div className="relative">
                    <Input
                      id="password"
                      type={showPassword ? "text" : "password"}
                      placeholder="Enter password"
                      value={newAdmin.password}
                      onChange={(e) => setNewAdmin({ ...newAdmin, password: e.target.value })}
                      className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40 pr-10"
                      required
                    />
                    <button
                      type="button"
                      onClick={() => setShowPassword(!showPassword)}
                      className="absolute right-3 top-1/2 transform -translate-y-1/2 text-white/50 hover:text-white/80"
                    >
                      {showPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
                    </button>
                  </div>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="confirmPassword" className="text-white">Confirm Password</Label>
                  <Input
                    id="confirmPassword"
                    type="password"
                    placeholder="Confirm password"
                    value={newAdmin.confirmPassword}
                    onChange={(e) => setNewAdmin({ ...newAdmin, confirmPassword: e.target.value })}
                    className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                    required
                  />
                </div>
                <Button
                  type="submit"
                  className="w-full bg-white text-purple-600 hover:bg-white/90 font-medium"
                >
                  <UserPlus className="w-4 h-4 mr-2" />
                  Create Administrator
                </Button>
              </form>
            </CardContent>
          </Card>

          {/* Existing Admins */}
          <Card className="bg-white/10 backdrop-blur-lg border-white/20">
            <CardHeader>
              <CardTitle className="text-white flex items-center gap-2">
                <Users className="w-5 h-5" />
                Existing Administrators
              </CardTitle>
              <CardDescription className="text-white/70">
                Manage existing administrator accounts
              </CardDescription>
            </CardHeader>
            <CardContent>
              <div className="space-y-4">
                {admins.map((admin) => (
                  <div
                    key={admin.id}
                    className="p-4 bg-white/5 rounded-lg border border-white/10 hover:bg-white/10 transition-colors"
                  >
                    <div className="flex items-center justify-between">
                      <div className="flex items-center gap-3">
                        <div className="w-10 h-10 bg-gradient-to-br from-purple-400 to-blue-400 rounded-full flex items-center justify-center text-white font-bold text-sm">
                          {admin.username.charAt(0).toUpperCase()}
                        </div>
                        <div>
                          <p className="font-medium text-white">{admin.username}</p>
                          <p className="text-sm text-white/60">{admin.email}</p>
                          {admin.roll_number && (
                            <p className="text-xs text-white/50">Roll: {admin.roll_number}</p>
                          )}
                        </div>
                      </div>
                      <div className="flex items-center gap-2">
                        <div className={`px-2 py-1 rounded-full text-xs ${
                          admin.status === 'active' 
                            ? 'bg-green-500/20 text-green-300 border border-green-500/30' 
                            : 'bg-red-500/20 text-red-300 border border-red-500/30'
                        }`}>
                          {admin.status}
                        </div>
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => toggleAdminStatus(admin.id)}
                          className="bg-white/10 border-white/20 text-white hover:bg-white/20 text-xs"
                        >
                          {admin.status === 'active' ? 'Deactivate' : 'Activate'}
                        </Button>
                      </div>
                    </div>
                    <div className="mt-3 grid grid-cols-2 gap-4 text-xs text-white/60">
                      <div>
                        <span className="block">Created:</span>
                        <span>{admin.created_at.toLocaleDateString()}</span>
                      </div>
                      <div>
                        <span className="block">Last Login:</span>
                        <span>{admin.last_login ? admin.last_login.toLocaleDateString() : 'Never'}</span>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </CardContent>
          </Card>
        </div>

        {/* Stats */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mt-8">
          <Card className="bg-white/10 backdrop-blur-lg border-white/20">
            <CardContent className="p-6">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-white/70 text-sm">Total Admins</p>
                  <p className="text-2xl font-bold text-white">{admins.length}</p>
                </div>
                <Users className="w-8 h-8 text-white/50" />
              </div>
            </CardContent>
          </Card>
          <Card className="bg-white/10 backdrop-blur-lg border-white/20">
            <CardContent className="p-6">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-white/70 text-sm">Active Admins</p>
                  <p className="text-2xl font-bold text-white">{admins.filter(a => a.status === 'active').length}</p>
                </div>
                <Shield className="w-8 h-8 text-green-400" />
              </div>
            </CardContent>
          </Card>
          <Card className="bg-white/10 backdrop-blur-lg border-white/20">
            <CardContent className="p-6">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-white/70 text-sm">Lab Status</p>
                  <p className="text-2xl font-bold text-white">Active</p>
                </div>
                <div className="w-3 h-3 bg-green-400 rounded-full animate-pulse"></div>
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  );
};

export default SuperAdminPanel;
