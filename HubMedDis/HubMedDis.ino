// Code for the Hub node Of my Health Care System App
// Uses the following Libraries:
//     ESP8266WiFi
//     ESP8266WebServer
//     LiquidCrystal_I2C
//     Servo
//     time
// The node runs on an ESP8266 and has 3 components, a servo motor, a button and an LCD Screen

// Wifi modules
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>

// LCD module
#include <LiquidCrystal_I2C.h>

// Servo module
#include <Servo.h>

const char* wearableDeviceIP = "192.168.0.81"; 
const char* carerDeviceIP = "192.168.0.24";


// Time code adapted from https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
#include <time.h> 
#define MY_NTP_SERVER "tw.pool.ntp.org"           
#define MY_TZ "CST-8"
time_t now;                         // this are the seconds since Epoch (1970) - UTC
tm tm;

// Define days of the week
char daysOfTheWeek[7][6] = {"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"};

// Struct to hold reminder data
struct reminder{
    String type;
    int year;
    int month;
    int day;
    int hour;
    int min;
};

// Setup webserver
ESP8266WebServer server(80);

// Setup LCD screen
LiquidCrystal_I2C lcd(0x27, 16, 2);
int topRowStart = 0;
int bottomRowStart = 0;

// setup Servo
Servo myservo;

// Define dispenser variables
bool dispense = false;
int servoPos = 0;
int servoTarget = 0;

// Define wifi variables
const char *ssid = "MyHome";
const char *password = "0932697344";


// Array to hold reminders
reminder reminders[20];
// Store the location of last reminder
int reminderPos = 0;

// Array to hold alerts
reminder alerts[20];
// Store the location of last alerts
int alertPos = 0;

// Variables for alerts
unsigned long alertTimerStart;
bool checkAlertTimer = false;
unsigned long alertTimerOffset = 600000;

// Notification to be shown to user on webserver
String notification = "";

// Boolean to update webserver
bool updateWebserver = false;

// Default user name
String name = "User";

// define pins
const int buttonPin = D4;
const int servoPin = D8;

// Web page code. Has 3 pieces of data loaded dynamically, the username, notification data and alert data.
const char dashboard_html[] PROGMEM = R"rawliteral(
<html>

<head>
    <meta name="viewport" content="width-device-width, initial-scale=1.0">
    <style>
        body {
            margin: 0;
            display: flex;
            width: 100%;
            flex-direction: column;
            font-size: larger;
        }

        #header {
            background-color: #4054a6;
            width: 100%;
            padding-left: 50px;
            color: white;
        }

        #notification_container {
            padding-left: 20px;
            background-color: burlywood;
        }

        #notification_text {
            margin: 0px;
        }

        #main_container {
            display: flex;
            padding: 20px
        }

        #left_side {
            justify-content: center;
            align-items: center;
            flex-direction: row;
            width: 60%;
        }

        #name_container {
            display: flex;
            padding: 10;
            justify-content: center;
            align-items: center;
        }

        #name_input {
            height: 30;
        }

        .name_item {
            margin: 10
        }

        .submit_button {
            padding: 10;
            border-radius: 10px;
        }

        #schedule_form {
            display: flex;
            justify-content: center;
            align-items: flex-end;
        }

        .input_container {
            display: flex;
            flex-direction: column;
            width: 20%;
            margin: 2.5%
        }

        table {
            width: 100%;
        }

        tr:nth-child(even) {
            background-color: #D6EEEE;
        }

        #right_side {
            width: 40%;
            display: flex;
            flex-direction: column;
        }
    </style>
</head>

<body>
    <div id="header">
        <h1>Health Care System</h1>
    </div>

    <main>
        <div id="notification_container">
           <p id="notification_text">%NOTIFICATION%</p> 
        </div>
        <div id="main_container">
            <div id="left_side">
                <form action="changeName" method="get">
                    <div id="name_container">
                        <p class="name_item">Primary Users Name: </p>
                        <input type="text" id="name_input" class="name_item" name="name" value="%USER%" />
                        <input type="submit" value="Change Name" class="name_item submit_button" />
                    </div>
                </form>
                <form action="addToSchedule" method="get" id="schedule_form">
                    <div class="input_container">
                        <label for="type">Type of reminder</label>
                        <select id="type" name="type">
                            <option value="meal">Meal</option>
                            <option value="medicine">Medicine</option>
                        </select>
                    </div>
                    <div class="input_container">
                        <label for="date">Date:</label>
                        <input type="date" id="date" name="date">
                    </div>
                    <div class="input_container">
                        <label for="time">Time:</label>
                        <input type="time" id="time" name="time">
                    </div>
                    <input type="submit" value="Add To Schedule" class="name_item submit_button" />
                </form>
                <table>
                    <tr>
                        <th>Type</th>
                        <th>Date</th>
                        <th>Time</th>
                    </tr>
                    %REMINDERDATA%
                </table>
            </div>
            <div id="right_side">
                <h2>Alerts</h2>
                %ALERTDATA%
            </div>
        </div>
        <script>
            setInterval(function() {
                fetch('/check')
                    .then(response => response.text())
                    .then(data => {
                        if (data) {  // If there is a response, execute it
                            eval(data);
                        }
                    })
                    .catch(err => console.error('Error checking for updates:', err));
            }, 1000);  // Check every 5 seconds
        </script>
    </main>
