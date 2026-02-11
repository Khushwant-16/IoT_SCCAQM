# IoT_SCCAQM
Smart Cloud-Connected Air Quality Monitor (SCCAQM)

Overview

SCCAQM is an advanced IoT ecosystem designed to solve the problem of environmental sensor drift in low-cost gas sensors. By deploying an Adaptive Multi-Sensor Fusion Model (AMSFM), the system provides high-precision air quality diagnostics, energy-aware automation, and context-aware security logic synced with a real-time web dashboard.

System Architecture

The interaction between the Sensing Layer, Processing (Edge AI), Cloud Layer, and Actuation Layer.

<img width="502" height="414" alt="image" src="https://github.com/user-attachments/assets/fd81c04a-6a49-4471-89da-3ddb3ccd7adf" />

Core Features

AMSFM Calibration (ML): Uses a Linear Regression model trained on 12.7 Million records to neutralize Temperature and Humidity interference on MQ-series sensors.

Dual-Mode Security Engine: Bi-directional cloud synchronization allows the device to switch between Intrusion Detection (Away Mode) and Seismic Event Detection (Home Mode).

Energy-Aware Actuation: Intelligent Servo-driven ventilation that chooses between "Passive Cooling" (closing curtains to block heat) and "Passive Ventilation" (opening curtains for fresh air).

Real-Time Analytics: A comprehensive web dashboard featuring live gauges and historical time-series charts for trend analysis.

Hardware Prototype

Pin Mapping (ESP32 WROOM)

Component

ESP32 Pin

Type

Function

DHT22

GPIO 4

Digital

Temp & Humidity

MQ-7 (CO)

GPIO 36 (VP)

Analog

Raw CO Sensing

MQ-9 (VOC)

GPIO 39 (VN)

Analog

Raw VOC/CH4 Sensing

Flame Sensor

GPIO 33

Digital

Fire Detection

SW-420

GPIO 25

Digital

Vibration/Shock

SG90 Servo

GPIO 32

PWM

Window/Curtain Control

I2C LCD

21 (SDA), 22 (SCL)

I2C

Local Status (Optional)

<img width="502" height="265" alt="image" src="https://github.com/user-attachments/assets/4de4339a-d309-42ee-9db4-ea7451ddaa1b" />


Software & ML Stack

Firmware: C++ / Arduino Framework (ESP32)

Cloud Backend: Firebase Firestore & Node.js Cloud Functions

Frontend: HTML5, Tailwind CSS, JavaScript (Chart.js)

ML Development: Python, Scikit-learn, Pandas (Time-series Resampling)

Machine Learning: The AMSFM

The core innovation is the Adaptive Multi-Sensor Fusion Model. Standard MQ sensors suffer from chemical "drift" where humidity and temperature changes mimic gas presence.

Training: 12.7M records resampled into 1-minute intervals.

Deployment: The model is optimized into a lightweight mathematical coefficient array deployed on the ESP32 (Edge AI), enabling real-time calibration without cloud latency.

Installation & Setup

1. Firebase Setup

Create a Firebase Project.

Enable Firestore Database and Anonymous Authentication.

Deploy the Cloud Function provided in the /functions folder.

Create a collection named config with a document system containing { "mode": "Home" }.

2. Firmware Upload

Install the required libraries: ArduinoJson, DHT sensor library, ESP32Servo, LiquidCrystal_I2C.

Open /firmware/SCCAQM.ino.

Replace WIFI_SSID, WIFI_PASS, and functionUrl with your credentials.

Compile and upload to your ESP32.

3. Dashboard Deployment

Open /web-dashboard/index.html.

Replace the firebaseConfig object with your project credentials from the Firebase Console.

Open the file in any browser or host via Firebase Hosting.

Repository Structure

├── firmware/         # ESP32 Source Code (.ino)
├── functions/        # Node.js Cloud Functions
├── dashboard/        # Web Dashboard (HTML/JS)
├── ml-model/         # Jupyter Notebook & Training Logic
└── docs/             # Architecture and Prototype images
