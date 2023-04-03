/*
  this code will display the BPM on the left side of the screen and the Setlist index on the right with the name right under it..
  possibly add a state to name each song..
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
//#include <stdbool.h>

#define BPM_PIN 9
#define MOMENTARY_PIN 8
#define UP_PIN 7
#define DOWN_PIN 6

#define enc_pin_1A 2
#define enc_pin_2A 4

#define enc_pin_1B 3
#define enc_pin_2B 5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// display declaration
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum bpm_state {BP_init, BP_wait, BP_tap1, BP_tap2} BP_state;
enum screen_state {SS_init, SS_bpm, SS_Playlist,} SS_state;
enum title_state {TT_init, TT_editChar, TT_done} TT_state;
enum flashLetter {FL_init, FL_wait, FL_on, FL_off,} FL_state;

uint8_t ii = 0;
bool charEdit = false;
uint32_t previousTime = 0;
uint32_t previousTimeFlash = 0;
uint8_t titleIndex = 0;
bool callBPM = false;

typedef struct SetList {
  uint8_t order;
  char name[10];
  uint32_t difference1;
  uint32_t difference2;
  uint32_t time1;
  uint32_t time2;
  uint32_t time3;
  int32_t bpm;
  uint8_t previousEncoderValue;
  uint32_t tempVal;
};

SetList setLists[10];
Encoder myEnc(enc_pin_1A, enc_pin_1B);
Encoder myEnc2(enc_pin_2A, enc_pin_2B);

void setBPM (void) {
  setLists[ii].bpm = 0;
}

void seekUpTickFctn (void) {
  if(!charEdit) {
    if (ii >= 9) {
      ii = 9;
    }
    else {
      ++ii;
    }

    display.clearDisplay();
  }

  Serial.print("ii = ");
  Serial.println(ii);
}

void seekDownTickFctn (void) {
  if(!charEdit) {
    if (ii <= 0) {
      ii = 0;
    }
    else {
      --ii;
    }
      display.clearDisplay();
  }
}

void bpmTickFctn (void) {
  Serial.println("BPM");


  switch (BP_state) {
    case BP_init:

    break;

    case BP_wait:
      setLists[ii].time1 = millis();
    break;

    case BP_tap1:
      setLists[ii].time2 = millis();

      setLists[ii].difference1 = (setLists[ii].time1 - setLists[ii].time2); //the time between the first two taps
    break;

    case BP_tap2:
      setLists[ii].time3 = millis();

      setLists[ii].difference2 = (setLists[ii].time3 - setLists[ii].time2);

      Serial.print(setLists[ii].difference1);
      Serial.print(" ");
      Serial.print(setLists[ii].difference2);
      Serial.print(" ");
      Serial.print(setLists[ii].tempVal);
      Serial.print(" ");
      Serial.print(setLists[ii].bpm);
      Serial.print(" ");
      Serial.println(millis());

      setLists[ii].tempVal = (setLists[ii].difference1 + setLists[ii].difference2) / 2;

      //Serial.println(setLists[ii].tempVal);
      setLists[ii].bpm = (1000.0 / setLists[ii].tempVal) * 60.0;

      //callBPM = true;

    break;
  }

    switch (BP_state) {
    case BP_init:
      BP_state = BP_wait;
      Serial.println("init");
    break;

    case BP_wait:
      BP_state = BP_tap1;
      Serial.println("wait");
    break;

    case BP_tap1:
      BP_state = BP_tap2;
      Serial.println("tap1");
    break;

    case BP_tap2:
      BP_state = BP_wait;
      Serial.println("tap sequence complete");
    break;
  }
}

// edit the title
void titleISR (void) {
  static char titleChar;

  switch (TT_state) {
    case TT_init:
      charEdit = true;
      titleChar = 'A';
      titleIndex = 0;
      break;

    case TT_editChar:
      setLists[ii].name[titleIndex] = titleChar;
      
      // increase or Decrease the letter
      if (myEnc.read() > 1) {
        ++titleChar;
        myEnc.write(0);
      }
      else if ( myEnc.read() < -1) {
        --titleChar;
        myEnc.write(0);
      }

      // increase or decrease the letter index
      if (myEnc2.read() > 1) {
        ++titleIndex;
        myEnc2.write(0);
      }
      else if (myEnc2.read() < -1) {
        --titleIndex;
        myEnc2.write(0);
      }

      break;

    case TT_done:
      charEdit = false;
      break;
    
  }

  switch (TT_state) {
    case TT_init:
      TT_state = TT_editChar;
      break;

    case TT_editChar:
      if (digitalRead(UP_PIN) && digitalRead(DOWN_PIN)) {
        while (digitalRead(UP_PIN) && digitalRead(DOWN_PIN)) {}
        TT_state = TT_done;      
      }
      else {
        TT_state = TT_editChar;
      }
      break;

    case TT_done:
      TT_state = TT_init;
      break;
  }
}

void flashISR (void) {
  switch (FL_state) {
    case FL_init:      
    break;

    case FL_wait:
    break;

    case FL_on:
      display.setCursor(64, 16);
      display.setFont();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.print(setLists[ii].name);
      display.display();
    break;

    case FL_off:
      //display.setCursor((64 + (10*titleIndex)), 16);

      //display.setTextColor(WHITE);
      display.drawRect((64 + (12*titleIndex)), 16, 10, 16, BLACK);
      display.fillRect((64 + (12*titleIndex)), 16, 10, 16, BLACK);
      display.display();
      //delay(10);
    break;
  }

  switch (FL_state) {
    case FL_init:
      FL_state = FL_wait;
      break;

    case FL_wait:
      if (charEdit) {
        FL_state = FL_on;
      }
      else {
        FL_state = FL_wait;
      }
      break;

    case FL_on:
      if (charEdit) {
        FL_state = FL_off;
      }
      else {
        FL_state = FL_on;
      }
      break;

    case FL_off:
      FL_state = FL_on;
      break;
  }
}

void setup() {


  Serial.begin(115200);
  Serial.print("Begin initializations");

  pinMode(BPM_PIN, OUTPUT);
  pinMode(MOMENTARY_PIN, INPUT);

  Serial.print(".");

// init Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Screen Failed to initialize...");
  }
  display.clearDisplay();
  Serial.print(".");

// set bpm isr
  attachInterrupt(digitalPinToInterrupt(MOMENTARY_PIN), bpmTickFctn, RISING);
  Serial.print(".");

// seek up isr
  pinMode(UP_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(UP_PIN), seekUpTickFctn, RISING);
  Serial.print(".");

// seek down isr
  attachInterrupt(digitalPinToInterrupt(DOWN_PIN), seekDownTickFctn, RISING);
  Serial.print(".");

// init the setLists Struct
for (int jj = 0; jj < 10; ++jj) {
  setLists[jj].bpm = 120;
  setLists[jj].name[0] = 'N';
  setLists[jj].name[1] = 'A';
  setLists[jj].name[2] = 'M';
  setLists[jj].name[3] = 'E';
  setLists[jj].order = jj + 1;
}

// init the screen interrupt
  /*timerB0.initialize(); 
  timerB0.attachInterrupt(titleISR);
  timerB0.setPeriod(1000000);
  timerB0.start();*/
  Serial.print(".");

