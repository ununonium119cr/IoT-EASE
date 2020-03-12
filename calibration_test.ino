#define ANALOG_PIN  (36)
#define ANALOG_MAX  (4095)
#define ANALOG_MIN  (0)
#define DOTS_MAX    (30)
#define DOTS_MIN    (1)
#define DAC_PIN     (25)
#define V33         (255)
#define SAMPLE_RATE (500)
#define LED_PIN     (2)
#define NUM_SEC     (1)
#define ITERATIONS  (3)

struct calibration {
  int high_peak = ANALOG_MIN;
  int high_avg = ANALOG_MIN;
  int low_peak = ANALOG_MAX;
  int low_avg = ANALOG_MAX;
};

// calibration object
struct calibration cal;

void setup() {
  // intialize pins and serial connection
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);

  // TODO: remove DAC test code
//  // DAC output
//  dacWrite(DAC_PIN, V33 / 2);

  // enter calibration sequence
  long avg_high_tot = 0;
  long avg_low_tot = 0;

  // wait 2 seconds then start collection sequence
  delay(2000);
  blink3(500);
  
  // loop three times for high
  for(int i = 0; i < ITERATIONS; i++) {
    // poll for max with light on
    digitalWrite(LED_PIN, HIGH);
    for(int j = 0; j < (SAMPLE_RATE * NUM_SEC); j++) {
      int value = getAnalogDisplay();
      if(value > cal.high_peak) {
        cal.high_peak = value;
      }
      avg_high_tot += value;
      delay(1000 / SAMPLE_RATE);
    }

    // wait so user does not have to stay tensed
    digitalWrite(LED_PIN, LOW);
    delay(1000 * NUM_SEC);
  }

  // wait 2 seconds then start low sequence
  Serial.println("Finished high");
  delay(2000);
  blink3(500);

  // get relaxed data for low
  // poll for min with light on
  digitalWrite(LED_PIN, HIGH);
  for(int j = 0; j < (SAMPLE_RATE * NUM_SEC * ITERATIONS); j++) {
    int value = getAnalogDisplay();
    if(value < cal.low_peak) {
      cal.low_peak = value;
    }
    avg_low_tot += value;
    delay(1000 / SAMPLE_RATE);
  }

  // turn off LED to signify end of data collection
  Serial.println("Finished low");
  digitalWrite(LED_PIN, LOW);

  // calculate averages
  cal.high_avg = avg_high_tot / (SAMPLE_RATE * ITERATIONS * NUM_SEC);
  cal.low_avg = avg_low_tot / (SAMPLE_RATE * ITERATIONS * NUM_SEC);

  // display results
  Serial.print("high_peak: ");
  Serial.println(cal.high_peak);
  Serial.print("high_avg: ");
  Serial.println(cal.high_avg);
  Serial.print("low_peak: ");
  Serial.println(cal.low_peak);
  Serial.print("low_avg: ");
  Serial.println(cal.low_avg);
  Serial.println();
  delay(2000);
}

void loop() {
  // display live data
  getAnalogDisplay();
  delay(10);
}

// reads the sensor value and prints the magnitude in "."
int getAnalogDisplay() {
  int value = analogRead(ANALOG_PIN);
  int dots = map(value, ANALOG_MIN, ANALOG_MAX, DOTS_MIN, DOTS_MAX);
  int i;
  for(i = 0; i < dots; i++) {
    Serial.print(".");
  }
  for(i = dots; i < DOTS_MAX; i++) {
    Serial.print(" ");
  }
  Serial.println("|");

  return value;
}

// blinks 3 times for a total time of (period * 3)
void blink3(int period) {
  // wait two seconds then blink three times to signify start
  digitalWrite(LED_PIN, HIGH);
  delay(period / 2);
  digitalWrite(LED_PIN, LOW);
  delay(period / 2);
  digitalWrite(LED_PIN, HIGH);
  delay(period / 2);
  digitalWrite(LED_PIN, LOW);
  delay(period / 2);
  digitalWrite(LED_PIN, HIGH);
  delay(period / 2);
  digitalWrite(LED_PIN, LOW);
  delay(period / 2);
}
