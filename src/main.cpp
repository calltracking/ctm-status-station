// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

/**
 * configure and listen for account or team events
 */
#define CTM_PRODUCTION

//#define LIGHT_TEST
#ifndef HAS_DISPLAY
#ifndef WROOM_PINS
#include <TinyPICO.h>
#endif
#endif

#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <DNSServer.h>
#include <cstring>
#include <esp_system.h>
#include <esp_timer.h>
#include <ArduinoJson.h> // see: https://arduinojson.org/v6/api/jsonobject/containskey/
#include <ArduinoWebsockets.h>
#include <Adafruit_NeoPixel.h>

#include <WiFiClientSecure.h>

#ifdef HAS_DISPLAY
  #include <Adafruit_GFX.h>
  #include <Adafruit_ThinkInk.h>

  #ifdef ESP32
    #define SRAM_CS     32
    #define EPD_CS      15
    #define EPD_DC      33  
  #endif
#endif

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "settings.h"
#include "url_utils.h"
#include "light_logic.h"

#ifdef HAS_DISPLAY

#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)

// Uncomment the following line if you are using 2.13" Monochrome EPD with SSD1680
//ThinkInk_213_Mono_BN display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
ThinkInk_213_Mono_B72 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
//ThinkInk_213_Mono_B73 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
#define COLOR1 EPD_BLACK
#define COLOR2 EPD_RED

#endif

#ifdef TINYPICO_PINS
#define RESET_BUTTON 0           // TinyPICO BOOT button
#define RESET_BUTTON_ACTIVE LOW  // active-low (pulled up)
#else
#define RESET_BUTTON 27
#define RESET_BUTTON_ACTIVE HIGH
#endif
#define STATUS_LIGHT_OUT 25
#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)

#ifdef CTM_PRODUCTION
#if !defined(APP_HOST) || (EXPAND(APP_HOST) == 1)
#undef APP_HOST
#define APP_HOST "app.calltrackingmetrics.com"
#endif
#if !defined(API_HOST) || (EXPAND(API_HOST) == 1)
#undef API_HOST
#define API_HOST "api.calltrackingmetrics.com"
#endif
#if !defined(SOC_HOST) || (EXPAND(SOC_HOST) == 1)
#undef SOC_HOST
#define SOC_HOST "app.calltrackingmetrics.com"
#endif
#if !defined(CLIENTID) || (EXPAND(CLIENTID) == 1)
#undef CLIENTID
#define CLIENTID "udaRmKY2W_85tFPM6f92R9aG8i-VwPjfQT1Q8RI8qIg"
#endif

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
//#endif

#else
/* *.ngrok.io root cert */
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
//#endif

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define API_HOST STRINGIZE_VALUE_OF(CTM_API_HOST)
#define APP_HOST STRINGIZE_VALUE_OF(CTM_APP_HOST)
#define SOC_HOST STRINGIZE_VALUE_OF(CTM_SOC_HOST)
#define CLIENTID STRINGIZE_VALUE_OF(CTM_CLIENTID)


#endif



const char *default_ssid = "ctmlight";
const char *default_pass = "ctmstatus";
static char html_buffer[9892]; // we use about 5486 without custom statues
static char html_error[64];
bool socketClosed = false;
bool linkPending = false;
bool linkError = false;
bool linkTimerPending = false; // waiting for token device code
uint64_t resetPressStart = 0;
bool resetWasPressed = false;
const uint64_t RESET_HOLD_MS = 5000; // hold BOOT for 5s to factory reset
// allow multi-reset fallback (works even if BOOT button wiring resets the board)
RTC_DATA_ATTR uint64_t lastBootUs;      // retained across resets if RTC power stays up
RTC_DATA_ATTR uint8_t resetCycleCount;
RTC_DATA_ATTR uint32_t resetMagic;
uint32_t lastLinkTimerCheck = 0;
uint32_t lastPing = 0;
uint32_t lastStatusCheck = 0;
int LocateLED = -1;
int locateCycles = 0;
String captoken;
bool hasAuthGranted = false;
Ringer ringers[RINGERS+1]; // set of led's to blink for ringing

Settings conf;
websockets::WebsocketsClient socket;
IPAddress DeviceIP;
unsigned long WIFIReConnectInteval = 30000;
unsigned long previousWifiMillis = 0;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;


bool IsLocalAP = true;
bool apActive = false; // true while softAP is running
bool staConnectInProgress = false;
bool staConnectFailed = false;
uint64_t staConnectStart = 0;
uint64_t apGraceStart = 0;
uint64_t AP_GRACE_MS = 12000; // keep AP alive after STA connect so UI can redirect
Adafruit_NeoPixel *pixels;
bool hasSocketConnected = false;

void setup_https(WiFiClientSecure *client, HTTPClient *http, const String &host, const String &path) {
  client->setCACert(root_ca);
  Serial.println("setup secure wifi client");
  http->setConnectTimeout(10000);// timeout in ms
  http->setTimeout(10000); // 10 seconds
  Serial.println("timers set");
  String url = String("https://") + host + path;
  Serial.println("begin client");
  http->begin(*client, url);
  Serial.println("https client ready");
}

void handle_Style();
void handle_Script();
void handle_Main();
void handle_Conf();
void handle_LinkSetup();
void handle_LinkStatus();
void handle_Link();
void handle_Unlink();
void handle_Linked();
void handle_FactoryReset();
void handle_AgentLookup();
void handle_CaptivePortal();
void handle_IpStatus();
void handle_SaveAgents();
void handle_SaveColors();
void handle_LocateLED();
void handle_FlipRedGreen();
void handleNotFound();
void checkTokenStatus();
void socketEvent(websockets::WebsocketsEvent event, String data);
void socketMessage(websockets::WebsocketsMessage message);
void updateAgentStatusLed(int index, const String status);
void refreshAccessToken();
bool refreshCapToken(int attempts=0);
void startWebsocket();
void dnsPreload(const char *name);

char from_hex(char ch);
char to_hex(char code);
String url_encode(String str);
bool red_green_flipped = false;

void blinkGreen();
void blinkOrange();
void blinkBlue();

void lightTestCycle();
void tickLoopOnce(uint32_t now);

void refreshAllAgentStatus();
void fetchCustomStatus(); 

#ifndef CTM_UNIT_TEST
// fetch current status information for the given agent for the led at index
void fetchLedAgentStatus(int index) {
  if (!conf.leds[index]) { return; }
  int agentId = conf.leds[index];
  Serial.printf("fetching status for agent: %d\n", agentId);

  WiFiClientSecure client;
  HTTPClient http;
  String path = String("/api/v1/accounts/") + conf.account_id + "/agents/" + agentId;
  setup_https(&client, &http, API_HOST, path);

  http.addHeader("Authorization", String("Bearer ") + conf.access_token);
  int r = http.GET();
  String body = http.getString();
  http.end();
  Serial.println(body);
  if (r < 0) {
    Serial.println("error issuing device request");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(path);
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("name")) {
    Serial.printf("update name for agent: %s\n", (const char*)obj["name"]);
    snprintf(conf.agentNames[index], 32, "%s", (const char*)obj["name"]);
    conf.save();
  }

  if (obj.containsKey("access") && obj["access"] == "denied") { // {"access":"denied"}
    Serial.printf("access denied\n");
    //updateAgentStatusLed(index, "offline");
    setError(index);
    pixels->show();
  } else if (obj.containsKey("status") && obj["status"]) {
    Serial.printf("got status: %s for led %d\n", (const char*)obj["status"], index);
    updateAgentStatusLed(index, obj["status"]);
  } else {
    Serial.printf("no status mark offline\n");
    updateAgentStatusLed(index, "offline");
  }
}
#else
void fetchLedAgentStatus(int) {}
#endif

void testdrawtext(const char *text, uint16_t color, int line=0) {
#ifdef HAS_DISPLAY
  display.setCursor(20, (line+1)*20);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
#endif
}
#ifdef HAS_DISPLAY
#else
#endif

