
<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/ed2d1f14-5be5-40e4-816f-91c22de6710f" />

# BenchVolt-PD
BenchVolt PD is an open-source, USB-C powered multi-channel lab power supply delivering up to 100 W. Features 5 outputs (0 V–32 V), STM32 control, USB-PD, low-noise LDOs, and a Python GUI. Compact, portable, and perfect for makers and engineers.

 <img width="1370" height="885" alt="image" src="https://github.com/user-attachments/assets/c5a5bef1-8a05-4daa-b570-c7627bd95224" />


**How It Works;**
When **BenchVolt PD** powered on, all regulators and converters start in the **disabled state**. The STM32 microcontroller first powers up and performs safety checks by monitoring **temperature, current, and voltage**. It then **enables** the DC-DC converters, followed by the linear regulators in sequence.

Throughout operation, the MCU **continuously monitors** system all parameters to maintain safe operating conditions.

An **additional safety layer** can be used by setting a power limit through the **USB PD interface**, ensuring the system never exceeds the predefined power threshold. This limit can be configured either from the device’s on-screen menu / rotary encoder or through the Python control interface.


**Each DC-DC converter** is monitored to ensure no more than 5 A is drawn from its output. The **1.8 V** and **2.5 V LDO** regulators share the same **4 V / 5 A** pre-regulator rail, while the **3.3 V** and **Adjustable (0.5 V – 5.5 V)** LDOs share the **5.5 V / 5 A** rail. Therefore, when both LDOs on the same rail are heavily loaded, their combined output current should not exceed 5 A total (typically below 3 A per channel). 

**The third buck-boost output (2.5 V – 32 V)** operates independently and is capable of delivering up to **3A**. Since this channel’s output comes directly from the DC-DC converter, its ripple and noise levels are relatively higher; however, overall stability and performance remain excellent for most applications.

**The other outputs** — regulated through LDOs — provide exceptionally low ripple, offering clean and stable voltages ideal **for sensitive analog and digital circuits.**


Note:

- In theory, the system is capable of delivering up to 100W of total power. However, due to conversion and regulation losses within the DC-DC converters and LDOs, the full **100W cannot be used.** 

- The maximum achievable power depends on the connected USB PD adapter / cable — for example, a 65 W charger will cap the system power at 65 W.



## **Features & Specifications**

### **Power & Outputs**
- Five independent output channels with adjustable voltage and current  
- Fixed outputs: **1.8 V, 2.5 V, 3.3 V @ up to 3 A**  
- Adjustable Output 1: **0.5 V – 5 V @ up to 3 A**  
- Adjustable Output 2: **2.5 V – 32 V @ up to 3 A**  
- 2.54 mm (100 mil) pin headers for powering multiple evaluation boards  
- Arbitrary waveform generation and predefined waveforms (**Square, Sine, Triangle, Ramp**) available on adjustable channels  

---

### **Output Noise & Ripple**
**_Image or Table will be added here..._**
---

### **Arbitrary Functions**
- Number of Points: **1024**  
- Resolution: **12-bit**  
- Point Parameters: **Dwell time** and **Voltage**  
- Dwell Time Range: **4 ms – 16,384 ms**  
- Repetition Rate: **1 – 255 times** or **continuous**

---

### **USB Power Delivery Support**
- USB-C input supporting **PD sink mode**  
- Up to **100 W** USB-PD power input  

---

### **Controls**
- **1.9″ TFT display (170 × 320)** for real-time voltage, current, and PD status  
- **Rotary encoder** for fast menu navigation and fine (5 mV) step adjustments  
- **SCPI command support** for remote programming  
- **Python GUI** for desktop monitoring and control  

---

### **Electronics**
- **Microcontroller:** STM32F030F4
- **USB-PD Controller:** STUSB4500 (sink mode)  
- Configurable **LDOs and boost converters** for precise output regulation  
- **Over-current protection** on all channels  
- **Firmware upgradeable via USB** through the Python interface (no ST-LINK required)
