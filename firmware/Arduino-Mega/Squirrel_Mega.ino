#define ESP_SIGNAL_PIN 7

#define RED_LED    10
#define YELLOW_LED 6
#define GREEN_LED  3

void setup() {
  pinMode(ESP_SIGNAL_PIN, INPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Start with all LEDs OFF
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

  Serial.begin(9600);

  Serial.println("LED + ESP32-S3 test started");
}

void loop() {

  int signal = digitalRead(ESP_SIGNAL_PIN);

  if (signal == HIGH) {

    // Squirrel detected
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);

    Serial.println("ESP32 = HIGH -> RED");

  } else {

    // No squirrel
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);

    Serial.println("ESP32 = LOW -> GREEN");
  }

  delay(500);
}
