🛡️ ArmGuard — IoT Wearable Safety & Localization System for Plantation Workers[suspicious link removed]ArmGuard is an open-source, edge-computing IoT wearable designed to protect high-risk lone workers in dense agricultural environments, such as oil palm plantations. The system combines kinematic movement tracking, physiological vital monitoring, and hardware-level power-gating to deliver real-time fall detection, health anomaly detection, dynamic GPS localization, and dual-tier supervisor alerting.  📌 Features🤸 Two-Stage Sensor Fusion Fall Detection: Combines a 6-axis IMU (MPU-6050) for free-fall/impact detection with physiological verification to eliminate false alarms caused by routine manual labor (sweeping, harvesting, arm swinging).  🫀 Physiological Vital Signs Monitoring: Real-time heart rate monitoring using a pulse sensor/PPG module (MAX30102) with moving-average noise filtering.  🛰️ Geospatial Tracking: Integrated NEO-6M GPS module for real-time coordinate logging during emergency events.  ⚡ Hardware Power-Gating (Up to 96+ Hours Life): Utilizes an SI2301 P-Channel MOSFET circuit controlled via active-low GPIO to physically isolate high-draw peripherals during sleep cycles, reducing baseline current draw down to 1.34 mA.  💻 Real-time Supervisor Web Dashboard: WebSockets-driven live dashboard displaying worker location on maps, status indicators, and event logs.  🔔 Instant Telegram Bot Alerts: Asynchronous Node.js backend pushes instant alerts with GPS location links directly to supervisors.  📐 System Architecture                       +-----------------------------------+
                       |      ArmGuard Wearable Node       |
                       |                                   |
  [GY-521 MPU-6050] --->|  Seeed XIAO ESP32-C3              |
  [MAX30102 Pulse ] --->|  (Edge Data Filtering & Fusion)   |
  [SI2301 MOSFET  ] --->|                                   |
                       +-----------------+-----------------+
                                         |
                                         | (Wi-Fi / Gateway Telemetry) [cite: 509, 588]
                                         v
                          +------------------------------+
                          |   Firebase Realtime Database  | [cite: 509, 229]
                          +--------------+---------------+
                                         |
               +-------------------------+-------------------------+
               |                                                   |
               v                                                   v
+-------------------------------+               +-------------------------------+
|     Web Supervisor Dashboard  |               |      Node.js Telegram Bot     | 
| (Live Map & Health Dashboard) |               |  (Instant Push Emergency Log) |
+-------------------------------+               +-------------------------------+
🛠️ Hardware Requirements & PinoutComponentFunctionInterface / Pin (XIAO ESP32-C3)Seeed XIAO ESP32-C3Main Microcontroller Edge ProcessingCPU CoreGY-521 (MPU-6050)6-Axis Motion Tracking (IMU)$I^2C$ (SDA: D4, SCL: D5)MAX30102 / Pulse SensorHeart Rate & Vital Signs$I^2C$ / Analog (A0/D0)NEO-6M GPS ModuleOutdoor Coordinate LocationHardware UART (D6 TX, D7 RX)SI2301 P-ch MOSFETHardware Power-Gating CircuitMOSFET Gate Control Pin (D2)LiPo Battery (3.7V)Main Power SupplyBat Pin / 3.3V LDO📊 Software & Algorithmic Logic1. Kinetic Fall Detection LogicThe algorithm calculates the continuous total acceleration vector magnitude ($A_{total}$):$$A_{total} = \sqrt{A_x^2 + A_y^2 + A_z^2}$$  When $A_{total} < \text{FREEFALL\_THRESHOLD}$ ($< 0.4g$), a potential fall window opens. If followed within $250\,\text{ms}$ by an impact spike ($> 3.0g$) and post-impact immobility/angular velocity shifts, a fall condition is latched.  2. Multi-Parameter Sensor FusionTo prevent false alarms during heavy agricultural tasks (e.g., pruning, fruit collection):  If Fall Detected OR Heart Rate Anomaly Triggered ($\text{BPM} < 40$ or $\text{BPM} > 130$):Node powers UP the GPS module via MOSFET power gate.  Fetches precise outdoor coordinates.  Pushes JSON payload to Firebase Cloud Database.  📁 Repository StructureCode snippetArmGuard/
├── firmware/
│   ├── ArmGuard_XIAO_ESP32C3.ino   # Core ESP32 C/C++ Firmware
│   └── config.h                     # Thresholds, Wi-Fi & Firebase Credentials
├── backend/
│   ├── bot.js                       # Node.js Telegram Bot Alert Server
│   └── package.json                 # Node dependencies
├── web-dashboard/
│   ├── index.html                   # Live Web Interface
│   ├── app.js                       # Firebase listener & Map integration
│   └── style.css                    # UI Styling
├── hardware/
│   ├── schematics/                  # Schematic diagrams & CAD enclosure files
│   └── bom.md                       # Bill of Materials & Cost Breakdown
├── LICENSE                          # MIT License
└── README.md                        # Project Documentation
🚀 Quick Start & Setup1. Firmware Flashing (ESP32-C3)Open firmware/ArmGuard_XIAO_ESP32C3.ino in Arduino IDE.Install the required libraries via Arduino Library Manager:Adafruit_MPU6050TinyGPS++FirebaseClient / Firebase-ESP-ClientEdit config.h with your credentials:C++#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_secret_key"
Select board Seeed Studio XIAO ESP32C3 and upload.2. Backend Notification BotNavigate to the backend directory:Bashcd backend
npm install
Set your environment variables or update bot.js:JavaScriptconst TELEGRAM_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
const CHAT_ID = "YOUR_TELEGRAM_CHAT_ID";
Run the alert service:Bashnode bot.js
📈 Performance & Experimental ValidationFall Classification Accuracy: 95.0% overall reliability in simulated agricultural environments.  Heart Rate Monitor Accuracy: 97.57% resting accuracy (Mean Error: 2.43%) compared against commercial medical benchmarks.  GPS Fix & Precision: Time To First Fix (TTFF) of ~35 seconds under open skies with an average positional drift of 2.5 m.  Power Consumption: Throttled sleep current of 1.34 mA, enabling continuous operation for up to 96–97 hours on a 1500 mAh battery.  👥 Authors & AcknowledgmentsDeveloped as an Integrated Design Project (BERN 3863) at Universiti Teknikal Malaysia Melaka (UTeM), Faculty of Electronics & Computer Technology & Engineering:  Muhammad Musa bin Aminuddin (Hardware Design & Performance Evaluation)Ariff Izamuddin bin Roselan (Software Development & Project Lead)Aiesya Humaira binti Abdul Rahem (Documentation & Market Research)Supervisor: Ts. Dr. Mohamad Harris bin Misran   📜 LicenseThis project is open-source and available under the MIT License.
