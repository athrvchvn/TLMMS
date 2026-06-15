
import React, { useState } from 'react';
import { UserPlus, Eye, EyeOff } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { toast } from '@/hooks/use-toast';
import { addUser, hashPin } from '@/lib/firebase';
import { useFirebaseData } from '@/contexts/Firebase';

const AddUserForm: React.FC = () => {
  const [newUser, setNewUser] = useState({
    name: '',
    roll_number: '',
    branch: '',
    year: '',
    pin: ''
  });
  const [showPin, setShowPin] = useState(false);
  const [loading, setLoading] = useState(false);
  const { email  } = useFirebaseData();

  const branches = [
    'Computer Science',
    'Electrical Engineering',
    'Mechanical Engineering',
    'Civil Engineering',
    'Chemical Engineering'
  ];

  const years = ['1st Year', '2nd Year', '3rd Year', '4th Year'];

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);

    try {
      // Validate roll number format (9 digits)
      if (!/^\d{9}$/.test(newUser.roll_number)) {
        toast({
          title: "Invalid Roll Number",
          description: "Roll number must be 9 digits (e.g., 240003021)",
          variant: "destructive",
        });
        return;
      }

      // Validate PIN (4 digits)
      if (!/^\d{4}$/.test(newUser.pin)) {
        toast({
          title: "Invalid PIN",
          description: "PIN must be exactly 4 digits",
          variant: "destructive",
        });
        return;
      }

      // FIX: use setDoc with roll_number as doc ID; hash the PIN (was raw plaintext)
      await addUser({
        roll_number: newUser.roll_number,
        name:        newUser.name,
        email:       '',
        branch:      newUser.branch,
        year:        newUser.year,
      });
      
      // Reset form
      setNewUser({
        name: '',
        roll_number: '',
        branch: '',
        year: '',
        pin: ''
      });

      toast({
        title: "User Added Successfully",
        description: `${newUser.name} has been added to the system`,
      });

    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to add user. Please try again.",
        variant: "destructive",
      });
    } finally {
      setLoading(false);
    }
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <UserPlus className="w-5 h-5" />
          Add New User
        </CardTitle>
        <CardDescription>
          Add a new user to the lab management system
        </CardDescription>
      </CardHeader>
      <CardContent>
        <form onSubmit={handleSubmit} className="space-y-4">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="name">Full Name</Label>
              <Input
                id="name"
                placeholder="Enter full name"
                value={newUser.name}
                onChange={(e) => setNewUser({ ...newUser, name: e.target.value })}
                required
              />
            </div>
            <div className="space-y-2">
              <Label htmlFor="roll_number">Roll Number</Label>
              <Input
                id="roll_number"
                placeholder="240003021"
                value={newUser.roll_number}
                onChange={(e) => setNewUser({ ...newUser, roll_number: e.target.value.replace(/\D/g, '').slice(0, 9) })}
                required
              />
            </div>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="branch">Branch</Label>
              <Select value={newUser.branch} onValueChange={(value) => setNewUser({ ...newUser, branch: value })}>
                <SelectTrigger>
                  <SelectValue placeholder="Select branch" />
                </SelectTrigger>
                <SelectContent>
                  {branches.map((branch) => (
                    <SelectItem key={branch} value={branch}>{branch}</SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label htmlFor="year">Year</Label>
              <Select value={newUser.year} onValueChange={(value) => setNewUser({ ...newUser, year: value })}>
                <SelectTrigger>
                  <SelectValue placeholder="Select year" />
                </SelectTrigger>
                <SelectContent>
                  {years.map((year) => (
                    <SelectItem key={year} value={year}>{year}</SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>
          </div>

          <div className="space-y-2">
            <Label htmlFor="pin">4-Digit PIN</Label>
            <div className="relative">
              <Input
                id="pin"
                type={showPin ? "text" : "password"}
                placeholder="Enter 4-digit PIN"
                value={newUser.pin}
                onChange={(e) => setNewUser({ ...newUser, pin: e.target.value.replace(/\D/g, '').slice(0, 4) })}
                className="pr-10"
                required
              />
              <button
                type="button"
                onClick={() => setShowPin(!showPin)}
                className="absolute right-3 top-1/2 transform -translate-y-1/2 text-gray-400 hover:text-gray-600"
              >
                {showPin ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
              </button>
            </div>
          </div>

          <Button type="submit" className="w-full" disabled={loading}>
            {loading ? 'Adding User...' : 'Add User'}
          </Button>
        </form>
      </CardContent>
    </Card>
  );
};

export default AddUserForm;
