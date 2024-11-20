// Code for the Hub node Of my Health Care System App
// Uses the following Libraries:
//     ESP8266WiFi
//     ESP8266WebServer
//     Wire
//     Adafruit_Sensor
//     Adafruit_ADXL345_U
// The node runs on an ESP8266 and has 4 components, an accelerometer, a buzzer, a button and an LCD Screen

// Import libraries
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>

// Setup WiFi
const char *ssid = "MyHome";
const char *password = "0932697344";
const char* hubDeviceIP = "192.168.0.48"; 

ESP8266WebServer server(80);

// Accelerometer code adapted from the Adafruit example code
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Define Pins
const int led = D0;
const int button = D4;
const int buzzer = D5;

// Setup variables to store the previous position of the accelerometer
float prevX;
float prevY;
float prevZ;

// Alert variables
bool activateBuzzer = false;
bool flashLED = false;
int LEDOn;
unsigned long prevLedTime;
int LEDInterval = 1000;

// Const float to store the threshold for a fall to be detected
const float fallThreshold = 20;

void setup() {
    Serial.begin(9600);
    pinMode(button, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);
    pinMode(led, OUTPUT);

    // Initilize the accelerometer. Code taken from Adafruit example code
    if(!accel.begin()){
        Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
        while(1);
    }
    accel.setRange(ADXL345_RANGE_16_G);
    sensors_event_t event; 
    accel.getEvent(&event);

    setupWebServer();

    // Saves the starting position of the accelerometer
    prevX = event.acceleration.x;
    prevY = event.acceleration.y;
    prevZ = event.acceleration.z;
}

void loop() {
    server.handleClient();
    checkButton();
    getAccelData();
    checkLED();
}

// Flashes the LED if a notification is active
void checkLED(){
    if (flashLED){
        // Uses millis to time the led, so that no delays are introduced into the node
        if(millis() - prevLedTime > LEDInterval){
            LEDOn = (LEDOn == LOW) ? HIGH : LOW;
            digitalWrite(led, LEDOn);
            prevLedTime = millis();
        }
    }
}

// Checks if the alert button is pressed and sends an alert if so
void checkButton(){
    int button_state = digitalRead(button);
    if(button_state == LOW){
        sendNotificationToHub("alert");
        digitalWrite(led, LOW);
    } 
}

// Gets the data from the accelerometer and calculates the movement
void getAccelData(){
     /* Get a new sensor event */ 
    sensors_event_t event; 
    accel.getEvent(&event);
    
    // Calculates the distance between each of the vectors between loops
    float xMovement = abs(prevX - event.acceleration.x);
    float yMovement = abs(prevY - event.acceleration.y);
    float zMovement = abs(prevZ - event.acceleration.z);

    // Calculates the overall magnitude of the movement vector
    float accel = sqrt(sq(xMovement) + sq(yMovement) + sq(zMovement));
    // IF the magnitude exceeds the threshold, sends an alert
    if(accel > fallThreshold){
        digitalWrite(led, HIGH);
        sendNotificationToHub("fall");
        delay(1000);
        digitalWrite(led, LOW);
    }
    // resets the position of the accelerometer
    prevX = event.acceleration.x;
    prevY = event.acceleration.y;
    prevZ = event.acceleration.z;
}

// Boilerplate web server code
void setupWebServer(){
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Single route to listen for notifications
    server.on("/notify", handleNotification);

    server.begin();

    Serial.println("Server Listening");
}

// Function to handle the two different types of notification
void handleNotification(){
    if (server.hasArg("message")) {
        String message = server.arg("message");
        Serial.println("Received message: " + message);
        if (message == "reminder") {
            flashLED = true;
            LEDOn = LOW;
            prevLedTime = millis();
            digitalWrite(buzzer, HIGH);
            delay(1000);
            digitalWrite(buzzer, LOW);
        }
        if (message == "endReminder") {
            flashLED = false;
            digitalWrite(led, LOW);
        }
    }
    server.send(200, "text/plain", "Notification received");
}

// Sends a notificatio to the hub node with the input string as a query string
void sendNotificationToHub(String message) {
    WiFiClient client;
    if (client.connect(hubDeviceIP, 80)) {
        String url = "/notify?message=" + message;
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                    "Host: " + hubDeviceIP + "\r\n" +
                    "Connection: close\r\n\r\n");
        Serial.println("Notification sent to Device B with message: " + message);
    } else {
        Serial.println("Connection to Device B failed");
    }
    client.stop();
}
    
