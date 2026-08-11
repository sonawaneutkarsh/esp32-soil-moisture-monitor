# esp32 plant monitor

a small hobby electronics project using an esp32-c3 super mini and a capacitive soil moisture sensor to monitor the moisture level of a plant.

the esp32 takes multiple sensor readings, averages them, converts the calibrated reading into a 0–100% moisture value, and checks it against a configurable threshold. if the moisture level falls below the threshold, the esp32 sends an email notification over wi-fi.

# features

* capacitive soil moisture sensing
* sensor calibration using dry and wet reference values
* 10-sample averaging for each measurement
* moisture values mapped to a 0–100% scale
* configurable moisture threshold
* configurable measurement interval
* wi-fi connectivity
* gmail email notifications
* prevents repeated alerts while the soil remains dry
* resets the alert state after the soil recovers
* esp32-c3 based

# how it works

the capacitive soil moisture sensor outputs an analog signal to gpio1 on the esp32-c3.

the system works in the following steps:

1. the esp32 takes 10 analog readings from the sensor.
2. the readings are averaged to reduce measurement variation.
3. the averaged raw adc value is mapped between the calibrated dry and wet values.
4. the resulting value is constrained to a 0–100% moisture scale.
5. the moisture value is compared against `ALERT_BELOW_PCT`.
6. if the moisture level is below the threshold, the esp32 sends an email alert.
7. after an alert is sent, another alert is not sent until the moisture level recovers sufficiently.

# hardware requirements

1. esp32-c3 super mini
2. mb102 breadboard power supply
3. breadboard
4. capacitive soil moisture sensor v2.0.0
5. 6× male-to-male jumper wires
6. barrel jack with a compatible wall adapter or a suitable battery power source
7. usb-c cable
8. plant

# software requirements

* arduino ide
* esp32 board support for arduino
* esp mail client library by mobizt

# installation

