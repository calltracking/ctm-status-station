/**
 * configure and listen for account or team events
 */
#include <TinyPICO.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h> // see: https://arduinojson.org/v6/api/jsonobject/containskey/
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "settings.h"


#define RESET_BUTTON 27
#define STATUS_LIGHT_OUT 25
#define STATUS_LIGHT_IN 26

// amazon root ca for api.calltrackingmetrics.com secure connections
const char *ca_cert="-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----";


const char *default_ssid = "ctmlight";
const char *default_pass = "ctmstatus";
static char html_buffer[4096];
static char html_error[64];
volatile  bool linkPending = false;
volatile  bool linkError = false;
volatile  bool timerActive = false;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

TinyPICO tp = TinyPICO();
WebServer server(80);
Settings conf;
WebSocketsClient webSocket;
hw_timer_t * timer = NULL;
Adafruit_NeoPixel *pixels;

void handle_Main();
void handle_Conf();
void handle_LinkSetup();
void handle_LinkStatus();
void handle_Link();
void handle_Unlink();
void handle_Linked();
void handleNotFound();
void checkTokenStatus();

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  timerActive = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  bool localAP = false;
  Serial.begin(115200);

  delay(3000);
  conf.begin();
  pinMode(RESET_BUTTON, INPUT_PULLDOWN);
  pinMode(STATUS_LIGHT_OUT, OUTPUT);
  pinMode(STATUS_LIGHT_IN, INPUT);

  pixels = new Adafruit_NeoPixel(1, STATUS_LIGHT_IN, NEO_GRB + NEO_KHZ800);
  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  pixels->setPixelColor(0, pixels->Color(150, 150, 0));
  pixels->show();   // Send the updated pixel colors to the hardware.


  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 5000000, false);
  timerAlarmDisable(timer);

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
  server.on("/link_setup", HTTP_GET, handle_LinkSetup);
  server.on("/link", HTTP_POST, handle_Link);
  server.on("/link_status", HTTP_GET, handle_LinkStatus);
  server.on("/unlink", HTTP_POST, handle_Unlink);
  server.on("/linked", HTTP_POST, handle_Linked);
  server.onNotFound(handleNotFound);

  server.begin();
  delay(1000);
  Serial.println("setup complete");
  if (localAP) {
    Serial.println("Connect to AP:");
    Serial.println(default_ssid);
    Serial.println(default_pass);
  }

  if (conf.ctm_user_pending) {
    timerAlarmEnable(timer);
  } else if (conf.ctm_configured) {
    tp.DotStar_SetPixelColor(0, 255, 0);
  }
}

void loop() {
  server.handleClient();
  pixels->setPixelColor(0, pixels->Color(150, 150, 0));
  pixels->show();   // Send the updated pixel colors to the hardware.

  if (digitalRead(RESET_BUTTON) == HIGH) {
    Serial.println("reset pressed");
    conf.ctm_configured = false;
    conf.wifi_configured = false;
    conf.save();
    delay(2000);
    ESP.restart();
    return;
  }

  if (conf.ctm_user_pending) {
    tp.DotStar_CycleColor(25);
    if (timerActive) {
      Serial.println("timer triggered");
      portENTER_CRITICAL(&timerMux);
      timerActive = false;
      portEXIT_CRITICAL(&timerMux);
      // execution polling status connection check
      checkTokenStatus();
    }
  } else if (conf.ctm_configured) {
    delay(10);
  }
}

void handle_Main() {
  snprintf(html_buffer, sizeof(html_buffer), "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
    ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".button2 {background-color: #555555;}</style></head>"
    
    "<body><h1>Configure your Wifi</h1>"
    "<form method='POST' action='/conf'>"
        "<p>%s</p>"
        "<input type='hidden' name='ssid_config' value='1'/>"
        "<label>SSID</label><input type='text' name='ssid' value='%s'/><br/>"
        "<label>PASS</label><input type='text' name='pass' value='%s'/><br/>"
        "<input type='submit' value='Save'/>"
        "</form>"
        "<a href='/link_setup'>Connect Device</a>"
    "</body></html>", html_error, conf.ssid, conf.pass);
  server.send(200, "text/html", html_buffer);
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
      snprintf(html_error, sizeof(html_error), "Missing ssid or pass");
      handle_Main();
      return;
    }
  }

  server.sendHeader("Location","/");
  server.send(303);
}

void handle_LinkStatus() {
  Serial.println("status request");
  snprintf(html_buffer, 4096, "{\"status\":\"%s\"}", linkPending ? "pending" : (linkError ? "error" : "success"));
  server.send(200, "application/json", html_buffer);
}

