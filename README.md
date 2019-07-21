## Project Description
This project uses a PIR sensor to detect human movement in a room. When the device is booting up for the first time then it synchronizes the datetime (GMT time zone) on the device using the SNTP protocol and goes into deep sleep. The app wakes up each time a movement is detected and optionally connects to a WiFi access point and uploads a movement-detected message (device id + datetime + battery voltage) to a UDP server of your choice.

The app remains in deep sleep most of the time to minimize power consumption. In deep sleep a decent ESP32 board consumes 70 microAmp and the quiescent current of the PIR sensor is 20 microAmp (for the MH-SR602 model). The most power will be consumed when uploading data to the UDP server because it uses Wifi but that timespan is kept as short as possible.

A LDR photoresistor model 5528 (bright = 8-20 KiloOhm, dark = 1 MegaOhm) can be soldered to the PIR sensor so that the sensor is only enabled when it is dark.

Tip: you can also replace the PIR sensor with another type of switch such as the MC-38 Normally Opened magnetic reed switch. 



## Related Projects

`esp32_door_sensor_reed_switch` Demonstrates how to use a magnetic door/window sensor which is based on a reed switch. This app is developed for a door sensor which contains a MC-38 N.O. (Normally Opened) magnetic reed switch. This means that the switch is open (and no current flowing) when the 2 plastic blocks are away from each other.

`esp32_hcsr501_pir_sensor_using_lib` This project demonstrates the basics of using the MJD component "mjd_hcsr501".



## What are the HW SW requirements of the ESP32 MJD Starter Kit?

### Hardware

