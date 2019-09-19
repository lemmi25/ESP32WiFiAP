# ESP32 to WiFi via Access Point

This simple example provides

- Opening of an access point (AP) with the name **ESP32_Server**
- Under the static IP **192.168.1.1** you will be asked to type in your **ssid** name and **wifi** psw
- After successfull connection to the AP the RGB LED on you Wroover-Kit will light up
- Your ESP32 is now connected to WiFi (IP is not static any more)

Use this as a template for your projects it can be very handy in various ways ;). Please let me know if you have anything to improve or commit.

Cheers

## How to use example

### Configure the project

First of all clone the project

```
git clone
cd
```

- Set serial port under Serial Flasher Options.

```
make menuconfig
```

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4
make flash monitor
```

(To exit the serial monitor, type `Ctrl-]`.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example ESP32 to WiFi Connection

After flashing you will see this on your board

![](/images/RGB0.jpg)

and this in the consol

```
...
I (431) ESP32_Server: Starting server on port: '80'
I (431) ESP32_Server: wifi_event_handler wifi_event_handler wifi_event_handler
I (441) ESP32_Server: Registering URI handlers
```

After connection successfully to ssid:**ESP32_Server** and psw:**ESP32_Server**

![](/images/WiFi.png)

```
...
I (133931) ESP32_Server: station 9c:b6:d0:e7:30:0f join, AID=1
I (133931) ESP32_Server: wifi_event_handler wifi_event_handler wifi_event_handler
I (133991) tcpip_adapter: softAP assign IP to station,IP is: <IP of your AP>
```

After typing **192.168.1.1** into firefox and providing your WiFi AP ssid and psw

![](/images/Firefox.png)

```
...
I (330) wifi: mode : sta (30:ae:a4:ef:4a:60)
I (1060) wifi: new:<6,1>, old:<1,0>, ap:<255,255>, sta:<6,1>, prof:1
I (2040) wifi: state: init -> auth (b0)
I (2050) wifi: state: auth -> assoc (0)
I (2060) wifi: state: assoc -> run (10)
I (2330) wifi: connected with <Name of your WiFi AP>, channel 6, bssid = <bssid>
I (2330) wifi: pm start, type: 1

I (3210) tcpip_adapter: sta ip: <new IP of ESP32>, mask: <mask>, gw: <gw ip>
I (3210) ESP32_Server: SYSTEM_EVENT_STA_GOT_IP
I (3210) ESP32_Server: Login Success
I (3220) ESP32_Server: Starting server on port: '80'
I (3220) ESP32_Server: Registering URI handlers

```

You are no connected to the internet with your ESP32 and the RGB LED is lightening up ;). The AP **ESP32_Server** will not exist anymore.

![](/images/RGB1.jpg)

Note:

Only one device at a time can be connected to the **ESP32_Server** AP for safety.
If you type in the wrong psw the **ESP32_Server** Server will stay up.