#ifndef CTM_UNIT_TEST
// GCOVR_EXCL_START
void setup() {
  Serial.begin(115200);

  delay(1000);
#ifdef HAS_DISPLAY
  display.begin();
  display.clearBuffer();
  testdrawtext("CallTrackingMetrics... Loading", COLOR1, 1);
  display.display();
#endif

  conf.begin();
  // keep UI/link status in sync across reboots while waiting for user auth
  linkPending = conf.ctm_user_pending;
  // wifi setup required
  if (conf.ssid && conf.pass && strnlen(conf.ssid, 32) > 0) {
    if (strnlen(conf.ssid, 33) == 33) {
      Serial.printf("ssid and pass not configured or eeprom is corrupted and must be reset\n");
      conf.reset();
      conf.save();
    } else {
      Serial.printf("ssid: %s, pass: %s\n", conf.ssid, conf.pass);
    }
  }
#ifdef HAS_BUTTON
  // TinyPICO BOOT button is active-low; use pull-up. Dev kits keep existing wiring.
#ifdef TINYPICO_PINS
  pinMode(RESET_BUTTON, INPUT_PULLUP);
#else
  pinMode(RESET_BUTTON, INPUT);
#endif
  Serial.println("button setup");
#endif

#ifdef HAS_BUTTON
  // allow factory reset if BOOT/RESET button is held during power-up
  if (digitalRead(RESET_BUTTON) == RESET_BUTTON_ACTIVE) {
    Serial.println("BOOT held on startup -> clearing WiFi/CTM settings");
    setRedAll();
    conf.reset();
    conf.save();
    delay(500);
    ESP.restart();
  }

  // fallback: triple quick reset within 8 seconds to clear settings (works even if BOOT triggers a reset)
  uint64_t nowUs = esp_timer_get_time();
  if (resetMagic != 0xCAFEBABE) {
    resetMagic = 0xCAFEBABE;
    resetCycleCount = 0;
    lastBootUs = 0;
  }
  Serial.printf("Reset reason: %d\n", esp_reset_reason());
  Serial.printf("RTC resetCycleCount=%u lastBootUs=%llu now=%llu\n", resetCycleCount, (unsigned long long)lastBootUs, (unsigned long long)nowUs);
  if (lastBootUs == 0 || (nowUs - lastBootUs) > 8000000ULL) { // >8s since last boot
    resetCycleCount = 1;
  } else {
    resetCycleCount++;
  }
  lastBootUs = nowUs;
  if (resetCycleCount >= 3) {
    Serial.println("Triple reset detected -> clearing WiFi/CTM settings");
    setRedAll();
    conf.reset();
    conf.save();
    delay(300);
    ESP.restart();
  }
#endif

  pixels = new Adafruit_NeoPixel(LED_COUNT, STATUS_LIGHT_OUT, NEO_GRB + NEO_KHZ800);
  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  setBlueAll();
  for (int i = 0; i < RINGERS; ++i) {
    ringers[i].on = false;
    ringers[i].high = false;
    ringers[i].lastRing = 0;
  }
#ifdef LIGHT_TEST
  delay(1000);
  return;
#endif

  if (conf.good() && conf.wifi_configured && strlen(conf.ssid) > 0) {
    bool wifiConnected = false;
    for (int attempt = 1; attempt <= 2; ++attempt) {
      Serial.printf("Connecting to network (attempt %d)...\n", attempt);
      WiFi.begin(conf.ssid, conf.pass);
      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        wifiConnected = true;
        setGreenAll();
        break;
      }

#ifdef HAS_DISPLAY
      display.clearBuffer();
      testdrawtext("WiFi Connection Failed", COLOR1, 0);
      display.display();
#endif
      Serial.println("Connection Failed!");
      setRedAll();
      delay(1200);
      setOffAll();
      delay(400);
      WiFi.disconnect(true);
      delay(300);
    }

    if (wifiConnected) {
      // this has a dramatic effect on packet RTT
      WiFi.setSleep(WIFI_PS_NONE);
      DeviceIP = WiFi.localIP();
      IsLocalAP = false;
      Serial.print("Connected! IP address: ");
      Serial.println(DeviceIP);
#ifdef HAS_DISPLAY
      display.clearBuffer();
      testdrawtext("Network Connected", COLOR1, 0);
      testdrawtext((String("Configure at IP: ") + DeviceIP.toString()).c_str(), COLOR1, 3);
      display.display();
#endif
    } else {
      Serial.println("Failed to connect after retries. Enabling setup AP.");
      conf.wifi_configured = false; // show config form again, keep entered SSID/pass for convenience
      conf.save();
      WiFi.mode(WIFI_AP);
      WiFi.softAPmacAddress();
      WiFi.softAP(default_ssid, default_pass, 12, false, 8);
      DeviceIP = WiFi.softAPIP();
      IsLocalAP = true;
      apActive = true;
      Serial.print("AP IP address: ");
      Serial.println(DeviceIP);
    }
  } else {
    conf.wifi_configured = false;
    conf.ctm_user_pending = false;
    conf.save();

    Serial.print("Setting AP...");
    Serial.println(default_ssid);
    Serial.println(default_pass);
    WiFi.softAPmacAddress();
    WiFi.softAP(default_ssid, default_pass, 12, false, 8);
    DeviceIP = WiFi.softAPIP();

    IsLocalAP = true;
    apActive = true;
    Serial.print("IP address: ");
    Serial.println(DeviceIP);

    Serial.printf("Stations connected to soft-AP = %d\n", WiFi.softAPgetStationNum());

  }
 
  if (!IsLocalAP) {
    red_green_flipped = conf.red_green_flipped;
    if (red_green_flipped) {
      Serial.println("red green flipped: to true");
    } else {
      Serial.println("red green flipped: to false");
    }
  }

  server.on("/", HTTP_GET, handle_Main);
  server.on("/script.js", HTTP_GET, handle_Script);
  server.on("/style.css", HTTP_GET, handle_Style);
  server.on("/conf", HTTP_POST, handle_Conf);
  server.on("/link_setup", HTTP_GET, handle_LinkSetup);
  server.on("/link", HTTP_POST, handle_Link);
  server.on("/link_status", HTTP_GET, handle_LinkStatus);
  server.on("/unlink", HTTP_POST, handle_Unlink);
  server.on("/linked", HTTP_POST, handle_Linked);
  server.on("/factory_reset", HTTP_POST, handle_FactoryReset);
  server.on("/agents", HTTP_GET, handle_AgentLookup);
  server.on("/save_agents", HTTP_POST, handle_SaveAgents);
  server.on("/save_colors", HTTP_POST, handle_SaveColors);
  server.on("/locate", HTTP_POST, handle_LocateLED);
  server.on("/flip_red_green", HTTP_POST, handle_FlipRedGreen);
  server.on("/ip_status", HTTP_GET, handle_IpStatus);
  server.on("/generate_204", HTTP_GET, handle_CaptivePortal); // Android/Chrome captive portal trigger
  server.on("/gen_204", HTTP_GET, handle_CaptivePortal);
  server.on("/hotspot-detect.html", HTTP_GET, handle_CaptivePortal); // Apple captive portal trigger
  server.on("/ncsi.txt", HTTP_GET, handle_CaptivePortal); // Windows captive portal trigger
  server.on("/connecttest.txt", HTTP_GET, handle_CaptivePortal);
  server.onNotFound(handleNotFound);
  Serial.println("start the web server");

  server.begin();
  delay(1000);
  Serial.printf("server started with %d LED's\n", LED_COUNT);

  if (apActive) {
    // wildcard DNS to force all hosts to our captive portal page
    dnsServer.start(DNS_PORT, "*", DeviceIP);
  }

  Serial.println("setup complete");
    
  if (IsLocalAP) {
    Serial.println("Connect to AP:");
    Serial.println(default_ssid);
    Serial.println(default_pass);
#ifdef HAS_DISPLAY
    display.clearBuffer();
    testdrawtext("Finish Setup", COLOR1, 0);
    testdrawtext((String("Connect to WiFi: ") + default_ssid).c_str(), COLOR1, 1);
    testdrawtext((String("WiFi Password: ") + default_pass).c_str(), COLOR1, 2);
    testdrawtext((String("Configure at IP: ") + DeviceIP.toString()).c_str(), COLOR1, 3);
    display.display();
#endif
    return;
  }

  if (conf.ctm_user_pending && conf.wifi_configured) {
    linkPending = true;
    blinkOrange();
    Serial.println("pending user configuration to link device");
    conf.ctm_configured = false;
    conf.save();
    linkTimerPending = true;
#ifdef HAS_DISPLAY
    display.clearBuffer();
    testdrawtext("Finish Setup", COLOR1, 0);
    testdrawtext((String("Connect to WiFi: ") + default_ssid).c_str(), COLOR1, 1);
    testdrawtext((String("WiFi Password: ") + default_pass).c_str(), COLOR1, 2);
    testdrawtext((String("Configure at IP: ") + DeviceIP.toString()).c_str(), COLOR1, 3);
    testdrawtext("Pending user link to https://app.calltrackingmetrics.com/", COLOR1, 4);
    display.display();
#endif
  } else if (conf.ctm_configured && conf.wifi_configured) {
    // fetch available statues
    fetchCustomStatus();
    startWebsocket();
#ifdef HAS_DISPLAY
    display.clearBuffer();
    testdrawtext("CallTrackingMetrics Status Station", COLOR1, 0);
    testdrawtext((String("Configure at IP: ") + DeviceIP.toString()).c_str(), COLOR1, 3);
    display.display();
#endif
  } else {
    blinkBlue();
    Serial.println("device is reset");
  }
  lastPing = millis();
}
#endif // CTM_UNIT_TEST