- A decent ESP development board. I suggest to buy a popular development board with good technical documentation and a significant user base. Examples: [LOLIN D32](https://wiki.wemos.cc/products:d32:d32),  [Adafruit HUZZAH32](https://www.adafruit.com/product/3405),  [Espressif ESP32-DevKitC](http://espressif.com/en/products/hardware/esp32-devkitc/overview), [Pycom](https://pycom.io/hardware/).
- The peripherals that are used in the project. The README of each component contains a section "Shop Products".

### Software: ESP-IDF v3.2

- A working installation of the **Espressif ESP-IDF *V3.2* development framework**** (detailed instructions @ http://esp-idf.readthedocs.io/en/latest/get-started/index.html).

```
mkdir ~/esp
cd    ~/esp
git clone -b v3.2 --recursive https://github.com/espressif/esp-idf.git esp-idf-v3.2
```

- A C language editor or the Eclipse IDE CDT (instructions also @ http://esp-idf.readthedocs.io/en/latest/get-started/index.html).



## Shop Product
- HC-SR501 PIR motion sensor module with lens. **Recommended** if you want to adjust the module manually (with the potentiometers).
- MH-SR602 Micro PIR motion sensor module with lens. **Recommended** but cannot be adjusted.
- AM312 Mini PIR motion sensor module with lens. **Not recommended** because it is very sensitive to noise.



## Wiring Instructions
Go to the subdirectory "_doc".

Go to the subdirectory "components/mjd_hcsr501" for installation/wiring/usage instructions, data sheets, FAQ, photo's, etc. for the hardware and software.



## Running the app
- Goto the project directory, for example 
  `cd ~/esp32_hcsr501_pir_sensor_deep_sleep_udp_using_lib`
- Run `make menuconfig`
  - Select "PROJECT CONFIGURATION"
    - Specify the on-board LED GPIO#
    - Specify the GPIO# of the input pin (default 27)
    - Specify the WiFi SSID and Password.
    - Specify if you want to Upload to UDP Server y/n
    - Specify the UDP Server hostname and IPV4 address.
- Exit menuconfig
- Run `make flash monitor`



## Example Output in the Debug Log

```
//
// Case: first boot
//
ets Jun  8 2016 00:22:57
...

I (100) myapp: app_main()
I (100) mjd: ESP free HEAP space: 272016 bytes | FreeRTOS free STACK space (current task): 2044 bytes
I (110) myapp: OK Task has been created, and is running right now
I (110) mjd: ESP free HEAP space: 263436 bytes | FreeRTOS free STACK space (current task): 7528 bytes
I (110) myapp: END app_main()
I (160) mjd: *** DATETIME 19700101000000 Thu Jan  1 00:00:00 1970
I (160) mjd: *** Wakeup reason: UNKNOWN (not after a deep sleep period, probably after a normal reset)
I (170) mjd: *** boot count: 1

===SECTION: LED===
I (180) myapp: MY_LED_ON_DEVBOARD_GPIO_NUM:    5
I (180) myapp: MY_LED_ON_DEVBOARD_WIRING_TYPE: 2
I (190) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0

===*SECTION: WIFI===
I (200) myapp: MY_WIFI_SSID:     AREA51
I (260) mjd_wifi: OK: WIFI initialized!
I (260) mjd_wifi: Connecting to the WIFI network...
I (330) phy: phy_version: 4008, 544f89f, Jan 24 2019, 14:54:06, 0, 0
I (340) mjd_wifi: _mjd_wifi_sta_event_handler case SYSTEM_EVENT_STA_START
I (3010) mjd_wifi: _mjd_wifi_sta_event_handler case SYSTEM_EVENT_STA_CONNECTED
I (3040) event: sta ip: 192.168.0.145, mask: 255.255.255.0, gw: 192.168.0.1
I (3040) mjd_wifi: _mjd_wifi_sta_event_handler case SYSTEM_EVENT_STA_GOT_IP
I (3160) myapp: OK Internet reachable

===SECTION: SNTP sync datetime===
I (3160) myapp:   Sync required when first boot bootcount==1
I (3160) mjd_net: mjd_net_sync_current_datetime(). SNTP Sync is REQUIRED - timeinfo.tm_year is 70 (baseyear is 1900)
or param_forced=true
I (4180) mjd_net: mjd_net_sync_current_datetime(). OK time synced with SNTP
I (4180) mjd_net: mjd_net_sync_current_datetime(). Actual time (UTC):
I (4180) mjd_net:   - Sat Jul 20 18:22:48 2019
I (4180) mjd_net:   - day 201 of year 2019
I (4190) mjd_net:   - 20/07/2019 18:22:48
I (4190) mjd_net:   - Saturday, 20 July 2019
I (4200) myapp:

===SECTION: cleanup, WIFI DEINIT===
I (4210) mjd_wifi: Disconnecting from WIFI network...
I (4230) mjd_wifi: _mjd_wifi_sta_event_handler case SYSTEM_EVENT_STA_DISCONNECTED
W (4230) mjd_wifi:   SYSTEM_EVENT_STA_DISCONNECTED: ssid: AREA51 | ssid_len: 6 | reason: 8 (WIFI_REASON_ASSOC_LEAVE)
I (4240) mjd_wifi: OK: WIFI disconnected!

===SECTION: DEEP SLEEP===
I (4260) myapp: MY_SENSOR_GPIO_INPUT_PIN: 27
I (4270) mjd: ESP free HEAP space: 231160 bytes | FreeRTOS free STACK space (current task): 5848 bytes
I (4280) mjd: *** DATETIME 20190720182248 Sat Jul 20 18:22:48 2019
I (4280) myapp: Entering deep sleep. The MCU will wake up when RTC GPIO#27 is HIGH, or becomes HIGH...
I (4290) myapp: LED blink 2x before going into deep sleep


//
// Case: the PIR sensor detected a movement
//
ets Jun  8 2016 00:22:57
...

I (100) myapp: app_main()
I (100) mjd: ESP free HEAP space: 272016 bytes | FreeRTOS free STACK space (current task): 2044 bytes
I (110) myapp: OK Task has been created, and is running right now
I (110) mjd: ESP free HEAP space: 263436 bytes | FreeRTOS free STACK space (current task): 7528 bytes
I (110) myapp: END app_main()
I (160) mjd: *** DATETIME 20190720182253 Sat Jul 20 18:22:53 2019
I (160) mjd: *** Wakeup reason: EXT0 RTC_IO
I (160) mjd: *** boot count: 2

===SECTION: LED===
I (170) myapp: MY_LED_ON_DEVBOARD_GPIO_NUM:    5
I (180) myapp: MY_LED_ON_DEVBOARD_WIRING_TYPE: 2
I (180) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0

===*SECTION: WIFI===
I (200) myapp: MY_WIFI_SSID:     AREA51
I (250) mjd_wifi: OK: WIFI initialized!
I (3070) myapp: OK Internet reachable

===SECTION: SNTP sync datetime===
I (3070) myapp:   Sync NOT required (@dep bootcount 2)

===SECTION: UDP CLIENT===
I (3080) myapp: MY_DEVICE_ID:               pir001
I (3090) myapp: MY_DO_UPLOAD_TO_UDP_SERVER: 1
I (3090) myapp: MY_UDP_SERVER_HOSTNAME:     192.168.0.94
I (3100) myapp: MY_UDP_SERVER_PORT:         3001
I (3100) myapp: LOLIN D32 battery voltage (V): 3.89
I (3584) myapp: UDP message: <~> devpir001 20190720182255 4.24 1
I (3594) myapp: OK UDP buffer sent

===SECTION: cleanup, WIFI DEINIT===
I (3110) mjd_wifi: Disconnecting from WIFI network...
I (3140) mjd_wifi: _mjd_wifi_sta_event_handler case SYSTEM_EVENT_STA_DISCONNECTED
W (3140) mjd_wifi:   SYSTEM_EVENT_STA_DISCONNECTED: ssid: AREA51 | ssid_len: 6 | reason: 8 (WIFI_REASON_ASSOC_LEAVE)
I (3150) mjd_wifi: OK: WIFI disconnected!

===SECTION: DEEP SLEEP===
I (3170) myapp: MY_SENSOR_GPIO_INPUT_PIN: 27
I (3180) mjd: ESP free HEAP space: 231056 bytes | FreeRTOS free STACK space (current task): 5848 bytes
I (3190) mjd: *** DATETIME 20190720182256 Sat Jul 20 18:22:56 2019
I (3190) myapp: Entering deep sleep. The MCU will wake up when RTC GPIO#27 is HIGH, or becomes HIGH...
I (3200) myapp: LED blink 2x before going into deep sleep

```



## Reference: the ESP32 MJD Starter Kit SDK

Do you also want to create innovative IoT projects that use the ESP32 chip, or ESP32-based modules, of the popular company Espressif? Well, I did and still do. And I hope you do too.

The objective of this well documented Starter Kit is to accelerate the development of your IoT projects for ESP32 hardware using the ESP-IDF framework from Espressif and get inspired what kind of apps you can build for ESP32 using various hardware modules.

Go to https://github.com/pantaluna/esp32-mjd-starter-kit

