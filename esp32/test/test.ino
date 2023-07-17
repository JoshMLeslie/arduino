#include <WiFiManager.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

WiFiManager wm;

const byte PIN_RESET = 0;  // placeholder

WebServer server(80);

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",
           hr, min % 60, sec % 60);
  server.send(200, "text/html", temp);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(PIN_RESET, INPUT_PULLUP);
  bool res;

  WiFi.mode(WIFI_STA);

  wm.setClass("invert");  // dark theme

  // wm.resetSettings(); // reset for testing
  // start AP instance "uTab" with pw "uTab"
  res = wm.autoConnect("uTab", "uTabuTab");
  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("Connected...");
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("waiting for connection...");
  }

  if (MDNS.begin("uTab")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {

  server.handleClient();
  delay(2);  //allow the cpu to switch to other tasks

  if (!digitalRead(PIN_RESET)) {
    // poor mans debounce/press-hold, code not ideal for production
    Serial.println("Pressed!");
    delay(50);
    if (!digitalRead(PIN_RESET)) {
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideal for production
      delay(3000);  // reset delay hold
      if (!digitalRead(PIN_RESET)) {
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
    }
  }
}