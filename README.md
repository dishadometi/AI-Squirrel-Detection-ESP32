# 🐿️ AI-Based Squirrel Detection System

An embedded AI system that uses an **ESP32-S3 camera** and a machine-learning model trained with **Edge Impulse** to detect squirrels in real time.

When a squirrel is detected, the ESP32-S3 sends a signal to an **Arduino Mega 2560**, which controls red, yellow, and green LEDs to indicate the detection status.

---

## 🎯 Project Goal

The goal of this project is to combine:

- Computer vision
- Machine learning
- Embedded AI
- ESP32-S3 camera hardware
- Arduino hardware
- 3.3V-to-5V logic-level communication

into a complete real-world animal detection system.

---

## 🧠 How It Works

The system follows this process:

1. The ESP32-S3 CAM captures an image.
2. The Edge Impulse machine-learning model analyzes the image.
3. The model classifies the image as either:
   - `squirrel`
   - `non squirrel`
4. If the squirrel confidence reaches the detection threshold, ESP32 GPIO1 is set HIGH.
5. The GPIO signal passes through a bidirectional logic-level converter.
6. The Arduino Mega reads the signal on digital pin 7.
7. The Mega controls the status LEDs.

### System Architecture

```text
                 ┌─────────────────────┐
                 │    ESP32-S3 CAM     │
                 │                     │
                 │ Camera + AI Model   │
                 └──────────┬──────────┘
                            │
                         GPIO 1
                            │
                            ▼
                 ┌─────────────────────┐
                 │  Logic Level        │
                 │  Converter          │
                 │  3.3V → 5V          │
                 └──────────┬──────────┘
                            │
                          D7
                            │
                            ▼
                 ┌─────────────────────┐
                 │   Arduino Mega      │
                 │                     │
                 │   LED Controller    │
                 └──────┬──┬──┬────────┘
                        │  │  │
                       🔴 🟡 🟢
