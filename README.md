# 🛰️ GPS Vehicle Tracker

A full-stack GPS vehicle tracking system that combines an **Arduino-based hardware tracker** with a **React web dashboard**. The device collects real-time GPS coordinates and pushes them to Firebase, where the web app visualizes live vehicle location on an interactive map.

---

## ✨ Features

- 📍 **Real-time GPS tracking** — live location updates from the Arduino device to Firebase
- 🗺️ **Interactive map** — powered by Leaflet, renders the vehicle's position and movement path
- 🔐 **Authentication** — Firebase Auth for secure user login
- 🔥 **Firebase Realtime Database / Firestore** — stores and syncs GPS data instantly
- 🚨 **Toast notifications** — instant feedback via `react-hot-toast`
- ⚡ **Fast frontend** — Vite + React 19 + TypeScript for a snappy dev and production experience

---

## 🏗️ Project Structure

```
GPS-Vehicle-Tracker/
├── Arduino/               # Arduino firmware (C++)
│   └── ...                # GPS module reading & Firebase data push
├── public/                # Static assets
├── src/                   # React frontend source
│   └── ...                # Components, pages, Firebase config
├── index.html
├── package.json
├── vite.config.ts
└── tsconfig.json
```

---

## 🔧 Tech Stack

### Hardware
| Component | Model / Spec | Purpose |
|-----------|-------------|---------|
| Microcontroller | Arduino Uno | Main logic controller (5V) |
| GPS Module | NEO-6M (3.3V logic) | Acquires NMEA GPS coordinates via UART |
| GPRS Module | SIM800L (3.3V logic) | Sends location data to Firebase over cellular |
| Buck Converter | LM2596 — 4V 2A+ | Steps down 12V battery to 4V for GPRS & GPS |
| Voltage Divider | 1kΩ + 2kΩ resistors | Converts Arduino's 5V TX → 3.2V for GPRS/GPS RX (logic level shift) |
| Capacitors | 470µF–1000µF / 0.1µF | Absorbs battery spikes and filters high-frequency noise |
| Protection Diode | — | Prevents reverse polarity from battery |
| Power Switch | — | Safety cutoff between battery and system |

### Frontend
| Technology | Role |
|------------|------|
| React 19 + TypeScript | UI framework |
| Vite | Build tool & dev server |
| Leaflet + react-leaflet | Interactive map rendering |
| Firebase (Auth + DB) | Authentication & real-time data storage |
| react-hot-toast | Toast notifications |

---

## 🚀 Getting Started

### Prerequisites

- Node.js ≥ 18
- Arduino IDE with the following libraries:
  - `TinyGPS++` — parses NMEA data from the NEO-6M GPS module
  - `SoftwareSerial` — communicates with GPS and GPRS on custom pins
  - `FirebaseArduino` or `Firebase ESP Client` — pushes data to Firebase
  - SIM800L AT command support (built-in via SoftwareSerial)

---

### 1. Firebase Setup

