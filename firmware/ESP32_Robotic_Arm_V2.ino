
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =====================================================
// ESP32 ROBOTIC ARM - V2
// 5 DOF | PCA9685 | Wi-Fi Joystick Control
// =====================================================

// ---------- Wi-Fi Access Point ----------
// IMPORTANT: Change AP_PASSWORD before flashing the ESP32.
const char* AP_SSID = "RobotArm_AP";
const char* AP_PASSWORD = "CHANGE_THIS_PASSWORD";

// ---------- PCA9685 ----------
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
WebServer server(80);

#define SDA_PIN 21
#define SCL_PIN 22

// ---------- Servo configuration ----------
const uint8_t BASE_CH     = 0;
const uint8_t SHOULDER_CH = 1;
const uint8_t ELBOW_CH    = 2;
const uint8_t WRIST_CH    = 3;
const uint8_t GRIPPER_CH  = 4;

const int SERVOMIN = 150;
const int SERVOMAX = 600;

// ---------- Motion settings ----------
const unsigned long UPDATE_INTERVAL = 20;  // ms
const float MAX_STEP = 2.0f;               // degrees per update
const float DEADBAND = 2.0f;               // degrees

// Servo limits
const float BASE_MIN = 0;
const float BASE_MAX = 180;

const float SHOULDER_MIN = 0;
const float SHOULDER_MAX = 180;

const float ELBOW_MIN = 0;
const float ELBOW_MAX = 180;

const float WRIST_MIN = 0;
const float WRIST_MAX = 180;

const float GRIPPER_MIN = 60;
const float GRIPPER_MAX = 150;

// Current servo positions
float currentBase = 90;
float currentShoulder = 0;
float currentElbow = 0;
float currentWrist = 0;
float currentGripper = 60;

// Target servo positions
float targetBase = 90;
float targetShoulder = 0;
float targetElbow = 0;
float targetWrist = 0;
float targetGripper = 60;

unsigned long lastUpdate = 0;

// =====================================================
// Utility functions
// =====================================================

float clampAngle(float angle, float minAngle, float maxAngle) {
  return constrain(angle, minAngle, maxAngle);
}

uint16_t angleToPulse(float angle) {
  angle = clampAngle(angle, 0, 180);
  return (uint16_t)(SERVOMIN +
         (angle / 180.0f) * (SERVOMAX - SERVOMIN));
}

void writeServo(uint8_t channel, float angle) {
  pwm.setPWM(channel, 0, angleToPulse(angle));
}

// Move current angle gradually toward target
void smoothServo(float &current, float target) {
  float difference = target - current;

  if (fabs(difference) <= DEADBAND) {
    current = target;
    return;
  }

  if (difference > MAX_STEP) {
    current += MAX_STEP;
  } else if (difference < -MAX_STEP) {
    current -= MAX_STEP;
  } else {
    current = target;
  }
}

void updateServos() {
  unsigned long now = millis();

  if (now - lastUpdate < UPDATE_INTERVAL) {
    return;
  }

  lastUpdate = now;

  smoothServo(currentBase, targetBase);
  smoothServo(currentShoulder, targetShoulder);
  smoothServo(currentElbow, targetElbow);
  smoothServo(currentWrist, targetWrist);
  smoothServo(currentGripper, targetGripper);

  writeServo(BASE_CH, currentBase);
  writeServo(SHOULDER_CH, currentShoulder);
  writeServo(ELBOW_CH, currentElbow);
  writeServo(WRIST_CH, currentWrist);
  writeServo(GRIPPER_CH, currentGripper);
}

// =====================================================
// HTTP handlers
// =====================================================

void handleLeftJoystick() {
  if (server.hasArg("base")) {
    targetBase = clampAngle(
      server.arg("base").toFloat(), BASE_MIN, BASE_MAX
    );
  }

  if (server.hasArg("shoulder")) {
    targetShoulder = clampAngle(
      server.arg("shoulder").toFloat(),
      SHOULDER_MIN, SHOULDER_MAX
    );
  }

  server.send(200, "text/plain", "OK");
}

void handleRightJoystick() {
  if (server.hasArg("wrist")) {
    targetWrist = clampAngle(
      server.arg("wrist").toFloat(), WRIST_MIN, WRIST_MAX
    );
  }

  if (server.hasArg("elbow")) {
    targetElbow = clampAngle(
      server.arg("elbow").toFloat(), ELBOW_MIN, ELBOW_MAX
    );
  }

  server.send(200, "text/plain", "OK");
}

void handleGripper() {
  if (server.hasArg("angle")) {
    targetGripper = clampAngle(
      server.arg("angle").toFloat(),
      GRIPPER_MIN, GRIPPER_MAX
    );
  }

  server.send(200, "text/plain", "OK");
}

