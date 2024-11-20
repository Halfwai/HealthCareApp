// Code for the Carer node Of my Health Care System App
// Uses the following Libraries:
//     Wire
//     Adafruit_GFX
//     Adafruit_SH110X
//     ESP8266WiFi
//     ESP8266WebServer
// The node runs on an ESP8266 and has 4 components, an RGB LED, a buzzer, a button and an OLED Screen

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// OLED code taken from Adafruit sample code
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Setup webserver
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);
const char *ssid = "MyHome";
const char *password = "0932697344";

// Define pins
const int red_led_pin = D8;
const int green_led_pin = D6;
const int blue_led_pin = D5;
const int button = D4;
const int buzzer = D3;

// Variables to control alert
bool alertActivated = false;
unsigned long alertStartTime;
const unsigned long alertOffset = 1000;
int buzzerOn = LOW;



void setup()   {
    Serial.begin(9600);

    delay(250); // wait for the OLED to power up
    display.begin(i2c_Address, true); // Address 0x3C default  
    write("No new notifications");

    setupWebServer();    

    pinMode(red_led_pin, OUTPUT);
    pinMode(green_led_pin, OUTPUT);
    pinMode(blue_led_pin, OUTPUT);

    pinMode(button, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);
}

// Function to control the RGB LED taken from the course videos
void rgbLed(int red_led_amount, int green_led_amount, int blue_led_amount){
    analogWrite(red_led_pin, red_led_amount);
    analogWrite(green_led_pin, green_led_amount);
    analogWrite(blue_led_pin, blue_led_amount);
}


void loop() {
    checkButton();
    checkAlert();
    server.handleClient();
}

// Writes the input string onto the OLED Screen
void write(String s){
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 10);
    display.println(s);
    display.display();
}

// Checks if the alert has been activated and runs the code for the alert if so
void checkAlert(){
    if(alertActivated){
        // uses millis to time the buzzer without having to introduce delays into the circuit.
        if(millis() - alertStartTime > alertOffset){
            alertStartTime = millis();
            if(buzzerOn == LOW){
                buzzerOn = HIGH;
                rgbLed(255, 0, 0);
            } else {
                buzzerOn = LOW;
                rgbLed(255, 255, 0);
            }
            digitalWrite(buzzer, buzzerOn);
        }
    }
}

// Checks if the button has been pressed, and dismisses the alert if so
void checkButton(){
    int button_state = digitalRead(button);
    if(button_state == LOW){
        alertActivated = false;
        rgbLed(0, 0, 0);
        digitalWrite(buzzer, LOW);
        write("No new notifications");
    }
}

// Boilerplate webserver code
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

    // Route to listen for notifications
    server.on("/notify", handleNotification);

    server.begin();

    Serial.println("Server Listening");
}

// Function to handle a notification
void handleNotification(){
    if (server.hasArg("message")) {
        // Decides what to print to the OLED screen based on the message recieved
        String message = server.arg("message");
        Serial.println("Received message: " + message);
        // Starts the alert sequence
        alertActivated = true;
        alertStartTime = millis();
        if (message == "fall") {
            write("Fall Detected");
            return;
        }
        if (message == "alert") {
            write("Alert Button Pressed");
            return;
        }
        if (message == "meds") {
            write("Medication not taken");
            return;
        }
        if (message == "refill") {
            write("Please refill dispenser");
            return;
        }
        // If the message is outside of the normal bounds prints message to this effect.
        write("Unknown alert recieved");
    }
    server.send(200, "text/plain", "Notification received");
}
