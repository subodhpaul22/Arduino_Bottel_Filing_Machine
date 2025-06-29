# Arduino_Bottel_Filing_Machine
🔌 ইলেকট্রনিক্স 
# Bottle Filler Automation System

This project automates bottle filling using Arduino and a custom-built GUI application.

---

## Project Overview

This system controls a bottle filling machine using Arduino hardware. It manages filling time, bottle count, bottle distance, and repeat delays.

A desktop GUI built with Python (PyQt5) allows users to configure parameters and communicate with the Arduino via serial communication.

---

## Features

- Adjustable fill time, bottle count, bottle distance, and repeat delay
- Start/Stop control via GUI buttons
- Repeat mode toggle from GUI
- Live status updates from Arduino displayed in GUI
- Serial port selection and baud rate configuration in GUI
- Cross-platform GUI application (Windows/Linux/MacOS)

---

## Hardware Requirements

- Arduino Uno (or compatible board)
- Solenoid valve or filling mechanism
- Stepper motor (for bottle movement)
- Sensors (e.g., bottle detection sensors)
- Power supply and wiring

---

## Software Requirements

- Arduino IDE for uploading firmware
- Python 3.x
- PyQt5 (`pip install pyqt5`)
- pyserial (`pip install pyserial`)

---

## Arduino Firmware

The Arduino firmware listens for commands over serial to set parameters and control the filling process.

Upload the code from the `arduino_firmware/` folder.

---

## GUI Application

The GUI app allows controlling the bottle filler parameters and machine operation.

Run the GUI from the `gui_app/` folder:

```bash
python bottle_filler_gui.py
