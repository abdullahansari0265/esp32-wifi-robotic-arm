# ESP32 Wi-Fi-Controlled Robotic Arm

A 5-DOF robotic arm controller using an ESP32, PCA9685 PWM servo driver, and a mobile-friendly web interface with two virtual joysticks and a gripper slider.

## Features

- ESP32 creates its own Wi-Fi access point.
- Browser-based control interface; no separate mobile app required.
- PCA9685 servo driver over I²C at address `0x40`.
- Two joystick controls:
  - Left joystick: base rotation and shoulder elevation.
  - Right joystick: wrist and elbow.
- Gripper slider limited in software to 60°–150°.
- Gradual servo movement to reduce abrupt target changes.
- Reset control for the configured starting positions.
- Responsive interface designed with landscape phone use in mind.

## Hardware

- ESP32 development board
- PCA9685 16-channel PWM servo driver
- Compatible servo motors and mechanical arm
- External, suitably rated servo power supply
- Common ground between ESP32, PCA9685, and servo power supply

### I²C wiring

| ESP32 | PCA9685 |
|---|---|
| GPIO 21 (SDA) | SDA |
| GPIO 22 (SCL) | SCL |
| 3.3 V | VCC (logic power) |
| GND | GND |

**Power warning:** Do not power high-torque servos from the ESP32's 3.3 V pin. Use a separate supply rated for the servos' voltage and stall current, and connect grounds together. Verify each servo's voltage range and mechanical limits before operation.

## Software requirements

- Arduino IDE
- ESP32 board support installed in Arduino IDE
- Adafruit PWM Servo Driver Library (`Adafruit_PWMServoDriver`)
- Libraries included with the ESP32 Arduino core: `WiFi`, `WebServer`, and `Wire`

## Setup

1. Open `firmware/ESP32_Robotic_Arm_V2.ino` in Arduino IDE.
2. Set `AP_SSID` and change `AP_PASSWORD` to a private password of at least 8 characters.
3. Select your ESP32 board and serial port.
4. Install the **Adafruit PWM Servo Driver Library** if it is not installed.
5. Check servo wiring, channel assignments, supply voltage, and mechanical travel.
6. Upload the sketch.
7. Open Serial Monitor at **115200 baud** and note the printed access-point IP address.
8. Connect your phone or computer to the configured Wi-Fi network.
9. Open the IP address shown in Serial Monitor in a browser. The default ESP32 soft-AP address is commonly `192.168.4.1`.

## Servo channels and limits

| Joint | PCA9685 channel | Configured angle range | Initial target |
|---|---:|---:|---:|
| Base | 0 | 0°–180° | 90° |
| Shoulder | 1 | 0°–180° | 0° |
| Elbow | 2 | 0°–180° | 0° |
| Wrist | 3 | 0°–180° | 0° |
| Gripper | 4 | 60°–150° | 60° |

These are software angle settings, not a guarantee of the physical servo's true range. Calibrate each joint carefully before normal use. If a servo is a continuous-rotation type, angle commands will not behave like position commands.

## Control endpoints

The ESP32 web server exposes these routes for the included interface:

- `/` — control page
- `/joystickLeft?base=...&shoulder=...` — base and shoulder targets
- `/joystickRight?wrist=...&elbow=...` — wrist and elbow targets
- `/gripper?angle=...` — gripper target
- `/reset` — return targets to configured starting positions

This sketch uses HTTP GET requests on a local access point and is intended for a trusted local test setup, not direct exposure to the public internet.

## Safety and limitations

- Keep hands and objects clear of the arm's movement area during testing.
- Test without a payload first, at reduced speed where possible.
- Software limits do not replace physical stops or careful calibration.
- The sketch assumes positional servos and a compatible PCA9685 setup.
- Confirm the firmware matches your current wiring and hardware before use.
- Change the example Wi-Fi credentials before flashing.

## Project status

This repository documents the controller firmware and web UI. Add photos, a short demonstration video, wiring diagrams, and a list of known limitations as the project documentation improves.

## License

Choose a license before making the repository public. If you want others to reuse and modify the code, consider an open-source license such as MIT after confirming you are comfortable with those terms.
