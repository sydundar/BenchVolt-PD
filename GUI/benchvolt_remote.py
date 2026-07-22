import serial
import time
import threading

class BenchVoltRemote:
    """Handles SCPI communication with the BenchVolt-PD hardware."""

    def __init__(self):
        self.ser = None
        self.is_connected = False
        # Use an RLock to prevent deadlocks during nested calls
        self.lock = threading.RLock()

    def connect(self, port, baudrate=115200):
        """Connects to the STM32 via USB CDC."""
        try:
            # Use a low timeout to prevent UI freezing during serial operations
            self.ser = serial.Serial(port, baudrate, timeout=0.1)
            self.is_connected = True
            return True
        except Exception as e:
            print(f"Connection Error: {e}")
            return False

    def disconnect(self):
        """Closes the serial connection safely."""
        with self.lock:  # Prevent closing the port while another thread is reading
            if self.ser and self.ser.is_open:
                try:
                    self.ser.close()
                except Exception:
                    pass  # Ignore the error if the USB connection was already physically lost
            self.is_connected = False

    def send_scpi(self, command):
        """Sends a raw SCPI command to the device."""
        if self.is_connected:
            with self.lock:  # Serialize access to the serial port
                try:
                    print(f"TX > {command}")
                    self.ser.write(f"{command}\n".encode('ascii'))
                except Exception as e:
                    print(f"Write Error: Device connection was lost! ({e})")
                    self.is_connected = False  # Treat the device as disconnected after a communication failure

    def query(self, command):
        """Sends a query and waits for a response safely."""
        if self.is_connected:
            with self.lock:  # Serialize access to the serial port
                try:
                    self.ser.reset_input_buffer()
                    self.ser.write(f"{command}\n".encode('ascii'))
                    time.sleep(0.1)

                    if self.ser.in_waiting > 0:
                        raw = self.ser.readline().decode('ascii').strip()
                        return raw
                    return None
                except Exception as e:
                    print(f"Remote Query Error: USB connection was physically lost! ({e})")
                    self.is_connected = False  # Treat the device as disconnected after a communication failure
                    return None
        return None

    # --- High-Level Control API ---
    def set_voltage(self, channel, voltage):
        """Sets the target voltage for CH4 or CH5."""
        self.send_scpi(f"SOUR:VOLT:CH{channel} {voltage:.2f}")

    def set_pdo(self, slot, v_mv, i_ma):
        """Sets a specific PDO slot with target voltage (mV) and current (mA)."""
        self.send_scpi(f"SOUR:PDO:SET {slot} {v_mv} {i_ma}")

    def get_build_date(self):
        """Queries the firmware build date and time."""
        return self.query("SYST:BUILD?")

    def set_output(self, channel, state):
        """Enables (1) or Disables (0) a specific output channel."""
        self.send_scpi(f"OUTP:CH{channel}:STAT {state}")

    def get_voltage_measurement(self, channel):
        """Reads the actual voltage measurement from the device."""
        response = self.query(f"MEAS:VOLT:CH{channel}?")
        try:
            return float(response)
        except (ValueError, TypeError):
            return 0.0

    def query_pdo_list(self):
        """Sends the SOUR:PD:LIST? query and collects data between the response markers."""
        if not self.is_connected:
            return []

        with self.lock:  # Serialize access to the serial port here as well
            try:
                # Clear stale data before sending the query
                self.ser.reset_input_buffer()
                print("TX > SOUR:PD:LIST?")
                self.ser.write(b"SOUR:PD:LIST?\n")

                pdo_lines = []
                recording = False
                start_time = time.time()

                # Listen to the port for up to 2 seconds or until the END marker is received
                while (time.time() - start_time) < 2.0:
                    if self.ser.in_waiting > 0:
                        line = self.ser.readline().decode('ascii').strip()

                        if "UI_PDO_LIST_START" in line:
                            recording = True
                            continue

                        if "UI_PDO_LIST_END" in line:
                            recording = False
                            break  # Data transfer completed

                        if recording and line:
                            pdo_lines.append(line)

                return pdo_lines
            except Exception as e:
                print(f"PDO Query Error: Device communication was lost! ({e})")
                self.is_connected = False
                return []