#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h> 

// --- WiFi Credentials ---
// NOTE: These are for an OPEN network. If your network has a password, enter it here.
const char* ssid = "DMTs-WiFi";
const char* password = ""; 

// --- GPS Setup ---
// We use HardwareSerial 1 for the GPS module
HardwareSerial gpsSerial(1); 
static const int RXPin = 16, TXPin = 17; // ESP32 Pins connected to NEO-6M TX and RX
static const uint32_t GPSBaud = 9600;    // Standard baud rate for NEO-6M

TinyGPSPlus gps;

// --- Web Server Setup ---
WebServer server(80); // Web server runs on standard HTTP port 80

// --- Function Prototypes ---
void handleRoot();
void handleNotFound();

/**
 * @brief Handles the root URL (/) request from a web client.
 * Generates the HTML page content dynamically with the current GPS coordinates.
 */
void handleRoot() {
    Serial.println(">>> Server: Received GET request for /"); // DEBUG

    // Get current GPS data or display 'WAITING' if invalid
    String latitude = gps.location.isValid() ? String(gps.location.lat(), 6) : "WAITING FOR GPS FIX...";
    String longitude = gps.location.isValid() ? String(gps.location.lng(), 6) : "WAITING FOR GPS FIX...";
    
    // --- Dynamic HTML Generation ---
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='5'>"; // Auto-refresh every 5 seconds
    html += "<title>ESP32 GPS Tracker</title>";
    html += "<style>body{font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9;} h1{color: #007bff;} .data{font-size: 28px; font-weight: bold; color: #333; margin: 10px 0;} .label{font-size: 16px; color: #666;}</style>";
    html += "</head><body>";
    html += "<h1> Real-Time GPS Location</h1>";
    html += "<p class='label'>Last Update: <span class='data' style='font-size: 20px;'>" + String(millis() / 1000) + "s Ago</span></p>";
    html += "<hr style='width: 80%; border-top: 1px solid #ddd;'>";
    
    html += "<p class='label'>Latitude:</p><p class='data'>" + latitude + "</p>";
    html += "<p class='label'>Longitude:</p><p class='data'>" + longitude + "</p>";

    html += "<hr style='width: 80%; border-top: 1px solid #ddd;'>";
    
    html += "<p class='label'>Satellites:</p><p class='data'>" + String(gps.satellites.value()) + "</p>";
    html += "<p class='label'>Altitude:</p><p class='data'>" + String(gps.altitude.meters(), 1) + " m</p>";

    html += "<hr style='width: 80%; border-top: 1px solid #ddd;'>";
    html += "<p style='font-size: 14px;'>Access Point: *http://" + WiFi.localIP().toString() + "*</p>";
    html += "</body></html>";
    
    // Send the response back to the client
    server.send(200, "text/html", html);
    Serial.println("<<< Server: Response sent successfully."); // DEBUG
}

/**
 * @brief Sends a 404 (Not Found) message for unhandled requests.
 */
void handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

void setup() {
    // Start debug console
    Serial.begin(115200); 

    // Initialize GPS communication
    gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin); 
    Serial.println("\nGPS Serial initialized.");

    // --- Connect to Wi-Fi ---
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // Connection attempt loop with a timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) { // Max 20 seconds (40 * 500ms)
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("Web Server IP: ");
        Serial.println(WiFi.localIP());

        // --- Start Web Server ---
        server.on("/", handleRoot); 
        server.onNotFound(handleNotFound);
        server.begin(); 
        Serial.println("HTTP server started successfully.");
    } else {
        Serial.println("\nWiFi connection FAILED. Please check credentials or range.");
    }
}

void loop() {
    // 1. Handle incoming client requests (MUST be first and fast)
    server.handleClient();

    // 2. Process GPS data (non-blocking)
    // Feeds incoming characters from the GPS module to the TinyGPSPlus library
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    // 3. Serial Debugging and Status
    
    // Report success when location data updates
    if (gps.location.isUpdated()) {
        Serial.print(" GPS FIX: ");
        Serial.print("LAT=");
        Serial.print(gps.location.lat(), 6); 
        Serial.print(" | LNG=");
        Serial.print(gps.location.lng(), 6);
        Serial.print(" | Sats=");
        Serial.println(gps.satellites.value());
    } 
    
    // Report 'No Fix' status every few seconds if valid data isn't available
    else if (millis() > 5000 && !gps.location.isValid() && gps.charsProcessed() > 0 && (millis() % 3000 == 0)) { 
        Serial.print(" NO FIX: ");
        Serial.print(gps.satellites.value());
        Serial.println(" satellites visible. Waiting for lock...");
    }
    
    // 4. Introduce a small pause to stabilize Wi-Fi tasks
    delay(5); 
}