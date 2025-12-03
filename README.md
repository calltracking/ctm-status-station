# CTM Status Station

Small ESP32 light bar that shows agent call status from CallTrackingMetrics.

## What it does
- Broadcasts its own setup AP on first boot so you can enter Wi‑Fi.
- Lets you link the device to a CTM account via the built-in web UI.
- Listens for ringing/active calls over WebSocket and updates LEDs.

## Hardware
- Tested on TinyPICO / ESP32-S3 dev kits with NeoPixel strip (8 LEDs by default).
- One button: BOOT/RESET. On TinyPICO it hard-resets the board; use the web “Factory Reset” instead.

## Quick start (first-time setup)
1) Power the device. It starts a hotspot:
   - ssid: `ctmlight`
   - pass: `ctmstatus`
2) Connect to the hotspot. A captive portal should pop up; if it doesn’t, open `http://192.168.4.1/` manually.
3) Enter your Wi‑Fi SSID and password. The device reboots and joins your network (LEDs blink green once connected).
4) Find its LAN IP (router/DHCP list or `http://ctmlight.local` if mDNS works) and open `http://<device-ip>/`.
5) Click **Connect Device** to start linking. You’ll get a code and a link to `https://app.calltrackingmetrics.com/accesscode`.
6) Approve the device in CTM. The page polls automatically; once linked the device reboots and starts listening.

## Relinking / factory reset
- From the UI: click **Factory Reset** (on the main page or `/link_setup`) to clear Wi‑Fi and CTM link, then the device reboots back to the `ctmlight` AP.
- If you’re still linked and you reopen `/link`, it will tell you the device is already linked and offer **Unlink Device**.

## Build & flash
Prereqs: PlatformIO (`pio`), Python, USB cable.

Common targets (default `make` builds/uploads TinyPICO):
```
make                         # TinyPICO: build, upload, then serial monitor
pio run -e tinypico          # build only TinyPICO
pio run -e s3wroom_1         # build only ESP32-S3 devkit
make wroom_s3                # ESP32-S3: build + upload
pio device monitor -b 115200 # serial monitor (921600 for s3wroom_1)
make clean                   # cleanup
```

## Development / tunnels
When `CTM_PRODUCTION` is undefined (staging/tunnel), provide hosts and client id:
```
CTM_SOC_HOST=<ngrok>.ngrok.io \
CTM_API_HOST=<ngrok>.ngrok.io \
CTM_APP_HOST=<ngrok>.ngrok.io \
CTM_CLIENTID=<client_id> \
make tinypico
```
Get a client id from https://app.calltrackingmetrics.com/oauth_apps/.

## Ethernet (experimental)
ESP32 Ethernet needs different PEM handling. Use the helper from OPEnSLab-OSU/SSLClient to generate BearSSL certs:
```
python3 pycert_bearssl.py download app.calltrackingmetrics.com --output include/certs.h
```
