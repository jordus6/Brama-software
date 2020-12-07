#include <Timers.h>
#include <Bounce2.h>
#include <IRremote.h> // library for IR reader

#define RECIEVEPIN 10
#define SENSOR_UP 9
#define SENSOR_DOWN 8
#define OBSTACLE 7
#define LIGHT 2
#define MOTOR_UP 3 
#define MOTOR_DOWN 4

IRrecv irrecv(RECIEVEPIN); 
decode_results results;
volatile int State       = 1;
const long int PILOTCODE = 0000000000; // Code on the pilot
int PreviousState        = 0; //0 when last state was closing, 1 when last state was opening
bool AutoClose; // If true closing after GATE_OPENED_TIME is active
bool KeepOpened;  // If true the gate keeps being opened 
bool PilotSignal; // true when recieved right code from IR reader
const int GATE_OPENED_TIME = 5000; // Here we set time for the gate to stay opened
Bounce PilotRisingEgde = Bounce(PilotSignal, 50); // Object that detects edges after 50ms
Bounce PilotLongPress = Bounce(PilotSignal, 1000); // Object that detects edges after 1000ms
Timer Timer1;

void setup() {
  // Set pin modes
  //pinMode(PILOT, INPUT);
  pinMode(SENSOR_UP, INPUT);
  pinMode(SENSOR_DOWN, INPUT);
  pinMode(OBSTACLE, INPUT);
  pinMode(LIGHT, OUTPUT); 
  pinMode(MOTOR_UP, OUTPUT); 
  pinMode(MOTOR_DOWN, OUTPUT); 
  Timer1.begin(GATE_OPENED_TIME); // Time to keep gate open
  irrecv.enableIRIn(); // Enable IR reader
}

void loop() {
  // put your main code here, to run repeatedly:
  if(irrecv.decode(&results) && results.value == PILOTCODE){ // If recieved value corresponds value on the pilot set PilotSignal variable
    PilotSignal = true;
  }
  else{
    PilotSignal = false;
  }

  if (PilotLongPress.risingEdge()){ // if 1000ms press on pilot was detected set KeepOpened
    KeepOpened = true;
  }
   if (KeepOpened){ // Just to indicate KeepOpened state - for testing only
    digitalWrite(LIGHT,HIGH);
  }
  else{
    digitalWrite(LIGHT,LOW);    
  }
  
  switch(State){

   case 1: //MOTOR STOP
   
     digitalWrite(MOTOR_UP,LOW); //Stop the motor
     digitalWrite(MOTOR_DOWN,LOW);
     //digitalWrite(LIGHT,LOW);

      if(PilotRisingEgde.risingEdge() && PreviousState == 0) { // If signal on the pilot detected and in previous state gate was closing
        State = 2;
     } 
      else if (PilotRisingEgde.risingEdge() && PreviousState == 1){ // If signal on the pilot detected and in previous state gate was opening
        State = 3;
     }

     if(AutoClose){ // If AutoClose is active 
      
      if(Timer1.available()){ // True when time has elapsed - ofter that close the gate and reset AutoClose
       State = 3;
       AutoClose = false;
      }
     }

   break;

   case 2: //MOTOR UP (opening)
   
    PreviousState = 1; // Set PreviousState on 1 (opening)
    digitalWrite(MOTOR_UP,HIGH); // Open the gate
    digitalWrite(MOTOR_DOWN,LOW);
    Timer1.restart(); // Restart the timer when OPENING state is active - when program moves to STOP state timer starts (stops resetting)
    if(PilotRisingEgde.risingEdge()){ // When signal from the pilot detected
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
    digitalWrite(MOTOR_UP,LOW); // Close the gate
    digitalWrite(MOTOR_DOWN,HIGH);

    if(PilotRisingEgde.risingEdge()){ // If signal from pilot detected stop the motor (go to STOP state)
      State = 1;
    }

    if(digitalRead(SENSOR_DOWN) == HIGH){ // If signal from SENSOR_DOWN detected stop the motor
      State = 1;
    }

    if(digitalRead(OBSTACLE) == LOW){ // If obstacle detected stop the motor
      State = 1;
    }
    
   break;

}  

//Update objects so they can detest rising of falling egdes
PilotRisingEgde.update();
PilotLongPress.update();

}
