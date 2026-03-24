import { useEffect, useState } from "react";
import { onAuthStateChanged, type User } from "firebase/auth";
import { auth } from "./firebase";
import { getDatabase, ref, get } from "firebase/database";
import Home from "./Home";
import Login from "./Login";
import "./App.css";

export default function App() {
  const [user,    setUser]    = useState<User | null>(null);
  const [loading, setLoading] = useState(true);
  const [valid,   setValid]   = useState(false);

  useEffect(() => {
    const unsub = onAuthStateChanged(auth, async (u) => {
      setUser(u);
      if (u) {
        // check if user email exists in DB
        const emailKey = u.email!.replace(/\./g, "_");
        const db       = getDatabase();
        const snap     = await get(ref(db, emailKey));
        setValid(snap.exists());
      } else {
        setValid(false);
      }
      setLoading(false);
    });
    return () => unsub();
  }, []);

  if (loading) return (
    <b>Loading...</b>
  );

  if (!user)  return <Login />;
  if (!valid) return <Invalid />;
  return <Home email={user.email!} photoURL={user.photoURL ?? ""} />;
}

function Invalid() {
  return (
    <div className="invalid-container">
      <span className="material-symbols-outlined invalid-icon">gpp_bad</span>
      <h2>Invalid Authentication</h2>
      <p>Your account is not authorized to view any vehicle.</p>
      <button onClick={() => auth.signOut()}>Sign Out</button>
    </div>
  );
}