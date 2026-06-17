import { Progress } from "@/components/ui/progress";
import { useFirebaseData } from "@/contexts/Firebase";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import {
    Shield, UserPlus, LogOut, Users, Eye, EyeOff,
    Upload, Trash2, RefreshCw
} from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { signOut } from "firebase/auth";
import { auth, db, triggerOta, revokeCard, unrevokeCard } from "@/lib/firebase";
import { toast } from '@/hooks/use-toast';
import { collection, addDoc, getDocs, where, query, updateDoc, onSnapshot, doc } from "firebase/firestore";
import Requests from "@/lib/requests";

interface Admin {
    id: string;
    username: string;
    email: string;
    roll_number?: string;
    created_at: Date;
    last_login?: Date;
    status: 'active' | 'inactive';
}

interface RevokedCard {
    uid: string;
    roll_number: string;
    reason: string;
    revoked_at: Date;
    revoked_by: string;
}

export default function SuperAdmin() {
    const [newAdmin, setNewAdmin] = useState({
        username: '', email: '', roll_number: '', password: '', confirmPassword: ''
    });
    const [showPassword, setShowPassword] = useState(false);
    const { loading, loggedIn, email, role } = useFirebaseData();
    const navigate = useNavigate();
    const [admins,       setAdmins]       = useState<Admin[]>([]);
    const [revokedCards, setRevokedCards] = useState<RevokedCard[]>([]);

    // OTA state
    const [otaVersion,  setOtaVersion]  = useState('');
    const [otaUrl,      setOtaUrl]      = useState('');
    const [otaLoading,  setOtaLoading]  = useState(false);

    // New revoke form state
    const [revokeUid,    setRevokeUid]    = useState('');
    const [revokeRoll,   setRevokeRoll]   = useState('');
    const [revokeReason, setRevokeReason] = useState('lost');
    const [revokeLoading, setRevokeLoading] = useState(false);

    const handleCreateAdmin = async (e: React.FormEvent) => {
        e.preventDefault();
        if (newAdmin.password !== newAdmin.confirmPassword) {
            toast({ title: "Passwords don't match", variant: "destructive" });
            return;
        }
        try {
            await Requests.POST("/auth/create-admin", {}, {
                email: newAdmin.email,
                password: newAdmin.password,
            });
        } catch {
            toast({ title: "Cannot create admin", description: "Admin with this email already exists.", variant: "destructive" });
            return;
        }
        await addDoc(collection(db, "admins"), {
            username:    newAdmin.username,
            email:       newAdmin.email,
            roll_number: newAdmin.roll_number,
            super_admin: email,
            status:      "active",
            created_at:  new Date(),
        });
        setNewAdmin({ username: '', email: '', roll_number: '', password: '', confirmPassword: '' });
        toast({ title: "Admin Created", description: `Admin account for ${newAdmin.username} created successfully.` });
    };

    const toggleAdminStatus = async (adminEmail: string) => {
        const q = query(collection(db, "admins"), where("email", "==", adminEmail));
        const snap = await getDocs(q);
        if (snap.empty) return;
        const adminDoc = snap.docs[0];
        const newStatus = adminDoc.data().status === "active" ? "inactive" : "active";
        await updateDoc(adminDoc.ref, { status: newStatus });
        setAdmins(prev => prev.map(a => a.email === adminEmail ? { ...a, status: newStatus as any } : a));
    };

    const handleTriggerOta = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!otaVersion.trim() || !otaUrl.trim()) return;
        setOtaLoading(true);
        try {
            await triggerOta(otaVersion.trim(), otaUrl.trim());
            toast({ title: "OTA Triggered", description: `Version ${otaVersion} queued for all nodes.` });
            setOtaVersion('');
            setOtaUrl('');
        } catch {
            toast({ title: "OTA Failed", description: "Could not write OTA command to RTDB.", variant: "destructive" });
        } finally {
            setOtaLoading(false);
        }
    };

    const handleRevokeCard = async (e: React.FormEvent) => {
        e.preventDefault();
        const uid = revokeUid.trim().toUpperCase();
        const roll = revokeRoll.trim();
        if (!uid || !roll) return;
        setRevokeLoading(true);
        try {
            await revokeCard(uid, roll, email ?? 'super_admin', revokeReason);
            toast({ title: "Card Revoked", description: `UID ${uid} has been revoked.` });
            setRevokeUid(''); setRevokeRoll(''); setRevokeReason('lost');
        } catch {
            toast({ title: "Revoke Failed", variant: "destructive" });
        } finally {
            setRevokeLoading(false);
        }
    };

    const handleUnrevokeCard = async (uid: string) => {
        try {
            await unrevokeCard(uid);
            toast({ title: "Card Unrevoked", description: `UID ${uid} restored.` });
        } catch {
            toast({ title: "Unrevoke Failed", variant: "destructive" });
        }
    };

    const onLogout = async () => {
        await signOut(auth);
        navigate("/login");
    };

    useEffect(() => {
        if (loading) return;
        if (!loggedIn) return navigate("/login");
        if (role !== "super-admin") return navigate("/login");

        const adminsQ = query(collection(db, "admins"), where("super_admin", "==", email));
        const unsubAdmins = onSnapshot(adminsQ, snap => {
            setAdmins(snap.docs.map(d => ({
                ...d.data(),
                id:         d.id,
                created_at: d.data().created_at?.toDate?.() ?? new Date(),
                last_login: d.data().last_login?.toDate?.(),
            })) as Admin[]);
        });

        const unsubRevoked = onSnapshot(collection(db, "revoked"), snap => {
            setRevokedCards(snap.docs.map(d => ({
                uid:         d.id,
                roll_number: d.data().roll_number ?? '',
                reason:      d.data().reason ?? '',
                revoked_at:  d.data().revoked_at?.toDate?.() ?? new Date(),
                revoked_by:  d.data().revoked_by ?? '',
            })));
        });

        return () => { unsubAdmins(); unsubRevoked(); };
    }, [loading, loggedIn]);

    if (loading) {
        return (
            <div className="absolute w-screen h-screen place-items-center grid">
                <Progress value={50} />
            </div>
        );
    }

    return (
        <div className="min-h-screen bg-gradient-to-br from-purple-600 via-blue-600 to-indigo-800">
            <div className="absolute inset-0 overflow-hidden">
                <div className="absolute -top-40 -right-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse" />
                <div className="absolute -bottom-40 -left-40 w-80 h-80 bg-white/10 rounded-full blur-3xl animate-pulse delay-1000" />
            </div>

            <header className="relative z-10 bg-white/10 backdrop-blur-sm border-b border-white/20">
                <div className="flex flex-col sm:flex-row items-center justify-between px-4 sm:px-6 py-4 gap-4 sm:gap-0">
                    <div className="flex items-center gap-3">
                        <div className="p-2 bg-white/20 rounded-lg backdrop-blur-sm">
                            <img
                                src="/lovable-uploads/4eecbf7d-95ff-44fb-b13d-35d974c4d878.png"
                                alt="Tinkerer's Lab Logo"
                                className="w-6 h-6"
                            />
                        </div>
                        <div>
                            <h1 className="text-lg sm:text-xl font-bold text-white">Tinkerer's Lab — Super Admin</h1>
                            <p className="text-xs sm:text-sm text-white/70">Administrator Management System</p>
                        </div>
                    </div>
                    <Button
                        variant="outline"
                        onClick={onLogout}
                        className="w-full sm:w-auto bg-white/10 border-white/20 text-white hover:bg-white/20"
                    >
                        <LogOut className="w-4 h-4 mr-2" />
                        Logout
                    </Button>
                </div>
            </header>

            <div className="relative z-10 max-w-7xl mx-auto p-6 space-y-8">

                {/* Row 1: Create Admin + Existing Admins */}
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
                    {/* Create Admin */}
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
                                <Button type="submit" className="w-full bg-white text-purple-600 hover:bg-white/90 font-medium">
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
                            <div className="space-y-4 max-h-96 overflow-y-auto">
                                {admins.map((admin) => (
                                    <div key={admin.id} className="p-4 bg-white/5 rounded-lg border border-white/10 hover:bg-white/10 transition-colors">
                                        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
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
                                            <div className="flex flex-col sm:flex-row items-start sm:items-center gap-2">
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
                                                    onClick={() => toggleAdminStatus(admin.email)}
                                                    className="bg-white/10 border-white/20 text-white hover:bg-white/20 text-xs w-full sm:w-auto"
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
                                {admins.length === 0 && (
                                    <p className="text-white/50 text-sm text-center py-4">No administrators yet</p>
                                )}
                            </div>
                        </CardContent>
                    </Card>
                </div>

                {/* Row 2: OTA Trigger + Card Revocation */}
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
                    {/* OTA Trigger */}
                    <Card className="bg-white/10 backdrop-blur-lg border-white/20">
                        <CardHeader>
                            <CardTitle className="text-white flex items-center gap-2">
                                <Upload className="w-5 h-5" />
                                Firmware OTA Update
                            </CardTitle>
                            <CardDescription className="text-white/70">
                                Push a new firmware version to all connected nodes via RTDB.
                                Nodes will download from the Firebase Storage URL and restart.
                            </CardDescription>
                        </CardHeader>
                        <CardContent>
                            <form onSubmit={handleTriggerOta} className="space-y-4">
                                <div className="space-y-2">
                                    <Label htmlFor="ota_version" className="text-white">Target Version</Label>
                                    <Input
                                        id="ota_version"
                                        placeholder="e.g. 2.1.0"
                                        value={otaVersion}
                                        onChange={(e) => setOtaVersion(e.target.value)}
                                        className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                                        required
                                    />
                                </div>
                                <div className="space-y-2">
                                    <Label htmlFor="ota_url" className="text-white">Firmware URL (Firebase Storage)</Label>
                                    <Input
                                        id="ota_url"
                                        placeholder="https://firebasestorage.googleapis.com/..."
                                        value={otaUrl}
                                        onChange={(e) => setOtaUrl(e.target.value)}
                                        className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40"
                                        required
                                    />
                                </div>
                                <div className="p-3 bg-yellow-500/10 border border-yellow-500/30 rounded-lg">
                                    <p className="text-yellow-300 text-xs">
                                        All online nodes will receive this update immediately.
                                        Offline nodes will receive it on next reconnect.
                                        Nodes will turn off the relay before updating.
                                    </p>
                                </div>
                                <Button
                                    type="submit"
                                    disabled={otaLoading}
                                    className="w-full bg-yellow-500 text-black hover:bg-yellow-400 font-medium"
                                >
                                    <RefreshCw className={`w-4 h-4 mr-2 ${otaLoading ? 'animate-spin' : ''}`} />
                                    {otaLoading ? 'Triggering...' : 'Trigger OTA Update'}
                                </Button>
                            </form>
                        </CardContent>
                    </Card>

                    {/* Card Revocation */}
                    <Card className="bg-white/10 backdrop-blur-lg border-white/20">
                        <CardHeader>
                            <CardTitle className="text-white flex items-center gap-2">
                                <Shield className="w-5 h-5" />
                                Card Revocation
                            </CardTitle>
                            <CardDescription className="text-white/70">
                                Revoke lost or stolen cards. All connected nodes are updated within seconds.
                            </CardDescription>
                        </CardHeader>
                        <CardContent className="space-y-6">
                            {/* New revoke form */}
                            <form onSubmit={handleRevokeCard} className="space-y-3">
                                <div className="grid grid-cols-2 gap-3">
                                    <div className="space-y-1">
                                        <Label className="text-white text-xs">Card UID (hex)</Label>
                                        <Input
                                            placeholder="A1B2C3D4"
                                            value={revokeUid}
                                            onChange={(e) => setRevokeUid(e.target.value.toUpperCase().replace(/[^0-9A-F]/g, ''))}
                                            className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40 text-sm"
                                            required
                                        />
                                    </div>
                                    <div className="space-y-1">
                                        <Label className="text-white text-xs">Roll Number</Label>
                                        <Input
                                            placeholder="240001001"
                                            value={revokeRoll}
                                            onChange={(e) => setRevokeRoll(e.target.value.replace(/\D/g, '').slice(0, 9))}
                                            className="bg-white/10 border-white/20 text-white placeholder:text-white/50 focus:border-white/40 text-sm"
                                            required
                                        />
                                    </div>
                                </div>
                                <div className="space-y-1">
                                    <Label className="text-white text-xs">Reason</Label>
                                    <select
                                        value={revokeReason}
                                        onChange={(e) => setRevokeReason(e.target.value)}
                                        className="w-full rounded-md bg-white/10 border border-white/20 text-white text-sm px-3 py-2 focus:outline-none focus:border-white/40"
                                    >
                                        <option value="lost" className="text-black">Lost</option>
                                        <option value="stolen" className="text-black">Stolen</option>
                                        <option value="expired" className="text-black">Expired</option>
                                    </select>
                                </div>
                                <Button
                                    type="submit"
                                    disabled={revokeLoading}
                                    variant="destructive"
                                    className="w-full"
                                >
                                    <Trash2 className="w-4 h-4 mr-2" />
                                    {revokeLoading ? 'Revoking...' : 'Revoke Card'}
                                </Button>
                            </form>

                            {/* Revoked cards list */}
                            {revokedCards.length > 0 && (
                                <div className="space-y-2">
                                    <p className="text-white/70 text-xs font-medium uppercase tracking-wide">
                                        Revoked Cards ({revokedCards.length})
                                    </p>
                                    <div className="space-y-2 max-h-48 overflow-y-auto">
                                        {revokedCards.map((card) => (
                                            <div key={card.uid} className="flex items-center justify-between p-2 bg-red-500/10 border border-red-500/20 rounded-lg">
                                                <div>
                                                    <p className="text-white text-xs font-mono">{card.uid}</p>
                                                    <p className="text-white/60 text-xs">
                                                        {card.roll_number} · {card.reason} · {card.revoked_at.toLocaleDateString()}
                                                    </p>
                                                </div>
                                                <Button
                                                    size="sm"
                                                    variant="outline"
                                                    onClick={() => handleUnrevokeCard(card.uid)}
                                                    className="bg-white/10 border-white/20 text-white hover:bg-white/20 text-xs ml-2 flex-shrink-0"
                                                >
                                                    Restore
                                                </Button>
                                            </div>
                                        ))}
                                    </div>
                                </div>
                            )}
                        </CardContent>
                    </Card>
                </div>

                {/* Stats row */}
                <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
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
                                    <p className="text-2xl font-bold text-white">
                                        {admins.filter(a => a.status === 'active').length}
                                    </p>
                                </div>
                                <Shield className="w-8 h-8 text-green-400" />
                            </div>
                        </CardContent>
                    </Card>
                    <Card className="bg-white/10 backdrop-blur-lg border-white/20">
                        <CardContent className="p-6">
                            <div className="flex items-center justify-between">
                                <div>
                                    <p className="text-white/70 text-sm">Revoked Cards</p>
                                    <p className="text-2xl font-bold text-white">{revokedCards.length}</p>
                                </div>
                                <Trash2 className="w-8 h-8 text-red-400" />
                            </div>
                        </CardContent>
                    </Card>
                </div>
            </div>
        </div>
    );
}
