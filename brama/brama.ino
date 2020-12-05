//Libraries for push notification via Pushsafer
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Pushsafer.h>

#include <Timers.h>
#include <Bounce2.h>

#define PILOT 10
#define SENSOR_UP 9
#define SENSOR_DOWN 8
#define OBSTACLE 7
#define LIGHT_YELLOW 2
#define LIGHT_RED 6
#define MOTOR_UP 3
#define MOTOR_DOWN 4
#define SERVICE 5

volatile int State; //0 when last state was closing, 1 when last state was opening
int PreviousState;
bool AutoClose; // If true closing after GATE_OPENED_TIME is active
bool KeepOpened;  // If true the gate keeps being opened
const int GATE_OPENED_TIME = 5000; // Here we set time for the gate to stay opened
Bounce PilotRisingEgde = Bounce(PILOT, 50);
Bounce SensorUpRisingEgde = Bounce(SENSOR_UP, 50);
Timer Timer1;
//|| digitalRead(SENSOR_UP)== HIGH || digitalRead(SENSOR_DOWN)== HIGH || digitalRead(OBSTACLE)== HIGH

// Initialize Wifi connection to the router
char ssid[] = "house_wifi";     // your network SSID (name)
char password[] = "123456"; // your network key
// Pushsafer private or alias key
#define PushsaferKey "XXXXXXXXXXXXX"
//Creating connection with pushsafer
WiFiClient client;
Pushsafer pushsafer(PushsaferKey, client);



void setup() {
  pinMode(PILOT, INPUT);
  pinMode(SENSOR_UP, INPUT);
  pinMode(SENSOR_DOWN, INPUT);
  pinMode(OBSTACLE, INPUT);
  pinMode(SERVICE, INPUT);
  pinMode(LIGHT_YELLOW, OUTPUT);
  pinMode(LIGHT_RED, OUTPUT);
  pinMode(MOTOR_UP, OUTPUT);
  pinMode(MOTOR_DOWN, OUTPUT);
  int liczba_uruchomien;

  State = 1;
  PreviousState = 0; //0 when last state was closing, 1 when last state was opening
  Timer1.begin(GATE_OPENED_TIME);
  Serial.begin(9600);// Simulate information for user.

//Service control
  int Service(){
  liczba_uruchomien = liczba_uruchomien +1;
  if(liczba_uruchomien == 100){
    Serial.println(pushsafer.sendEvent(input1));
    }
  return liczba_uruchomien;
  }

  int Service_reset(){
  liczba_uruchomien = 0
  return liczba_uruchomien;

//Lights control
  int Light_alarm(){
    digitalWrite(LIGHT_YELLOW, HIGH);
    digitalWrite(LIGHT_RED, LOW);
    delay(1000);
    digitalWrite(LIGHT_RED, HIGH);
    digitalWrite(LIGHT_YELLOW, LOW);
    delay(1000);
  }

  int Light_alarm_stop(){
    digitalWrite(LIGHT_YELLOW, LOW);
    digitalWrite(LIGHT_RED, LOW);
  }

//WiFi configuration and functions
// Set WiFi to station mode and disconnect from an AP if it was Previously
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Error, problem z WiFi");
    delay(500);
  }

  //Information about connection
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pushsafer.debug = true;
  //Definition of push message.
  struct PushSaferInput input,input1;
  input.message = "Ktoś otworzył bramę!";
  input1.message = "Wymagane smarowanie.";
  input.title = "Warning";
  input1.title = "Service_information";
  input.sound = "10";
  input1.sound = "5";
  input.vibration = "1";
  input1.vibration = "1";
  input.icon = "1";
  input1.icon = "1";
  input.iconcolor = "#FFCCCC";
  input1.iconcolor = "#FFCCCC";
  input.priority = "1";
  input1.priority = "2";
  input.device = "phone";
  input1.device = "phone";
  input.url = "https://www.pushsafer.com";
  input1.url = "https://www.pushsafer.com";
  input.urlTitle = "Open Pushsafer.com";
  input1.urlTitle = "Open Pushsafer.com";

  // Specify IP address to ping
  String hostName = 127.*.*.**;
  int pingResult;

  //Push notification function
  int Send_message(){
    pingResult = WiFi.ping(hostName);

    if (pingResult < 0) {
    Serial.println(pushsafer.sendEvent(input));
  }
  }
}

void loop() {
  // Service
  if(digitalRead(SERVICE) == HIGH){
      Service_reset();
    }
  //Long press detection
  if (PilotLongPress.risingEdge()){ // if 1000ms press on pilot was detected set KeepOpened
    KeepOpened = true;
  }
  //States of work
  switch(State){

   case 1: //MOTOR STOP
     digitalWrite(MOTOR_UP,LOW);
     digitalWrite(MOTOR_DOWN,LOW);
     Light_alarm_stop();
     //digitalWrite(LIGHT,LOW);

      if(PilotRisingEgde.risingEdge() && PreviousState == 0) {
        State=2;
     }
      else if (PilotRisingEgde.risingEdge() && PreviousState == 1){
        State=3;
        Send_message();//chceking if defined ip is available if not sending alert
     }

      if(AutoClose){ // If AutoClose is active

      if(Timer1.available()){ // True when time has elapsed - ofter that close the gate and reset AutoClose
       State = 3;
       AutoClose = false;
      }
     }
   break;

   case 2: //MOTOR UP (opening)
    PreviousState = 1;
    digitalWrite(MOTOR_UP,HIGH);
    digitalWrite(MOTOR_DOWN,LOW);
    Light_alarm();
    //digitalWrite(LIGHT,HIGH);
    Timer1.restart();
    Service();// counting of actions
    if(PilotRisingEgde.risingEdge()){
      State = 1;
    }

    if(digitalRead(SENSOR_UP) == HIGH && !KeepOpened){ // If gate reached SENSOR_UP and KeepOpened is false (Pilot button was pressed shorter than 1000ms) closr the gate automatically after GATE_OPENED_TIME
      State = 1;
      AutoClose = true;
    }
    else if(digitalRead(SENSOR_UP) == HIGH && KeepOpened){ // If gate reached SENSOR_UP and KeepOpened is true go to STOP state. Gate will stay opened, because  AutoClose is false
      State = 1;
      KeepOpened = false; // Reset KeepOpened
    }
   break;

   case 3: //MOTOR DOWN (closing)
    PreviousState = 0;
    digitalWrite(MOTOR_UP,LOW);
    digitalWrite(MOTOR_DOWN,HIGH);
    Light_alarm();
    Service();// counting of actions
    //digitalWrite(LIGHT,HIGH);

    if(PilotRisingEgde.risingEdge()){
      State = 1;
    }

    if(digitalRead(SENSOR_DOWN) == HIGH){
      State = 1;
    }

    if(digitalRead(OBSTACLE) == LOW){
      State = 1;
    }

   break;

}

PilotRisingEgde.update();
PilotLongPress.update();
}
