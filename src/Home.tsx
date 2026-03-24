import { useEffect, useState, useRef } from "react";
import toast, { Toaster } from "react-hot-toast";
import { auth } from "./firebase";
import { getDatabase, ref, onValue, get, set } from "firebase/database";
import { signOut } from "firebase/auth";
import { MapContainer, TileLayer, Marker, Popup, Circle, useMap, ZoomControl } from "react-leaflet";
import { Map as LeafletMap } from "leaflet";
import "leaflet/dist/leaflet.css";
import L from "leaflet";
import "./Home.css";

// fix leaflet default marker
delete (L.Icon.Default.prototype as unknown as Record<string, unknown>)._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: "https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon-2x.png",
  iconUrl:       "https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png",
  shadowUrl:     "https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png",
});

// car icon
const carIcon = L.divIcon({
  html: `<div class="car-icon">
    <span class="material-symbols-outlined" style="color: white;">
      directions_car
    </span>
  </div>`,
  className: "",
  iconSize:   [48, 48],
  iconAnchor: [24, 24],
});

// calculate distance between two coordinates in meters (Haversine formula)
function getDistanceMeters(lat1: number, lng1: number, lat2: number, lng2: number): number {
  const R    = 6371000;
  const dLat = (lat2 - lat1) * Math.PI / 180;
  const dLng = (lng2 - lng1) * Math.PI / 180;
  const a    = Math.sin(dLat / 2) ** 2 +
               Math.cos(lat1 * Math.PI / 180) *
               Math.cos(lat2 * Math.PI / 180) *
               Math.sin(dLng / 2) ** 2;
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}

function RecenterMap({ lat, lng }: { lat: number; lng: number }) {
  const map = useMap();
  useEffect(() => { map.setView([lat, lng], map.getZoom()); }, [lat, lng]);
  return null;
}

function SetMapRef({ mapRef }: { mapRef: React.MutableRefObject<LeafletMap | null> }) {
  const map = useMap();
  useEffect(() => { mapRef.current = map; }, [map]);
  return null;
}

interface CurrentLoc {
  lat:  number;
  lng:  number;
  spd:  number;
  alt:  number;
  time: string;
}

interface PastLoc {
  lat: number;
  lng: number;
}

