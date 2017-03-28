// thanks 2 https://github.com/biacz
//include all libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//basic defines
#define MQTT_VERSION                                MQTT_VERSION_3_1_1
#define LED_PIN D0 // GPIO 16
#define LED_PIN2 D1 // GPIO 5
#define LED_PIN3 D2 // GPIO 4
//wifi credentials
const char* WIFI_SSID =                             "SSID";
const char* WIFI_PASSWORD =                         "SECRET_PASSWORD";

//mqtt variables
const PROGMEM char* MQTT_CLIENT_ID =                "esp8266_XX";
const PROGMEM char* MQTT_STATUS_TOPIC =             "sensor_esp8266_XX/status";
const PROGMEM char* MQTT_STATUS_IP =                "sensor_esp8266_XX/status_ip";
const PROGMEM char* MQTT_SERVER_IP =                "192.168.0.27";
const PROGMEM uint16_t MQTT_SERVER_PORT =           1883;
const PROGMEM char* MQTT_USER =                     "hauser";
const PROGMEM char* MQTT_PASSWORD =                 "hapasswd";
const PROGMEM char* MQTT_LED_STATE_TOPIC[] = {    "rot/status", "gelb/status", "gruen/status" };
const PROGMEM char* MQTT_LED_COMMAND_TOPIC[] = {  "rot/switch", "gelb/switch", "gruen/switch" };

boolean m_led_state[] = {                         false, false, false }; //variable to store LED states
const char* LED_ON =                              "ON";
const char* LED_OFF =                             "OFF";
// PIN ARRAY
const int ledPins[] = { 16, 5, 4 };

unsigned long wait = 30000;
unsigned long now = millis();
unsigned long last_millis = 0;

int lastState = LOW;

//initialize classes
WiFiClient wifiClient;
PubSubClient client(wifiClient);



void callback(char* p_topic, byte* p_payload, unsigned int p_length) { //handle mqtt callbacks
  String payload;
  for (uint8_t i = 0; i < p_length; i++) { //concatenate payload
    payload.concat((char)p_payload[i]);
  }
  Serial.println(p_topic);
  for (int i = 0; i < 3; i++) { //for loop to run though the MQTT_LED_COMMAND_TOPIC array. if the topic equals, switch the corresponding RC
    if (String(MQTT_LED_COMMAND_TOPIC[i]).equals(p_topic)) { //if topic matches
      if (payload.equals(String(LED_ON))) {
        m_led_state[i] = true; //set state for the corresponding LED
        digitalWrite(ledPins[i], HIGH);
        Serial.print("Gewaehlter Pin: ");
        Serial.println(ledPins[i]);
        Serial.print("INFO: Turn LED: ");
        Serial.print(MQTT_LED_COMMAND_TOPIC[i]);
        Serial.println(" on!");
        client.publish(MQTT_LED_STATE_TOPIC[i], LED_ON, true); //publish the state to mqtt
      }
      else if (payload.equals(String(LED_OFF))) {
        m_led_state[i] = false;
        digitalWrite(ledPins[i], LOW);
        Serial.print("Gewaehlter Pin: ");
        Serial.println(ledPins[i]);
        Serial.print("INFO: Turn LED: ");
        Serial.print(MQTT_LED_COMMAND_TOPIC[i]);
        Serial.println(" off!");
        client.publish(MQTT_LED_STATE_TOPIC[i], LED_OFF, true); //publish state to mqtt
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) { //loop until we're reconnected
    Serial.println("INFO: Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
      for (int i = 0; i < 3; i++) {
        client.subscribe(MQTT_LED_COMMAND_TOPIC[i]); //subscribe to all LED topics
      }
      Serial.println("Sage dem MQTT Server Hallo!");
      client.publish(MQTT_STATUS_TOPIC, "hello world");
      // IP-Adresse umwandeln und publish
      IPAddress localAddr = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localAddr[0], localAddr[1], localAddr[2], localAddr[3]);
      client.publish(MQTT_STATUS_IP, s);
      client.publish(MQTT_STATUS_TOPIC, "Sensor aktiv (LED Switch)!", {retain: true});
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      delay(5000); //wait 5 seconds before retrying
    }
  }
}

void setupWifi() {
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //connect to wifi

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(WiFi.status());
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());
}




void setup() {
  Serial.begin(115200); //init the serial
  setupWifi();
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LED_PIN2, OUTPUT);
  digitalWrite(LED_PIN2, LOW);
  pinMode(LED_PIN3, OUTPUT);
  digitalWrite(LED_PIN3, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