</body>
</html>
)rawliteral";

void setup() {

    Serial.begin(9600);

    // Setup time servers
    configTime(MY_TZ, MY_NTP_SERVER);

    // Setup lcd screen
    lcd.init();
    lcd.backlight();

    // Setup pins
    pinMode(buttonPin, INPUT_PULLUP);
    myservo.attach(servoPin, 550, 2400);
    
    // resets servo to default position
    myservo.write(servoPos);

    // Sets up the webserver
    setupWebServer();    
}

void loop() {
    server.handleClient();
    getTime();
    checkSchedule();
    displayData();
    buttonCheck();
    checkAlert();
    rotateServo();
}


// Webserver adapted from the HelloServer
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

    // define routes
    server.on("/", get_index);
    server.on("/changeName", changeName);
    server.on("/addToSchedule", addToSchedule);
    server.on("/check", handleUpdateCheck);
    server.on("/notify", handleNotification);    

    server.begin();
    Serial.println("Server Listening");
}

// Gets the current time
void getTime(){
    time(&now);
    localtime_r(&now, &tm);
    delay(500);
}

// Checks the reminders array to see if it's time to activate it.
void checkSchedule() {    
    if(reminderPos != 0){
        if(checkTime(reminders[0])){
            // if the reminder is a medicine reminder sets the alert timer up.
            if(reminders[0].type == "medicine" && !checkAlertTimer){
                alertTimerStart = millis();
                checkAlertTimer = true;
            }
            // Sends the notification to the wearable device.
            sendNotification("reminder", wearableDeviceIP);
            alerts[alertPos] = reminders[0];
            Serial.println(alerts[alertPos].type);
            alertPos++;
            // Clears the reminder from the reminders list
            for(int i = 0; i < reminderPos - 1; i++){
                reminders[i] = reminders[i+1];
            }
            reminderPos--;
            updateWebserver = true;
        }
    }
}

// Checks the alert timer to see if it has overrun. Sends an alert to the carer device so.
void checkAlert(){
    if(checkAlertTimer){
        if(millis() - alertTimerStart > alertTimerOffset){
            sendNotification("meds", carerDeviceIP);
            checkAlertTimer = false;
        }
    }
}

// Handles functionality for the touch sensor
void buttonCheck(){
    int sensorPressed = digitalRead(buttonPin);
    Serial.println(sensorPressed);
    if(sensorPressed == LOW){
        if(alertPos != 0){
            // Stops the alert timer if the user is taking medicine
            if(alerts[0].type == "medicine"){
                Serial.println("Dispense");
                checkAlertTimer = false;
                servoTarget += 45;
            }
            removeAlert();
            updateWebserver = true;
            // Checks to see if any other alerts are related to medicine, and restarts the timer if so
            for(int i = 0; i < alertPos; i++){
                if (alerts[i].type == "medicine"){
                    alertTimerStart = millis();
                    checkAlertTimer = true;
                    break;
                }
            }
            // Sends a notificaiton to the wearable device if there are no more notifications.
            if(alertPos == 0){
                sendNotification("endReminder", wearableDeviceIP);               
            }
        }   
    }
}

void rotateServo(){
    while(servoPos < servoTarget){
        servoPos++;
        myservo.write(servoPos);
    }
    // If the servo has rotated 180 degrees, sends a notification to the carer to refill the device
    if(servoPos == 180){
        delay(1000);
        servoPos = 0;
        servoTarget = 0;
        myservo.write(servoPos);
        sendNotification("refill", carerDeviceIP);
    }
}

// Removes alert from the alerts array
void removeAlert(){
    bool remove = false;
    for(int i = 0; i < alertPos; i++){
        if (!remove){
            remove = true;
            alertPos--;
        }
        if (remove){
            alerts[i] = alerts[i+1];
        }
    }
}

// Handles the LCD display
void displayData(){
    String topRow = "";
    String bottomRow = "";
    // If no notifications, says hi to the user and displays the time;
    if(alertPos == 0){
        topRow += "Hi " + name;
        bottomRow += String(daysOfTheWeek[tm.tm_wday]) + " " + String(tm.tm_hour) + ":" + String(tm.tm_min) +  ":" + String(tm.tm_sec);
    // Else displays the oldest notification
    } else {
        if(alerts[0].type == "medicine"){
            topRow += "Med reminder!";
            bottomRow += "Press button to dispense your medication";
        } else {
            topRow += "Meal reminder!";
            bottomRow += "Press button to dismiss";
        }
    }
    // If the strings are too long for the display, this code makes the screen scroll through it
    if(topRow.length() - topRowStart > 16){
        topRowStart++;
    } else {
        topRowStart = 0;
    }
    String formattedTopRow = topRow.substring(topRowStart);
    if(bottomRow.length() - bottomRowStart > 16){
        bottomRowStart++;
    } else {
        bottomRowStart = 0;
    }
    String formattedBottomRow = bottomRow.substring(bottomRowStart);
    // Prints to the display
    lcd.clear();
    lcd.print(formattedTopRow);
    lcd.setCursor(0, 1);
    lcd.print(formattedBottomRow);
}

