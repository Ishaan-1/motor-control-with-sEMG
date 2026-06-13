import serial
import time

# Configure your port (Double check this matches your Device Manager!)
PORT = 'COM14'
BAUD = 115200
USER = 'thesy'

STEPS = [
    ("RELAX", 5),
    ("GESTURE_1", 5),
    ("RELAX", 5),
    ("GESTURE_2", 5),
    ("RELAX", 5),
    ("GESTURE_3", 5),
    ("RELAX", 5),
    ("GESTURE_4", 5),
    ("RELAX", 5)
]

try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    # Clear any residual junk out of the hardware serial buffer before starting
    ser.reset_input_buffer() 
except Exception as e:
    print(f"Error opening port {PORT}: {e}")
    exit()

filename = f"C:/Users/{USER}/Downloads/labeled_emg_data_{int(time.time())}.csv"
print(f"Preparing to record your dataset to: {filename}")
time.sleep(2) 

with open(filename, "w", encoding="utf-8") as f:
    # Write CSV header
    f.write("Timestamp,Channel1,Channel2,Wave,Label\n")
    
    for label, duration in STEPS:
        start_time = time.time()
        print(f"\n>>> CURRENT GESTURE: {label} <<<")
        
        while time.time() - start_time < duration:
            if ser.in_waiting > 0: # Only try to read if bytes are physically waiting
                try:
                    # Use 'ignore' to bypass any weird corrupted serial bytes
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if line and "," in line: # Ensure it's a valid data line
                        current_timestamp = time.time()
                        f.write(f"{current_timestamp},{line},{label}\n")
                        
                        # Live data preview in terminal so you know it's working!
                        print(f"[{label}] Data incoming: {line} ", end="\r")
                except Exception:
                    pass

print("\n\nRecording complete! Check your Downloads folder now.")
ser.close()