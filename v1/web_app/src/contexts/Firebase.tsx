import { auth, db } from "@/lib/firebase";
import { onAuthStateChanged } from "firebase/auth";
import { collection, getDocs, where, query } from "firebase/firestore";
import { createContext, useContext, useState, useEffect, useMemo } from "react";

type Context = {
  username: string | null;
  email: string | null;
  role: 'admin' | 'super-admin' | 'student' | null;
  loggedIn: boolean;
  loading: boolean;
};

const context = createContext<Context>({} as any)

export function useFirebaseData() {
  return useContext(context);
}

export default function FirebaseProvider({ children }: { children: React.ReactNode }) {
  const [username, setUsername] = useState<string | null>(null);
  const [email, setEmail] = useState<string | null>(null);
  const [role, setRole] = useState<Context["role"]>(null);
  const [loggedIn, setLoggedIn] = useState(false);
  const [loading, setLoading] = useState(true);
  const superAdminCollection = useMemo(() => collection(db, "super_admin"), [db]);
  const adminCollection = useMemo(() => collection(db, "admin"), [db]);

  useEffect(() => {
    const cancel = onAuthStateChanged(auth, async (user) => {
      setLoading(true);
      if (!user) {
        setLoading(false);
        setLoggedIn(false);
        return;
      }

      setEmail(user.email);
      let data = (await getDocs(
        query(superAdminCollection, where("email", "==", user.email))
      )).docs.map(doc => doc.data());

      if (data.length >= 1) {
        setUsername(data[0].name);
        setRole("super-admin");
      } else {
        data = (await getDocs(query(adminCollection, where("email", "==", user.email))))
          .docs.map(doc => doc.data());
        if (data.length >= 1) {
          setUsername(data[0].username);
          setRole("admin");
        }
      }
      setLoading(false);
      setLoggedIn(true);
    })

    return () => cancel();
  }, [])

  const values: Context = {
    username,
    email,
    role,
    loggedIn,
    loading,
  };

  return (
    <context.Provider value={values}>
      {children}
    </context.Provider>
  )
}
