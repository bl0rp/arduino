// Thanks 2 https://alexbloggt.com/funksteckdosensteuerung-mit-esp8266/ modified for MQTT status updates
//data tf an D4
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <RCSwitch.h>
#include <PubSubClient.h>

// MQTT Settings
#define mqtt_server "192.168.0.27"
#define mqtt_user "homeassistant"
#define mqtt_password "homeassistant"

// MQTT Topics
#define status_topic "sensor_esp8266_01/status"
#define status_ip "sensor_esp8266_01/status_ip"
#define rfrx_topic "sensor_esp8266_01/433rfrx"
#define receive_topic "esp8266_01_receive"
WiFiClient espClient;
PubSubClient client(espClient);

RCSwitch mySwitch = RCSwitch();
MDNSResponder mdns;
// Replace with your network credentials
const char* ssid = "erisnet";
const char* password = "3pyADXXNVk3iBWbzIruuO1RrDagIJjj2EQ8CWEgN9vVN0MsqoTOwojKF2URKxiy";
ESP8266WebServer server(80);
// replace with your values
char* housecode = "11001";
char* socketcodes[] = {"10000", "01000", "00100", "00010"};
char* socketnames[] = {"MPD DOSE", "Schreibtischlicht", "Flurlicht", "Test"};
int numofsockets = sizeof(socketcodes)/4;
// you can write your own css and html code (head) here
String css = "body {background-color:#ffffff; color: #000000; font-family: 'Century Gothic', CenturyGothic, AppleGothic, sans-serif;}h1 {font-size: 2em;}";
String head1 = "<!DOCTYPE html> <html> <head> <title>Steckdosensteuerung</title> <style>";
String head2 = "</style></head><body><center>";
String header = head1 + css + head2;
String body = "";
String website(String h, String b){
  String complete = h+b;
  return complete;
}
void setup(void){
  // if you want to modify body part of html start here
  body = "<h1>Steckdosensteuerung ESP8266 #1</h1>";
  // socket names and buttons are created dynamical
  for(int i = 0; i < numofsockets; i++){
    String namesocket = socketnames[i];
    body = body + "<p>" + namesocket + " <a href=\"socket" + String(i) + "On\"><button>AN</button></a>&nbsp;<a href=\"socket" + String(i) + "Off\"><button>AUS</button></a></p>";
  }
  body += "</center></body>";
  mySwitch.enableTransmit(2);
  delay(1000);
  Serial.begin(74880);
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // serial output of connection details
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.println("Sage dem MQTT Server Hallo!");
  client.publish(status_topic, "hello world");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  // this page is loaded when accessing the root of esp8266´s IP
  server.on("/", [](){
    String webPage = website(header, body);
    server.send(200, "text/html", webPage);
  });
  // pages for all your sockets are created dynamical
  for(int i = 0; i < numofsockets; i++){
    String pathOn = "/socket"+String(i)+"On";
    const char* pathOnChar = pathOn.c_str();
    String pathOff = "/socket"+String(i)+"Off";
    const char* pathOffChar = pathOff.c_str();
    server.on(pathOnChar, [i](){
      String webPage = website(header, body);
      server.send(200, "text/html", webPage);
      mySwitch.switchOn(housecode, socketcodes[i]);
      delay(1000);
    });
    server.on(pathOffChar, [i](){
      String webPage = website(header, body);
      server.send(200, "text/html", webPage);
      mySwitch.switchOff(housecode, socketcodes[i]);
      delay(1000);
    });
  }
  server.begin();
  Serial.println("HTTP server started");
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Verbinde mit MQTT Server....");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("Verbunden");
      Serial.println("Sage dem MQTT Server Hallo!");
      client.publish(status_topic, "hello world");
      // IP-Adresse umwandeln und publish
      IPAddress localAddr = WiFi.localIP();
      char s[16];  
      sprintf(s, "%d.%d.%d.%d", localAddr[0], localAddr[1], localAddr[2], localAddr[3]);    
      client.publish(status_ip,s);
      client.publish(status_topic, "Sensor aktiv (RFRX 433Mhz)!", {retain: true});
      client.publish(rfrx_topic, "Steuerung über das Webinterface");
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
void loop(void){
  reconnect();
  server.handleClient();
}
