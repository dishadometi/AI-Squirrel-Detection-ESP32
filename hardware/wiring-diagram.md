# Hardware Wiring

## ESP32-S3 CAM → Logic-Level Converter

| ESP32-S3 CAM | BOB-12009 |
|---|---|
| 3V3 | LV |
| GND | GND |
| GPIO1 | LV1 |

## Arduino Mega → Logic-Level Converter

| Arduino Mega | BOB-12009 |
|---|---|
| 5V | HV |
| GND | GND |
| Digital Pin 7 | HV1 |

## Arduino Mega LEDs

| Mega Pin | Component |
|---|---|
| Digital 10 | Red LED |
| Digital 3 | Green LED |

## Detection Flow

ESP32-S3 camera → Edge Impulse inference → GPIO1 → BOB-12009 → Mega D7 → LED status

The ESP32-S3 sends the squirrel detection signal through GPIO1. The BOB-12009 converts the 3.3V logic signal to a 5V-compatible signal for the Arduino Mega.