// Allows the user to set up the users name on their display
void changeName(){
    notification = "";
    if (server.arg("name") != ""){
        name = server.arg("name");
    } 
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
}

// Adds a reninder the schedule
void addToSchedule(){
    notification = "";
    if (server.arg("date") != "" && server.arg("time") != ""){
        String type = server.arg("type");
        int year = server.arg("date").substring(0, 4).toInt();
        int month = server.arg("date").substring(5, 7).toInt();
        int day = server.arg("date").substring(8).toInt();
        int hour = server.arg("time").substring(0, 2).toInt();
        int minute = server.arg("time").substring(3).toInt();
        reminder newReminder = {type, year, month, day, hour, minute};
        // Validates the input to make sure it's a future time/date
        if(checkTime(newReminder)){
            notification = "Please pick future time and date";
        } else {
            if(reminderPos == 0){
                reminders[reminderPos] = newReminder;
            } else {
                insertReminder(newReminder);
            }        
            reminderPos++;
        }
    } else {
        notification = "Please enter schedule data again";
    }
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
}

// Inserts a reminder in the correct position in the reminders array
void insertReminder(reminder r){
    reminder insertedReminder = r;
    for(int i = 0; i < reminderPos; i++){
        if(compareDates(insertedReminder, reminders[i])){
            reminder tempReminder = reminders[i];
            reminders[i] = insertedReminder;
            insertedReminder = tempReminder;
        }
    }
    reminders[reminderPos] = insertedReminder;
}

// creates a reminder object with the current time and compares it with a reminder to see if the time has elapsed
bool checkTime(reminder r){
    reminder now = {"", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min};
    return compareDates(r, now);
}

// Returns true if newReminder is after oldReminder. If times are the same, also returns true
bool compareDates(reminder newReminder, reminder oldReminder){
    if(newReminder.year > oldReminder.year){
        return false;
    } else if (newReminder.year < oldReminder.year){
        return true;
    }
    if(newReminder.month > oldReminder.month){
        return false;
    } else if(newReminder.month < oldReminder.month){
        return true;
    }
    if(newReminder.day > oldReminder.day){
        return false;
    } else if(newReminder.day < oldReminder.day){
        return true;
    }
    if(newReminder.hour > oldReminder.hour){
        return false;
    } else if(newReminder.hour < oldReminder.hour){
        return true;
    }
    if(newReminder.min > oldReminder.min){
        return false;
    } else if(newReminder.min < oldReminder.min){
        return true;
    }
    return true;
}

// Displays webserver page
void get_index(){
    String html = FPSTR(dashboard_html);
    String reminderData = "";    
    for(int i = 0; i < reminderPos; i++){
        reminderData += "<tr><td>" + reminders[i].type +"</td><td>" + getDateString(reminders[i]) + "</td><td>" + getTimeString(reminders[i]) + "</td></tr>";
    }
    html.replace("%REMINDERDATA%", reminderData);

    String alertData = "";
    for(int i = 0; i < alertPos; i++){
        alertData += "<ul><li>" + alerts[i].type + " reminder issued on " + getDateString(alerts[i]) + " at " + getTimeString(alerts[i]) + "</li></ul>";
    }
    // Loads dynamic data
    html.replace("%ALERTDATA%", alertData);
    html.replace("%USER%", String(name));
    html.replace("%NOTIFICATION%", notification);
    server.send(200, "text/html", html);
}

// Creates a string of reminder date
String getDateString(reminder r){
   return String(r.day) + "/" + String(r.month) + "/" + String(r.year);
}

// Creates a string of the reminder time
String getTimeString(reminder r){
    String time = String(r.hour) + ":";
    if(r.min < 10){
        time += "0";
    }
    time += String(r.min);
    return time;
}

// Lets the web page know if an update is needed
void handleUpdateCheck() {
    if (updateWebserver) {
        String response = "location.reload();";
        server.send(200, "text/plain", response);
        updateWebserver = false;  // Reset the flag after notifying the client
    } else {
        // If no update is needed, just send a blank response
        server.send(200, "text/html", "");
    }
}

// Decides what to do when a notification is recieved from the wearable device
void handleNotification() {
    if (server.hasArg("message")) {
        String message = server.arg("message");
        Serial.println("Received message: " + message);
        if (message == "fall") {
            sendNotification("fall", carerDeviceIP);
        }
        if (message == "alert") {
            sendNotification("alert", carerDeviceIP);
        }
    }
    server.send(200, "text/plain", "Notification received");
}

// Sends notifications code adapted from HTTPSRequests example
void sendNotification(String message, const char* device) {
    WiFiClient client;
    if (client.connect(device, 80)) {
        String url = "/notify?message=" + message;
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                    "Host: " + device + "\r\n" +
                    "Connection: close\r\n\r\n");
        Serial.println("Notification sent: " + message);
    } else {
        Serial.println("Connection failed");
    }
    client.stop();
}