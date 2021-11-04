# CTM Status Station

The goal this project is to make it easy to have light in a room that helps people in the area know when someone is on a phone call.

We'll subscribe to incoming phone calls and active call events and change the light color to make this obvious


## Setup

Before the device is configured it will expose an access point: 
  ssid: ctmlight
  pass: ctmstatus

After connecting navigate browser to

http://192.168.4.1/

From the web interface configure your networks ssid and password

Once the device joins your network it'll blink green

You'll have to find the device now on your network

http://your-device-ip/

From here you can click connect and you'll be taken to a page on https://app.calltrackingmetrics give access to the device and you should 
next be presented with some options to choose a specific team or your whole account.


openssl x509 -inform der -in ~/Desktop/ISRG\ Root\ X1.cer -out ~/Desktop/ISRG_ROOT_X1.pem
