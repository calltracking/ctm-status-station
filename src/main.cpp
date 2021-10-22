/**
 * configure and listen for account or team events
 */
#include <TinyPICO.h>
#include <WiFi.h>
#include <WebServer.h>
#include "settings.h"
const char *default_ssid = "ctmlight";
const char *default_pass = "ctmstatus";

TinyPICO tp = TinyPICO();
WebServer server(80);
Settings conf;

void handle_Main();
void handle_Conf();
void handle_Controls();
void handleNotFound();

void setup() {
  bool localAP = false;
  Serial.begin(115200);

  delay(3000);
  conf.begin();


  if (conf.good() && conf.wifi_configured) {
    Serial.print("Connecting to network...");

    WiFi.begin(conf.ssid, conf.pass);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed!");
      delay(2000);
      Serial.println("Reset SSID...");
      memset(conf.ssid, 0, 32);
      memset(conf.pass, 0, 32);
      conf.wifi_configured = false;
      conf.save();
      delay(2000);
      Serial.println("Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("Connected!");
    }
    // this has a dramatic effect on packet RTT
    WiFi.setSleep(WIFI_PS_NONE);
    IPAddress localIP = WiFi.localIP();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("Setting AP...");
    Serial.println(default_ssid);
    Serial.println(default_pass);
    WiFi.softAP(default_ssid, default_pass);
    localAP = true;
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }

  server.on("/", HTTP_GET, handle_Main);
  server.on("/conf", HTTP_POST, handle_Conf);
  server.on("/controls", HTTP_GET, handle_Controls);
  server.onNotFound(handleNotFound);


  server.begin();
  Serial.println("setup complete");
  if (localAP) {
    Serial.println("Connect to AP:");
    Serial.println(default_ssid);
    Serial.println(default_pass);
  }
}

void loop() {
  tp.DotStar_CycleColor(25);
  server.handleClient();
}

void handle_Main() {
  char buffer[4096];

  snprintf(buffer, 4096, "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
    ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".button2 {background-color: #555555;}</style></head>"
    
    "<body><h1>CTM Status</h1>"
    "<form method='POST' action='/conf'>"
        "<input type='hidden' name='ssid_config' value='1'/>"
        "<label>SSID</label><input type='text' name='ssid' value='%s'/><br/>"
        "<label>PASS</label><input type='text' name='pass' value='%s'/><br/>"
        "<input type='submit' value='Save'/>"
        "</form>"
    "</body></html>", conf.ssid, conf.pass);
  server.send(200, "text/html", buffer);
}

void handle_Conf() {
  if (server.hasArg("ssid_config")) {
    // configuring the wifi
    if (server.hasArg("ssid") && server.hasArg("pass") && server.arg("ssid") != NULL && server.arg("pass") != NULL) {
      // save updated settings and restart the wifi
      memcpy(conf.ssid, server.arg("ssid").c_str(), 32);
      memcpy(conf.pass, server.arg("pass").c_str(), 32);
      conf.wifi_configured = true;
      conf.save();
      Serial.println("Saved user settings... rebooting");
      delay(2000);
      ESP.restart();
    } else {
      // display a configuration error
    }
  }

  server.sendHeader("Location","/");
  server.send(303);
}

void handle_Controls() {
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
