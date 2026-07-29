#🛡️ **ArmGuard — IoT Wearable Safety & Localization System for Plantation Workers**
ArmGuard is an edge-computing IoT wearable designed to protect high-risk lone workers in dense agricultural environments, such as oil palm plantations. The system combines kinematic movement tracking, physiological vital monitoring, and hardware-level power-gating to deliver real-time fall detection, health anomaly detection, dynamic GPS localization, and dual-tier supervisor alerting.

##📌 **Features**
>🤸 Two-Stage Sensor Fusion Fall Detection: Combines a 6-axis IMU (MPU-6050) for free-fall/impact detection with physiological verification to eliminate false alarms caused by routine manual labor (sweeping, harvesting, arm swinging).  
>🫀 Physiological Vital Signs Monitoring: Real-time heart rate monitoring using a pulse sensor/PPG module (MAX30102) with moving-average noise filtering.  🛰️ Geospatial Tracking: Integrated NEO-6M GPS module for real-time coordinate logging during emergency events.  
>⚡ Hardware Power-Gating (Up to 96+ Hours Life): Utilizes an SI2301 P-Channel MOSFET circuit controlled via active-low GPIO to physically isolate high-draw peripherals during sleep cycles, reducing baseline current draw down to 1.34 mA.  
>💻 Real-time Supervisor Web Dashboard: WebSockets-driven live dashboard displaying worker location on maps, status indicators, and event logs.  
>🔔 Instant Telegram Bot Alerts: Asynchronous Node.js backend pushes instant alerts with GPS location links directly to supervisors.

###**System Architecture**

