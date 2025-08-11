#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>

// ==== Wi-Fi Credentials ====
const char* ssid = "ENTER SSID HERE";
const char* password = "ENTER PASSWORD HERE";

// ==== Relay Pins ====
const int relayPins[4] = {5, 18, 19, 0}; // Adjust as needed. These are the GPIO pins on the ESP32 which connect to the relay board.
bool relayState[4] = {false, false, false, false};
String relayNames[4] = {"Antenna 1", "Antenna 2", "Antenna 3", "Antenna 4"};

Preferences prefs;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// === Main control page HTML ===
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Antenna Control</title>
<style>
  body {
    background-color: #121212;
    color: #e0e0e0;
    font-family: Arial, sans-serif;
    text-align: center;
    padding: 20px;
    margin: 0;
  }
  h2 {
    color: #00bcd4;
    margin-bottom: 30px;
  }
  #buttons {
    display: flex;
    justify-content: center;
    flex-wrap: wrap;
    gap: 15px;
    margin-bottom: 30px;
  }
  button {
    padding: 15px 25px;
    font-size: 18px;
    border: none;
    border-radius: 6px;
    cursor: pointer;
    min-width: 160px;
    transition: background-color 0.3s, transform 0.1s;
  }
  .on {
    background-color: #4caf50;
    color: white;
  }
  .off {
    background-color: #f44336;
    color: white;
  }
  button:hover {
    opacity: 0.9;
    transform: scale(1.05);
  }
  /* Settings cog */
  #settingsCog {
    position: fixed;
    top: 10px;
    right: 10px;
    font-size: 28px;
    color: #00bcd4;
    cursor: pointer;
    user-select: none;
  }
  #settingsCog:hover {
    color: #80deea;
  }
</style>
</head>
<body>
<div id="settingsCog" title="Settings" onclick="location.href='/settings'">&#9881;</div>
<h2>ESP32 Antenna Control</h2>
<div id="buttons"></div>
<script>
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

function initWebSocket() {
  websocket = new WebSocket(gateway);
  websocket.onmessage = function(event) {
    let data = JSON.parse(event.data);
    let states = data.states;
    let names = data.names;
    let html = "";
    for (let i = 0; i < states.length; i++) {
      let btnClass = states[i] ? "on" : "off";
      html += `<button class='${btnClass}' onclick='toggleRelay(${i})'>${names[i]}</button>`;
    }
    document.getElementById("buttons").innerHTML = html;
  };
  websocket.onclose = function() {
    // Try reconnect every 2 seconds if connection lost
    setTimeout(initWebSocket, 2000);
  };
}

function toggleRelay(id) {
  websocket.send(JSON.stringify({ action: "toggle", id: id }));
}

window.addEventListener('load', initWebSocket);
</script>
</body>
</html>
)rawliteral";

// === Settings page HTML ===
const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings - Antenna Names</title>
<style>
  body {
    background-color: #121212;
    color: #e0e0e0;
    font-family: Arial, sans-serif;
    padding: 20px;
    margin: 0;
  }
  h2 {
    color: #00bcd4;
    margin-bottom: 20px;
    text-align: center;
  }
  form {
    max-width: 400px;
    margin: 0 auto;
  }
  label {
    display: block;
    margin: 15px 0 5px 0;
  }
  input[type=text] {
    width: 100%;
    padding: 8px;
    font-size: 16px;
    border-radius: 5px;
    border: none;
  }
  button {
    margin-top: 25px;
    width: 100%;
    padding: 12px;
    background-color: #00bcd4;
    border: none;
    border-radius: 6px;
    font-size: 18px;
    color: white;
    cursor: pointer;
    transition: background-color 0.3s;
  }
  button:hover {
    background-color: #008c9e;
  }
  a {
    color: #00bcd4;
    display: block;
    text-align: center;
    margin-top: 15px;
    text-decoration: none;
  }
  a:hover {
    text-decoration: underline;
  }
