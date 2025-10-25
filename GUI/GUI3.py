import customtkinter
import tkinter as tk
from tkinter import messagebox, filedialog

# Set the appearance mode and color theme
customtkinter.set_appearance_mode("Dark")
customtkinter.set_default_color_theme("blue")


class PowerSupplyApp(customtkinter.CTk):
    def __init__(self):
        super().__init__()

        self.title("USB PD Power Supply Control Panel")
        self.geometry("900x800")

        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=0)
        self.grid_columnconfigure(0, weight=1)

        # -------------------
        self.adjustable_outputs = {}
        self.fixed_outputs = {}
        # -------------------

        # Main frame
        self.main_frame = customtkinter.CTkFrame(self)
        self.main_frame.grid(row=0, column=0, padx=20, pady=20, sticky="nsew")

        # Title
        title_label = customtkinter.CTkLabel(self.main_frame, text="USB PD Power Supply",
                                             font=customtkinter.CTkFont(size=24, weight="bold"))
        title_label.pack(pady=(20, 10))

        # --- Fixed Outputs Section ---
        fixed_frame = customtkinter.CTkFrame(self.main_frame, corner_radius=10)
        fixed_frame.pack(fill="x", padx=20, pady=10)
        fixed_title = customtkinter.CTkLabel(fixed_frame, text="Fixed Outputs",
                                             font=customtkinter.CTkFont(size=18, weight="bold"))
        fixed_title.pack(pady=(10, 5))

        for v in [3.3, 2.5, 1.8]:
            frame = customtkinter.CTkFrame(fixed_frame, fg_color="transparent")
            frame.pack(fill="x", padx=10, pady=5)

            label = customtkinter.CTkLabel(frame, text=f"{v} V:", width=50)
            label.pack(side="left", padx=(10, 5))

            switch = customtkinter.CTkSwitch(frame, text="OFF", command=lambda v=v: self.toggle_fixed_output(v))
            switch.pack(side="left", padx=5)

            current_entry = customtkinter.CTkEntry(frame, width=60)
            current_entry.insert(0, "0.5")
            current_entry.pack(side="left", padx=(10, 5))
            customtkinter.CTkLabel(frame, text="A").pack(side="left", padx=(0, 10))

            set_button = customtkinter.CTkButton(frame, text="Set", width=60,
                                                 command=lambda v=v: self.set_fixed_output(v))
            set_button.pack(side="left", padx=5)

            save_button = customtkinter.CTkButton(frame, text="Save", width=60,
                                                  command=lambda v=v: self.save_fixed_output(v))
            save_button.pack(side="left", padx=5)

            status_label = customtkinter.CTkLabel(frame, text="Status: Ready", text_color="grey")
            status_label.pack(side="left", padx=10)

            self.fixed_outputs[v] = {
                "switch": switch,
                "status": status_label,
                "current_entry": current_entry
            }

        # --- Adjustable Outputs Section ---
        adjustable_frame = customtkinter.CTkFrame(self.main_frame, corner_radius=10)
        adjustable_frame.pack(fill="x", padx=20, pady=10)

        adjustable_title = customtkinter.CTkLabel(adjustable_frame, text="Adjustable Outputs",
                                                  font=customtkinter.CTkFont(size=18, weight="bold"))
        adjustable_title.pack(pady=(10, 5))

        self.create_adjustable_output(adjustable_frame, "Output 1", "0.5V – 5V", (0.5, 5.0))
        self.create_adjustable_output(adjustable_frame, "Output 2", "2.5V – 32V", (2.5, 32.0))

        # --- Status Bar ---
        status_bar = customtkinter.CTkFrame(self, height=30, corner_radius=0)
        status_bar.grid(row=1, column=0, sticky="nsew")
        self.connection_status = customtkinter.CTkLabel(status_bar,
                                                        text="Connection Status: Device Disconnected",
                                                        font=customtkinter.CTkFont(size=12, weight="bold"),
                                                        text_color="red")
        self.connection_status.pack(padx=10, pady=5, side="left")

    def create_adjustable_output(self, parent, title, range_text, voltage_range):
        frame = customtkinter.CTkFrame(parent, fg_color="transparent")
        frame.pack(fill="x", padx=10, pady=5)

        # Channel label
        label = customtkinter.CTkLabel(frame, text=f"{title} ({range_text}):", width=180)
        label.pack(side="left", padx=(10, 5))

        # Toggle button
        toggle_button = customtkinter.CTkButton(frame, text="ON", width=60,
                                                command=lambda t=title: self.toggle_adjustable_output(t))
        toggle_button.pack(side="left", padx=5)

        # Mode selection
        mode_menu = customtkinter.CTkOptionMenu(frame,
                                                values=["Constant (DC)", "Sine", "Square", "Triangle", "Ramp", "Arbitrary"],
                                                command=lambda val, t=title: self.toggle_waveform_controls(val, t))
        mode_menu.pack(side="left", padx=10)

        # DC Controls
        dc_controls_frame = customtkinter.CTkFrame(frame, fg_color="transparent")
        dc_controls_frame.pack(side="left")

        voltage_entry = customtkinter.CTkEntry(dc_controls_frame, width=60)
        voltage_entry.insert(0, str(voltage_range[0]))
        voltage_entry.pack(side="left", padx=(10, 5))
        customtkinter.CTkLabel(dc_controls_frame, text="V").pack(side="left")

        current_entry = customtkinter.CTkEntry(dc_controls_frame, width=60)
        current_entry.insert(0, "0.5")
        current_entry.pack(side="left", padx=(10, 5))
        customtkinter.CTkLabel(dc_controls_frame, text="A").pack(side="left")

        # Waveform Controls
        waveform_controls_frame = customtkinter.CTkFrame(frame, fg_color="transparent")

        frequency_entry = customtkinter.CTkEntry(waveform_controls_frame, width=80)
        frequency_entry.insert(0, "1000")
        frequency_entry.pack(side="left", padx=(10, 5))
        customtkinter.CTkLabel(waveform_controls_frame, text="Hz").pack(side="left")

        amplitude_entry = customtkinter.CTkEntry(waveform_controls_frame, width=80)
        amplitude_entry.insert(0, "1.0")
        amplitude_entry.pack(side="left", padx=(10, 5))
        customtkinter.CTkLabel(waveform_controls_frame, text="Vp-p").pack(side="left")

        # Arbitrary file button
        arb_file_button = customtkinter.CTkButton(frame, text="Load ARB File", width=120,
                                                  command=lambda t=title: self.load_arb_file(t))
        arb_file_button.pack_forget()

        # Set / Save
        set_button = customtkinter.CTkButton(frame, text="Set", width=60,
                                             command=lambda t=title: self.set_channel(t))
        set_button.pack(side="left", padx=5)

        save_button = customtkinter.CTkButton(frame, text="Save", width=60,
                                              command=lambda t=title: self.save_channel(t))
        save_button.pack(side="left", padx=5)

        status_label = customtkinter.CTkLabel(frame, text="Status: Ready", text_color="grey")
        status_label.pack(side="left", padx=10)

        self.adjustable_outputs[title] = {
            "toggle_button": toggle_button,
            "mode_menu": mode_menu,
            "dc_controls_frame": dc_controls_frame,
            "waveform_controls_frame": waveform_controls_frame,
            "arb_file_button": arb_file_button,
            "voltage_entry": voltage_entry,
            "current_entry": current_entry,
            "frequency_entry": frequency_entry,
            "amplitude_entry": amplitude_entry,
            "set_button": set_button,
            "save_button": save_button,
            "status": status_label,
            "arb_data": None
        }

    def toggle_waveform_controls(self, value, title):
        w = self.adjustable_outputs[title]
        w["dc_controls_frame"].pack_forget()
        w["waveform_controls_frame"].pack_forget()
        w["arb_file_button"].pack_forget()

        if value == "Constant (DC)":
            w["dc_controls_frame"].pack(side="left")
        else:
            w["waveform_controls_frame"].pack(side="left")
            if value == "Arbitrary":
                w["arb_file_button"].pack(side="left", padx=(10, 0))

    def load_arb_file(self, title):
        file_path = filedialog.askopenfilename(defaultextension=".txt",
                                               filetypes=[("Text files", "*.txt"), ("CSV files", "*.csv")])
        if file_path:
            try:
                with open(file_path, 'r') as f:
                    data_points = [float(line.strip()) for line in f if line.strip()]
                    self.adjustable_outputs[title]["arb_data"] = data_points
                    messagebox.showinfo("Success", f"Arbitrary waveform loaded from {file_path}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to read file: {e}")
                self.adjustable_outputs[title]["arb_data"] = None

    def toggle_fixed_output(self, voltage):
        switch = self.fixed_outputs[voltage]["switch"]
        status_label = self.fixed_outputs[voltage]["status"]
        if switch.get() == 1:
            switch.configure(text="ON")
            status_label.configure(text="Status: Active", text_color="green")
            print(f"{voltage}V output turned ON.")
        else:
            switch.configure(text="OFF")
            status_label.configure(text="Status: Ready", text_color="grey")
            print(f"{voltage}V output turned OFF.")

    def toggle_adjustable_output(self, title):
        w = self.adjustable_outputs[title]
        btn = w["toggle_button"]
        mode = w["mode_menu"].get()

        if btn.cget("text") == "ON":
            try:
                if mode == "Constant (DC)":
                    v = float(w["voltage_entry"].get())
                    c = float(w["current_entry"].get())
                    print(f"Turning ON {title} DC: {v}V, {c}A")
                elif mode == "Arbitrary":
                    if not w["arb_data"]:
                        messagebox.showerror("Error", "No ARB data loaded.")
                        return
                    print(f"Turning ON {title} ARB: {len(w['arb_data'])} points")
                else:
                    f = float(w["frequency_entry"].get())
                    a = float(w["amplitude_entry"].get())
                    print(f"Turning ON {title} {mode}: {f}Hz, {a}Vp-p")

                btn.configure(text="OFF")
                self.configure_widgets_state(title, "disabled")
            except ValueError:
                messagebox.showerror("Error", "Invalid values.")
        else:
            print(f"Turning OFF {title}")
            btn.configure(text="ON")
            self.configure_widgets_state(title, "normal")

    def configure_widgets_state(self, title, state):
        w = self.adjustable_outputs[title]
        w["mode_menu"].configure(state=state)
        w["voltage_entry"].configure(state=state)
        w["current_entry"].configure(state=state)
        w["frequency_entry"].configure(state=state)
        w["amplitude_entry"].configure(state=state)
        w["arb_file_button"].configure(state=state)

    def set_fixed_output(self, voltage):
        c = self.fixed_outputs[voltage]["current_entry"].get()
        print(f"[SET] {voltage}V limit={c}A")

    def save_fixed_output(self, voltage):
        c = self.fixed_outputs[voltage]["current_entry"].get()
        print(f"[SAVE] {voltage}V limit={c}A saved")

    def set_channel(self, channel, fixed=False):
        if fixed:
            c = self.fixed_outputs[channel]["current_entry"].get()
            print(f"[SET] Fixed {channel}V, {c}A")
        else:
            v = self.adjustable_outputs[channel]["voltage_entry"].get()
            c = self.adjustable_outputs[channel]["current_entry"].get()
            print(f"[SET] {channel}: {v}V, {c}A")

    def save_channel(self, channel, fixed=False):
        if fixed:
            c = self.fixed_outputs[channel]["current_entry"].get()
            print(f"[SAVE] Fixed {channel}V, {c}A saved")
        else:
            v = self.adjustable_outputs[channel]["voltage_entry"].get()
            c = self.adjustable_outputs[channel]["current_entry"].get()
            print(f"[SAVE] {channel}: {v}V, {c}A saved")


if __name__ == "__main__":
    app = PowerSupplyApp()
    app.mainloop()