void dnsPreload(const char *name) {
  IPAddress ipaddr;
  int ret;
  ret = WiFi.hostByName (name, ipaddr);
  Serial.printf("hostbyname: %s ret= %d %s \n", name, ret, ipaddr.toString().c_str() );
}

#ifndef CTM_UNIT_TEST
void startWebsocket() {
  Serial.println("startWebsocket");
  bool didRefresh = refreshCapToken();
  if (didRefresh && captoken.length() > 0) {
    Serial.println("connecting socket");

    //tp.DotStar_SetPixelColor(0, 255, 0);
    dnsPreload(SOC_HOST);

    socket.setCACert(root_ca);
    socket.onEvent(socketEvent);
    socket.onMessage(socketMessage);

    if (!socket.connectSecure(SOC_HOST, 443, "/socket.io/?EIO=4&transport=websocket")) {
      Serial.println("Error connecting");
      hasSocketConnected = false;
    } else {
      Serial.println("connected to socket server");
      hasSocketConnected = true;
      lastPing = millis();
    }

    setOffAll();
    refreshAllAgentStatus();

    socket.send(String("40"));
  } else {
    Serial.printf("unable to init captoken '%s' is invalid!\n", captoken.c_str());
    hasSocketConnected = false;
    socketClosed = true;
    //tp.DotStar_SetPixelColor(100, 255, 100);
  }
}
#else
void startWebsocket() {}
#endif

void lightTestCycle() {
  pixels->clear();
  delay(500); // Pause before next pass through loop
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    Serial.printf("set:%d\n", i);
    delay(500); // Pause before next pass through loop
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
    pixels->show();   // Send the updated pixel colors to the hardware.
    delay(500); // Pause before next pass through loop

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    if (i == 0) {
      Serial.println("set red");
      pixels->setPixelColor(i, pixels->Color(0, 150, 0));
    } else if (i == 1) {
      Serial.println("set green");
      pixels->setPixelColor(i, pixels->Color(150, 0, 0));
    } else if (i == 2) {
      Serial.println("set blue");
      pixels->setPixelColor(i, pixels->Color(0, 0, 150));
    } else if (i == 3) {
      Serial.println("set purple");
      pixels->setPixelColor(i, pixels->Color(0, 150, 150));
    } else if (i == 4) {
      Serial.println("set orange");
      pixels->setPixelColor(i, pixels->Color(150, 150, 0));
    } else if (i == 5) {
      Serial.println("set orange");
      pixels->setPixelColor(i, pixels->Color(150, 150, 0));
    } else if (i == 6) {
      Serial.println("set orange");
      pixels->setPixelColor(i, pixels->Color(150, 150, 0));
    } else if (i == 7) {
      Serial.println("set orange");
      pixels->setPixelColor(i, pixels->Color(150, 150, 0));
    } else if (i == 8) {
      Serial.println("set orange");
      pixels->setPixelColor(i, pixels->Color(150, 150, 0));
    }

    pixels->show();   // Send the updated pixel colors to the hardware.

    delay(500); // Pause before next pass through loop
  }
}
void blinkGreen() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(150, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}
void blinkOrange() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for(int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(150, 150, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}
void blinkBlue() {
  pixels->clear();
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 0));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
  for (int i=0; i< LED_COUNT; ++i) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color(0, 0, 150));
  }
  pixels->show();   // Send the updated pixel colors to the hardware.
  delay(500); // Pause before next pass through loop
}

void tickLoopOnce(uint32_t now) {
  uint32_t deltaSeconds;
#ifdef LIGHT_TEST
  lightTestCycle();
  return;
#endif

  bool staConnected = (WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != (uint32_t)0);

  if (staConnectInProgress) {
    if (staConnected) {
      staConnectInProgress = false;
      staConnectFailed = false;
      WiFi.setSleep(WIFI_PS_NONE);
      DeviceIP = WiFi.localIP();
      IsLocalAP = false;
      apGraceStart = now;
      Serial.print("STA connected during config. IP: ");
      Serial.println(DeviceIP);
      red_green_flipped = conf.red_green_flipped;
      setGreenAll();
    } else if ((now - staConnectStart) > 20000) {
      staConnectInProgress = false;
      staConnectFailed = true;
      setRedAll();
      Serial.println("STA connect timeout; staying in AP mode for reconfig");
      conf.wifi_configured = false;
      conf.save();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_AP);
      apActive = true;
      IsLocalAP = true;
      DeviceIP = WiFi.softAPIP();
      dnsServer.start(DNS_PORT, "*", DeviceIP);
    }
  }

  if (apActive && !IsLocalAP && apGraceStart > 0 && (now - apGraceStart) > AP_GRACE_MS) {
    Serial.println("Grace period over, stopping AP");
    WiFi.softAPdisconnect(true);
    dnsServer.stop();
    apActive = false;
  }
  if (conf.ctm_configured && conf.wifi_configured && conf.account_id > 0) {
    if (socketClosed || !hasSocketConnected) { 
      delay(1000);
      Serial.println("lost connection - reconnect?");
      // try to reconnect
      startWebsocket();
    } else {
      socket.poll();
      deltaSeconds = (now - lastPing) / 1000;
      if (deltaSeconds > 20) {
        if (hasAuthGranted) {
          // a maybe more effective ping?
          socket.send(String("42[\"calls.active\",{\"account\":") + conf.account_id + "}]");
        } else {
          Serial.print("ping socket: ");
          Serial.print(deltaSeconds);
          Serial.println("seconds past");
        }
        //socket.send(String("40"));
        socket.ping(); // ping every 25 seconds
        lastPing = now;
      }

      // handle ringers
      for (int i = 0; i < RINGERS; ++i) {
        if (ringers[i].on) {
          uint32_t delta = (uint32_t)(now - ringers[i].lastRing);
          //Serial.printf("ringer on for: %d, time past: %d seconds\n", i, delta);
          if (delta > 500) {
            if (ringers[i].high) {
              ringers[i].high = false;
              //Serial.printf("ringer on go low for: %d, time past: %d milliseconds\n", i, delta);
              setOff(i);
              pixels->show();
            } else {
              ringers[i].high = true;
              setOrange(i);
              pixels->show();
              //Serial.printf("ringer on go high for: %d, time past: %d milliseconds\n", i, delta);
            }
            ringers[i].lastRing = now;
          }
        }
      }
			if (lastStatusCheck == 0) {
				refreshAllAgentStatus();
			} else {
				deltaSeconds = (now - lastStatusCheck) / 1000;
					
				if (deltaSeconds > 60) {
				  refreshAllAgentStatus();
				}
			}
    }
  }

  if (apActive) {
    dnsServer.processNextRequest();
  }

  server.handleClient();

  // press and hold
