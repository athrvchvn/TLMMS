import Dashboard from "@/components/Dashboard";
import { Progress } from "@/components/ui/progress";
import { useFirebaseData } from "@/contexts/Firebase";
import { useEffect } from "react";
import { useNavigate } from "react-router-dom";

export default function Admin() {
  const { loading, loggedIn, role } = useFirebaseData();
  const navigate = useNavigate();

  useEffect(() => {
    if (loading) return;

    if (!loggedIn) return navigate("/login")

    if (role !== "admin") return navigate("/login");
  }, [loading, loggedIn, role])

  if (loading) {
    return (
      <div className="absolute w-screen h-screen place-items-center grid">
        <Progress value={50} />
      </div>
    )
  }

  return <Dashboard />
}
