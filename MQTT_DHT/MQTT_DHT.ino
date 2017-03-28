// thanks 2 https://home-assistant.io/blog/2015/10/11/measure-temperature-with-esp8266-and-report-to-mqtt/
// https://gist.github.com/balloob/1176b6d87c2816bd07919ce6e29a19e9
// modified to publish a "state topic" after (re)connect
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define wifi_ssid "SSID"
#define wifi_password "SECRET_PASSWORD"

#define mqtt_server "192.168.0.27"
#define mqtt_client "esp8266_X"
#define mqtt_user "user"
#define mqtt_password "password"

#define humidity_topic "sensor_esp8266_XX/humidity"
#define temperature_topic "sensor_esp8266_XX/temperature"

#define DHTTYPE DHT22
#define DHTPIN  D1

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

void setup() {
  Serial.begin(74880);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect(mqtt_client, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      Serial.println("Sage dem MQTT Server Hallo!");
      client.publish(MQTT_STATUS_TOPIC, "hello world");
      // IP-Adresse umwandeln und publish
      IPAddress localAddr = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localAddr[0], localAddr[1], localAddr[2], localAddr[3]);
      client.publish(MQTT_STATUS_IP, s);
      client.publish(MQTT_STATUS_TOPIC, "Sensor aktiv (LED Switch)!", {retain: true});
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
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
  }
}