#ifdef HAS_BUTTON
  bool pressed = digitalRead(RESET_BUTTON) == RESET_BUTTON_ACTIVE;
  if (pressed) {
    if (!resetWasPressed) {
      resetWasPressed = true;
      resetPressStart = millis();
      Serial.println("Reset button pressed, hold 5s to clear WiFi/CTM");
    }
    uint64_t held = millis() - resetPressStart;
    if (held > RESET_HOLD_MS) {
      Serial.println("Reset hold detected -> clearing settings and rebooting to setup AP");
      setRedAll();
      conf.reset();
      conf.save();
      WiFi.disconnect(true);
      delay(500);
      ESP.restart();
    } else if (held > 2000 && (held / 500) % 2 == 0) {
      // brief visual heartbeat while holding (non-destructive)
      setErrorAll();
      pixels->show();
    }
  } else if (resetWasPressed) {
    uint64_t held = millis() - resetPressStart;
    Serial.printf("Reset button released after %llu ms\n", (unsigned long long)held);
    resetWasPressed = false;
    resetPressStart = 0;
  }
#endif

  if (!IsLocalAP) {
    if (conf.ctm_user_pending) {
			Serial.println("user pending");
      //tp.DotStar_CycleColor(25);
      uint32_t linkCheckStatusDeltaSeconds = (uint32_t)(now - lastLinkTimerCheck) / 1000;
      if (linkTimerPending && linkCheckStatusDeltaSeconds > 5) {
        blinkGreen();
        lastLinkTimerCheck = now;
        // execution polling status connection check
        checkTokenStatus();
      } else {
        blinkOrange();
      }
    }
    if (!conf.ctm_user_pending && !conf.ctm_configured) {
      Serial.printf("not linked configure at: %s\n", DeviceIP.toString().c_str());
      blinkBlue();
    }

    if (!IsLocalAP  && WiFi.status() != WL_CONNECTED && (now - previousWifiMillis) > WIFIReConnectInteval) {
      // wifi connection lost
      Serial.println("Reconnecting to WiFi...");
      setErrorAll();
      WiFi.disconnect();
      WiFi.reconnect();
      previousWifiMillis = now;
    }

    locateTick();
  }
}
// GCOVR_EXCL_STOP
void handle_Style() {
  Serial.print("GET /style.css");
  snprintf(html_buffer, sizeof(html_buffer),
    "{ font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
      ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
      "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
      ".button2 {background-color: #555555;} "
      ".logo-trigger_wrapper { display: flex;justify-content: space-between;align-items: center;height: 70px;padding-left: 15px; } "
      ".logo-trigger_wrapper img { width: 232px; }"
      "details summary { display: flex; justify-content: align-items: center; }"
      "details summary::before { content: \"+\"; margin-right: 0.5rem; }"
      "details[open] summary::before {  content: \"-\"; }"
      "details {  display:block; border: 1px solid #ccc; padding: 0.5rem; }");
  Serial.println("  200 OK");
  server.send(200, "text/css", html_buffer);
}

void handle_Script() {
  Serial.print("GET /script.js");
  snprintf(html_buffer, sizeof(html_buffer), "$('.led-agent').select2({ "
"  allowClear: true, minimumInputLength: 3, placeholder: \"Enter agent's name\","
"  templateResult: (r) => { return r.text + ' ' + r.description; },"
"  ajax: { delay: 250, url: '/agents', dataType: 'json' } "
"});\n"
"function hexToRgb(hex) { const result = /^#?([a-f\\d]{2})([a-f\\d]{2})([a-f\\d]{2})$/i.exec(hex); \n"
" return result ? { r: parseInt(result[1], 16), g: parseInt(result[2], 16), b: parseInt(result[3], 16) } : null; \n"
"}\n"
"function componentToHex(c) { const hex = parseInt(c).toString(16); return hex.length == 1 ? '0' + hex : hex; }\n"
"function rgbToHex(r, g, b) { return '#' + componentToHex(r) + componentToHex(g) + componentToHex(b); }\n"
"$('input[type=color]').each(function() { const rgb = $(this).closest('p').find('[type=hidden]').map(function() { return this.value; }); $(this).val(rgbToHex(rgb[0], rgb[1], rgb[2])) })\n"
"$('input[type=color]').change(function() { const v = $(this).val(); const rgb = hexToRgb(v); const a = ['r','g','b']; for (i=0;i<3;++i) { $(this).closest('p').find(`[type=hidden].${a[i]}`).val(rgb[a[i]]); } })\n"
"$('.locate').click( function(e) { e.preventDefault(); let i = this.innerHTML.replace(/LED /,''); console.log('locate led: ', i); $.post('/locate?led=' + i); });\n"

      );
  Serial.println("  200 OK");
  server.send(200, "text/javascript", html_buffer);
}

