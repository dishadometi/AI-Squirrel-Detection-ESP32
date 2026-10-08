# AI-Based Squirrel Detection System

1. Project Overview

I built this project to detect squirrels using a camera and an AI model. The main idea was to use an ESP32-S3 CAM to take pictures and use a machine-learning model to decide whether a squirrel is in the picture or not.

I used Edge Impulse to train the AI model. The model has two classes: squirrel and non squirrel.

After the ESP32 makes the detection, it sends a signal to an Arduino Mega. The Arduino Mega controls two LEDs. A red LED turns on when a squirrel is detected, and a green LED turns on when there is no squirrel.

One of the main reasons I wanted to build this project was to learn how AI can actually be connected to physical hardware instead of only running as software on a computer.

2. How the System Works

The project has two main parts. The first part is the ESP32-S3 CAM, which handles the camera and AI detection. The second part is the Arduino Mega, which handles the LEDs.

The ESP32-S3 takes an image from the camera and sends it through the Edge Impulse model. The model gives a confidence value for the different classes. I used a threshold of 0.50 for the squirrel class.

When the ESP32 detects a squirrel, it sets GPIO1 HIGH. This signal goes through a BOB-12009 logic-level converter and then reaches Digital Pin 7 on the Arduino Mega.

The Mega reads that signal and turns on the correct LED.

The basic flow is:
ESP32-S3 Camera
       ↓
Edge Impulse AI Model
       ↓
Squirrel / Non-Squirrel
       ↓
ESP32 GPIO1
       ↓
BOB-12009 Level Shifter
       ↓
Arduino Mega Pin 7
       ↓
LED

I used the level shifter because the ESP32-S3 uses 3.3V logic while the Arduino Mega uses 5V logic.

3. Machine Learning

I trained the machine-learning part of the project using Edge Impulse. I used 140 images for the project and created two classes: squirrel and non squirrel.

The model results from Edge Impulse were:

| Result | Accuracy |
|---|---:|
| Validation accuracy | 65.2% |
| Test-set accuracy | 81.5% |

The 81.5% is the test-set accuracy reported by Edge Impulse. I don't consider this to mean that the system will be 81.5% accurate in every real-world situation. The results can change depending on things like lighting, distance from the camera, background, and the way the squirrel appears in the image.

After training the model, I deployed it to the ESP32-S3 using the Edge Impulse Arduino library. This allowed the ESP32 to run the model directly on the device instead of sending the images to a computer for every prediction.

This was one of the parts of the project I found most interesting because I could actually see the machine-learning model running on a small physical device.

4. Hardware

The main hardware I used was an ESP32-S3 CAM V1.2, an Arduino Mega 2560, and a SparkFun BOB-12009 logic-level converter.

I also used two LEDs to show the detection status.

The ESP32-S3 is responsible for taking the camera image and running the AI model. I used GPIO1 as the output signal from the ESP32.

The BOB-12009 connects the ESP32-S3 to the Arduino Mega. The low-voltage side is connected to the ESP32's 3.3V reference, and the high-voltage side is connected to the Mega's 5V reference.

The main connections are:
ESP32-S3 3V3 → BOB-12009 LV
ESP32-S3 GND → BOB-12009 GND
ESP32-S3 GPIO1 → BOB-12009 LV1

Arduino Mega 5V → BOB-12009 HV
Arduino Mega GND → BOB-12009 GND
Arduino Mega D7 → BOB-12009 HV1

For the LEDs, I used:
Mega D10 → Red LED
Mega D3  → Green LED

5. Squirrel Detection

When the ESP32-S3 takes a picture, the Edge Impulse model analyzes the image.

The model produces a confidence value for the squirrel class. I set the squirrel detection threshold to 0.50.

If the squirrel confidence is at least 0.50, my program considers the image a squirrel detection and sets GPIO1 HIGH.

If the confidence is below 0.50, GPIO1 is set LOW.

The Arduino Mega then reads the signal from Digital Pin 7.

The detection process is:

Camera takes picture
        ↓
Edge Impulse analyzes picture
        ↓
Squirrel confidence ≥ 0.50?
        ↓
      YES
        ↓
ESP32 GPIO1 = HIGH
        ↓
Mega receives HIGH
        ↓
Red LED turns ON

If the confidence is below the threshold, the ESP32 sends LOW and the green LED turns on.

6. LED Status System

I used two LEDs to show the result of the squirrel detection.