1. Go to [Firebase Console](https://console.firebase.google.com/) and create a new project.
2. Enable **Authentication** (Email/Password or your preferred provider).
3. Create a **Realtime Database** or **Firestore** collection to store GPS data.
4. Copy your Firebase config object — you'll need it for both the frontend and Arduino sketch.

---

### 2. Hardware Wiring

#### Power Supply
- **Prototype:** 9V + USB
- **Final product:** 12V 100–200Ah vehicle battery
- GND is common across the entire system
- Arduino's `VIN` and the LM2596 buck converter `IN` connect directly to battery `+ve`
- A **protection diode and switch** sit between battery and system for safety
- LM2596 steps voltage down to **4V** to power the SIM800L (GPRS) and NEO-6M (GPS)

#### Logic Level
> Arduino is 5V logic; GPS and GPRS modules are 3.2V logic.
> A voltage divider (1kΩ + 2kΩ) is used on the Arduino TX → GPRS/GPS RX line for safety.

#### Recommended Capacitors
| Location | Type | Value | Voltage Rating | Purpose |
|----------|------|-------|---------------|---------|
| Across 12V input (before buck converter) | Electrolytic | 470µF – 1000µF | 25V or more | Absorbs large spikes from vehicle battery |
| Near GPRS SIM800L | Low ESR Electrolytic | 470µF – 1000µF | 6.3V or more | Handles sudden ~2A current draw |
| Near GPS | Ceramic | 0.1µF | 10V | Filters high-frequency noise |

#### Pin Connections (Arduino Uno)
| Arduino Pin | Connected To |
|-------------|-------------|
| VIN | Battery +ve (through diode & switch) → LM2596 IN |
| GND | Common GND |
| RX (PD0) | GPRS TXD (via direct connection) |
| TX (PD1) | GPRS RXD (through 1kΩ+2kΩ voltage divider → 3.2V) |
| A0–A5 / D2–D7 | GPS TXD / RXD (SoftwareSerial pins as defined in sketch) |

#### Arduino Sketch Setup

1. Open the sketch inside the `Arduino/` folder in Arduino IDE.
2. Fill in your credentials:

```cpp
#define WIFI_SSID     "your-sim-apn"         // APN for SIM800L
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "your-database-secret-or-token"
```

3. Verify the SoftwareSerial pin assignments match your physical wiring.
4. Upload the sketch to the Arduino Uno.

The device will acquire a GPS fix via NEO-6M, then transmit `{ latitude, longitude, timestamp }` to Firebase over the SIM800L GPRS connection.

---

### 3. Frontend Setup

```bash
# Clone the repository
git clone https://github.com/piyush-kokane/GPS-Vehicle-Tracker.git
cd GPS-Vehicle-Tracker

# Install dependencies
npm install
```

Create a `.env` file in the project root with your Firebase config:

```env
VITE_FIREBASE_API_KEY=your_api_key
VITE_FIREBASE_AUTH_DOMAIN=your_project.firebaseapp.com
VITE_FIREBASE_DATABASE_URL=https://your_project.firebaseio.com
VITE_FIREBASE_PROJECT_ID=your_project_id
VITE_FIREBASE_STORAGE_BUCKET=your_project.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=your_sender_id
VITE_FIREBASE_APP_ID=your_app_id
```

```bash
# Start the development server
npm run dev
```

Open `http://localhost:5173` in your browser.

---

### Available Scripts

| Command | Description |
|---------|-------------|
| `npm run dev` | Start local dev server with HMR |
| `npm run build` | Type-check and build for production |
| `npm run preview` | Preview the production build locally |
| `npm run lint` | Run ESLint |

---

## 📐 Circuit Schematic

The circuit was designed in **EasyEDA** (schematic rev 1.0, drawn by Piyush K., 2025-07-31).

Key design decisions from the schematic:
- The **LM2596 buck converter** regulates the 12V vehicle battery down to 4V, rated at 2A+ to handle SIM800L's peak current draws (~2A burst)
- A **1000µF capacitor** near the buck converter output absorbs sudden current spikes
- The **voltage divider** (1kΩ / 2kΩ) safely shifts Arduino's 5V TX signal down to 3.2V before it reaches the 3.3V-logic GPRS and GPS modules
- GND is unified across all components

---



```
NEO-6M GPS Module → Arduino Uno (reads NMEA via SoftwareSerial)
                           ↓
                   Parses lat/lng with TinyGPS++
                           ↓
             SIM800L GPRS Module transmits over cellular
                           ↓
                  Writes to Firebase Realtime DB
                           ↓
             React app subscribes via Firebase SDK
                           ↓
               Leaflet map re-renders in real-time
```

1. The NEO-6M GPS module outputs NMEA sentences over UART; TinyGPS++ parses them into coordinates on the Arduino Uno.
2. The SIM800L GPRS module connects to the internet via a SIM card and pushes `{ latitude, longitude, timestamp }` to Firebase.
3. The React frontend subscribes to Firebase and updates the Leaflet map marker on every new data write.
4. Users authenticate via Firebase Auth before accessing tracker data.

---

## 🔒 Security

- Never commit your `.env` file or hardcode Firebase credentials in source code.
- Use Firebase Security Rules to restrict database read/write access to authenticated users only.
- Rotate your Firebase database secret if it was ever exposed.

---

## 📦 Dependencies

```json
"firebase": "^12.11.0",
"leaflet": "^1.9.4",
"react": "^19.2.4",
"react-dom": "^19.2.4",
"react-hot-toast": "^2.6.0",
"react-leaflet": "^5.0.0"
```

---

## 🛠️ Troubleshooting

**No GPS fix** — Ensure the NEO-6M module has a clear view of the sky. Cold fix can take 1–2 minutes. Avoid testing indoors without an external antenna.

**SIM800L not connecting** — Check that your SIM card has an active data plan and the correct APN is set in the sketch. The SIM800L requires a stable 3.7–4.2V supply; insufficient current causes random resets. Add a 470µF–1000µF low-ESR capacitor right at its power pins.

**Voltage divider** — If the GPRS/GPS modules aren't receiving serial data, verify the 1kΩ + 2kΩ divider is correctly placed on the Arduino TX line. Incorrect logic levels can damage the modules.

**Firebase connection refused** — Double-check `FIREBASE_HOST` and `FIREBASE_AUTH` in the Arduino sketch, and verify your database rules allow writes.

**Map not showing** — Make sure Leaflet's CSS is imported in your frontend. Missing the stylesheet is the most common cause of a blank map.

**Blank page on `npm run dev`** — Confirm all `VITE_FIREBASE_*` environment variables are set in `.env`.

---

## 🤝 Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you'd like to change.

---

## 📄 License

This project is open source. Feel free to use, modify, and distribute it.

---

*Built with ❤️ using Arduino Uno, NEO-6M GPS, SIM800L GPRS, React, Firebase & Leaflet.*