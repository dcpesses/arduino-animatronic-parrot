/* Animatronic Parrot v2
   by Danny Pesses

   Uses a WiiChuck and a MSGEQ7 to manipulate several servo motors
   connected to a Pololu Micro Maestro 6-Channel USB Servo Controller
*/


/* Before using this sketch, you should open the Serial Settings tab
  in the Maestro Control Center software and apply these settings:

  Serial mode: UART, fixed baud rate
  Baud rate: 9600
  CRC disabled

  Be sure to click "Apply Settings" after making any changes.

  This sketch also assumes you have connected your Arduino to your
  Maestro appropriately. If you have not done so, please see
  https://github.com/pololu/maestro-arduino for more details on how
  to make the connection between your Arduino and your Maestro. */

#include <PololuMaestro.h>

// Wiichuck includes
#include <Wire.h>
#include "nunchuck_funcs.h"

/* On boards with a hardware serial port available for use, use
  that port to communicate with the Maestro. For other boards,
  create a SoftwareSerial object using pin 10 to receive (RX) and
  pin 11 to transmit (TX). */

#ifdef SERIAL_PORT_HARDWARE_OPEN
#define maestroSerial SERIAL_PORT_HARDWARE_OPEN
#else
#include <SoftwareSerial.h>
SoftwareSerial maestroSerial(10, 11);
#endif

/* Next, create a Maestro object using the serial port.

  Uncomment one of MicroMaestro or MiniMaestro below depending
  on which one you have. */
MicroMaestro maestro(maestroSerial);
//MiniMaestro maestro(maestroSerial);

/* Setup MSGEQ7 vars */
int analogPin = 0; // read from multiplexer using analog input 0
int strobePin = 2; // strobe is attached to digital pin 2
int resetPin = 3; // reset is attached to digital pin 3
int spectrumValue[7]; // to hold a2d values

// Constraints for MSGEQ7's output values
// NOTE: You WILL need to edit these values based on
// the loudness of your audio source
int minValue = 100;
int maxValue = 500;

int averageVal = 0; // average values across several spectrums
int levelsVal = 0;  // stores audio level 0-10
String levelsStr = "";
String logVal = "";


/* Setup WiiChuck vars */
byte joyx, joyy, cbut, zbut, accx, accy;
bool wiichuckReady = false;
bool wiichuckDisconnected = false;

// Servo variables
int beak, head_pan, head_tilt, body_tilt, wings;

int beak_opened = 1550;
int beak_closed = 1350;

int servo0_value;   // beak open & close
int servo1_value;   // head pan left & right
int servo2_value;   // head tilt up & down
int servo3_value;   // body tilt up & down
int servo4_value;   // wings open & close (untested)
int servo5_value;   // currently unused

int servo0_speed = 0;
int servo1_speed = 100;
int servo2_speed = 100;
int servo3_speed = 50;
int servo4_speed = 100;
int servo5_speed = 100;

int servo0_accel = 0;
int servo1_accel = 30;
int servo2_accel = 30;
int servo3_accel = 50;
int servo4_accel = 30;
int servo5_accel = 30;

// autopilot
bool autopilot_enabled = false;

int autopilot_count = 0;

int auto_head_pan = -1;
int auto_head_tilt = -1;

int ap_speed = 50;
int ap_accel = 50;


//
bool debug_serial = false;


long spectrumUpdateInterval = 30;
long wiichuckUpdateInterval = 10;
long autoPilotUpdateInterval = 250; // will be updated with a random number
unsigned long previousMillis = 0;
unsigned long lastSpectrumUpdate = 0;
unsigned long lastWiichuckUpdate = 0;
unsigned long lastMaestroUpdate = 0;

bool is_ready = false;