// present UI to link to CTM
void handle_LinkSetup() {
  Serial.println("link setup request");
  snprintf(html_buffer, 4096, "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
    ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".button2 {background-color: #555555;}</style></head>"
    
    "<body><h1>Link to CallTrackingMetrics</h1>"
    "<form method='POST' action='/link'>"
        "%s"
        "<label>Account ID <input type='hidden' name='account_id' value='%d'/></label>"
        "<label>Team ID <input type='hidden' name='team_id' value='%d'/></label>"
        "</form>%s"
    "</body></html>", (conf.ctm_configured ? "" : "<input type='submit' value='Connect Device'/>"),
    conf.account_id, conf.team_id, (conf.ctm_configured ? "<form method='POST' action='/unlink'><input type='submit' value='Unlink Device'/></form>" : ""));
  server.send(200, "text/html", html_buffer);
}

void handle_Link() {
  Serial.println("link request");
  linkPending = true;
  linkError   = false;
  WiFiClientSecure client;
  HTTPClient http;
  client.setCACert(ca_cert);

  // secure requests read: https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/
  const char *url = "https://api.calltrackingmetrics.com/oauth2/device_token";
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
//  http.addHeader("Content-Type", "application/json");

  int r =  http.POST("client_id=udaRmKY2W_85tFPM6f92R9aG8i-VwPjfQT1Q8RI8qIg");
  String body = http.getString();
  http.end();
  Serial.println(body);
  Serial.println(body.length());
  if (r < 0) {
    Serial.println("error issuing device request");
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  snprintf(html_buffer, 4096, "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
    ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".button2 {background-color: #555555;}</style></head>"
    
    "<body><h1>Link to CallTrackingMetrics</h1>"
    "<h3>Your Code: %s</h3>"
    "<a target='_blank' href='%s'>Authorize device</a>"
    "<div id='status'></div>"
    "<script>function checkStatus() {"
    "fetch('/link_status').then(response => response.json()).then( (data) => {"
      "console.log(data);"
      "if (data.status == 'pending') { setTimeout(checkStatus, 5000); } else if (data.status == 'error') { "
        "document.getElementById('status').innerHTML = 'Error Try Again'; } else { document.getElementById('status').innerHTML = 'Link Successful';"
      "}"
    "});"
    "} setTimeout(checkStatus, 5000);</script>"
    "</body>"
    "</html>", (char*)((const char*)obj["user_code"]), (char*)((const char*)obj["verification_uri"]));
  server.send(200, "text/html", html_buffer);

  memcpy(conf.device_code, (const char*)obj["device_code"], sizeof(conf.device_code)); 
  conf.ctm_user_pending = true;
  conf.save();
  timerAlarmDisable(timer);
  timerAlarmEnable(timer);
}
// helpful: https://savjee.be/2020/01/multitasking-esp32-arduino-freertos/
void checkTokenStatus() {
  const char *url = "https://api.calltrackingmetrics.com/oauth2/token";
  Serial.println("checking token status");
  // send request to check device code
  //
  // possibly schedule the task again or free the device_code and give up
  WiFiClientSecure client;
  client.setCACert(ca_cert);
  HTTPClient http;
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded ");

  int r =  http.POST(String("client_id=udaRmKY2W_85tFPM6f92R9aG8i-VwPjfQT1Q8RI8qIg&device_code=") + conf.device_code + "&grant_type=device_code");
  String body = http.getString();
  http.end();
  Serial.println(body);
  Serial.println(body.length());
  if (r < 0) {
    Serial.println("error fetching link status");
    linkPending = false;
    linkError   = true;
    timerAlarmDisable(timer);
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    timerAlarmDisable(timer);
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("error") && (obj["error"] == "authorization_pending" || obj["error"] == "slow_down")) {
    linkPending = true;
    linkError = false;
    timerAlarmDisable(timer);
    timerAlarmEnable(timer);
  } else {
    Serial.println("disable timer");
    timerAlarmDisable(timer);
    linkPending = false;
    if (obj.containsKey("error")) {
      Serial.println("link error");
      linkError = true;
    } else {
      Serial.println("link success!");
      linkError = false;
      conf.account_id = (int)obj["account_id"];
      memcpy(conf.access_token, (const char *)obj["access_token"], strlen((const char *)obj["access_token"]));
      memcpy(conf.refresh_token, (const char *)obj["refresh_token"], strlen((const char *)obj["refresh_token"]));
      conf.ctm_user_pending = false;
      conf.ctm_configured = true;
      conf.expires_in = (int)obj["expires_in"];
      conf.save();
      tp.DotStar_Clear();
      tp.DotStar_SetPixelColor(0, 255, 0);
    }
  }
}
void handle_Unlink() {
  conf.ctm_configured = false;
  conf.ctm_user_pending = false;
  conf.expires_in = 0;
  conf.account_id = 0;
  conf.team_id = 0;

  memset(conf.device_code, 0, sizeof(conf.device_code));
  memset(conf.access_token, 0, sizeof(conf.access_token));
  memset(conf.refresh_token, 0, sizeof(conf.refresh_token));
  
  conf.save();
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_Linked() {
  snprintf(html_buffer, 4096, "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
    ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".button2 {background-color: #555555;}</style></head>"
    
    "<body><h1>Success</h1><p>Status light should be following your agents</p>"
    "</body>"
    "</html>");
  server.send(200, "text/html", html_buffer);
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