The **red LED** turns on when the system detects a squirrel. The **green LED** turns on when the system does not detect a squirrel.

The LEDs are connected to the Arduino Mega, which receives the detection signal from the ESP32-S3.

| Mega Pin | LED | Meaning |
|:---|:---|:---|
| **D10** | **Red** | Squirrel detected |
| **D3** | **Green** | No squirrel detected |

This gives me a simple visual way to see the AI result without having to look at the Serial Monitor.

This makes the project easier to understand when looking at the physical hardware because the AI result is shown directly through the LEDs.

7. Problems I Ran Into

This project did not work perfectly on the first try. I had to troubleshoot both the software and the hardware.

One problem was the difference between the ESP32-S3's 3.3V logic and the Arduino Mega's 5V logic. I learned that I should not directly connect the ESP32 signal to a 5V logic system, so I used the BOB-12009 level shifter.

I also had a problem compiling the Edge Impulse library. The ESP-NN portion of the library caused a compilation error. I eventually disabled the ESP-NN option and removed the related ESP-NN folder so that the project could compile successfully.

Another problem happened when I tested the Arduino Mega input pin. When Digital Pin 7 was not connected to anything, the LED sometimes changed between red and green. I learned that the input was floating and could randomly read HIGH or LOW. Testing the pin with a definite HIGH or LOW signal fixed this issue.

I also had camera capture problems during testing. The ESP32 produced camera errors under one of the power and USB configurations I was using. I changed the power setup and eventually got the camera capture working correctly.

These problems were actually an important part of the project for me because I had to test each part separately instead of assuming that the entire system would work immediately.

8. Testing

I tested the project in stages.

First, I tested the LEDs on the Arduino Mega to make sure the outputs were working. I tested the red LED and green LED separately and confirmed that they could be controlled by the Mega.

Next, I tested Digital Pin 7 as the input from the ESP32 side. I used HIGH and LOW signals to make sure the Mega was reading the input correctly.

After that, I tested the ESP32 GPIO1 output. The ESP32 printed messages to the Serial Monitor showing whether it was detecting a squirrel or not.

I then connected the ESP32 and Mega through the BOB-12009 level shifter and tested the complete signal path.

Finally, I tested the camera and Edge Impulse model together.

The complete system was tested as:

Camera
  ↓
AI model
  ↓
ESP32 GPIO1
  ↓
Level shifter
  ↓
Mega D7
  ↓
LED

Testing the system one part at a time helped me figure out where problems were coming from instead of trying to debug everything at once.

9. What I Would Improve

There are several things I would like to improve if I continue working on this project.

First, I would increase the number of images in the training dataset. My current dataset has 140 images, and adding more images with different backgrounds, lighting conditions, distances, and squirrel positions could help make the model more reliable.

I would also test the system more in real outdoor environments instead of only testing it with images during development.

Another improvement would be adding more animal classes. Instead of only detecting squirrels versus non-squirrels, I could train the system to recognize different animals.

I would also like to add wireless notifications. For example, the ESP32 could send a notification to a phone whenever a squirrel is detected.

In the future, I could also add a weather-resistant enclosure and make the system suitable for being installed outside for longer periods of time.

10. Project Links and Files

I documented the project using Edge Impulse and GitHub.

The Edge Impulse project contains the machine-learning dataset, training information, model, and evaluation results.

Edge Impulse:

https://studio.edgeimpulse.com/public/1121907/latest

The GitHub repository contains the source code, hardware documentation, project photos, and project report.

GitHub:

[AI-Squirrel-Detection-ESP32](YOUR_GITHUB_REPOSITORY_URL)

The repository is organized like this:

```text
AI-Squirrel-Detection-ESP32/
├── README.md
├── firmware/
│   ├── ESP32/
│   │   └── Squirrel_ESP32.ino
│   └── Arduino-Mega/
│       └── Squirrel_Mega.ino
├── hardware/
│   └── wiring-diagram.md
├── photos/
│   ├── system-overview.jpg
│   ├── esp32-camera.jpg
│   ├── level-shifter.jpg
│   └── squirrel-detection.jpg
├── docs/
│   └── project-report.md
└── LICENSE
```

Overall, this project gave me experience with machine learning, embedded programming, electronics, hardware communication, and debugging. The most useful part for me was not just getting the final LEDs to work, but figuring out the problems along the way and understanding why each part of the system was needed.
