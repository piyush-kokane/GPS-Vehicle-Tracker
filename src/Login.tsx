import { signInWithPopup } from "firebase/auth";
import { auth, provider } from "./firebase";
import "./Login.css";

export default function Login() {
  const handleLogin = async () => {
    try {
      await signInWithPopup(auth, provider);
    } catch (err) {
      console.error(err);
    }
  };

  return (
    <div className="login-card">
      <span className="material-symbols-outlined login-icon">directions_car</span>
      <h1>Vehicle Tracker</h1>
      <p>Sign in to view your vehicle location</p>
      <button className="login-btn" onClick={handleLogin}>
        <img src="https://www.gstatic.com/firebasejs/ui/2.0.0/images/auth/google.svg" width={20} />
        Sign in with Google
      </button>
    </div>
  );
}