import React, { useState } from 'react';
import { Search, Edit, UserCheck, Clock } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Badge } from '@/components/ui/badge';
import { Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle } from '@/components/ui/dialog';
import { Checkbox } from '@/components/ui/checkbox';
import { toast } from '@/hooks/use-toast';
import { User, updateUserPermissions, permMaskToIds, idsToPermMask } from '@/lib/firebase';
import { useMachines } from '@/contexts/Machines';

interface UserManagementProps {
    users: User[];
}

const UserManagement: React.FC<UserManagementProps> = ({ users }) => {
    const [searchTerm, setSearchTerm] = useState('');
    const [selectedUser, setSelectedUser] = useState<User | null>(null);
    const [isEditAccessOpen, setIsEditAccessOpen] = useState(false);
    const [selectedMachineIds, setSelectedMachineIds] = useState<number[]>([]);
    const { machines } = useMachines();

    const filteredUsers = users.filter(user =>
        user.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
        user.roll_number.toLowerCase().includes(searchTerm.toLowerCase()) ||
        (user.branch ?? '').toLowerCase().includes(searchTerm.toLowerCase())
    );

    const handleEditAccess = (user: User) => {
        setSelectedUser(user);
        setSelectedMachineIds(permMaskToIds(user.machine_permissions));
        setIsEditAccessOpen(true);
    };

    const handleSaveAccess = async () => {
        if (!selectedUser) return;
        try {
            await updateUserPermissions(selectedUser.roll_number, idsToPermMask(selectedMachineIds));
            setIsEditAccessOpen(false);
            setSelectedUser(null);
            setSelectedMachineIds([]);
            toast({ title: 'Access Updated', description: `Machine access updated for ${selectedUser.name}` });
        } catch {
            toast({ title: 'Error', description: 'Failed to update access.', variant: 'destructive' });
        }
    };

    const handleMachineToggle = (machineId: number) => {
        setSelectedMachineIds(prev =>
            prev.includes(machineId)
                ? prev.filter(id => id !== machineId)
                : [...prev, machineId]
        );
    };

    const getBranchColor = (branch: string) => {
        const colors: Record<string, string> = {
            'Computer Science':      'bg-blue-100 text-blue-800',
            'Electrical Engineering':'bg-yellow-100 text-yellow-800',
            'Mechanical Engineering':'bg-green-100 text-green-800',
            'Civil Engineering':     'bg-orange-100 text-orange-800',
            'Electronics':           'bg-purple-100 text-purple-800',
        };
        return colors[branch] || 'bg-gray-100 text-gray-800';
    };

    const getMachineNames = (permMask: number): string[] => {
        return permMaskToIds(permMask).map(
            id => machines.find(m => m.id === String(id))?.name ?? `Machine ${id}`
        );
    };

    return (
        <div className="space-y-4 md:space-y-6">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
                <div>
                    <h2 className="text-xl md:text-2xl font-bold text-gray-900">User Management</h2>
                    <p className="text-gray-500 text-sm md:text-base">Manage user accounts and machine access permissions</p>
                </div>
                <div className="relative flex-1 sm:flex-none">
                    <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 text-gray-400 w-4 h-4" />
                    <Input
                        placeholder="Search users..."
                        value={searchTerm}
                        onChange={(e) => setSearchTerm(e.target.value)}
                        className="pl-10 w-full sm:w-64"
                    />
                </div>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 md:gap-6">
                {filteredUsers.map((user) => {
                    const machineNames = getMachineNames(user.machine_permissions);
                    return (
                        <Card key={user.roll_number} className="group hover:shadow-lg transition-all duration-200 border-2 hover:border-purple-200">
                            <CardHeader className="pb-3">
                                <div className="flex items-start justify-between gap-2">
                                    <div className="flex items-center gap-2 md:gap-3 min-w-0 flex-1">
                                        <div className="w-10 h-10 md:w-12 md:h-12 bg-gradient-to-br from-purple-500 to-blue-500 rounded-full flex items-center justify-center text-white font-bold flex-shrink-0">
                                            {user.name.charAt(0)}
                                        </div>
                                        <div className="min-w-0">
                                            <CardTitle className="text-base md:text-lg truncate">{user.name}</CardTitle>
                                            <p className="text-sm text-gray-500 truncate">{user.roll_number}</p>
                                        </div>
                                    </div>
                                    <Button
                                        size="sm"
                                        variant="outline"
                                        onClick={() => handleEditAccess(user)}
                                        className="opacity-0 group-hover:opacity-100 transition-opacity p-1 md:p-2 flex-shrink-0"
                                    >
                                        <Edit className="w-3 h-3 md:w-4 md:h-4" />
                                    </Button>
                                </div>
                            </CardHeader>
                            <CardContent className="space-y-3 md:space-y-4">
                                <div className="flex items-center justify-between flex-wrap gap-2">
                                    <Badge className={getBranchColor(user.branch ?? '') + ' whitespace-nowrap'}>
                                        {user.branch ?? '—'}
                                    </Badge>
                                    <Badge variant="outline" className="whitespace-nowrap">
                                        {user.year ?? '—'}
                                    </Badge>
                                </div>

                                <div>
                                    <p className="text-sm font-medium text-gray-700 mb-1 md:mb-2">Machine Access:</p>
                                    <div className="flex flex-wrap gap-1">
                                        {machineNames.length > 0 ? (
                                            machineNames.map((name) => (
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
                                        <Clock className="w-3 h-3 flex-shrink-0" />
                                        <span className="whitespace-nowrap">
                                            {(user.issued_at as any)?.toDate?.()?.toLocaleDateString() ?? 'N/A'}
                                        </span>
                                    </div>
                                    <div className="flex items-center gap-1">
                                        <div className={`w-2 h-2 rounded-full ${user.card_uid ? 'bg-green-500' : 'bg-gray-300'} flex-shrink-0`} />
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

            <Dialog open={isEditAccessOpen} onOpenChange={setIsEditAccessOpen}>
                <DialogContent className="sm:max-w-md max-h-[90vh] overflow-y-auto">
                    <DialogHeader>
                        <DialogTitle>Edit Machine Access</DialogTitle>
                        <DialogDescription>
                            {selectedUser && `Manage machine access for ${selectedUser.name} (${selectedUser.roll_number})`}
                        </DialogDescription>
                    </DialogHeader>
                    <div className="space-y-4">
                        <div className="space-y-3 max-h-[300px] overflow-y-auto">
                            {machines.map((machine) => {
                                const numId = parseInt(machine.id);
                                return (
                                    <div key={machine.id} className="flex items-center space-x-2">
                                        <Checkbox
                                            id={`m_${machine.id}`}
                                            checked={selectedMachineIds.includes(numId)}
                                            onCheckedChange={() => handleMachineToggle(numId)}
                                        />
                                        <label
                                            htmlFor={`m_${machine.id}`}
                                            className="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70"
                                        >
                                            {machine.name}
                                        </label>
                                    </div>
                                );
                            })}
                            {machines.length === 0 && (
                                <p className="text-sm text-muted-foreground">Loading machines...</p>
                            )}
                        </div>
                        <div className="flex justify-end gap-2 pt-4 flex-wrap">
                            <Button variant="outline" onClick={() => setIsEditAccessOpen(false)} className="flex-1 sm:flex-none">
                                Cancel
                            </Button>
                            <Button onClick={handleSaveAccess} className="flex-1 sm:flex-none">
                                <UserCheck className="w-4 h-4 mr-2" />
                                Save Access
                            </Button>
                        </div>
                    </div>
                </DialogContent>
            </Dialog>

            {filteredUsers.length === 0 && (
                <Card>
                    <CardContent className="flex flex-col items-center justify-center py-8 md:py-12">
                        <Search className="w-8 h-8 md:w-12 md:h-12 text-gray-400 mb-3 md:mb-4" />
                        <h3 className="text-base md:text-lg font-medium text-gray-900 mb-1 md:mb-2">No users found</h3>
                        <p className="text-gray-500 text-center text-sm md:text-base">
                            {searchTerm ? `No users match "${searchTerm}"` : 'No users have been added yet'}
                        </p>
                    </CardContent>
                </Card>
            )}
        </div>
    );
};

export default UserManagement;
