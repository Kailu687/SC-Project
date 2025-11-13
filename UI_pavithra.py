import tkinter as tk
from tkinter import messagebox, simpledialog, scrolledtext
import serial
import serial.tools.list_ports
import threading
from datetime import datetime

API_KEY = "ABC123"  # Replace with your real key

class LaserCommApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Laser Link Controller")
        self.root.geometry("540x480")
        self.root.config(bg="#d0e6ff")  # soft pastel blue background

        self.serial_conn = None

        # === API KEY INPUT ===
        tk.Label(root, text="Enter API Key:", bg="#d0e6ff", font=("Segoe UI", 10, "bold")).pack(pady=5)
        self.api_entry = tk.Entry(root, show="*", width=30, font=("Consolas", 10))
        self.api_entry.pack()

        # === PORT SELECTION ===
        tk.Label(root, text="Select COM Port:", bg="#d0e6ff", font=("Segoe UI", 10, "bold")).pack(pady=5)
        self.port_var = tk.StringVar()
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if not ports:
            ports = ["No ports found"]
        self.port_var.set(ports[0])
        self.port_menu = tk.OptionMenu(root, self.port_var, *ports)
        self.port_menu.config(bg="#a8dadc", fg="black", font=("Segoe UI", 9, "bold"), relief="groove", activebackground="#457b9d")
        self.port_menu.pack()

        # === CONNECT / REFRESH ===
        connect_frame = tk.Frame(root, bg="#d0e6ff")
        connect_frame.pack(pady=8)

        self.make_button(connect_frame, "Connect", "#4ecdc4", "#3bb3ad", self.connect_arduino).grid(row=0, column=0, padx=8)
        self.make_button(connect_frame, "Refresh Ports", "#ffb703", "#e09e00", self.refresh_ports).grid(row=0, column=1, padx=8)

        # === STATUS LABEL ===
        self.status_label = tk.Label(root, text="Status: Disconnected", fg="red", bg="#d0e6ff", font=("Segoe UI", 10, "bold"))
        self.status_label.pack(pady=5)

        # === MAIN CONTROL BUTTONS (CAL, TX, RX) ===
        control_frame = tk.Frame(root, bg="#d0e6ff")
        control_frame.pack(pady=10)

        self.make_button(control_frame, "CAL", "#a8dadc", "#8ecae6", lambda: self.send_cmd("CAL")).grid(row=0, column=0, padx=10)
        self.make_button(control_frame, "TX", "#ffd6a5", "#ffb703", self.tx_popup).grid(row=0, column=1, padx=10)
        self.make_button(control_frame, "RX", "#cdb4db", "#b388eb", lambda: self.send_cmd("RX")).grid(row=0, column=2, padx=10)

        # === LOG WINDOW ===
        tk.Label(root, text="Log Output:", bg="#d0e6ff", font=("Segoe UI", 10, "bold")).pack()
        self.log = scrolledtext.ScrolledText(root, width=65, height=12, state='disabled',
                                             bg="#fefae0", fg="black", font=("Consolas", 9))
        self.log.pack(padx=10, pady=5)

        # Handle window close
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    # ============== HELPER TO MAKE COLORFUL BUTTONS ==============
    def make_button(self, parent, text, color, hover_color, command):
        btn = tk.Button(parent, text=text, width=14, font=("Segoe UI", 10, "bold"),
                        bg=color, fg="black", relief="raised", activebackground=hover_color,
                        command=command, cursor="hand2", bd=2)
        return btn

    # ================= CORE FUNCTIONS =================
    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        menu = self.port_menu["menu"]
        menu.delete(0, "end")
        if not ports:
            ports = ["No ports found"]
        for p in ports:
            menu.add_command(label=p, command=lambda val=p: self.port_var.set(val))
        self.port_var.set(ports[0])
        self.log_message("[Ports refreshed]")

    def connect_arduino(self):
        entered_key = self.api_entry.get()
        if entered_key != API_KEY:
            messagebox.showerror("Auth Error", "Invalid API Key!")
            return

        port = self.port_var.get()
        if not port or port == "No ports found":
            messagebox.showerror("Connection Failed", "Please select a valid COM port first.")
            return

        try:
            self.serial_conn = serial.Serial(port, 9600, timeout=1)
            self.log_message(f"[Connected to {port}]")
            self.status_label.config(text=f"Status: Connected to {port}", fg="green")
            threading.Thread(target=self.read_serial, daemon=True).start()
        except Exception as e:
            messagebox.showerror("Connection Failed", str(e))

    def send_cmd(self, cmd):
        if not self.serial_conn or not self.serial_conn.is_open:
            messagebox.showwarning("Not Connected", "Connect to Arduino first.")
            return
        self.serial_conn.write((cmd + "\n").encode())
        self.log_message(f"> {cmd}")

    def tx_popup(self):
        if not self.serial_conn or not self.serial_conn.is_open:
            messagebox.showwarning("Not Connected", "Connect to Arduino first.")
            return
        msg = simpledialog.askstring("Transmit Message", "Enter message to send:")
        if msg:
            self.send_cmd(f"TX {msg.strip()}")

    def read_serial(self):
        while True:
            if self.serial_conn and self.serial_conn.in_waiting:
                line = self.serial_conn.readline().decode(errors='ignore').strip()
                if line:
                    self.log_message(line)

    def log_message(self, text):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log.config(state='normal')
        self.log.insert(tk.END, f"[{timestamp}] {text}\n")
        self.log.see(tk.END)
        self.log.config(state='disabled')

    def on_close(self):
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = LaserCommApp(root)
    root.mainloop()