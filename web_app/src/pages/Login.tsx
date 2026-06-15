import { useMemo, useState } from "react";
import { Shield, LogIn } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { toast } from '@/hooks/use-toast';
import { useNavigate } from "react-router-dom";
import { useSignInWithEmailAndPassword } from "react-firebase-hooks/auth";
import { auth, db } from "@/lib/firebase";
import { collection, getDocs, where, query, updateDoc } from "firebase/firestore";

export default function LoginPage() {
  const [loginType, setLoginType] = useState<'admin' | 'super_admin'>('admin');
  const [credentials, setCredentials] = useState({ email: '', password: '' });
  const navigate = useNavigate();
  const [signInWithEmailAndPassword, user, loading, error] = useSignInWithEmailAndPassword(auth)
  const superAdminCollection = useMemo(() => collection(db, "super_admins"), [db]);
  const adminCollection = useMemo(() => collection(db, "admins"), [db]);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    if (loginType === "super_admin") {
      const data = (await getDocs(
        query(superAdminCollection, where("email", "==", credentials.email))
      )).docs.map(doc => doc.data());
      if (data.length < 1) {
        toast({
          title: "Invalid login credentials",
          description: "No such super admin with the email address was found in the database",
          variant: "destructive"
        });
        return;
      }
      const user = await signInWithEmailAndPassword(credentials.email, credentials.password);
      if (!user) {
        toast({
          title: "Invalid login credentials",
          description: "Invalid password provided",
          variant: "destructive"
        });
        return;
      }
      navigate("/admin/super");
    } else {
      const data = (await getDocs(query(adminCollection, where("email", "==", credentials.email))))
        .docs;
      if (data.length < 1) {
        toast({
          title: "Invalid login credentials",
          description: "No such super admin with the email address was found in the database",
          variant: "destructive"
        });
        return;
      }
      const user = await signInWithEmailAndPassword(credentials.email, credentials.password);
      if (!user) {
        toast({
          title: "Invalid login credentials",
          description: "Invalid password provided",
          variant: "destructive"
        });
        return;
      }
      const docRef = data[0];
      await updateDoc(docRef.ref, {
        last_login: new Date(),
      });
      navigate("/admin");
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-600 via-blue-600 to-indigo-800 flex items-center justify-center p-4">
      {/* Animated Background Elements */}
      <div className="absolute inset-0 overflow-hidden">
        <div className="absolute -top-40 -right-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse"></div>
        <div className="absolute -bottom-40 -left-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse delay-1000"></div>
      </div>

      <div className="relative z-10 w-full max-w-md">
        {/* Header */}
        <div className="text-center mb-8">
          <div className="flex justify-center mb-4">
            <div className="p-3 bg-white/20 rounded-full backdrop-blur-sm">
              <img
                src="/lovable-uploads/4eecbf7d-95ff-44fb-b13d-35d974c4d878.png"
                alt="Tinkerer's Lab Logo"
                className="w-8 h-8"
              />
            </div>
          </div>
          <h1 className="text-3xl font-bold text-white mb-2">Tinkerer's Lab</h1>
          <p className="text-white/80">Machine Management System</p>
        </div>

        {/* Login Type Selector */}
        <div className="flex mb-6 p-1 bg-white/10 rounded-lg backdrop-blur-sm">
          <button
            type="button"
            onClick={() => setLoginType('admin')}
            className={`flex-1 py-2 px-4 rounded-md transition-all duration-200 flex items-center justify-center gap-2 ${loginType === 'admin'
              ? 'bg-white text-purple-600 shadow-lg'
              : 'text-white hover:bg-white/10'
              }`}
          >
            <LogIn className="w-4 h-4" />
            Admin Login
          </button>
          <button
            type="button"
            onClick={() => setLoginType('super_admin')}
            className={`flex-1 py-2 px-4 rounded-md transition-all duration-200 flex items-center justify-center gap-2 ${loginType === 'super_admin'
              ? 'bg-white text-purple-600 shadow-lg'
              : 'text-white hover:bg-white/10'
              }`}
          >
            <Shield className="w-4 h-4" />
            Super Admin
          </button>
        </div>

        {/* Login Form */}
        <Card className="bg-white/10 backdrop-blur-lg border-white/20 shadow-2xl">
          <CardHeader className="text-center">
            <CardTitle className="text-white flex items-center justify-center gap-2">
              {loginType === 'admin' ? (
                <>
                  <LogIn className="w-5 h-5" />
                  Admin Login
                </>
              ) : (
                <>
                  <Shield className="w-5 h-5" />
                  Super Admin Login
                </>
              )}
            </CardTitle>
            <CardDescription className="text-white/70">
              {loginType === 'admin'
                ? 'Access the lab management dashboard'
                : 'Manage administrator accounts'}
            </CardDescription>
          </CardHeader>
          <CardContent>
            <form onSubmit={handleLogin} className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="username" className="text-white">Email</Label>
                <Input
                  id="email"
                  type="email"
                  placeholder="Enter email"
                  value={credentials.email}
                  onChange={(e) => setCredentials({ ...credentials, email: e.target.value })}
                  className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                  required
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="password" className="text-white">Password</Label>
                <Input
                  id="password"
                  type="password"
                  placeholder="Enter password"
                  value={credentials.password}
                  onChange={(e) => setCredentials({ ...credentials, password: e.target.value })}
                  className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                  required
                />
              </div>
              <div className="flex gap-2">
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => navigate("/")}
                  className="flex-1 bg-white/10 border-white/20 text-white hover:bg-white/20"
                >
                  Back
                </Button>
                <Button
                  type="submit"
                  className="flex-1 bg-white text-purple-600 hover:bg-white/90 font-medium transition-all duration-200"
                  disabled={loading}
                >
                  {loading ? 'Signing In...' : 'Sign In'}
                </Button>
              </div>
            </form>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
