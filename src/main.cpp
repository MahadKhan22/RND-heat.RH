#include <Arduino.h>
#include <DHT.h>
#include <Keypad.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- Network Configuration ---
const char* ssid = "RND";
const char* password = "Norton351@";
const char* targetIP = "192.168.1.XXX"; 
const int targetPort = 8080;

WiFiUDP udp;

// --- Hardware Pin Definitions ---
#define DHTPIN 15      // Data pin for DHT22 sensor
#define DHTTYPE DHT22  // Sensor model designation

#define HEATER_PIN 17
#define HUMIDIFIER_PIN 16

// --- Component Initialization ---
DHT dht(DHTPIN, DHTTYPE); 
TFT_eSPI tft = TFT_eSPI(); 

// Keypad matrix configuration (4x4)
byte ROW_PINS[4] = {27, 14, 13, 4}; 
byte COL_PINS[4] = {32, 33, 25, 26}; 

char keys[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
}; 

Keypad keypad = Keypad(makeKeymap(keys), ROW_PINS, COL_PINS, 4, 4); 

// --- State Variables ---
float currentTemp = NAN; // Initialized to NAN to indicate no valid reading yet
float currentHum = NAN;
float targetTemp = 50.0; 
float targetHum = 95.0;  

String entryBuffer = ""; 
enum InputMode { NONE, SET_TEMP, SET_HUM };
InputMode currentMode = NONE;

unsigned long lastDHTRead = 0;
const unsigned long READ_INTERVAL = 2000; // 2 seconds between DHT reads

// --- Function Prototypes ---
void handleKeypad();
void readSensorAndUpdate();
void controlRelays();
void updateDisplay();
void sendDataUDP(float temp, float hum);

void setup() {
  Serial.begin(115200); 
  while (!Serial) { 
    delay(10); 
  }

  // Initialize network connection with a 10-second timeout
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  const unsigned long WIFI_TIMEOUT = 10000; 

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi connection failed. Operating in offline mode.");
  }

  // Initialize DHT sensor
  dht.begin(); 

  // Configure relay control pins
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(HUMIDIFIER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(HUMIDIFIER_PIN, LOW);

  // Initialize and configure TFT display
  tft.init(); 
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2); 
  
  updateDisplay();
}


void loop() {
  handleKeypad();
  readSensorAndUpdate();
  controlRelays();
}

void readSensorAndUpdate() {
  // Enforce reading interval required by DHT sensors
  if (millis() - lastDHTRead > READ_INTERVAL) { 
    lastDHTRead = millis();
    
    float h = dht.readHumidity(); 
    float t = dht.readTemperature(); 
    
    currentHum = h;
    currentTemp = t;
    
    // Transmit data only if readings are valid
    if (!isnan(h) && !isnan(t)) { 
      sendDataUDP(t, h);
    }
    
    updateDisplay();
  }
}

void sendDataUDP(float temp, float hum) {
  if (WiFi.status() == WL_CONNECTED) {
    String payload = String(temp) + "," + String(hum);
    udp.beginPacket(targetIP, targetPort);
    udp.print(payload);
    udp.endPacket();
  }
}

void controlRelays() {
  // Prevent relay activation if sensor data is invalid (prevents runaway heating/humidity)
  if (isnan(currentTemp) || isnan(currentHum)) {
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(HUMIDIFIER_PIN, LOW);
    return;
  }

  // Activate heater if current temperature is below target
  if (currentTemp < targetTemp) {
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
  }
  
  // Activate humidifier if current humidity is below target
  if (currentHum < targetHum) {
    digitalWrite(HUMIDIFIER_PIN, HIGH);
  } else {
    digitalWrite(HUMIDIFIER_PIN, LOW);
  }
}

void handleKeypad() {
  char key = keypad.getKey(); 
  if (key == NO_KEY) return; 

  switch (key) {
    case 'A': 
      // Enter target temperature configuration mode
      currentMode = SET_TEMP;
      entryBuffer = ""; 
      updateDisplay();
      break;

    case 'B': 
      // Enter target humidity configuration mode
      currentMode = SET_HUM;
      entryBuffer = ""; 
      updateDisplay();
      break;

    case 'C': 
      // Append decimal point if not already present in buffer
      if (entryBuffer.indexOf('.') == -1) { 
        entryBuffer += '.'; 
        updateDisplay();
      }
      break;

    case '*': 
      // Act as backspace or cancel current input mode
      if (entryBuffer.length() > 0) { 
        entryBuffer.remove(entryBuffer.length() - 1); 
      } else {
        currentMode = NONE;
      }
      updateDisplay();
      break;

    case '#': 
      // Confirm input and apply to the selected target variable
      if (entryBuffer.length() > 0) { 
        float newSetpoint = entryBuffer.toFloat(); 
        if (currentMode == SET_TEMP) {
          targetTemp = newSetpoint;
        } else if (currentMode == SET_HUM) {
          targetHum = newSetpoint;
        }
      }
      entryBuffer = ""; 
      currentMode = NONE;
      updateDisplay();
      break;

    default:
      // Append numerical characters up to a length limit
      if (key >= '0' && key <= '9' && entryBuffer.length() < 5) { 
        entryBuffer += key; 
        updateDisplay();
      }
      break;
  }
}

void updateDisplay() {
  // Clear screen and reset cursor
  tft.fillScreen(TFT_BLACK); 
  tft.setCursor(0, 0); 
  
  tft.println("--- Current Status ---");
  
  tft.print("Temp: "); 
  if (isnan(currentTemp)) {
    tft.println("Error");
  } else {
    tft.print(currentTemp); 
    tft.println(" C"); 
  }

  tft.print("Hum:  "); 
  if (isnan(currentHum)) {
    tft.println("Error");
  } else {
    tft.print(currentHum); 
    tft.println(" %"); 
  }
  
  tft.println("\n--- Target Setpoints ---");
  tft.print("Set Temp: "); tft.print(targetTemp); tft.println(" C");
  tft.print("Set Hum:  "); tft.print(targetHum); tft.println(" %");
  
  tft.println("\n--- Input Mode ---");
  tft.println("A:Set Temp  B:Set Hum");
  tft.println("C:Decimal   *:Delete");
  tft.println("#:Confirm & Save");
  
  if (currentMode == SET_TEMP) {
    tft.println("\nEntering Target Temp:");
    tft.println(entryBuffer); 
  } else if (currentMode == SET_HUM) {
    tft.println("\nEntering Target Hum:");
    tft.println(entryBuffer); 
  }
}