</style>
</head>
<body>
<h2>Settings - Rename Antennas</h2>
<form method="POST" action="/save-names">
  <label for="name0">Antenna 1 Name:</label>
  <input type="text" id="name0" name="name0" required maxlength="30">
  
  <label for="name1">Antenna 2 Name:</label>
  <input type="text" id="name1" name="name1" required maxlength="30">
  
  <label for="name2">Antenna 3 Name:</label>
  <input type="text" id="name2" name="name2" required maxlength="30">
  
  <label for="name3">Antenna 4 Name:</label>
  <input type="text" id="name3" name="name3" required maxlength="30">
  
  <button type="submit">Save Names</button>
</form>
<a href="/">&#8592; Back to Control</a>
<script>
  // Populate inputs with current names from URL parameters if present (or we fill from JS later)
  window.onload = function() {
    fetch('/get-names')
      .then(response => response.json())
      .then(data => {
        for (let i=0; i<4; i++) {
          document.getElementById('name'+i).value = data.names[i];
        }
      });
  }
</script>
</body>
</html>
)rawliteral";

// Serve the control page
void onRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
}

// Serve the settings page
void onSettings(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", settings_html);
}

// Serve current antenna names as JSON for settings page to load
void onGetNames(AsyncWebServerRequest *request) {
  String json = "{\"names\":[";
  for (int i = 0; i < 4; i++) {
    json += "\"" + relayNames[i] + "\"";
    if (i < 3) json += ",";
  }
  json += "]}";
  request->send(200, "application/json", json);
}

// Handle saving names from settings page POST
void onSaveNames(AsyncWebServerRequest *request) {
  // Expecting form parameters: name0, name1, name2, name3
  for (int i = 0; i < 4; i++) {
    if (request->hasParam("name" + String(i), true)) {
      String newName = request->getParam("name" + String(i), true)->value();
      newName.trim();
      if (newName.length() > 0) {
        relayNames[i] = newName;
        prefs.putString(("name" + String(i)).c_str(), relayNames[i]);
      }
    }
  }
  // Redirect back to settings page after save
  request->redirect("/settings");
}

// Send relay states and names to all WebSocket clients
void notifyClients() {
  String json = "{\"states\":[";
  for (int i = 0; i < 4; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 3) json += ",";
  }
  json += "],\"names\":[";
  for (int i = 0; i < 4; i++) {
    json += "\"" + relayNames[i] + "\"";
    if (i < 3) json += ",";
  }
  json += "]}";
  ws.textAll(json);
}

// WebSocket message handler
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String msg = (char*)data;

    if (msg.indexOf("\"action\":\"toggle\"") >= 0) {
      int idStart = msg.indexOf("\"id\":") + 5;
      int relayID = msg.substring(idStart, msg.indexOf("}", idStart)).toInt();

      if (relayID >= 0 && relayID < 4) {
        // Turn OFF all relays first
        for (int i = 0; i < 4; i++) {
          relayState[i] = false;
          digitalWrite(relayPins[i], HIGH); // active-low OFF
        }
        // Turn ON selected relay
        relayState[relayID] = true;
        digitalWrite(relayPins[relayID], LOW); // active-low ON
        prefs.putInt("lastRelay", relayID);
        notifyClients();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    notifyClients();
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // all off
  }

  prefs.begin("antenna", false);

  // Load saved relay names
  for (int i = 0; i < 4; i++) {
    String savedName = prefs.getString(("name" + String(i)).c_str(), relayNames[i]);
    relayNames[i] = savedName;
  }

  // Restore last active relay
  int savedRelay = prefs.getInt("lastRelay", -1);
  if (savedRelay >= 0 && savedRelay < 4) {
    relayState[savedRelay] = true;
    digitalWrite(relayPins[savedRelay], LOW); // ON active-low
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, onRoot);
  server.on("/settings", HTTP_GET, onSettings);
  server.on("/get-names", HTTP_GET, onGetNames);
  server.on("/save-names", HTTP_POST, onSaveNames);

  server.begin();
}

void loop() {
  ws.cleanupClients();
}
