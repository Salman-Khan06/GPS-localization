#include <WiFi.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h> 
#include <FirebaseESP32.h>

//  FIREBASE & WIFI CREDENTIALS 
#define FIREBASE_HOST "gpstracking-2db8a-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyA0O6Ilwg8Y6GsyC36VQWVAB2YeMwhtowM"

const char* ssid = "DMTs-WiFi";
const char* password = "";

// FIREBASE OBJECTS 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// GPS SETUP 
HardwareSerial gpsSerial(1); 
static const int RXPin = 16, TXPin = 17; 
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

// Variables for timing the upload
unsigned long lastUploadTime = 0;
const long uploadInterval = 5000; // Upload every 5 seconds

void sendDataToFirebase() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Firebase: WiFi not connected. Skipping upload.");
        return;
    }
    
    if (gps.location.isValid()) {
        FirebaseJson json;
        
        // Adding timestamp using millis() as fallback
        unsigned long timestamp = millis();
        json.set("timestamp/.sv", "timestamp"); // Server timestamp
        json.set("latitude", gps.location.lat());
        json.set("longitude", gps.location.lng());
        json.set("altitude", gps.altitude.meters());
        json.set("satellites", gps.satellites.value());
        json.set("speed", gps.speed.kmph());
        json.set("course", gps.course.deg());
        
        // Add GPS date/time 
        if (gps.date.isValid() && gps.time.isValid()) {
            char dateTime[32];
            sprintf(dateTime, "%04d-%02d-%02d %02d:%02d:%02d", 
                    gps.date.year(), gps.date.month(), gps.date.day(),
                    gps.time.hour(), gps.time.minute(), gps.time.second());
            json.set("gps_datetime", dateTime);
        }
        
        String path = "/tracker/current_location";
        
        if (Firebase.setJSON(fbdo, path, json)) {
            Serial.println("Firebase: Data SENT successfully.");
            Serial.print("   LAT: "); Serial.print(gps.location.lat(), 6);
            Serial.print(" | LNG: "); Serial.println(gps.location.lng(), 6);
        } else {
            Serial.print(" Firebase: FAILED. Reason: ");
            Serial.println(fbdo.errorReason());
        }
    } else {
        Serial.println(" Firebase: No valid GPS fix. Skipping upload.");
        Serial.print("   Satellites: "); Serial.println(gps.satellites.value());
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== ESP32 GPS Tracker Starting ===");
    
    // Initialize GPS communication
    gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
    Serial.println("GPS Serial initialized on RX=16, TX=17");
    
    // --- Connect to Wi-Fi ---
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n WiFi connected!");
        Serial.print("Local IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal Strength (RSSI): ");
        Serial.println(WiFi.RSSI());
        
        // --- Configure Firebase ---
        config.host = FIREBASE_HOST;
        config.signer.tokens.legacy_token = FIREBASE_AUTH;
        
        // Set timeout
        config.timeout.serverResponse = 10 * 1000;
        
        // Initialize Firebase
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        
        Serial.println(" Firebase initialized.");
        
        // Test connection
        Serial.println("Testing Firebase connection...");
        if (Firebase.setString(fbdo, "/tracker/status", "online")) {
            Serial.println(" Firebase connection test successful!");
        } else {
            Serial.print(" Firebase test failed: ");
            Serial.println(fbdo.errorReason());
        }
    } else {
        Serial.println("\n WiFi connection FAILED.");
    }
    
    Serial.println("\n=== Setup Complete - Starting Main Loop ===\n");
}

void loop() {
    // 1. Process GPS data (non-blocking)
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        gps.encode(c);
        // Uncomment to see raw GPS data
        // Serial.write(c);
    }
    
    // 2. Upload Data to Firebase on interval
    if (WiFi.status() == WL_CONNECTED && millis() - lastUploadTime >= uploadInterval) {
        sendDataToFirebase();
        lastUploadTime = millis();
    }
    
    // 3. Serial Debug (only when location updates)
    if (gps.location.isUpdated()) {
        Serial.print(" GPS Update: LAT=");
        Serial.print(gps.location.lat(), 6); 
        Serial.print(" | LNG=");
        Serial.print(gps.location.lng(), 6);
        Serial.print(" | Sats=");
        Serial.println(gps.satellites.value());
    }
    
    // 4. Check WiFi connection status
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) { // Check every 30 seconds
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(" WiFi disconnected. Reconnecting...");
            WiFi.reconnect();
        }
        lastWiFiCheck = millis();
    }
    
    delay(10);
}