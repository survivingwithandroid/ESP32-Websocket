#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">  <title>ESP32 Websocket</title>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js">
  <script src="https://cdn.jsdelivr.net/npm/spectrum-colorpicker2/dist/spectrum.min.js"></script>
  
  <link rel="stylesheet" type="text/css" href="https://cdn.jsdelivr.net/npm/spectrum-colorpicker2/dist/spectrum.min.css">
  <script language="javascript">
  
    window.alert(location.host);
    var gwUrl = "ws://" + location.host + "/ws";
    var webSocket = new WebSocket(gwUrl);
    webSocket.onopen = function(e) {
        console.log("open");
    }
    webSocket.onclose = function(e) {
        console.log("close");
    }
  
   webSocket.onmessage = function(e) {
        console.log("message");
    }
    function handleColor() {
      var val = document.getElementById('type-color-on-page').value;
      webSocket.send(val.substring(1));         
    }
  </script>
  
  <style>
    h2 {background: #3285DC;
        color: #FFFFFF;
        align:center;
    }
  
    .content {
        border: 1px solid #164372;
        padding: 5px;
    }
    
    .button {
       background-color: #00b300; 
       border: none;
       color: white;
       padding: 8px 10px;
       text-align: center;
       text-decoration: none;
       display: inline-block;
       font-size: 14px;
  }
  </style>
</head>
<body>
  <h2>ESP32 Websocket</h2>
  <div class="content">
  <p>Pick a color</p>
  <div id="qunit"></div>
  
  <input type="color" id="type-color-on-page"  />
   <p>
     <input type="button" class="button" value="Send to ESP32" id="btn" onclick="handleColor()" />
   </p>
  
  </div>
</body>
</html>
)rawliteral";  


// WiFi config
const char *SSID = "your_ssid";
const char *PWD = "your_wifi_pwd";

// Web server running on port 80
AsyncWebServer server(80);
// Web socket
AsyncWebSocket ws("/ws");

// Neopixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, 15, NEO_GRB + NEO_KHZ800);

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}


void handlingIncomingData(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String hexColor = "";
    for (int i=0; i < len; i++)
      hexColor += ((char) data[i]);

    Serial.println("Hex Color: " + hexColor);
    
    long n = strtol(&hexColor[0], NULL, 16);
    Serial.println(n);
    strip.fill(n);
    strip.show();
  }
}

// Callback for incoming event
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, 
             void * arg, uint8_t *data, size_t len){
   switch(type) {
      case WS_EVT_CONNECT:
        Serial.printf("Client connected: \n\tClient id:%u\n\tClient IP:%s\n", 
             client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
         Serial.printf("Client disconnected:\n\tClient id:%u\n", client->id());
         break;
      case WS_EVT_DATA:
         handlingIncomingData(arg, data, len);
         break;
      case WS_EVT_PONG:
          Serial.printf("Pong:\n\tClient id:%u\n", client->id());
          break;
      case WS_EVT_ERROR:
          Serial.printf("Error:\n\tClient id:%u\n", client->id());
          break;     
   }
  
}

void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.setBrightness(255);
  strip.fill(strip.Color(200,0,0));
  strip.show();

  connectToWiFi();
  ws.onEvent(onEvent);
  server.addHandler(&ws);
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, NULL);
  });

   server.begin();
}

void loop() {
  ws.cleanupClients();
}