void setup()
{
  // Set the Maestro serial baud rate.
  maestroSerial.begin(9600);

  nunchuck_setpowerpins();
  nunchuck_init(); // send the initilization handshake

  Serial.begin(9600);
  
  // Setup pins for MSGEQ7
  pinMode(analogPin, INPUT);
  pinMode(strobePin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  analogReference(DEFAULT);
  
  delay(250);

  // clear out serial monitor and avoid
  // double-printing the first few lines
  Serial.println("                                                                           ");

  // should be good to print from here
  Serial.println("\n/=========================\\");
  Serial.println("*  Animatronic Parrot v2  *");
  Serial.println("*     by Danny Pesses     *");
  Serial.println("\\=========================/\n");

  setupMaestroDefaults();
  delay(250);
  
  updateNunchuckValues();
  delay(250);
}

void setupMaestroDefaults() {

  /* setSpeed takes channel number you want to limit and the
    speed limit in units of (1/4 microseconds)/(10 milliseconds).

    A speed of 0 makes the speed unlimited, so that setting the
    target will immediately affect the position. Note that the
    actual speed at which your servo moves is also limited by the
    design of the servo itself, the supply voltage, and mechanical
    loads. */
  maestro.setSpeed(0, 0);
  maestro.setSpeed(1, servo1_speed);
  maestro.setSpeed(2, servo2_speed);
  maestro.setSpeed(3, servo3_speed);
  maestro.setSpeed(4, servo4_speed);
  maestro.setSpeed(5, servo5_speed);



  /* setAcceleration takes channel number you want to limit and
    the acceleration limit value from 0 to 255 in units of (1/4
    microseconds)/(10 milliseconds) / (80 milliseconds).

    A value of 0 corresponds to no acceleration limit. An
    acceleration limit causes the speed of a servo to slowly ramp
    up until it reaches the maximum speed, then to ramp down again
    as the position approaches the target, resulting in relatively
    smooth motion from one point to another. */

  maestro.setAcceleration(0, 0);    // always run at full speed
  maestro.setAcceleration(1, servo1_accel);
  maestro.setAcceleration(2, servo2_accel);
  maestro.setAcceleration(3, servo3_accel);
  maestro.setAcceleration(4, servo4_accel);
  maestro.setAcceleration(5, servo5_accel);

}

void enableAutoPilot() {
  autopilot_enabled = true;
  (debug_serial == true) && Serial.println("*** AUTOPILOT ENGAGED! ***");

  //  maestro.setSpeed(0, ap_speed);
  maestro.setSpeed(1, ap_speed);
  maestro.setSpeed(2, ap_speed);
  maestro.setSpeed(3, ap_speed);
  maestro.setSpeed(4, ap_speed);
  maestro.setSpeed(5, ap_speed);

//  maestro.setAcceleration(0, ap_accel);
  maestro.setAcceleration(1, ap_accel);
  maestro.setAcceleration(2, ap_accel);
  maestro.setAcceleration(3, ap_accel);
  maestro.setAcceleration(4, ap_accel);
  maestro.setAcceleration(5, ap_accel);
}

void disableAutoPilot() {
  autopilot_enabled = false;
  setupMaestroDefaults();
  (debug_serial == true) && Serial.println("*** AUTOPILOT DISENGAGED ***");
}

void debugNunchuckValues() {
  Serial.print("joyx: "); Serial.print((byte)joyx, DEC);
  Serial.print("\tjoyy: "); Serial.print((byte)joyy, DEC);
  Serial.print("\taccx: "); Serial.print((byte)accx, DEC);
  Serial.print("\taccy: "); Serial.print((byte)accy, DEC);
  Serial.print("\tzbut: "); Serial.print((byte)zbut, DEC);
  Serial.print("\tcbut: "); Serial.println((byte)cbut, DEC);
}

void printSerialOutput() {
  Serial.print("joy x: "); Serial.print((byte)joyx, DEC);
  Serial.print(", y: ");  Serial.print((byte)joyy, DEC);
  Serial.print("\ts0: "); Serial.print(servo0_value);
  Serial.print("\ts1: "); Serial.print(servo1_value);
  Serial.print("\ts2: "); Serial.print(servo2_value);
  Serial.print("\ts3: "); Serial.print(servo3_value);
  Serial.print("\ts4: "); Serial.print(servo4_value);
  Serial.print("\ts5: "); Serial.println(servo5_value);
}

void updateNunchuckValues() {
  nunchuck_get_data();

  joyx = nunchuck_joyx(); // avg range b/w 24-240
  joyy = nunchuck_joyy(); // avg range b/w 24-240
  accx = nunchuck_accelx(); // ranges from approx 70 - 182
  accy = nunchuck_accely(); // ranges from approx 65 - 173
  zbut = nunchuck_zbutton();
  cbut = nunchuck_cbutton();

  if ((joyx == 0 || joyy == 0) && wiichuckDisconnected == false) {
    wiichuckDisconnected = true;
    (debug_serial == true) && Serial.println("*** NUNCHUCK NOT DETECTED ***");
  } else if ((joyx != 0 || joyy != 0) && wiichuckDisconnected == true) {
    wiichuckDisconnected = false;
    (debug_serial == true) && Serial.println("*** NUNCHUCK DETECTED ***");
  }
}


int to_pulse(int microsecs) {
  return constrain(map(microsecs, 1000, 2000, 4000, 8000), 4000, 8000);
}

void loop() {

  unsigned long currentMillis = millis();
//
//  if (!is_ready) {
//    Serial.print("Time: ");
//    Serial.print(currentMillis);
//    Serial.print(", Last: ");
//    Serial.println(lastSpectrumUpdate);
//  }
  
  
  if ((currentMillis - lastSpectrumUpdate) > spectrumUpdateInterval ) {  // time to update
    lastSpectrumUpdate = currentMillis;
    updateSpectrum();
  }

  long maestroUpdateInt = (autopilot_enabled == true) 
      ? autoPilotUpdateInterval 
      : wiichuckUpdateInterval;
      
  if ((currentMillis - lastMaestroUpdate) > maestroUpdateInt ) {
    lastMaestroUpdate = currentMillis;

    // reads wiichuck to variables
    updateNunchuckValues();

    // enables/disables autopilot based on nunchuck state
    onLoopStart();

    if (autopilot_enabled == false) {

      // converts wiichuck variables into pulses (PWM)
      mapWiichuckValues();
      
    } else {
      
      // generate semi-random values for some servos
      setAutoPilotValues();
      
      // set random interval for next loop
      autoPilotUpdateInterval = round(random(350) + 50);
    }

    // conform servo values based on physical installed position
    setServoValues();

    // send servo position to Maestro controller via serial interface
    writeMaestroSerial();

    if (debug_serial) {
      printSerialOutput();
    }
  }
}

/*
  To read the current loudness of the frequencies from of the MSGEQ7, 
  you must first reset the chip and then strobe the chip 7 times, 
  once for each frequency. Each strobe returns a value between 0-1023
  that corresponds to one of the 7 frequencies in the following order:
  63Hz, 160Hz, 400Hz, 1kHz, 2.5kHz, 6.25kHz, 16kHz
*/

void updateSpectrum() {

  averageVal = 0;

  for (int i = 0; i < 7; i++) {
    digitalWrite(strobePin, LOW);
    
    spectrumValue[i] = analogRead(analogPin);
    averageVal += spectrumValue[i];

	// For specific frequency monitoring
/*	
    if (i == 3) { //3 = 1khz range

      servo0_value = map(spectrumValue[i], minValue, maxValue, beak_closed, beak_opened);
      servo0_value = constrain(servo0_value, beak_closed, beak_opened);
	  servo0_value = to_pulse(servo0_value);
      maestro.setTarget(0, servo0_value);
      
      // serial output
      if (debug_serial == true) {
        levelsVal = map(spectrumValue[i], minValue, maxValue, 0, 10);
        levelsVal = constrain(levelsVal, 0, 10);
      
        levelsStr = "";
        for (int j = 0; j < levelsVal; j++) {
          levelsStr = levelsStr + "|";
        }
        for (int k = 0; k < 10 - levelsVal; k++) {
          levelsStr = levelsStr + "_";
        }
        levelsStr = levelsStr + " ";

        Serial.print(levelsStr); Serial.print(spectrumValue[i]);
        Serial.print(" (");
        Serial.print(servo0_value);
        Serial.print(")");
      }
    } */
    digitalWrite(strobePin, HIGH);
  }

  // get the average of spectrum output
  averageVal = round(averageVal/7);

  // map (and constrain) average value to beak position
  servo0_value = map(averageVal, minValue, maxValue, beak_closed, beak_opened);
  servo0_value = constrain(servo0_value, beak_closed, beak_opened);
  servo0_value = to_pulse(servo0_value);
  maestro.setTarget(0, servo0_value);

  if (debug_serial == true) {
    Serial.print("servo0_value: ");
    Serial.print(to_pulse(servo0_value));
    Serial.print(", averageVal: ");
    Serial.println(averageVal);
  }

  // trigger MSGEQ7 to process audio for next loop
  digitalWrite(resetPin, HIGH);
  digitalWrite(resetPin, LOW);
}

// Run at the beginning of loop to detect the current state
// of the nunchuck input and whether or not to substitute 
// auto-generated values for servo positions.
void onLoopStart() {
    
    // check if wiichuck is reporting valid data
    if (wiichuckReady == false && (joyx != 255 || joyy != 255) && wiichuckDisconnected == false) {
      wiichuckReady = true;
      Serial.println("- WiiChuck ready");
      // allow setting any debug option(s) from held button(s);
      // will be only checked during startup routine
      if (zbut == 1) {
        debug_serial = true;
        Serial.println("- Serial output enabled!");
      }
      if (cbut == 1) {
        // set something here
        // Serial.println("- Something else enabled!");
      }
    }

    // use autopilot if nunchuck is disconnected
    if (joyx == 0 && joyy == 0 && autopilot_enabled == false && wiichuckDisconnected == true) {
      autopilot_enabled = true;
      zbut = 0;
      cbut = 0;
      enableAutoPilot();
    }

    // if joystick is NOT in center position or any buttons are pressed
    else if (wiichuckDisconnected == false && joyx != 0 && joyy != 0 && (abs(joyx - 124) > 2 || abs(joyy - 134) > 2 || cbut == 1 || zbut == 1)) {
      // disable autopilot if manual control used
      if (autopilot_enabled == true) {
        disableAutoPilot();
      }
      autopilot_count = 0;
    }
    // if joystick IS in center position and no buttons pressed
    else {
      autopilot_count++;
      if (autopilot_enabled == false && autopilot_count > 50) {
        enableAutoPilot();
      }
    }

}
/*
void readWiichuckInput() {	// original loop
  
  if (loop_cnt > wiichuckUpdateInterval) { // get new data when loop_cnt exceeds refresh_msecs
    loop_cnt = 0;

    updateNunchuckValues();

    // check if wiichuck is reporting valid data
    if (wiichuckReady == false && (joyx != 255 || joyy != 255)) {
      wiichuckReady = true;
      Serial.println("- WiiChuck ready");
      // enable any startup options from held button(s)
      if (zbut == 1) {
        debug_serial = true;
        Serial.println("- Serial output enabled!");
      }
    }

    // ignore startup jitters
    if (loop_cnt < 2 && joyx == 255 && joyy == 255) {
      delay(refresh_msec);
      loop_cnt = refresh_msec;
      return;
    }


    // use autopilot if nunchuck is disconnected
    if (joyx == 0 && joyy == 0) {
      autopilot_enabled = true;
      (debug_serial == true) && Serial.print("*** NUNCHUCK NOT DETECTED ***");
      zbut = 0;
      cbut = 0;
      enableAutoPilot();
    }

    // if joystick is NOT in center position or any buttons are pressed
    else if (abs(joyx - 124) > 2 || abs(joyy - 134) > 2 || cbut == 1 || zbut == 1) {
      // disable autopilot if manual control used
      if (autopilot_enabled == true) {
        disableAutoPilot();
      }
      autopilot_count = 0;
    }
    // if joystick IS in center position and no buttons pressed
    else {
      autopilot_count++;
      if (autopilot_enabled == false && autopilot_count > 50) {
        enableAutoPilot();
      }
    }

    // debug Nunchuck values
    // if (debug_serial==true) {
    //   debugNunchuckValues();
    // }

    if (autopilot_enabled == false) {

      // min: 4000, max: 8000
      // map the input to pulse width values for servo movement
      head_pan = map(joyx, 20, 235, 0, 4000);
      head_tilt = map(joyy, 20, 235, 0, 4000);
      body_tilt = (zbut == 1) ? head_tilt : 2000;
      beak = (cbut == 0) ? to_pulse(beak_closed) : to_pulse(beak_opened);

    }
    // generate random values for autopilot
    else {


      if (auto_head_pan == -1 || head_pan == auto_head_pan) {
          auto_head_pan = random(2000) + 1000;
      } else {
          head_pan += round(auto_head_pan - head_pan);
      }

      if (auto_head_tilt == -1 || head_tilt == auto_head_tilt) {
          auto_head_tilt = random(1000) + 1500;
      } else {
          head_tilt += round(auto_head_tilt - head_tilt);
      }
      
    }



    servo0_value = beak;
    servo1_value = 8000 - head_pan;
    servo2_value = (zbut == 1) ? head_tilt + 4000 : 8000 - head_tilt;
    servo3_value = 8000 - body_tilt;
    servo4_value = 8000 - wings;



    if (debug_serial == true) {
      printSerialOutput();
    }



  }
  loop_cnt++;

  delay(1);

}*/

void mapWiichuckValues() {
  
    // min: 4000, max: 8000
    // map the input to pulse width values for servo movement
    head_pan = map(joyx, 20, 235, 0, 4000);
    head_tilt = map(joyy, 20, 235, 0, 4000);
    body_tilt = (zbut == 1) ? head_tilt : 2000;
    beak = (cbut == 0) ? to_pulse(beak_closed) : to_pulse(beak_opened);

}

void setAutoPilotValues() {
  
      if (auto_head_pan == -1 || head_pan == auto_head_pan) {
          auto_head_pan = random(2000) + 1000;
      } else {
          head_pan += round(auto_head_pan - head_pan);
      }

      if (auto_head_tilt == -1 || head_tilt == auto_head_tilt) {
          auto_head_tilt = random(1000) + 1500;
      } else {
          head_tilt += round(auto_head_tilt - head_tilt);
      }

      // set random interval for next loop
      autoPilotUpdateInterval = round(random(350) + 50);
}

void setServoValues() {
    servo0_value = beak;
    servo1_value = 8000 - head_pan;
    servo2_value = (zbut == 1) ? head_tilt + 4000 : 8000 - head_tilt;
    servo3_value = 8000 - body_tilt;
    servo4_value = 8000 - wings;
}

void writeMaestroSerial() {

    /* setTarget takes the channel number you want to control, and
      the target position in units of 1/4 microseconds. A typical
      RC hobby servo responds to pulses between 1 ms (4000) and 2
      ms (8000). */
  
//    maestro.setTarget(0, servo0_value);
    maestro.setTarget(1, servo1_value);
    maestro.setTarget(2, servo2_value);
    maestro.setTarget(3, servo3_value);
    maestro.setTarget(4, servo4_value);
    maestro.setTarget(5, servo5_value);

}