export default function Home({ email, photoURL }: { email: string; photoURL: string }) {
  const [location,     setLocation]     = useState<CurrentLoc>({ lat: 0, lng: 0, spd: 0, alt: 0, time: "--", });
  const [userPos,      setUserPos]      = useState<{ lat: number; lng: number } | null>(null);
  const [showSignOut,  setShowSignOut]  = useState(false);
  const [showSettings, setShowSettings] = useState(false);
  const [notifyOn,     setNotifyOn]     = useState(true);
  const mapRef                          = useRef<LeafletMap | null>(null);

  // request notification permission
  useEffect(() => {
    if ("Notification" in window) {
      Notification.requestPermission();
    }
  }, []);

  // firebase listener
  useEffect(() => {
    const emailKey   = email.replace(/\./g, "_");
    const db         = getDatabase();
    const currentRef = ref(db, `${emailKey}/current_loc`);
    const pastRef    = ref(db, `${emailKey}/past_loc`);

    const unsub = onValue(currentRef, async (snap) => {
      const current = snap.val();
      if (!current) return;

      setLocation(current);

      // get past location
      const pastSnap             = await get(pastRef);
      const past: PastLoc | null = pastSnap.val();

      if (!past) {
        // no past location yet — set it
        await set(pastRef, { lat: current.lat, lng: current.lng });
        return;
      }

      // calculate distance between current and past
      const dist = getDistanceMeters(past.lat, past.lng, current.lat, current.lng);

      if (dist > 10) {
        // update past_loc
        await set(pastRef, { lat: current.lat, lng: current.lng });

        // only notify if user is also far from vehicle
        const userDist = userPos
          ? getDistanceMeters(userPos.lat, userPos.lng, current.lat, current.lng)
          : Infinity; // no user location — always notify

        if (userDist > 10 && notifyOn == true) {
          // notification
          if (Notification.permission === "granted") {
            new Notification("🚗 Vehicle Moved!", {
              body: `Vehicle moved ${dist >= 1000
                ? (dist / 1000).toFixed(1) + " km"
                : dist.toFixed(0) + " m"} from last position`,
              icon: "/icon.png",
            });
          }

          // toast
          toast.dismiss();
          toast("🚗 Vehicle moved " + (dist >= 1000
            ? (dist / 1000).toFixed(1) + " km"
            : dist.toFixed(0) + " m") + " from last position", { duration: 500000, }
          );
        }
      }
    });

    return () => unsub();
  }, [email, notifyOn, userPos]);

  // get user location
  useEffect(() => {
    navigator.geolocation.getCurrentPosition((pos) => {
      setUserPos({ lat: pos.coords.latitude, lng: pos.coords.longitude });
    });
  }, []);

  // close signout when clicking outside
  useEffect(() => {
    const handleClick = () => { 
      setShowSignOut(false);
      setShowSettings(false);
    };
    document.addEventListener("click", handleClick);
    return () => document.removeEventListener("click", handleClick);
  }, []);

  // calculate second stat — altitude or distance
  const secondStat = (() => {
    if (!userPos) {
      // no user location — show altitude
      return { label: "ALTITUDE", value: location.alt, unit: "m" };
    }
    const dist = getDistanceMeters(userPos.lat, userPos.lng, location.lat, location.lng);
    if (dist > 30) {
      // far from vehicle — show distance
      const display = dist >= 1000
        ? { value: +(dist / 1000).toFixed(1), unit: "km" }
        : { value: +dist.toFixed(0),          unit: "m"  };
      return { label: "DISTANCE", ...display };
    }
    // close to vehicle — show altitude
    return { label: "ALTITUDE", value: location.alt, unit: "m" };
  })();

  return (
    <div className="container">
      {/*** Toaster ***/}
      <div onClick={(e) => e.stopPropagation()}>
        <Toaster
          position="top-right"
          containerClassName={`custom-toaster ${showSettings ? "open" : ""}`}
          toastOptions={{ className: "custom-toast" }}
        />
      </div>

      {/*** MAP ***/}
      <div className="map-wrapper">
        <MapContainer
          center={[location.lat, location.lng]}
          zoom={15}
          zoomControl={false}
          attributionControl={false}
          maxBounds={[[-90, -180], [90, 180]]}
          maxBoundsViscosity={1.0}
          minZoom={3}
        >
          <TileLayer url="https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png" />
          <ZoomControl position="bottomright" />
          <RecenterMap lat={location.lat} lng={location.lng} />
          <SetMapRef mapRef={mapRef} />

          <Circle center={[location.lat, location.lng]} radius={30} color="rgba(66, 134, 244, 0.6)" fillOpacity={0.15} />
          <Marker position={[location.lat, location.lng]} icon={carIcon}>
            <Popup>Vehicle</Popup>
          </Marker>

          {userPos && (
            <Marker position={[userPos.lat, userPos.lng]}>
              <Popup>You{/*", " + userPos.lat.toFixed(4) + ", " + userPos.lng.toFixed(4)*/}</Popup>
            </Marker>
          )}
        </MapContainer>

        <div className="map-buttons">
          <button className="map-btn" title="Center on Vehicle"
            onClick={() => mapRef.current?.setView([location.lat, location.lng], 15)}>
            <span className="material-symbols-outlined">directions_car</span>
          </button>
          <button className="map-btn" title="Center on You"
            onClick={() => userPos && mapRef.current?.setView([userPos.lat, userPos.lng], 15)}>
            <span className="material-symbols-outlined">my_location</span>
          </button>
        </div>
      </div>

      {/*** Vehicle Status ***/}
      <div 
        className="vehicle-status"
        onClick={(e) => e.stopPropagation()}
      >
        <h2>Vehicle Status</h2>

        <span
          className={`material-symbols-outlined settings-btn ${showSettings ? "active" : ""}`}
          onClick={() => { setShowSettings(prev => !prev) }}
        >
          settings
        </span>

        <div className="stats-grid">
          <div>
            <div className="stat-label">SPEED</div>
            <div className="stat-value speed">{location.spd} <span>km/h</span></div>
          </div>
          <div>
            <div className="stat-label">{secondStat.label}</div>
            <div className="stat-value">{secondStat.value} <span>{secondStat.unit}</span></div>
          </div>
          <div>
            <div className="stat-label">LATITUDE</div>
            <div className="stat-value">{location.lat.toFixed(4)}</div>
          </div>
          <div>
            <div className="stat-label">LONGITUDE</div>
            <div className="stat-value">{location.lng.toFixed(4)}</div>
          </div>
        </div>

        <div className="last-updated">
          <div className="stat-label">LAST UPDATED</div>
          <div className="stat-value">{location.time}</div>
        </div>

        <div className={`settings-wrapper ${showSettings ? "open" : ""}`}>
          <div className="settings">
            <div className="stat-label">Get notified when vehicle moves</div>
            <label className="toggle">
              <input
                type="checkbox"
                checked={notifyOn}
                onChange={() => setNotifyOn(prev => !prev)}
              />
              <span className="slider" />
            </label>
          </div>
        </div>
      </div>

      {/*** User Info ***/}
      <div
        className="user-info"
        onClick={(e) => { e.stopPropagation()}}
      >
        <img
          src={photoURL}
          className="user-avatar"
          title="Click for options"
          referrerPolicy="no-referrer"
          onClick={() => { setShowSignOut(prev => !prev) }}
        />

        {showSignOut || (
          <span className="user-email">{email}</span>
        )}

        {showSignOut && (
          <button
            className="signout-btn"
            onClick={() => { signOut(auth) }}
          >
            <span className="material-symbols-outlined arrow">arrow_left</span>
            <span className="material-symbols-outlined">logout</span>
            Sign Out
          </button>
        )}
      </div>

    </div>
  );
}