install [arduino ide](https://www.arduino.cc/en/software/).

after downloading the repository, open the project files in arduino ide.

# esp32 board support

1. open arduino ide and go to settings.
2. find `additional boards manager urls`.
3. add the following url:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

4. go to `tools` → `board` → `boards manager`.
5. search for `esp32`.
6. install `esp32 by espressif systems`.
7. go to `tools` → `board` → `esp32 arduino` and select `esp32c3 dev module`.
8. connect the esp32-c3 to your computer.
9. go to `tools` → `port` and select the usb serial port corresponding to the connected esp32.

# email setup

the project uses the `esp mail client` library by mobizt to send email notifications through gmail's smtp server.

1. open arduino ide.
2. go to `sketch` → `include library` → `manage libraries`.
3. search for `esp mail client`.
4. install `esp mail client by mobizt`.

## email credentials

the main program uses the following values:

* wi-fi ssid
* wi-fi password
* sender email
* gmail app password
* recipient email

these values are stored in `secrets.h`.

`secrets.h` is included in `.gitignore` and should not be committed to the repository.

# wiring

| component            | pin  | connection      |
| -------------------- | ---- | --------------- |
| soil moisture sensor | vcc  | mb102 3.3v rail |
| soil moisture sensor | gnd  | mb102 gnd rail  |
| soil moisture sensor | aout | esp32-c3 gpio1  |
| esp32-c3             | 3v3  | mb102 3.3v rail |
| esp32-c3             | gnd  | mb102 gnd rail  |

for my setup, the wire colors are:

* red → 3.3v
* black → gnd
* yellow → gpio1 / aout

for connections to the power rails, the exact breadboard position does not matter as long as the component is connected to the correct rail.

![esp32 connections](assets/image.png)

![breadboard connections](assets/connections.JPG)

# powering the circuit

> **important:** set the mb102 power supply to 3.3v before powering the circuit. supplying excessive voltage can damage the esp32-c3.

after connecting the power source, the power indicators on the mb102 and esp32-c3 should turn on.

the circuit can be powered using the barrel jack and a compatible wall adapter. battery operation is also possible, but the appropriate charging, protection, and voltage regulation circuitry should be used.

# calibration

the repository contains a separate calibration program at [`moisture_calibrator.ino`](moisture_calibrator/moisture_calibrator.ino).

the calibration program continuously prints the raw adc reading from the sensor.

1. open `moisture_calibrator.ino` in arduino ide.
2. connect the esp32-c3 to your computer.
3. select the `esp32c3 dev module` board and the correct port.
4. upload the program.
5. open the serial monitor at `115200` baud.
6. place the sensor in the conditions you want to use as calibration references.
7. record the raw readings.

the main program uses two calibration values:

```cpp
const int DRY_VALUE = 3890;
const int WET_VALUE = 1410;
```

set `DRY_VALUE` to the raw reading representing dry conditions and `WET_VALUE` to the raw reading representing wet conditions.

# moisture calculation

the main program averages 10 sensor readings:

```cpp
for (int i = 0; i < 10; i++) {
    sum += analogRead(MOISTURE_PIN);
    delay(10);
}
```

the averaged raw value is then mapped between `DRY_VALUE` and `WET_VALUE`:

```cpp
map(raw, DRY_VALUE, WET_VALUE, 0, 100)
```

the result is constrained between 0 and 100.

this means:

* a reading near `DRY_VALUE` corresponds to approximately 0%
* a reading near `WET_VALUE` corresponds to approximately 100%
* readings between them are mapped proportionally

# configuration

the main settings can be changed in `main.ino`.

## moisture threshold

```cpp
const int ALERT_BELOW_PCT = 15;
```

the esp32 sends an email alert when the calculated moisture level falls below this value.

for my succulents, i used a threshold of 15 based on the calibration results and the moisture level i wanted to maintain.

## measurement interval

```cpp
const unsigned long CHECK_EVERY_MS = 6UL * 60 * 60 * 1000;
```

the default configuration checks the soil every 6 hours.

change this value if you want the system to check more or less frequently.

# alert behavior

the system prevents repeated email notifications while the soil remains below the configured threshold.

after an alert is sent, `alertSentThisCycle` prevents another alert from being sent during subsequent checks.

the alert state is reset when the moisture level reaches at least 10 percentage points above the configured threshold:

```cpp
if (pct >= ALERT_BELOW_PCT + 10) {
    alertSentThisCycle = false;
}
```

this allows another alert to be sent if the soil dries again after being watered.

# wi-fi configuration

the esp32-c3 connects to wi-fi using the credentials stored in `secrets.h`.

the esp32-c3 requires a 2.4 ghz wi-fi network.

if using an iphone hotspot, enable `maximize compatibility` so the hotspot provides 2.4 ghz compatibility.

# running the main program

1. open [`main.ino`](main/main.ino).
2. make sure your `secrets.h` file is configured.
3. connect the esp32-c3 to your computer.
4. select `esp32c3 dev module` in arduino ide.
5. select the correct usb serial port.
6. click `upload`.
7. open the serial monitor at `115200` baud.
8. the esp32 will connect to wi-fi and periodically print the current moisture level.
9. when the moisture level falls below the configured threshold, an email alert will be sent.

example serial monitor output:

```text
connecting to wi-fi.... connected!
moisture: 18%
```

# project structure

```text
.
├── assets/
│   ├── connections.JPG
│   └── image.png
├── main/
│   └── main.ino
├── moisture_calibrator/
│   └── moisture_calibrator.ino
├── .gitignore
└── README.md
```

* `main/main.ino` — main moisture monitoring and email alert program
* `moisture_calibrator/moisture_calibrator.ino` — calibration program for reading raw sensor values
* `assets/` — wiring and project images
* `.gitignore` — excludes files such as `secrets.h` and `.ds_store`

# results

i tested the system with my succulents using a 6-hour measurement interval.

the esp32 successfully measured the soil moisture and sent an email notification when the calculated moisture level dropped below the configured threshold.

add screenshots of the serial monitor and email notification here if you have them.

# limitations

* moisture readings depend on the sensor, soil, and sensor placement.
* calibration values may need to be changed for different sensors or growing conditions.
* the current implementation monitors soil moisture but does not automatically water the plant.
* the esp32-c3 requires a 2.4 ghz wi-fi connection.
* the current version is intended as a hobby-scale plant monitoring system.

# future improvements

the original idea was to turn this into an automatic watering system.

possible improvements include:

* add a mini water pump for automatic watering
* control the pump using a mosfet or suitable motor driver
* add a water reservoir and tubing
* add battery-powered operation
* add persistent moisture data logging
* add a dashboard for viewing moisture measurements over time

# acknowledgements

huge thanks to mr. terry, who provided me with all the materials for this hobby project.

now my summer succulents can survive without me (still needs someone to water them :p)
