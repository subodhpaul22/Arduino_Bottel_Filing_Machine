import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QLabel, QLineEdit,
    QPushButton, QVBoxLayout, QHBoxLayout, QMessageBox,
    QGroupBox, QStatusBar, QComboBox, QSpinBox, QDoubleSpinBox
)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont
import serial
import serial.tools.list_ports

class BottleFillingControl(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Bottle Filling Control System")
        self.serial_port = None
        self.connected = False
        self.repeat_mode = False
        
        self.initUI()
        self.detect_ports()
        
        # Set up status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready to connect")

    def initUI(self):
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout()
        main_widget.setLayout(layout)
        
        # Connection Group
        conn_group = QGroupBox("Serial Connection")
        conn_layout = QHBoxLayout()
        
        self.port_combo = QComboBox()
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.baud_combo.setCurrentText("9600")
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white;")
        self.connect_btn.clicked.connect(self.toggle_connection)
        
        conn_layout.addWidget(QLabel("Port:"))
        conn_layout.addWidget(self.port_combo)
        conn_layout.addWidget(QLabel("Baud:"))
        conn_layout.addWidget(self.baud_combo)
        conn_layout.addWidget(self.connect_btn)
        conn_group.setLayout(conn_layout)
        layout.addWidget(conn_group)
        
        # Parameters Group
        param_group = QGroupBox("Filling Parameters")
        param_layout = QVBoxLayout()
        
        self.fill_time = self.create_spinbox("Fill Time (s):", QDoubleSpinBox, 0.1, 60, 5, 0.1)
        self.bottle_count = self.create_spinbox("Bottle Count:", QSpinBox, 1, 100, 4, 1)
        self.bottle_dist = self.create_spinbox("Bottle Distance (cm):", QDoubleSpinBox, 1, 100, 10, 0.5)
        self.repeat_delay = self.create_spinbox("Repeat Delay (s):", QDoubleSpinBox, 1, 3600, 10, 1)
        
        param_layout.addLayout(self.fill_time)
        param_layout.addLayout(self.bottle_count)
        param_layout.addLayout(self.bottle_dist)
        param_layout.addLayout(self.repeat_delay)
        param_group.setLayout(param_layout)
        layout.addWidget(param_group)
        
        # Control Group
        control_group = QGroupBox("Controls")
        control_layout = QHBoxLayout()
        
        self.start_btn = QPushButton("Start")
        self.start_btn.setStyleSheet("background-color: #2196F3; color: white;")
        self.start_btn.clicked.connect(self.start_filling)
        
        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setStyleSheet("background-color: #f44336; color: white;")
        self.stop_btn.clicked.connect(self.stop_filling)
        
        self.repeat_btn = QPushButton("Repeat OFF")
        self.repeat_btn.setStyleSheet("background-color: #555; color: white;")
        self.repeat_btn.clicked.connect(self.toggle_repeat_mode)
        
        control_layout.addWidget(self.start_btn)
        control_layout.addWidget(self.stop_btn)
        control_layout.addWidget(self.repeat_btn)
        control_group.setLayout(control_layout)
        layout.addWidget(control_group)
        
        # Set minimum size
        self.setMinimumSize(500, 400)
        
    def create_spinbox(self, label, widget_type, min_val, max_val, default, step):
        layout = QHBoxLayout()
        layout.addWidget(QLabel(label))
        spinbox = widget_type()
        spinbox.setRange(min_val, max_val)
        spinbox.setValue(default)
        if hasattr(spinbox, 'setSingleStep'):
            spinbox.setSingleStep(step)
        layout.addWidget(spinbox)
        return layout
    
    def detect_ports(self):
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        if ports:
            for port in ports:
                self.port_combo.addItem(port.device)
        else:
            self.port_combo.addItem("No ports found")
    
    def toggle_connection(self):
        if self.connected:
            self.disconnect_serial()
        else:
            self.connect_serial()
    
    def connect_serial(self):
        port = self.port_combo.currentText()
        baud = int(self.baud_combo.currentText())
        
        if port == "No ports found":
            QMessageBox.warning(self, "Error", "No serial ports available")
            return
        
        try:
            self.serial_port = serial.Serial(port, baud, timeout=1)
            self.connected = True
            self.connect_btn.setText("Disconnect")
            self.connect_btn.setStyleSheet("background-color: #f44336; color: white;")
            self.status_bar.showMessage(f"Connected to {port} at {baud} baud")
            
            # Start reading from serial
            self.read_timer = QTimer(self)
            self.read_timer.timeout.connect(self.read_serial)
            self.read_timer.start(100)
            
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to connect: {str(e)}")
    
    def disconnect_serial(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.connected = False
        self.connect_btn.setText("Connect")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white;")
        self.status_bar.showMessage("Disconnected")
        
        if hasattr(self, 'read_timer'):
            self.read_timer.stop()
    
    def read_serial(self):
        if self.serial_port and self.serial_port.in_waiting:
            try:
                data = self.serial_port.readline().decode('utf-8').strip()
                if data:
                    self.status_bar.showMessage(f"Received: {data}", 3000)
            except:
                pass
    
    def start_filling(self):
        if not self.connected:
            QMessageBox.warning(self, "Error", "Not connected to any device")
            return
        
        try:
            fill_time = self.fill_time.itemAt(1).widget().value()
            count = self.bottle_count.itemAt(1).widget().value()
            distance = self.bottle_dist.itemAt(1).widget().value()
            delay = self.repeat_delay.itemAt(1).widget().value()
            
            self.send_command(f"SET FILLTIME {fill_time}")
            self.send_command(f"SET COUNT {count}")
            self.send_command(f"SET DISTANCE {distance}")
            self.send_command(f"SET REPEATDELAY {delay}")
            self.send_command("START")
            
            self.status_bar.showMessage("Started filling process")
            
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to start: {str(e)}")
    
    def stop_filling(self):
        if self.connected:
            self.send_command("STOP")
            self.status_bar.showMessage("Stopped filling process")
    
    def toggle_repeat_mode(self):
        if not self.connected:
            QMessageBox.warning(self, "Error", "Not connected to any device")
            return
        try:
            self.send_command("TOGGLE REPEAT")
            self.repeat_mode = not self.repeat_mode
            if self.repeat_mode:
                self.repeat_btn.setText("Repeat ON")
                self.repeat_btn.setStyleSheet("background-color: #009688; color: white;")
                self.status_bar.showMessage("Repeat mode ON")
            else:
                self.repeat_btn.setText("Repeat OFF")
                self.repeat_btn.setStyleSheet("background-color: #555; color: white;")
                self.status_bar.showMessage("Repeat mode OFF")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to toggle repeat: {str(e)}")
    
    def send_command(self, command):
        try:
            self.serial_port.write((command + "\n").encode('utf-8'))
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to send command: {str(e)}")
    
    def closeEvent(self, event):
        if self.connected:
            self.disconnect_serial()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    # Set consistent font
    font = QFont()
    font.setFamily("Segoe UI")
    font.setPointSize(10)
    app.setFont(font)
    
    window = BottleFillingControl()
    window.show()
    sys.exit(app.exec_())
