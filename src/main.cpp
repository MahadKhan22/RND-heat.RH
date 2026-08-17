#include <Arduino.h>
#include <DHT.h>
#include <Keypad.h>
#include <TFT_eSPI.h>

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
void sendDataSerial(float temp, float hum);

void setup() {
  Serial.begin(115200); 
  while (!Serial) { 
    delay(10); 
  }

  Serial.println("\nSystem Initializing...");
  Serial.println("Telemetry configured for USB Serial. Format: DATA,<Temp>,<Hum>");

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
      sendDataSerial(t, h);
    }
    
    updateDisplay();
  }
}

void sendDataSerial(float temp, float hum) {
  // Output format: DATA,50.00,95.00
  Serial.print("DATA,");
  Serial.print(temp);
  Serial.print(",");
  Serial.println(hum);
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
  tft.setCursor(0, 0); 
  
  tft.println("--- Current Status | Target Setpoint ---");
  
  tft.print("Temp: "); 
  if (isnan(currentTemp)) {
    tft.print("Error        |      ");
  } else {
    tft.print(currentTemp); 
    tft.print(" C      |      "); 
  }
  tft.print(targetTemp); 
  tft.println(" C       "); // Padding to overwrite old digits

  tft.print("Hum:  "); 
  if (isnan(currentHum)) {
    tft.print("Error        |      ");
  } else {
    tft.print(currentHum); 
    tft.print(" %      |      "); 
  }
  tft.print(targetHum); 
  tft.println(" %       "); // Padding to overwrite old digits
  
  tft.println("\n--- Input Mode ---");
  tft.println("A:Set Temp  B:Set Hum");
  tft.println("C:Decimal   *:Delete");
  tft.println("#:Confirm & Save");
  
  if (currentMode == SET_TEMP) {
    tft.println("\nEntering Target Temp:        ");
    tft.print(entryBuffer); 
    tft.println("               "); // Padding for buffer
  } else if (currentMode == SET_HUM) {
    tft.println("\nEntering Target Hum:         ");
    tft.print(entryBuffer); 
    tft.println("               "); // Padding for buffer
  } else {
    tft.println("\n                             "); // Overwrite prompt text
    tft.println("                             "); // Overwrite buffer text
  }
}