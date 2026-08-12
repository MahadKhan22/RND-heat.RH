import serial # type: ignore
import csv
import datetime
import sys

# --- Configuration ---
# Update this to match the COM port assigned to the ESP32
SERIAL_PORT = 'COM12' 
BAUD_RATE = 115200
CSV_FILENAME = 'environmental_telemetry.csv'

# --- Initialization ---
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Established connection to {SERIAL_PORT} at {BAUD_RATE} baud.")
except serial.SerialException as e:
    print(f"System failure: Unable to open serial port. {e}")
    sys.exit(1)

print(f"Initiating data logging to {CSV_FILENAME}. Execute Ctrl+C to terminate.")

# --- Logging Loop ---
with open(CSV_FILENAME, mode='a', newline='') as csv_file:
    csv_writer = csv.writer(csv_file)
    
    try:
        while True:
            if ser.in_waiting > 0:
                raw_line = ser.readline()
                
                try:
                    line = raw_line.decode('utf-8').strip()
                except UnicodeDecodeError:
                    continue # Ignore garbled bytes during board boot sequence
                
                if line.startswith("DATA,"):
                    # Strip the "DATA," identifier to isolate the numerical payload
                    payload = line[5:]
                    data_values = payload.split(",")
                    
                    # Generate system timestamp
                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    
                    # Construct and write the row
                    csv_row = [timestamp] + data_values
                    csv_writer.writerow(csv_row)
                    
                    # Force buffer flush to ensure data is saved immediately
                    csv_file.flush() 
                    
                    print(f"Logged: {csv_row}")
                    
    except KeyboardInterrupt:
        print("\nInterrupt received. Terminating logging sequence.")
    finally:
        ser.close()
        print("Serial port connection closed.")