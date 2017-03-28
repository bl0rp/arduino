// DHT -> D1, PIR -> D2
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi Settings
#define wifi_ssid "SSID"
#define wifi_password "SECRET_PASSWORD"

// MQTT Settings
#define mqtt_server "192.168.0.27"
#define mqtt_client "esp8266_XX"
#define mqtt_user "hauser"
#define mqtt_password "hapasswd"

// MQTT Topics
#define status_topic "sensor_esp8266_XX/status"
#define status_ip "sensor_esp8266_XX/status_ip"
#define humidity_topic "sensor_esp8266_XX/humidity"
#define temperature_topic "sensor_esp8266_XX/temperature"
#define motion_topic "sensor_esp8266_XX/motion"
#define receive_topic "esp8266_XX_receive"

// Temperature Sensor 
#define DHTTYPE DHT22
#define DHTPIN  D1

#define PIRPIN D2
#define LEDPIN D8

//Var aus PIR
//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
int calibrationTime = 30;

//the time when the sensor outputs a low impulse
long unsigned int lowIn;

//the amount of milliseconds the sensor has to be low
//before we assume all motion has stopped
long unsigned int pause = 5000;

boolean lockLow = true;
boolean takeLowTime;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("MQTT push an das receive topic: [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(74880);
  pinMode(PIRPIN, INPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(PIRPIN, LOW);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.subscribe(receive_topic);
  //give the sensor some time to calibrate
  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" done");
  Serial.println("SENSOR ACTIVE");
  delay(50);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Verbinde mit: ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi verbunden");
  Serial.println("Die IP lautet: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Verbinde mit MQTT Server....");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect(mqtt_client, mqtt_user, mqtt_password)) {
      Serial.println("Verbunden");
      Serial.println("Sage dem MQTT Server Hallo!");
      client.publish(status_topic, "hello world");
      // IP-Adresse umwandeln und publish
      IPAddress localAddr = WiFi.localIP();
      char s[16];  
      sprintf(s, "%d.%d.%d.%d", localAddr[0], localAddr[1], localAddr[2], localAddr[3]);    
      client.publish(status_ip,s);
      client.publish(status_topic, "Sensor aktiv (PIR/DHT)! ");
      client.publish(motion_topic, "off");
      // ...  resubscribe an:
      client.subscribe(receive_topic);
    } else {
      Serial.print("Geht nicht :/, rc=");
      Serial.print(client.state());
      Serial.println(" Warte 5 Sekunden bis zum erneuten Versuch...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
// Bei welcher Abweichung soll an den MQTT Server gesendet werden?
float diff = 0.2;



void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
      if (digitalRead(PIRPIN) == HIGH) {
    digitalWrite(LEDPIN, HIGH);   //the led visualizes the sensors output pin state
    if (lockLow) {
      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;
      Serial.println("---");
      Serial.print("motion detected at "); 
      //tring this out
      client.publish(motion_topic, "on");
      Serial.print(millis() / 1000);
      Serial.println(" sec");
      delay(50);
    }
    takeLowTime = true;
  }

  if (digitalRead(PIRPIN) == LOW) {
    digitalWrite(LEDPIN, LOW);  //the led visualizes the sensors output pin state

    if (takeLowTime) {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    //if the sensor is low for more than the given pause,
    //we assume that no more motion is going to happen
    if (!lockLow && millis() - lowIn > pause) {
      //makes sure this block of code is only executed again after
      //a new motion sequence has been detected
      lockLow = true;
      client.publish(motion_topic, "off");
      Serial.print("motion ended at ");      //output
      Serial.print((millis() - pause) / 1000);
      Serial.println(" sec");
      delay(50);
    }
  }
  }
}
