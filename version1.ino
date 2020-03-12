#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#define LED (2)
#define MAX_MSG_LEN (128)
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


/* * * * * * Calibration and output code* * * * * */
struct calibration {
  int high_peak = ANALOG_MIN;
  int high_avg = ANALOG_MIN;
  int low_peak = ANALOG_MAX;
  int low_avg = ANALOG_MAX;
};

// calibration object
struct calibration cal;

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




/* * * * * MQTT code * * * * */
//wifi config
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

// MQTT Configuration
//const char *serverHostname = "your MQTT server hostname";
const IPAddress serverIPAddress(192, 168, 1, 9);

// the topic we want to use
const char *topic = "test/message";

//create the MQTT and WIFI stacks
WiFiClient espClient;
PubSubClient client(espClient);


// connect to wifi
void connectWifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.printf("\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected on IP address ");
  Serial.println(WiFi.localIP());
}

// connect to MQTT server
void connectMQTT() {
  // Wait until we're connected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
    Serial.printf("MQTT connecting as client %s...\n", clientId.c_str());
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT connected");
      // Once connected, publish an announcement...
      client.publish(topic, "hello from ESP8266");
      // ... and resubscribe
      client.subscribe(topic);
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
      // Wait before retrying
      delay(2500);
    }
  }
}

void callback(char *msgTopic, byte *msgPayload, unsigned int msgLength) {
  // copy payload to a static string
  static char message[MAX_MSG_LEN+1];
  if (msgLength > MAX_MSG_LEN) {
    msgLength = MAX_MSG_LEN;
  }
  strncpy(message, (char *)msgPayload, msgLength);
  message[msgLength] = '\0';
  
  Serial.printf("topic %s, message received: %s\n", topic, message);

  // decode message
  if (strcmp(message, "off") == 0) {
    setLedState(false);
  } else if (strcmp(message, "on") == 0) {
    setLedState(true);
  }
}

void setLedState(boolean state) {
  // LED logic is inverted, low means on
  digitalWrite(LED, !state);
}





/* * * * * Setup and Loop code * * * * */
void setup() {
  // intialize pins and serial connection
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);
  // Initialise wifi connection - this will wait until connected
  connectWifi();
  // connect to MQTT server  
  client.setServer(serverIPAddress, 1883);
  client.setCallback(callback);

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

int flag = 0;

void loop() {
  // display live data
  int output = getAnalogDisplay();
  
  if (!client.connected()) {
    connectMQTT();
  }
  // this is ESSENTIAL!
  client.loop();
  
  String msg = "";
  if (output > 3000){
    msg += "ACTIVE";
    if (flag >= 1){
      flag = 2;
    }
    else{
      flag = 1;
    }
  }
  else {
    flag = 0;
  }
  
  char msgBuf[1000];
  msg.toCharArray(msgBuf, 100);
  Serial.printf("read output: \"%s\"\n", msgBuf);
  if (flag == 1){
    client.publish(topic, msgBuf);
  }

  // check for message and send if available
  if(Serial.available()) {
    String msg = "";
    msg += Serial.read();
    char msgBuf[1000];
    msg.toCharArray(msgBuf, 100);
    Serial.printf("Trying to send: \"%s\"\n", msgBuf);
    client.publish(topic, msgBuf);
  }
  // idle
  delay(500);
}
