/**
 * configure and listen for account or team events
 */
//#define CTM_PRODUCTION
#include <TinyPICO.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h> // see: https://arduinojson.org/v6/api/jsonobject/containskey/
#include <WiFiClientSecure.h>
#include <ArduinoWebsockets.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "settings.h"

#define RESET_BUTTON 27
#define STATUS_LIGHT_OUT 25
#define STATUS_LIGHT_IN 26

#ifdef CTM_PRODUCTION
#define APP_HOST "app.calltrackingmetrics.com"
#define API_HOST "api.calltrackingmetrics.com"
#define SOC_HOST "app.calltrackingmetrics.com"
#define CLIENTID "udaRmKY2W_85tFPM6f92R9aG8i-VwPjfQT1Q8RI8qIg"

// amazon root ca for api.calltrackingmetrics.com secure connections
const char *root_ca="-----BEGIN CERTIFICATE-----\n"
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

#else
const char *root_ca="-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
"-----END CERTIFICATE-----";

#define APP_HOST "taf2.ngrok.io"
#define API_HOST "taf2.ngrok.io"
#define SOC_HOST "taf2.ngrok.io"
#define CLIENTID "aJOX4QK_QzADcxVG_ZVY2tCB1KgqffXpKuJUEQbcr48"

#endif



const char *default_ssid = "ctmlight";
const char *default_pass = "ctmstatus";
static char html_buffer[4096];
static char html_error[64];
bool socketClosed = false;
bool linkPending = false;
bool linkError = false;
bool linkTimerPending = false; // waiting for token device code
uint64_t lastLinkTimerCheck = 0;
uint64_t lastPing = 0;

String captoken;
bool hasAuthGranted = false;

TinyPICO tp = TinyPICO();
WebServer server(80);
Settings conf;
websockets::WebsocketsClient socket;

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
void socketEvent(websockets::WebsocketsEvent event, String data);
void socketMessage(websockets::WebsocketsMessage message);
void refreshAccessToken();
void refreshCapToken(int attempts=0);
void startWebsocket();
void dnsPreload(const char *name);

#define SET_RED pixels->clear(); pixels->setPixelColor(0, pixels->Color(0, 150, 0)); pixels->show();
#define SET_GREEN pixels->clear(); pixels->setPixelColor(0, pixels->Color(150, 0, 0)); pixels->show();
#define SET_BLUE pixels->clear(); pixels->setPixelColor(0, pixels->Color(0, 0, 150)); pixels->show();

void setup() {
  bool localAP = false;
  Serial.begin(115200);

  delay(3000);
  conf.begin();
  pinMode(RESET_BUTTON, INPUT_PULLDOWN);

  pixels = new Adafruit_NeoPixel(1, STATUS_LIGHT_OUT, NEO_GRB + NEO_KHZ800);
  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  SET_BLUE

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
    linkTimerPending = true;
  } else if (conf.ctm_configured) {
    refreshCapToken();
    startWebsocket();
  }
}
void dnsPreload(const char *name) {
  IPAddress ipaddr;
  int ret;
  ret = WiFi.hostByName (name, ipaddr);
  Serial.printf("hostbyname: %s ret= %d %s \n", name, ret, ipaddr.toString().c_str() );
}

void startWebsocket() {
  if (captoken.length() > 0) {

    tp.DotStar_SetPixelColor(0, 255, 0);
    dnsPreload(SOC_HOST);

    socket.setCACert(root_ca);
    socket.onEvent(socketEvent);
    socket.onMessage(socketMessage);

    socket.connectSecure(SOC_HOST, 443, "/socket.io/?EIO=3&transport=websocket");//, root_ca);

  } else {
    Serial.println("unable to init captoken is invalid!");
    tp.DotStar_SetPixelColor(100, 255, 100);
  }
}

