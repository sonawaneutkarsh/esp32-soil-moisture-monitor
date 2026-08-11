# esp32 plant monitor

a small hobby electronics project using an esp32-c3 super mini and a capacitive soil moisture sensor to monitor the moisture level of a plant.

the esp32 takes multiple sensor readings, averages them, converts the calibrated reading into a normalized 0–100 moisture value, and checks it against a configurable threshold. if the moisture level falls below the threshold, the esp32 sends an email notification over wi-fi.

# features

* capacitive soil moisture sensing
* sensor calibration using dry and wet reference values
* 10-sample averaging for each measurement
* normalized 0–100 moisture scale
* configurable moisture threshold
* configurable measurement interval
* wi-fi connectivity
* gmail email notifications
* prevents repeated alerts while the soil remains dry
* resets the alert state after the soil recovers
* immediate measurement after startup
* esp32-c3 based

# technologies

* c++
* arduino ide
* esp32-c3
* wi-fi
* smtp
* gmail
* capacitive analog sensing

# how it works

the capacitive soil moisture sensor outputs an analog signal to gpio1 on the esp32-c3.

the system works in the following steps:

1. the esp32 takes 10 analog readings from the sensor.
2. the readings are averaged to reduce short-term sensor variation.
3. the averaged raw adc value is mapped between the calibrated dry and wet values.
4. the resulting value is constrained to a normalized 0–100 scale.
5. the moisture value is compared against `ALERT_BELOW_PCT`.
6. if the moisture level is below the threshold, the esp32 sends an email alert.
7. after an alert is sent, another alert is not sent until the moisture level recovers sufficiently.
8. the first measurement is taken immediately after startup, followed by measurements at the configured interval.

the 0–100 value is a relative scale based on the sensor's calibration points. it is not a direct measurement of volumetric water content.

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

create `secrets.h` in the same directory as `main.ino`:

```cpp
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"

#define SENDER_EMAIL "your_sender@gmail.com"
#define SENDER_APP_PASS "your_gmail_app_password"

#define RECIPIENT_EMAIL "recipient@gmail.com"
```

`secrets.h` is included in `.gitignore` and should never be committed to the repository.

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

# powering the circuit

> **important:** set the mb102 power supply to 3.3v before powering the circuit. supplying excessive voltage can damage the esp32-c3.

after connecting the power source, the power indicators on the mb102 and esp32-c3 should turn on.

the circuit can be powered using the barrel jack and a compatible wall adapter. battery operation is also possible, but the appropriate charging, protection, and voltage regulation circuitry should be used.

![breadboard connections](assets/connections.JPG)

# calibration

the repository contains a separate calibration program at [`moisture_calibrator.ino`](moisture_calibrator/moisture_calibrator.ino).

the calibration sketch continuously prints the raw adc reading from the sensor. it does not automatically calculate or save calibration values; the readings are recorded manually and then entered into `main.ino`.

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

* a reading near `DRY_VALUE`