void handleReset() {
  targetBase = 90;
  targetShoulder = 0;
  targetElbow = 0;
  targetWrist = 0;
  targetGripper = 60;

  server.send(200, "text/plain", "Reset requested");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// =====================================================
// Web interface
// =====================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport"
 content="width=device-width, initial-scale=1.0,
 maximum-scale=1.0, user-scalable=no">

<title>ESP32 Robotic Arm</title>

<style>
:root {
  --bg: #10141d;
  --panel: #1b2331;
  --accent: #35c7ff;
  --text: #f1f5f9;
  --muted: #a7b2c4;
}

* {
  box-sizing: border-box;
  -webkit-tap-highlight-color: transparent;
}

html {
  width: 100%;
  height: 100%;
  background: var(--bg);
}

body {
  margin: 0;
  width: 100%;
  min-height: 100vh;
  min-height: 100dvh;
  background: var(--bg);
  color: var(--text);
  font-family: Arial, sans-serif;
  overscroll-behavior: none;
}

button, input {
  font: inherit;
}

.app {
  min-height: 100vh;
  min-height: 100dvh;
  padding: 12px 20px;
  padding-bottom: max(20px, env(safe-area-inset-bottom));
  padding-left: max(20px, env(safe-area-inset-left));
  padding-right: max(20px, env(safe-area-inset-right));
  display: flex;
  flex-direction: column;
  gap: 12px;
}

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

h1 {
  margin: 0;
  font-size: clamp(18px, 3vw, 26px);
}

.subtitle {
  color: var(--muted);
  font-size: 12px;
  margin-top: 4px;
}

.status {
  color: #71e6a4;
  font-size: 12px;
  white-space: nowrap;
}

.panel {
  background: var(--panel);
  border: 1px solid #303c50;
  border-radius: 15px;
  padding: 12px;
}

.controls {
  flex: 1;
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
  gap: 16px;
  align-items: center;
}

.control {
  min-width: 0;
  text-align: center;
}

.control h2 {
  font-size: 15px;
  margin: 0 0 8px;
}

.joystick {
  width: min(100%, 330px);
  aspect-ratio: 1;
  margin: auto;
  position: relative;
  border-radius: 50%;
  border: 2px solid #45546c;
  background:
    linear-gradient(90deg, transparent 49.5%,
      #344256 49.5%, #344256 50.5%, transparent 50.5%),
    linear-gradient(transparent 49.5%,
      #344256 49.5%, #344256 50.5%, transparent 50.5%),
    radial-gradient(circle, #283447, #141b28);
  touch-action: none;
  user-select: none;
  overflow: hidden;
}

.joystick::before,
.joystick::after {
  content: "";
  position: absolute;
  border: 1px solid #344256;
  border-radius: 50%;
  pointer-events: none;
}

.joystick::before {
  inset: 25%;
}

.joystick::after {
  inset: 10%;
}

.knob {
  position: absolute;
  width: 29%;
  aspect-ratio: 1;
  border-radius: 50%;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  background: linear-gradient(145deg, #51d8ff, #1689ed);
  border: 3px solid #a7edff;
  box-shadow: 0 0 18px #35c7ff55;
  pointer-events: none;
  z-index: 2;
}

.axes {
  display: flex;
  justify-content: space-between;
  color: var(--muted);
  font-size: 11px;
  margin-top: 7px;
}

.readout {
  color: var(--accent);
  font-size: 12px;
  margin-top: 6px;
}

.gripper-panel {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 8px 14px;
  align-items: center;
}

.gripper-panel label {
  font-size: 14px;
  font-weight: bold;
}

#gripper {
  grid-column: 1 / -1;
  width: 100%;
  accent-color: var(--accent);
  min-height: 28px;
}

.gripper-range {
  color: var(--muted);
  font-size: 11px;
  grid-column: 1 / -1;
  display: flex;
  justify-content: space-between;
}

button {
  color: white;
  background: #26364b;
  border: 1px solid #435671;
  padding: 10px 15px;
  border-radius: 10px;
  cursor: pointer;
}

button:active {
  background: #365174;
}

.reset {
  background: #173b55;
  border-color: #2787b7;
}

footer {
  color: var(--muted);
  text-align: center;
  font-size: 11px;
}

#portraitWarning {
  display: none;
  color: #ffce78;
  text-align: center;
  font-size: 12px;
}

@media (max-width: 600px) {
  .app {
    padding: 10px;
    padding-bottom: max(20px, env(safe-area-inset-bottom));
    padding-left: max(12px, env(safe-area-inset-left));
    padding-right: max(12px, env(safe-area-inset-right));
    gap: 10px;
  }

  .controls {
    gap: 10px;
  }

  .panel {
    padding: 9px;
  }

  .joystick {
    width: 100%;
  }

  .knob {
    width: 32%;
  }

  .control h2 {
    font-size: 13px;
  }
}

@media (max-height: 500px) and (orientation: landscape) {
  .app {
    padding-top: 6px;
    gap: 6px;
  }

  header .subtitle {
    display: none;
  }

  .controls {
    gap: 20px;
  }

  .joystick {
    width: min(100%, 42vh);
  }

  .panel {
    padding: 6px 10px;
  }

  #portraitWarning {
    display: none !important;
  }
}

@media (orientation: portrait) and (max-width: 600px) {
  #portraitWarning {
    display: block;
  }
}
</style>
</head>

<body>
<div class="app">

  <header>
    <div>
      <h1>Robotic Arm Control</h1>
      <div class="subtitle">ESP32 · 5 DOF · Wi-Fi controller</div>
    </div>
    <div class="status" id="status">Ready</div>
  </header>

  <div id="portraitWarning">
    Landscape mode is recommended for better joystick control.
  </div>

  <main class="controls">

    <section class="control">
      <h2>Left Joystick</h2>
      <div class="joystick" id="leftJoystick">
        <div class="knob" id="leftKnob"></div>
      </div>
      <div class="axes">
        <span>Base rotation</span>
        <span>Shoulder elevation</span>
      </div>
      <div class="readout" id="leftReadout">
        Base: 90° · Shoulder: 0°
      </div>
    </section>

    <section class="control">
      <h2>Right Joystick</h2>
      <div class="joystick" id="rightJoystick">
        <div class="knob" id="rightKnob"></div>
      </div>
      <div class="axes">
        <span>Wrist</span>
        <span>Elbow</span>
      </div>
      <div class="readout" id="rightReadout">
        Wrist: 0° · Elbow: 0°
      </div>
    </section>

  </main>

  <section class="panel gripper-panel">
    <label for="gripper">Gripper</label>
    <span id="gripperValue">60°</span>

    <input type="range" id="gripper"
      min="60" max="150" value="60" step="1">

    <div class="gripper-range">
      <span>Closed end · 60°</span>
      <span>Open end · 150°</span>
    </div>
  </section>

  <div style="display:flex; justify-content:center;">
    <button class="reset" id="resetButton">
      Reset arm position
    </button>
  </div>

  <footer>
    Keep clear of the arm's movement area. Check mechanical limits before operation.
  </footer>

</div>

<script>
"use strict";

const statusElement = document.getElementById("status");

let lastRequestTime = 0;
const REQUEST_INTERVAL = 120;

let leftState = { base: 90, shoulder: 0 };
let rightState = { wrist: 0, elbow: 0 };

let requestQueue = Promise.resolve();

function setStatus(message, isError = false) {
  statusElement.textContent = message;
  statusElement.style.color = isError ? "#ff8585" : "#71e6a4";
}

function sendRequest(path, force = false) {
  const now = Date.now();
  const wait = Math.max(
    0,
    REQUEST_INTERVAL - (now - lastRequestTime)
  );

  // Joystick requests are throttled.
  // Reset and gripper changes may bypass this interval.
  if (!force && wait > 0) {
    return;
  }

  lastRequestTime = now;

  requestQueue = requestQueue
    .catch(() => {})
    .then(async () => {
      const response = await fetch(path, {
        method: "GET",
        cache: "no-store"
      });

      if (!response.ok) {
        throw new Error("HTTP " + response.status);
      }

      setStatus("Connected");
    })
    .catch(() => {
      setStatus("Connection error", true);
    });
}

function sendLeft() {
  const query = new URLSearchParams({
    base: String(leftState.base),
    shoulder: String(leftState.shoulder)
  });

  sendRequest("/joystickLeft?" + query.toString());
}

function sendRight() {
  const query = new URLSearchParams({
    wrist: String(rightState.wrist),
    elbow: String(rightState.elbow)
  });

  sendRequest("/joystickRight?" + query.toString());
}

function setupJoystick(areaId, knobId, onMove) {
  const area = document.getElementById(areaId);
  const knob = document.getElementById(knobId);

  let activePointer = null;

  function move(event) {
    const rect = area.getBoundingClientRect();

    const centerX = rect.width / 2;
    const centerY = rect.height / 2;

    let x = (event.clientX - rect.left - centerX)
            / (rect.width / 2);

    let y = (event.clientY - rect.top - centerY)
            / (rect.height / 2);

    // Keep the joystick knob inside the circular boundary.
    const length = Math.sqrt(x * x + y * y);

    if (length > 1) {
      x /= length;
      y /= length;
    }

    // Deadband near the center.
    if (Math.abs(x) < 0.02) x = 0;
    if (Math.abs(y) < 0.02) y = 0;

    knob.style.left = (50 + x * 34) + "%";
    knob.style.top = (50 + y * 34) + "%";

    onMove(x, y);
  }

  area.addEventListener("pointerdown", event => {
    event.preventDefault();

    if (activePointer !== null) return;

    activePointer = event.pointerId;
    area.setPointerCapture(event.pointerId);
    move(event);
  });

  area.addEventListener("pointermove", event => {
    if (event.pointerId !== activePointer) return;
    event.preventDefault();
    move(event);
  });

  function release(event) {
    if (event.pointerId !== activePointer) return;

    activePointer = null;

    // Deliberately retain the last commanded position.
    // Moving the joystick does not automatically reset a joint.
  }

  area.addEventListener("pointerup", release);
  area.addEventListener("pointercancel", release);
}

setupJoystick("leftJoystick", "leftKnob", (x, y) => {
  // Reversed base mapping: left side = 180°, right = 0°.
  leftState.base = Math.round((1 - (x + 1) / 2) * 180);

  // Top = 180°, bottom = 0°.
  leftState.shoulder = Math.round((1 - (y + 1) / 2) * 180);

  document.getElementById("leftReadout").textContent =
    "Base: " + leftState.base + "° · Shoulder: " +
    leftState.shoulder + "°";

  sendLeft();
});

setupJoystick("rightJoystick", "rightKnob", (x, y) => {
  // Right joystick horizontal axis controls wrist.
  rightState.wrist = Math.round((x + 1) * 90);

  // Right joystick vertical axis controls elbow.
  rightState.elbow = Math.round((1 - (y + 1) / 2) * 180);

  document.getElementById("rightReadout").textContent =
    "Wrist: " + rightState.wrist + "° · Elbow: " +
    rightState.elbow + "°";

  sendRight();
});

const gripper = document.getElementById("gripper");

gripper.addEventListener("input", () => {
  const angle = Math.max(
    60, Math.min(150, Number(gripper.value))
  );

  document.getElementById("gripperValue").textContent =
    angle + "°";

  sendRequest("/gripper?angle=" + angle, true);
});

document.getElementById("resetButton")
  .addEventListener("click", async () => {
    setStatus("Resetting...");

    try {
      const response = await fetch("/reset", {
        method: "GET",
        cache: "no-store"
      });

      if (!response.ok) throw new Error("Reset failed");

      leftState = { base: 90, shoulder: 0 };
      rightState = { wrist: 0, elbow: 0 };

      document.getElementById("leftKnob").style.left = "50%";
      document.getElementById("leftKnob").style.top = "50%";
      document.getElementById("rightKnob").style.left = "50%";
      document.getElementById("rightKnob").style.top = "50%";

      document.getElementById("leftReadout").textContent =
        "Base: 90° · Shoulder: 0°";

      document.getElementById("rightReadout").textContent =
        "Wrist: 0° · Elbow: 0°";

      gripper.value = 60;
      document.getElementById("gripperValue").textContent = "60°";

      setStatus("Reset commanded");
    } catch (error) {
      setStatus("Reset failed", true);
    }
  });
</script>

</body>
</html>
)rawliteral";

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(50);

  // Initial target positions
  targetBase = 90;
  targetShoulder = 0;
  targetElbow = 0;
  targetWrist = 0;
  targetGripper = 60;

  currentBase = targetBase;
  currentShoulder = targetShoulder;
  currentElbow = targetElbow;
  currentWrist = targetWrist;
  currentGripper = targetGripper;

  // Write the initial servo positions.
  writeServo(BASE_CH, currentBase);
  writeServo(SHOULDER_CH, currentShoulder);
  writeServo(ELBOW_CH, currentElbow);
  writeServo(WRIST_CH, currentWrist);
  writeServo(GRIPPER_CH, currentGripper);

  // Start Wi-Fi access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("Robotic Arm V2 started");
  Serial.print("Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // HTTP routes
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/joystickLeft", HTTP_GET, handleLeftJoystick);
  server.on("/joystickRight", HTTP_GET, handleRightJoystick);
  server.on("/gripper", HTTP_GET, handleGripper);
  server.on("/reset", HTTP_GET, handleReset);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("Web server started");
}

// =====================================================
// Main loop
// =====================================================

void loop() {
  server.handleClient();
  updateServos();
}