void handle_Main() {
  Serial.print("GET /");
  if (!conf.wifi_configured) {
    snprintf(html_buffer, sizeof(html_buffer), "<!doctype html><html>"
      "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<link rel=\"icon\" href=\"data:,\">"
      "<style>"
        "html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
        ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
        "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
      "</style>"
      "<title>CTM Status Station</title></head>"
      "<body>"
        "<header>"
          "<div class='logo-trigger_wrapper'>Configure the Wifi</div>"
        "</header>"
        "<form method='POST' action='/conf'>"
          "<p>%s</p>"
          "<input type='hidden' name='ssid_config' value='1'/>"
          "<label>SSID</label><input type='text' name='ssid' value='%s'/><br/>"
          "<label>PASS</label><input type='text' name='pass' value='%s'/><br/>"
          "<input type='submit' value='Save'/>"
        "</form>"
        "</body>"
        "</html>", html_error, conf.ssid, conf.pass);
    Serial.println("  200 OK");
    server.send(200, "text/html", html_buffer);
    return;
  }
  char led_opts[LED_COUNT+1][256];
  int led_opt_size = 0;

  for (int i = 0; i < LED_COUNT; ++i) {
    //Serial.printf("led[%d]: %d -> %s\n", i, conf.leds[i], conf.agentNames[i]);
    if (conf.leds[i] > 0 && conf.agentNames[i] && strlen(conf.agentNames[i]) > 0) {
      snprintf(led_opts[i], sizeof(led_opts[i]), "<option  selected='selected' value='%d'>%s</option>", conf.leds[i], conf.agentNames[i]);
    } else {
      memset(led_opts[i], 0, sizeof(led_opts[i]));
    }
    led_opt_size += strlen(led_opts[i]) + 1;
  }

  //Serial.printf("led_opt_size: %d\n", led_opt_size);

  const char *markup_for_led_selector =  "<p>"
                  "<a href='#' class='locate'>LED %d</a> <select style='width:300px' class='led-agent' type='text' name='led%d'>%s</select>"
                "</p>";

  int single_led_size = (strlen(markup_for_led_selector) + 32); // plus 32 for agent name 
  int markup_led_size = ((LED_COUNT+1) * single_led_size) + led_opt_size;
  char *led_input_buffer = (char*)calloc(markup_led_size, 1);
  int offset = 0;
  // append the led markup into led_input_buffer
  for (int i = 0; i < LED_COUNT; ++i) {// <option  selected='selected' value='%d'>%s</option>
    char *offsetpointer = led_input_buffer+offset;
    snprintf(led_input_buffer+offset, (single_led_size + strlen(led_opts[i])), markup_for_led_selector,
             (i+1), i, led_opts[i]);
    offset += strlen(offsetpointer);
  }
  const char *markup_for_status_selector =  "<p>"
                  "<label>%s</label> <input style='width:300px' class='status' type='color' name='%s'>"
                  "<input type='hidden' class='r' name='%d[red]' value=%d>"
                  "<input type='hidden' class='g' name='%d[green]' value=%d>"
                  "<input type='hidden' class='b' name='%d[blue]' value=%d>"
                "</p>";
  int single_status_size = (strlen(markup_for_status_selector) + 96); // 96 for status name 2x and color value
  int markup_status_size = (MAX_CUSTOM_STATUS * single_status_size);
  char *status_input_buffer = (char*)calloc(markup_status_size, 1); // ensure empty string when no statuses
  offset = 0;
  for (int i = 0; i < MAX_CUSTOM_STATUS; ++i) {
    char *offsetpointer = status_input_buffer+offset;
    if (conf.custom_status_index[i] && strlen(conf.custom_status_index[i]) > 0) { // avoid writing if there are no custom statues
      Serial.printf("custom status: %d -> %s\n", i, conf.custom_status_index[i]);
      snprintf(status_input_buffer+offset, single_status_size, markup_for_status_selector,
               conf.custom_status_index[i], conf.custom_status_index[i],
               i, conf.custom_status_color[i][0], i, conf.custom_status_color[i][1], i, conf.custom_status_color[i][2]);
      offset += strlen(offsetpointer);
    }
  }

  const char *fmt_string = "<!doctype html><html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css' rel='stylesheet' >"
    "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/select2/4.0.13/css/select2.min.css'/>"
    "<link rel='stylesheet' href='/style.css'/>"
    "<script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.6.0/jquery.min.js'></script>"
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js'></script>"
    "<script src='https://cdnjs.cloudflare.com/ajax/libs/select2/4.0.13/js/select2.min.js' ></script>"
    "<title>CTM Status Station</title></head>"
    "<body>"
      "<header>"
        "<div class='logo-trigger_wrapper'><img src='https://www.calltrackingmetrics.com/wp-content/themes/ctm-theme/img/ctm_logo.svg'/></div>"
        "<a href='/link_setup'>Connect Device</a>"
        "<form method='POST' action='/factory_reset' style='display:inline;margin-left:10px;'>"
          "<input type='submit' value='Factory Reset' onclick='return confirm(\"Clear WiFi and CTM link?\")'/>"
        "</form>"
      "</header>"
      "<div id='settings' %s>"
        "<details>"
          "<summary><h3>Wifi Settings</h3></summary>"
          "<div>"
            "<form method='POST' action='/conf'>"
              "<p>%s</p>"
              "<input type='hidden' name='ssid_config' value='1'/>"
              "<label>SSID</label><input type='text' name='ssid' value='%s'/><br/>"
              "<label>PASS</label><input type='password' name='pass' value='%s'/><br/>"
              "<input type='submit' value='Save'/>"
            "</form>"
          "</div>"
        "</details>"
        "<details>"
          "<summary><h3>Light Settings</h3></summary>"
          "<div>"
            "<p>Each Status Station has %d LED's connected. "
              "Assign an agent to each light. (click LED to locate)</p>"
            "<form method='POST' action='/save_agents'>"
            "%s"
              "<input type='submit' value='Save'/>"
            "</form>"
          "</div>"
        "</details>"
        "<details>\n"
          "<summary><h3>Status Settings</h3></summary>"
          "<div>"
            "<form method='POST' action='/flip_red_green'>"
              "<input type='submit' value='Flip Red and Green'/>"
            "</form>"
            "<p>Assign color to each status, available is green, on a call is red, wrapup is purple.</p>"
            "<form method='POST' action='/save_colors'>"
              "%s"
              "<input type='submit' value='Save'/>"
            "</form>"
          "</div>"
        "</details>"
      "</div>"
      "<script src='/script.js'></script>"
    "</body></html>";

  Serial.printf("write the html buffer: %d\n", sizeof(html_buffer));
  snprintf(html_buffer, sizeof(html_buffer), fmt_string, (conf.ctm_configured ? "" : "style='display:none'"),
                      html_error, conf.ssid, conf.pass,
                      LED_COUNT, led_input_buffer, status_input_buffer);

  Serial.printf("buffer written\n");
  free(led_input_buffer);
  free(status_input_buffer);
  Serial.printf("output size %d\n", strlen(html_buffer));

  Serial.println("  200 OK");
  server.send(200, "text/html", html_buffer);
}