void loop() {
  uint64_t now = millis();
  if (conf.ctm_configured) {
    if (socketClosed) { 
      delay(1000);
      Serial.println("lost connection - reconnect?");
    } else {
      socket.poll();
      uint64_t deltaSeconds = (now - lastPing) / 1000;
      if (deltaSeconds > 25) {
        if (hasAuthGranted) {
          // a maybe more effective ping?
          socket.send(String("42[\"calls.active\",{\"account\":") + conf.account_id + "}]");
        } else {
          Serial.print("ping socket: ");
          Serial.print(deltaSeconds);
          Serial.println("seconds past");
        }
        socket.ping(); // ping every 25 seconds
        lastPing = now;
      }
    }
  } else {
    server.handleClient();
  }

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
    uint64_t linkCheckStatusDeltaSeconds = (now - lastLinkTimerCheck) / 1000;
    if (linkTimerPending && linkCheckStatusDeltaSeconds > 5) {
      lastLinkTimerCheck = now;
      // execution polling status connection check
      checkTokenStatus();
    }
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
  Serial.printf("use root_ca: %s\n", root_ca);
  client.setCACert(root_ca);

  // secure requests read: https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/
  const char *url = "https://" API_HOST "/oauth2/device_token";
  Serial.printf("request device token at:%s\n", url);
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
//  http.addHeader("Content-Type", "application/json");

  int r =  http.POST("client_id=" CLIENTID);
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
  linkTimerPending = true;
}
// helpful: https://savjee.be/2020/01/multitasking-esp32-arduino-freertos/
void checkTokenStatus() {
  const char *url = "https://" API_HOST "/oauth2/token";
  Serial.println("checking token status");
  // send request to check device code
  //
  // possibly schedule the task again or free the device_code and give up
  WiFiClientSecure client;
  client.setCACert(root_ca);
  HTTPClient http;
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded ");

  int r =  http.POST(String("client_id=" CLIENTID "&device_code=") + conf.device_code + "&grant_type=device_code");
  String body = http.getString();
  http.end();
  Serial.println(body);
  Serial.println(body.length());
  if (r < 0) {
    Serial.println("error fetching link status");
    linkPending = false;
    linkError   = true;
    linkTimerPending = false;
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    linkTimerPending = false;
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("error") && (obj["error"] == "authorization_pending" || obj["error"] == "slow_down")) {
    linkPending = true;
    linkError = false;
    linkTimerPending = true;
  } else {
    Serial.println("disable timer");
    linkTimerPending = false;
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
      conf.user_id = (int)obj["user_id"];
      conf.expires_in = (int)obj["expires_in"];
      conf.save();
      tp.DotStar_Clear();
      tp.DotStar_SetPixelColor(0, 255, 0);
      startWebsocket();
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
/*
 *
-> 0{"sid":"wq...","upgrades":[],"pingInterval":25000,"pingTimeout":5000}	85  (12:18:53.065)
-> 40	2  (12:18:53.066)
-> 42["access.handshake"]	22  (12:18:53.066)
<- 42["access.account",{"id":19223,"account":18614,"captoken":".."}]	472  (12:19:04.138)
-> 42["auth.granted",{"account":18614}]	36  12:19:04.143
<- 42["calls.active",{"account":18614}]	36  12:19:04.183
-> 42["message",{"action":"active","what":"call","data":"0"}]	58  (12:19:04.185)
<- 42["calls.active",{"account":18614}]	36  (12:19:08.189)
-> 42["message",{"action":"active","what":"call","data":"0"}]
 */
void socketMessage(websockets::WebsocketsMessage message) {
  Serial.print("WS(msg): ");
  String data = message.data();
  const char *data_str = data.c_str();
  Serial.println(data);

  if (data == "42[\"access.handshake\"]") {
    StaticJsonDocument<1024> doc;
    doc["id"] = conf.user_id;
    doc["account"] = conf.account_id;
    doc["captoken"] = captoken;
    char json_string[1024];
    serializeJson(doc, json_string);

    snprintf(html_buffer, sizeof(html_buffer), "42[\"access.account\", %s]", json_string);
    Serial.printf("reply to access.handshake: %s\n", html_buffer);
    socket.send(html_buffer);
    socket.ping();
    lastPing = millis();
    // 42["auth.granted",{"account":18614}]
  } else if (data_str[0] == '4' && data_str[1] == '2' && data_str[2] == '[') {
    StaticJsonDocument<1024> doc;
    data.remove(0,2);
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonArray msg = doc.as<JsonArray>();
    String action = msg[0];
    JsonObject fields = msg[1].as<JsonObject>();
    Serial.println("received:" + action);
    if (action == "auth.granted") {
      hasAuthGranted = true;
    } else if (action == "message") {
      if (fields["action"] == "active" && fields["what"] == "call" ) {
        int total = atoi((const char*)fields["data"]);
        Serial.printf("total active %d\n", total);
        if (total > 0) {
          Serial.println("set red");
          SET_RED
        } else {
          Serial.println("set green");
          SET_GREEN
        }
      }
    }
  } else {
    Serial.printf("header packets? '%s'\n", data_str);
  }
}
// see: https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp32/WebSocketClientSocketIOack/WebSocketClientSocketIOack.ino
void socketEvent(websockets::WebsocketsEvent event, String data) {
  if (event == websockets::WebsocketsEvent::ConnectionOpened) {
    socketClosed  = false;
    Serial.println("WS(evt): Connnection Opened");
  } else if (event == websockets::WebsocketsEvent::ConnectionClosed) {
    socketClosed  = true;
    Serial.println("WS(evt): Connnection Closed");
  } else if (event == websockets::WebsocketsEvent::GotPing) {
    Serial.println("WS(evt): Got a Ping!");
    socket.pong();
  } else if (event == websockets::WebsocketsEvent::GotPong) {
    Serial.println("WS(evt): Got a Pong!");
  }
}
void refreshCapToken(int attempts) {
  captoken  = ""; // set to empty
  if (!conf.ctm_configured || !conf.access_token || !conf.account_id) {
    return;
  }
  Serial.println(String("fetch with access token:") + conf.access_token);
  WiFiClientSecure client;
  client.setCACert(root_ca);
  HTTPClient http;
  // get the captoken for websocket access
  // curl -i -H 'Authorization: Bearer token' -X POST 
  String captoken_url = String("https://" API_HOST "/api/v1/accounts/") + conf.account_id + "/users/captoken";
  Serial.println(captoken_url);

  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, captoken_url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded ");
  http.addHeader("Authorization", String("Bearer ") + conf.access_token);
  /*
   * curl -i -H 'Authorization: Bearer token' -X POST 'https://" API_HOST "/api/v1/accounts/{account_id}/users/captoken'
   */
//  http.addHeader("Content-Type", "application/json");

  int r =  http.POST(String("device_code=") + conf.device_code);
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
  if (doc.containsKey("error") && attempts < 1) {
    Serial.println("failed to get token try refreshing access token");
    refreshAccessToken();
    if (!linkError) {
      return refreshCapToken(attempts+1);
    }
  }
  captoken = (const char*)obj["token"]; // capture the token at start up
  Serial.println("captoken:");
  Serial.println(captoken);
}
void refreshAccessToken() {
  if (!conf.ctm_configured || !conf.access_token || !conf.refresh_token) {
    return;
  }
  const char *url = "https://" API_HOST "/oauth2/token";
  Serial.println("refresh access token");
  WiFiClientSecure client;
  client.setCACert(root_ca);
  HTTPClient http;
  http.setConnectTimeout(10000);// timeout in ms
  http.setTimeout(10000); // 10 seconds
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded ");

  int r = http.POST(String("client_id=" CLIENTID "&device_code=") + conf.device_code + "&grant_type=refresh_token&refresh_token=" + conf.refresh_token);
  String body = http.getString();
  http.end();
  Serial.println(body);
  Serial.println(body.length());
  if (r < 0) {
    Serial.println("error fetching link status");
    linkError   = true;
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    linkError = true;
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("error")) {
    Serial.println("refresh error");
    linkError = true;
  } else {
    Serial.println("refresh success!");
    linkError = false;
    conf.account_id = (int)obj["account_id"];
    memcpy(conf.access_token, (const char *)obj["access_token"], strlen((const char *)obj["access_token"]));
    memcpy(conf.refresh_token, (const char *)obj["refresh_token"], strlen((const char *)obj["refresh_token"]));
    conf.ctm_user_pending = false;
    conf.ctm_configured = true;
    conf.user_id = (int)obj["user_id"];
    conf.expires_in = (int)obj["expires_in"];
    conf.save();
    tp.DotStar_Clear();
    tp.DotStar_SetPixelColor(0, 255, 0);
  }
}