// init the encoders
  myEnc.write(0);
  Serial.println(".");

  interrupts();
}

void loop() {
// if too much time went by, reset the tap state machine to the wait state
 /* if ((BP_state == BP_tap1) && (millis() - setLists[ii].time1 > 1500)) {
    BP_state = BP_wait;
  }
  else if ((BP_state == BP_tap2) && (millis() - setLists[ii].time2 > 1500)) {
    BP_state = BP_wait;
  }*/

// if the bpm encoder is turned then increase or decrease the bpm value


  //Serial.println(setLists[ii].bpm);

  //display.clearDisplay();

  display.drawRect(0, 0, 64, 32, BLACK); // instead of clearing the display, draw a black rectangle so that the name doesnt flash
  display.fillRect(0, 0, 64, 32, BLACK);
  
// display the bpm
  if (setLists[ii].bpm > 99) { // if bpm is above 99 then shift the number to the right so that it looks natural
    display.setCursor(1, 1);
  }
  else {
    display.setCursor(19, 1);
  }
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(setLists[ii].bpm);
  display.display();

  display.drawRect(64, 0, 64, 15, BLACK); // instead of clearing the display, draw a black rectangle so that the name doesnt flash
  display.fillRect(64, 0, 64, 15, BLACK);

// display the position in the set list
  display.setCursor(64, 0);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.print(setLists[ii].order);
  display.display();
  
// display the name underneith the order  
  if (!charEdit) {
    display.setCursor(64, 16);
    display.setFont();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print(setLists[ii].name);
    display.display();
  }


  delay(20);

  //charEdit = true;
// interrupt for edit mode
  if (millis() - previousTime > 100) { //psuedo timer interrupt
    previousTime = millis();

    //Serial.println("test");

    if (charEdit) {
      titleISR();
    }
  }

  if (millis() - previousTimeFlash > 250) {
    previousTimeFlash = millis();
    
    if (charEdit) {
      flashISR();
    }
  }

//if up and down are pressed then go into edit mode
  if (digitalRead(UP_PIN) && digitalRead(DOWN_PIN)) {
    while(digitalRead(UP_PIN) && digitalRead(DOWN_PIN)){}
    charEdit = true;
  }

// if not in edit mode, then use the encoder to change the bpm
  if(!charEdit) {
    if (myEnc.read() > 1) {
      ++setLists[ii].bpm;
      myEnc.write(0);

      if (setLists[ii].bpm > 1000) {
        setLists[ii].bpm = 999;
      }
      
    }
    else if (myEnc.read() < -1) {
      --setLists[ii].bpm;
      myEnc.write(0);

      if (setLists[ii].bpm < 0) {
        setLists[ii].bpm = 0;
      }
    }
  }

// call bpm isr at the end so it displays the value
  if (callBPM) {
    bpmTickFctn();
    callBPM = false;
  }

  //Serial.println(myEnc.read());
}
  
