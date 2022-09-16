
#define nmos 5

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Keypad.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <WiFiUdp.h>

#include "FS.h"

Servo myServo int i = 0;
const byte rows = 3;  // four rows
const byte cols = 3;  // three columns
char keys[rows][cols] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};
byte rowPins[rows] = {0, 9, 10};  // connect to the row pinouts of the keypad
byte colPins[cols] = {12, 13,
                      14};  // connect to the column pinouts of the keypad
Keypad pad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

// Update these with values suitable for your network.
String actual_otp;
int flag = 0;
String otp_entered = "";
int x = 1;  // 1.st = ld(locked); 2.st=(standby); 3.st=lda(locked-agian)
const char* topic_pub = "iot/safe/lock";
const char* topic_sub = "iot/safe/otp";
const char* ssid = "ONEPLUs7";
const char* password = "abcde12345";
const char* AWS_endpoint = "a3reu3z1cvzfbi-ats.iot.ap-south-1.amazonaws.com";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void on_sub(char* topic, byte* payload, unsigned int length) {
    const int capacity = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    char otp_arrived[128];
    Serial.print("Message Arrived on Topic: ");
    Serial.print(topic);
    Serial.print(":");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        otp_arrived[i] = (char)payload[i];
    }
    DeserializationError err = deserializeJson(doc, otp_arrived);
    Serial.println();
    if (err) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(err.c_str());
    }
    actual_otp = doc["otp"].as<String>();
    ;
    Serial.println(actual_otp);
    Serial.println("");
}

WiFiClientSecure espClient;
PubSubClient client(
    AWS_endpoint, 8883, on_sub,
    espClient);  // set MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    espClient.setBufferSizes(512, 512);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    delay(100);
    WiFi.begin(ssid, password);
    Serial.println("-----------:------------");
    Serial.println("Waiting for connection and IP Address from DHCP");
    while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    while (!timeClient.update()) {
        timeClient.forceUpdate();
    }
    espClient.setX509Time(timeClient.getEpochTime());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT Connection..");
        if (client.connect("ESPthing")) {
            Serial.println("Connected:");
            client.setCallback(on_sub);
            client.subscribe(topic_sub, 0);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            char buf[256];
            espClient.getLastSSLError(buf, 256);
            Serial.print("WiFiClientSecure SSL error: ");
            Serial.println(buf);
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    g = millis();

    pinMode(nmos, OUTPUT);
    digitalWrite(nmos, HIGH);
    myservo.attach(9);

    Serial.begin(115200);
    Serial.setDebugOutput(true);

    setup_wifi();

    delay(1000);
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }
    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());

    // Load certificate file
    File cert = SPIFFS.open(
        "/cert.der", "r");  // replace cert.crt eith your uploaded file name
    if (!cert) {
        Serial.println("Failed to open cert file");
    } else
        Serial.println("Success to open cert file");
    delay(1000);
    if (espClient.loadCertificate(cert))
        Serial.println("cert loaded");
    else
        Serial.println("cert not loaded");

    // Load private key file
    File private_key = SPIFFS.open(
        "/private.der", "r");  // replace private eith your uploaded file name
    if (!private_key) {
        Serial.println("Failed to open private cert file");
    } else
        Serial.println("Success to open private cert file");
    delay(1000);
    if (espClient.loadPrivateKey(private_key))
        Serial.println("private key loaded");
    else
        Serial.println("private key not loaded");

    // Load CA file
    File ca =
        SPIFFS.open("/ca.der", "r");  // replace ca eith your uploaded file name
    if (!ca) {
        Serial.println("Failed to open ca ");
    } else
        Serial.println("Success to open ca");
    delay(1000);
    if (espClient.loadCACert(ca))
        Serial.println("ca loaded");
    else
        Serial.println("ca failed");
    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());
}

void pub(int x) {
    char msg_pub[128];
    const int capacity = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    if (x == 1) doc["st"] = "ld";
    if (x == 2) doc["st"] = "sb";
    if (x == 3) doc["st"] = "lda";
    serializeJson(doc, msg_pub);
    Serial.println("Publishing Message to the Cloud: ");
    while (!client.publish(topic_pub, msg_pub)) {
        Serial.print(".");
    }
    Serial.println(msg_pub);
}

void loop() {
    f = millis();
    i++;
    char key;
    if (otp_entered.length() == 4) {
        otp_entered = "";
    }
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (x == 1) {
        if (flag == 0) {
            pub(x);
            flag = 1;
        }
        if (otp_entered.length() != 4 && actual_otp != "") {
            key = pad.getKey();
            if (key != NO_KEY) {
                otp_entered += key;
            }
        }
        if (actual_otp == otp_entered && actual_otp != "") {
            x = 2;
            pub(x);
            myservo.write(180);
            delay(15);
        }
    }
    if (x == 2) {
        x = 3;
        delay(10000);
        pub(x);
        digitalWrite(relay, LOW);
        digitalWrite(nmos, LOW);
    }

    if (i == 2)
        digitalWrite(nmos, LOW);
    else
        ((f - g) > 60000) digitalWrite(nmos, LOW);
}