void handle_Conf() {

  if (server.hasArg("ssid_config")) {
    // configuring the wifi
    if (server.hasArg("ssid") && server.hasArg("pass") && server.arg("ssid") != NULL && server.arg("pass") != NULL) {
      // save updated settings and start STA while keeping AP alive briefly
      memset(conf.ssid, 0, sizeof(conf.ssid));
      memset(conf.pass, 0, sizeof(conf.pass));
      strncpy(conf.ssid, server.arg("ssid").c_str(), sizeof(conf.ssid) - 1);
      strncpy(conf.pass, server.arg("pass").c_str(), sizeof(conf.pass) - 1);
      conf.wifi_configured = true;
      conf.save();
      Serial.println("Saved user settings... connecting STA while keeping AP up");

      staConnectFailed = false;
      WiFi.mode(WIFI_AP_STA);
      apActive = true; // keep captive portal alive during transition
      WiFi.begin(conf.ssid, conf.pass);
      staConnectInProgress = true;
      staConnectStart = millis();

      snprintf(html_buffer, sizeof(html_buffer),
        "<!doctype html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Connecting...</title></head>"
        "<body style='font-family:Helvetica;text-align:center'>"
        "<h2>Connecting to %s</h2>"
        "<p>Stay on this page. We'll redirect once the station is online.</p>"
        "<p id='status'>Attempting connection...</p>"
        "<script>\n"
        "function poll(){fetch('/ip_status').then(r=>r.json()).then(d=>{\n"
        " if(d.connected){document.getElementById('status').innerText='Connected! Redirecting...';\n"
        "   setTimeout(()=>{window.location='http://'+d.ip+'/';},2000);\n"
        " } else if(d.failed){document.getElementById('status').innerText='Connection failed. Stay on the ctmlight network to reconfigure.';}\n"
        " else {document.getElementById('status').innerText='Connecting...'; setTimeout(poll,1000);}\n"
        "}).catch(()=>setTimeout(poll,1500));}\n"
        "setTimeout(poll,800);\n"
        "</script></body></html>", conf.ssid);
      server.send(200, "text/html", html_buffer);
      return;
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
  bool pending = (conf.ctm_user_pending || linkPending) && !linkError;
  bool success = conf.ctm_configured && !pending && !linkError;
  const char *status = linkError ? "error" : (success ? "success" : (pending ? "pending" : "pending"));
  snprintf(html_buffer, 4096, "{\"status\":\"%s\"}", status);
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
        "</form>%s"
    "<p id='link-status'>%s</p>"
    "<form method='POST' action='/factory_reset' style='margin-top:10px;'>"
      "<input type='submit' value='Factory Reset (clear WiFi & link)' onclick='return confirm(\"Reset WiFi and CTM link?\")'/>"
    "</form>"
    "%s"
    "</body></html>", (conf.ctm_configured ? "" : "<input type='submit' value='Connect Device'/>"),
    conf.account_id, (conf.ctm_configured ? "<form method='POST' action='/unlink'><input type='submit' value='Unlink Device'/></form>" : ""),
    (conf.ctm_user_pending ? "Waiting for authorization..." : (conf.ctm_configured ? "Device linked." : "")),
    ((conf.ctm_user_pending || conf.ctm_configured) ?
      "<script>function poll(){fetch('/link_status',{cache:'no-store'}).then(r=>r.json()).then(d=>{"
      "if(d.status==='success'){window.location='/';}"
      "else if(d.status==='error'){document.getElementById('link-status').innerHTML='Authorization failed, retrying...';setTimeout(poll,4000);}"
      "else{document.getElementById('link-status').innerHTML='Waiting for authorization...';setTimeout(poll,2000);}"
      "}).catch(()=>setTimeout(poll,3000));}"
      "setTimeout(poll,1500);</script>"
    : ""));
  server.send(200, "text/html", html_buffer);
}

void handle_Link() {
  Serial.println("link request");
  if (conf.ctm_configured && !conf.ctm_user_pending) {
    Serial.println("link request ignored: already linked. Unlink first.");
    snprintf(html_buffer, sizeof(html_buffer),
      "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Already linked</title></head>"
      "<body style='font-family:Helvetica;text-align:center'>"
      "<h3>Device is already linked.</h3>"
      "<form method='POST' action='/unlink'><input type='submit' value='Unlink Device'/></form>"
      "</body></html>");
    server.send(409, "text/html", html_buffer);
    return;
  }
  linkPending = true;
  linkError   = false;
  WiFiClientSecure client;
  HTTPClient http;
  const char *path = "/oauth2/device_token";
  setup_https(&client, &http, API_HOST, path);

  // secure requests read: https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/
  Serial.printf("request device token at:%s\n", path);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int r =  http.POST("client_id=" CLIENTID);
  String body = http.getString();
  http.end();
  Serial.println(body);
  Serial.println(body.length());
  if (r < 0) {
    Serial.println("error issuing device request");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(path);
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
    "fetch('/link_status',{cache:'no-store'}).then(response => response.json()).then((data) => {"
      "console.log(data);"
      "if (data.status == 'pending') { document.getElementById('status').innerHTML = 'Waiting for authorization...'; setTimeout(checkStatus, 2000); } "
      "else if (data.status == 'error') { "
        "document.getElementById('status').innerHTML = 'Authorization failed, retrying...'; setTimeout(checkStatus, 4000); } "
      "else { window.location='/'; "
      "}"
    "}).catch(() => { setTimeout(checkStatus, 3000); });"
    "} setTimeout(checkStatus, 1500);</script>"
    "</body>"
    "</html>", (char*)((const char*)obj["user_code"]), (char*)((const char*)obj["verification_uri"]));
  server.send(200, "text/html", html_buffer);

  memset(conf.device_code, 0, sizeof(conf.device_code));
  strncpy(conf.device_code, (const char*)obj["device_code"], sizeof(conf.device_code) - 1);
  conf.ctm_user_pending = true;
  conf.save();
  linkTimerPending = true;
}
// helpful: https://savjee.be/2020/01/multitasking-esp32-arduino-freertos/
void checkTokenStatus() {
  const char *path = "/oauth2/token";
  Serial.println("checking token status");
  // send request to check device code
  //
  // possibly schedule the task again or free the device_code and give up
  WiFiClientSecure client;
  HTTPClient http;
  setup_https(&client, &http, API_HOST, path);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

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
  JsonDocument doc;
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
      conf.ctm_user_pending = false;
      conf.save();
    } else if (obj.containsKey("access_token") && obj.containsKey("account_id") && obj.containsKey("user_id")) {
      Serial.println("link success!");
      linkError = false;
      conf.account_id = (int)obj["account_id"];
      size_t atlen = strlen((const char *)obj["access_token"]);
      if (atlen >= sizeof(conf.access_token)) {
        Serial.println("error access token overflow!");
        atlen = sizeof(conf.access_token) - 1;
      }
      size_t rtlen = strlen((const char *)obj["refresh_token"]);
      if (rtlen >= sizeof(conf.refresh_token)) {
        Serial.println("error refresh token overflow!");
        rtlen = sizeof(conf.refresh_token) - 1;
      }
      memset(conf.access_token, 0, sizeof(conf.access_token));
      memset(conf.refresh_token, 0, sizeof(conf.refresh_token));
      strncpy(conf.access_token, (const char *)obj["access_token"], atlen);
      strncpy(conf.refresh_token, (const char *)obj["refresh_token"], rtlen);
      conf.ctm_user_pending = false;
      conf.ctm_configured = true;
      conf.user_id = (int)obj["user_id"];
      conf.expires_in = (int)obj["expires_in"];
      Serial.printf("access_token: %s\n", conf.access_token);
      conf.save();
      Serial.println("Access token saved - doing one more reboot");
      //tp.DotStar_Clear();
      //tp.DotStar_SetPixelColor(0, 255, 0);
      delay(2000);
      Serial.println("Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }
}
void loop() {
  tickLoopOnce(millis());
}
void handle_Unlink() {
  conf.ctm_configured = false;
  conf.ctm_user_pending = false;
  conf.expires_in = 0;
  conf.account_id = 0;
  conf.team_id = 0;
  linkPending = false;
  linkError = false;

  memset(conf.device_code, 0, sizeof(conf.device_code));
  memset(conf.access_token, 0, sizeof(conf.access_token));
  memset(conf.refresh_token, 0, sizeof(conf.refresh_token));
  
  conf.save();
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_FactoryReset() {
  Serial.println("factory reset requested via UI");
  setRedAll();
  conf.reset();
  conf.save();
  WiFi.disconnect(true);
  snprintf(html_buffer, sizeof(html_buffer),
    "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Resetting</title></head>"
    "<body style='font-family:Helvetica;text-align:center'>"
    "<h2>Resetting...</h2><p>Clearing WiFi and CTM link, rebooting to setup AP.</p>"
    "</body></html>");
  server.send(200, "text/html", html_buffer);
  delay(500);
  ESP.restart();
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

void handle_SaveColors() {
  // reset
  for (int i = 0; i < MAX_CUSTOM_STATUS; ++i) {
    String redKey = i + String("[red]");
    if (server.hasArg(redKey)) {
      conf.custom_status_color[i][0] = server.arg(redKey).toInt();
    } else {
      conf.custom_status_color[i][0] = 0;
    }

    String greenKey = i + String("[green]");
    if (server.hasArg(greenKey)) {
      conf.custom_status_color[i][1] = server.arg(greenKey).toInt();
    } else {
      conf.custom_status_color[i][1] = 0;
    }

    String blueKey = i + String("[blue]");
    if (server.hasArg(blueKey)) {
      conf.custom_status_color[i][2] = server.arg(blueKey).toInt();
    } else {
      conf.custom_status_color[i][2] = 0;
    }
  }
  conf.save();
  refreshAllAgentStatus();
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_FlipRedGreen() {
  Serial.println("flip red green\n");
  if (red_green_flipped) {
    red_green_flipped = conf.red_green_flipped = false;
  } else {
    red_green_flipped = conf.red_green_flipped = true;
  }
  if (red_green_flipped) {
    Serial.println("now flipped: to true");
  } else {
    Serial.println("now flipped: to false");
  }
  conf.save();
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_LocateLED() {
  LocateLED = server.arg("led").toInt() - 1;
  locateCycles = 0; 
  server.send(200, "application/json", "{}");
}

void handle_SaveAgents() {
  // erase name data we'll get it again while fetching status
  for (int i = 0; i < LED_COUNT; ++i) {
    memset(conf.agentNames[i], 0, 32);
  }
  for (int i = 0; i < LED_COUNT; ++i) {
    String ledkey = String("led") + i;
    conf.leds[i] = 0; // zero the led out
    if (server.hasArg(ledkey)) {
      conf.leds[i] = server.arg(ledkey).toInt();
    }
  }
  conf.save();
  refreshAllAgentStatus();
  server.sendHeader("Location","/");
  server.send(303);
}

#ifndef CTM_UNIT_TEST
void handle_AgentLookup() {
  String path = String("/api/v1/");
  if (server.hasArg("q")) {
    path += "lookup.json?descriptions=1&global=0&idstr=0&object_type=user&search=" + url_encode(server.arg("q"));
  } else if (server.hasArg("term")) {
    path += "lookupids.json?descriptions=1&global=0&idstr=0&object_type=user&ids[]=" + server.arg("term");
  } else {
    path += "lookup.json?descriptions=1&global=0&object_type=user&idstr=0";
  }
  Serial.printf("lookup: %s\n", path.c_str());
  WiFiClientSecure client;
  HTTPClient http;
  setup_https(&client, &http, API_HOST, path);

  http.addHeader("Authorization", String("Bearer ") + conf.access_token);

  int r = http.GET();
  String body = http.getString();
  http.end();

  if (r < 0) {
    Serial.println("error issuing device request");
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(path);
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("authentication")) {
    refreshAccessToken();
    server.send(403, "application/json", body);// "{ \"results\": [], \"pagination\": { \"more\": false } }"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  } else {
    server.send(200, "application/json", body);// "{ \"results\": [], \"pagination\": { \"more\": false } }"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  }
}
#else
void handle_AgentLookup() { server.send(200, "application/json", "{}"); }
#endif

// Send a redirect back to the local setup page for captive portal probes
void handle_CaptivePortal() {
  if (IsLocalAP && !conf.wifi_configured) {
    server.sendHeader("Location", String("http://") + DeviceIP.toString());
    server.send(302, "text/plain", "Redirecting to setup...");
    return;
  }
  server.send(204, "text/plain", "");
}

// Report current network state so the captive portal page can redirect to the STA IP
void handle_IpStatus() {
  IPAddress staIp = WiFi.localIP();
  IPAddress apIp = WiFi.softAPIP();
  bool connected = WiFi.status() == WL_CONNECTED && staIp != (uint32_t)0;
  snprintf(html_buffer, sizeof(html_buffer),
           "{\"connected\":%s,\"ip\":\"%s\",\"ap\":\"%s\",\"failed\":%s}",
           connected ? "true" : "false",
           staIp.toString().c_str(),
           apIp.toString().c_str(),
           staConnectFailed ? "true" : "false");
  server.send(200, "application/json", html_buffer);
}

void handleNotFound() {
  if (IsLocalAP && !conf.wifi_configured) {
    // force captive portal browsers back to root
    server.sendHeader("Location", String("http://") + DeviceIP.toString());
    server.send(302, "text/plain", "Redirecting to setup...");
    return;
  }
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
#ifndef CTM_UNIT_TEST
void socketMessage(websockets::WebsocketsMessage message) {
  //Serial.print("WS(msg): ");
  String data = message.data();
  const char *data_str = data.c_str();
  //Serial.println(data);

  if (data == "42[\"access.handshake\"]") {
    JsonDocument reply; // small doc for handshake reply
    reply["id"] = conf.user_id;
    reply["account"] = conf.account_id;
    reply["captoken"] = captoken;
    reply["id"] = conf.user_id;
    reply["account"] = conf.account_id;
    reply["captoken"] = captoken;

    String payload;
    serializeJson(reply, payload);
    String frame = String("42[\"access.account\", ") + payload + "]";
    Serial.printf("reply to access.handshake: %s\n", frame.c_str());
    socket.send(frame);
    socket.ping();
    lastPing = millis();
    // 42["auth.granted",{"account":18614}]
  } else if (data_str[0] == '4' && data_str[1] == '2' && data_str[2] == '[') {
    data.remove(0,2);
    JsonDocument msgDoc;
    DeserializationError error = deserializeJson(msgDoc, data);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonArray msg = msgDoc.as<JsonArray>();
    String action = msg[0];
    //JsonObject fields = msg[1].as<JsonObject>();
    //Serial.println("received:" + action);
    if (action == "auth.granted") {
      hasAuthGranted = true;
    } else if (action == "message" && hasAuthGranted) {
      //Serial.printf("message: '%s'\n", data_str);
      //Serial.println(msg[1].as<String>());
      JsonDocument payloadDoc;
      DeserializationError error = deserializeJson(payloadDoc, msg[1].as<String>());
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      JsonObject fields = payloadDoc.as<JsonObject>();
      /*
       * : {"action":"status","what":"agent","data":{"id":1,"sid":"USREB077D06AC239BB5","status":"offline","logged_out":1,"queue_total":0}}
       */
      if (fields.containsKey("action") && fields.containsKey("what") && fields.containsKey("data") && conf.ledsConfigured()) {
        String action = fields["action"];
        String what   = fields["what"];
        // {"time":1636468005.6600325,"action":"status","what":"chat","data":{"id":423084,"time":1636468005.6600325}} and then boom?
        //Serial.printf("action: '%s', what: '%s'", action.c_str(), what.c_str());
        // 42["message","{\"time\":1636226258.348542,\"action\":\"status\",\"what\":\"online\",\"data\":{\"id\":19223,\"time\":1636226258.348542}}"]
        // ["message","{\"time\":1636224415.947346,\"action\":\"agent\",\"what\":\"ringing\",\"data\":{
        // 42["message","{\"time\":1636224452.3807418,\"action\":\"status\",\"what\":\"inbound\",\"data\":{\"id\":19223,\"time\":1636224452.3807418}}"
        if ((action == "status" || (action == "agent" && what == "ringing")) && fields["data"].containsKey("id")) {
          int agent_id = fields["data"]["id"];
          String status = (what == "agent" && fields["data"].containsKey("status")) ? fields["data"]["status"].as<String>() : "";
          int ledIndex = conf.getAgentLed(agent_id);
          if (ledIndex > -1 && ledIndex < LED_COUNT) {
            Serial.printf("status[%s] for %d, with led: %d\n", status.c_str(), agent_id, ledIndex);
            // {"action":"status","what":"agent","data":{"id":19223,"sid":"USR043E46722529BCB5F2411A7E1C8C06E1","status":"ringing","from_status":"online","logged_out":0,"queue_total":4}}
            // {"time":1636234298.814153,"action":"status","what":"online","data":{"id":19223,"time":1636234298.814153}}
            if (what == "agent" && status == "ringing") {
              updateAgentStatusLed(ledIndex, status);
            } else {
              ringers[ledIndex].on = false;
              // pixels->setPixelColor(index, pixels->Color(0, 150, 0));
              // "action\":\"agent\",\"what\":\"inbound\"
/*
 * message: '["message","{\"time\":1636903613.7469814,\"action\":\"status\",\"what\":\"inbound\",\"data\":{\"id\":1,\"time\":1636903613.7469814}}"]'
status[] for 1, with led: 0
*/
/*
 * video example
 * message: '["message","{\"action\":\"status\",\"what\":\"agent\",\"data\":{\"id\":1,\"sid\":\"USREB077D06AC239BB5\",\"status\":\"video_member\",\"logged_out\":1,\"queue_total\":1}}"]'
 */
              if ((action == "agent" || action == "status") && (what == "inbound" || what == "outbound" || what == "chat")) {
                status = what;
              } else if (action == "status" && what == "online") {
                status = "online";
              } else if (action == "status") {
                status = what; // custom statuses
              }
/*
 *  '["message","{\"time\":1636908806.6114316,\"action\":\"status\",\"what\":\"Lunch_and_Coffee_Break\",\"data\":{\"id\":1,\"time\":1636908806.6114316}}"]'
 */
              updateAgentStatusLed(ledIndex, status);
            }
          }
        }
      }
    }
  } else {
   // Serial.printf("header packets? '%s'\n", data_str);
  }
}
#else
void socketMessage(websockets::WebsocketsMessage) {}
#endif
// see: https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp32/WebSocketClientSocketIOack/WebSocketClientSocketIOack.ino
#ifndef CTM_UNIT_TEST
void socketEvent(websockets::WebsocketsEvent event, String data) {
  if (event == websockets::WebsocketsEvent::ConnectionOpened) {
    socketClosed  = false;
    hasSocketConnected = true;
    Serial.println("WS(evt): Connnection Opened");
    pixels->clear();
  } else if (event == websockets::WebsocketsEvent::ConnectionClosed) {
    socketClosed  = true;
    hasSocketConnected = false;
    Serial.println("WS(evt): Connnection Closed");
    pixels->clear();
  } else if (event == websockets::WebsocketsEvent::GotPing) {
    Serial.println("WS(evt): Got a Ping!");
    socket.pong();
  } else if (event == websockets::WebsocketsEvent::GotPong) {
    Serial.println("WS(evt): Got a Pong!");
    //socket.ping();
  } else {
    Serial.println("WS(evt): unknown!");
  }
}
#else
void socketEvent(websockets::WebsocketsEvent, String) {}
#endif
bool refreshCapToken(int attempts) {
  captoken  = ""; // set to empty
  if (!conf.ctm_configured || !conf.access_token || !conf.account_id) {
    if (!conf.account_id) {
      conf.ctm_configured = false;
      conf.save();
    }
    Serial.printf("unable to refresh token missing access token, account id or not configured yet? '%s'\n", conf.refresh_token);
    if (conf.refresh_token && attempts < 2) {
      refreshAccessToken();
      return refreshCapToken(attempts+1);
    }
    return false;
  }
  Serial.println(String("fetch with access token:") + conf.access_token);
  WiFiClientSecure client;
  HTTPClient http;
  // get the captoken for websocket access
  // curl -i -H 'Authorization: Bearer token' -X POST 
  String captoken_path = String("/api/v1/accounts/") + conf.account_id + "/users/captoken";
  Serial.println("Requesting captoken: ");
  Serial.println(captoken_path);
  Serial.println(conf.access_token);
  setup_https(&client, &http, API_HOST, captoken_path);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", String("Bearer ") + conf.access_token);
  /*
   * curl -i -H 'Authorization: Bearer token' -X POST 'https://" API_HOST "/api/v1/accounts/{account_id}/users/captoken'
   */
//  http.addHeader("Content-Type", "application/json");

  Serial.println(String("Authorization: Bearer ") + conf.access_token);
  Serial.println(String("device_code=") + conf.device_code);
  int r =  http.POST(String("device_code=") + conf.device_code);
  String body = http.getString();
  http.end();

  //Serial.println(body);
  //Serial.println(body.length());
  if (r < 0) {
    Serial.println("error issuing device request");
    return false;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(captoken_path);
    return false;
  }

  JsonObject obj = doc.as<JsonObject>();
  if (doc.containsKey("error") && attempts < 1) {
    Serial.println("failed to get token try refreshing access token");
    refreshAccessToken();
    if (!linkError) {
      return refreshCapToken(attempts+1);
    }
  } else if (doc.containsKey("authentication") && attempts < 1) {
    Serial.println("failed to get token try refreshing access token");
    refreshAccessToken();
    if (!linkError) {
      return refreshCapToken(attempts+1);
    }
  }
  captoken = (const char*)obj["token"]; // capture the token at start up
  if (captoken.length()) {
    Serial.println("captoken:");
    Serial.println(captoken);
    return true;
  } else {
    Serial.println("no captoken not authenticated!");
    return false;
  }
}
void refreshAccessToken() {
  if (!conf.ctm_configured || !conf.refresh_token) {
    Serial.println("unable to refresh without a refresh token!");
    return;
  }
  const char *path = "/oauth2/token";
  Serial.println("refresh access token");
  WiFiClientSecure client;
  HTTPClient http;
  setup_https(&client, &http, API_HOST, path);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

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
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(path);
    linkError = true;
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  Serial.printf("refresh_token: %s, user_id: %d, account_id: %d\n", conf.refresh_token, conf.user_id, conf.account_id);
  if (obj.containsKey("error")) {
    Serial.println("refresh error");
    linkError = true;
  } else if (obj.containsKey("access_token") && obj.containsKey("account_id")) {
    Serial.println("refresh success!");
    linkError = false;
    conf.account_id = (int)obj["account_id"];
    memset(conf.access_token, 0, sizeof(conf.access_token));
    memset(conf.refresh_token, 0, sizeof(conf.refresh_token));
    strncpy(conf.access_token, (const char *)obj["access_token"], sizeof(conf.access_token) - 1);
    strncpy(conf.refresh_token, (const char *)obj["refresh_token"], sizeof(conf.refresh_token) - 1);
    conf.ctm_user_pending = false;
    conf.ctm_configured = true;
    conf.user_id = (int)obj["user_id"];
    conf.expires_in = (int)obj["expires_in"];
    conf.save();
    //tp.DotStar_Clear();
    //tp.DotStar_SetPixelColor(0, 255, 0);
  } else {
    Serial.println("refresh failed; clearing tokens and waiting for re-link");
    setRedAll();
    memset(conf.access_token, 0, sizeof(conf.access_token));
    memset(conf.refresh_token, 0, sizeof(conf.refresh_token));
    memset(conf.device_code, 0, sizeof(conf.device_code));
    conf.ctm_configured = false;
    conf.ctm_user_pending = false;
    conf.save();
    linkError = true;
  }
}
#ifndef CTM_UNIT_TEST
void refreshAllAgentStatus() {
	Serial.printf("refreshAllAgentStatus\n");
  lastStatusCheck = millis();
  String idList;
  for (int i = 0; i < LED_COUNT; ++i) {
    if (conf.leds[i] > 0) {
      if (idList.length()) {
        idList += String(",") + conf.leds[i];
      } else {
        idList = conf.leds[i];
      }
    }
  }

  if (idList.length() == 0) {
    Serial.println("no agents assigned; skip status fetch");
    return;
  }

  Serial.printf("fetching status for agents\n");
  WiFiClientSecure client;
  HTTPClient http;
  String url = String("/api/v1/accounts/") + conf.account_id + "/agents/group_status?ids=" + idList;
  setup_https(&client, &http, API_HOST, url);
  http.addHeader("Authorization", String("Bearer ") + conf.access_token);
  int r = http.GET();
  String body = http.getString();
  http.end();
  Serial.println(url);
  Serial.println(body);

  if (r < 0) {
    Serial.println("error issuing device request");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(url);
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  // can also be {"authentication":"failed token expired"}
  if (obj.containsKey("access") && obj["access"] == "denied") { // {"access":"denied"}
    Serial.printf("access denied\n");
    setErrorAll();
  } else if (obj.containsKey("users")) {
    JsonArray users = obj["users"].as<JsonArray>();
			if (users.size() > 0) {
				for (JsonVariant userVar : users) {
          JsonObject user = userVar.as<JsonObject>();
				int ledIndex = conf.getAgentLed((int)user["uid"]);
				if (ledIndex > -1 && ledIndex < LED_COUNT) {
					Serial.printf("update for led: %d for user id: %d, %s\n", ledIndex, (int)user["uid"], (const char*)user["name"]);
					if (user.containsKey("name")) {
						Serial.printf("update name for agent: %s\n", (const char*)user["name"]);
						snprintf(conf.agentNames[ledIndex], 32, "%s", (const char*)user["name"]);
					}

					if (user.containsKey("status") && user["status"]) {
						Serial.printf("got status: %s for led %d\n", (const char*)user["status"], ledIndex);
						updateAgentStatusLed(ledIndex, user["status"]);
					} else if (user.containsKey("videos") && user["videos"] > 0) {
						updateAgentStatusLed(ledIndex, "inbound"); // active video treat like on a video 
					} else {
						Serial.printf("no status mark offline\n");
						updateAgentStatusLed(ledIndex, "offline");
					}
				}
			}

		}
    // save any updates to names
    conf.save();
  } else if (obj.containsKey("authentication")) {
    refreshAccessToken();
    // referesh token on this run and on the next one we'll hopefuly get updated
  }

}
#endif

void fetchCustomStatus() {
  Serial.printf("fetching available statues for account: %d", conf.account_id);
  WiFiClientSecure client;
  HTTPClient http;
    // fetch /api/v1/accounts/{account_id}/available_statuses?normalized=1
  String url = String("/api/v1/accounts/") + conf.account_id + "/available_statuses?normalized=1";
  setup_https(&client, &http, API_HOST, url);

  http.addHeader("Authorization", String("Bearer ") + conf.access_token);
  Serial.println("issue https get");
  int r = http.GET();
  Serial.printf("get: %d\n", r);
  String body = http.getString();
  Serial.println(body);
  http.end();
  if (r < 0) {
    Serial.println("error issuing device request");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(url);
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  // zero out the custom_status_index buffer
  for (int i = 0; i < MAX_CUSTOM_STATUS; ++i) {
    memset(conf.custom_status_index[i], 0, 31);
  }
  if (obj.containsKey("statuses")) {
    JsonArray statuses = obj["statuses"].as<JsonArray>();
    int i = 0;
    for (JsonVariant statusVar : statuses) {
      JsonObject status = statusVar.as<JsonObject>();
      Serial.printf("status[%d]: %s, normalized to: %s\n", i, (const char*)status["name"], (const char*)status["id"]);
      snprintf(conf.custom_status_index[i++], 31, "%s", (const char*)status["id"]);
    }
    conf.save();
  }
}
