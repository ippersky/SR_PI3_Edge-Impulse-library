// How to control the RGB Led and Power Led of the Nano 33 BLE boards.  
#include "Arduino.h"
#include "Switchable.h"
#include "VibrationMotor.h"

#define RED 22     
#define BLUE 24     
#define GREEN 23
#define VM1 3
#define VM2 4
#define BTN1 8
#define BTN2 7

VibrationMotor vibrationMotor1(VM1);
VibrationMotor vibrationMotor2(VM2);

void setup() {
 
 // intitialize the digital Pin as an output
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(VM1, OUTPUT);
  pinMode(VM2, OUTPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);

  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, HIGH);

}

// the loop function runs over and over again
void loop() {
  if(digitalRead(BTN1) == HIGH){
  
    digitalWrite(RED, LOW); // turn the LED off by making the voltage LOW
    delay(1000);            // wait for a second
    digitalWrite(GREEN, LOW);
    delay(1000);  
    digitalWrite(BLUE, LOW);
    delay(1000);  
    
    digitalWrite(RED, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                         
    digitalWrite(GREEN, HIGH);
    delay(1000);  
    digitalWrite(BLUE, HIGH);
    delay(1000);  
  
    vibrationMotor1.on();     // 1. turns on
    delay(500);           // 2. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
    vibrationMotor1.off();    // 3. turns off
    delay(500);           // 4. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
  }
  
  if (digitalRead(BTN2) == HIGH){
    
    digitalWrite(BLUE, LOW); // turn the LED off by making the voltage LOW
    delay(1000);            // wait for a second
    digitalWrite(RED, LOW);
    delay(1000);  
    digitalWrite(GREEN, LOW);
    delay(1000);  
    
    digitalWrite(GREEN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                         
    digitalWrite(RED, HIGH);
    delay(1000);  
    digitalWrite(BLUE, HIGH);
    delay(1000);  
  
    vibrationMotor2.on();     // 1. turns on
    delay(500);           // 2. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
    vibrationMotor2.off();    // 3. turns off
    delay(500);           // 4. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
  }

  
  
}

/*

// Include Libraries



// Pin Definitions
#define VIBRATIONMOTOR_PIN_COIL1  3



// Global variables and defines

// object initialization
VibrationMotor vibrationMotor(VIBRATIONMOTOR_PIN_COIL1);


// define vars for testing menu
const int timeout = 10000;       //define timeout of 10 sec
char menuOption = 0;
long time0;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup() 
{
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages
    Serial.begin(9600);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("start");
    
    
    menuOption = menu();
    
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop() 
{
    
    
    if(menuOption == '1') {
    // Vibration Motor - Test Code
    // The vibration motor will turn on and off for 500ms (0.5 sec)
    vibrationMotor.on();     // 1. turns on
    delay(500);           // 2. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
    vibrationMotor.off();    // 3. turns off
    delay(500);           // 4. waits 500ms (0.5 sec). change the value in the brackets (500) for a longer or shorter delay in milliseconds.
    }
    
    if (millis() - time0 > timeout)
    {
        menuOption = menu();
    }
    
}



// Menu function for selecting the components to be tested
// Follow serial monitor for instrcutions
char menu()
{

    Serial.println(F("\nWhich component would you like to test?"));
    Serial.println(F("(1) Vibration Motor"));
    Serial.println(F("(menu) send anything else or press on board reset button\n"));
    while (!Serial.available());

    // Read data from serial monitor if received
    while (Serial.available()) 
    {
        char c = Serial.read();
        if (isAlphaNumeric(c)) 
        {   
            
            if(c == '1') 
          Serial.println(F("Now Testing Vibration Motor"));
            else
            {
                Serial.println(F("illegal input!"));
                return 0;
            }
            time0 = millis();
            return c;
        }
    }
}
*/
