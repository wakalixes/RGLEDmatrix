// author: Albert Frisch, 2013
// email:  albert.frisch@gmail.com
// descr:  driving a 5x5 anti-parallel dual LED matrix (red/green) using a 74HCT573
//         time-multiplexes every row with frequency MPLEXFREQ (a few kHz)
//         red and green frames are shown alternatively, thus also color mixing is possible

#include <stdio.h>

#define PIXELOFF    0
#define PIXELGREEN  1
#define PIXELRED    2
#define PIXELORANGE 3

#define PINOE     2	// output enable of 74HCT573, Latch is set high by hardware
#define PINROW0   3	// Pin at which Row0 is connected
#define PINROW1   4
#define PINROW2   5
#define PINROW3   6
#define PINROW4   7
#define PINCOL0   8	// Pin for Column0
#define PINCOL1   9
#define PINCOL2   10
#define PINCOL3   11
#define PINCOL4   12

#define PATTERNSIZEX  5
#define PATTERNSIZEY  5
#define MPLEXFREQ     3000 // row multiplexing frequency
#define MPLEXTIMEMUS  1000000/MPLEXFREQ //multiplexing time in microseconds
#define BLINKTIME     200  // blinking time for simple color switching test

// frameBuffer
// colorcode: 0 = LEDs off, 1 = LED green, 2 = LED red, 3 = LED green and red (orange)
unsigned char colorPattern[PATTERNSIZEX][PATTERNSIZEY] = {
  { 2, 2, 1, 3, 0 },
  { 2, 2, 1, 3, 0 },
  { 1, 1, 1, 3, 0 },
  { 3, 3, 3, 3, 0 },
  { 0, 0, 0, 0, 0 }
};

// constants
unsigned char allLEDRed[PATTERNSIZEX][PATTERNSIZEY] = {
  { 2, 2, 2, 2, 2 },  
  { 2, 2, 2, 2, 2 },
  { 2, 2, 2, 2, 2 },
  { 2, 2, 2, 2, 2 },
  { 2, 2, 2, 2, 2 }
};

unsigned char allLEDGreen[PATTERNSIZEX][PATTERNSIZEY] = {
  { 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1 }
};

unsigned char allLEDOrange[PATTERNSIZEX][PATTERNSIZEY] = {
  { 3, 3, 3, 3, 3 },
  { 3, 3, 3, 3, 3 },
  { 3, 3, 3, 3, 3 },
  { 3, 3, 3, 3, 3 },
  { 3, 3, 3, 3, 3 }
};

unsigned char allLEDOff[PATTERNSIZEX][PATTERNSIZEY] = {
  { 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 }
};

// output pattern
unsigned char frameBuffer[PATTERNSIZEX][PATTERNSIZEY];
// LED pattern which will be used to calculate actual LED output pattern, depending on color
unsigned char LEDBuffer[PATTERNSIZEX][PATTERNSIZEY];

unsigned long mplexTime;
unsigned long blinkTime;

struct flagsStruct {
  boolean frameGreen;
  boolean frameRed;
  unsigned char rowNum;
} flags;

unsigned int framenum;

void setup() {
  initPins();
  initVariables();
  outputPattern(&allLEDOff[0][0]);
}

void loop() {
  if ((millis()-blinkTime)>=BLINKTIME) {
    blinkTime = millis();
    if (framenum==1) {
      framenum = 0;  
      outputPattern(&allLEDGreen[0][0]);
    } else {
      framenum = 1;
      outputPattern(&allLEDOrange[0][0]);
    }
  }
  
  if ((micros()-mplexTime)>=MPLEXTIMEMUS) {
    mplexTime = micros();
    if (flags.rowNum>=PATTERNSIZEX) {
      flags.rowNum = 0;
      flags.frameGreen = !flags.frameGreen;  //change color for next frame
      flags.frameRed = !flags.frameGreen;
      calcLEDBuffer(flags.frameGreen);
    }
    doMultiplexing(flags.frameGreen, flags.rowNum);
    flags.rowNum++;
  }
}

void initPins() {
  pinMode(PINOE, OUTPUT);
  digitalWrite(PINOE, HIGH);
  pinMode(PINROW0, OUTPUT);
  pinMode(PINROW1, OUTPUT);
  pinMode(PINROW2, OUTPUT);
  pinMode(PINROW3, OUTPUT);
  pinMode(PINROW4, OUTPUT);
  pinMode(PINCOL0, OUTPUT);
  pinMode(PINCOL1, OUTPUT);
  pinMode(PINCOL2, OUTPUT);
  pinMode(PINCOL3, OUTPUT);
  pinMode(PINCOL4, OUTPUT);
}

void initVariables() {
  flags.frameGreen = true;
  flags.frameRed = false;
  flags.rowNum = 0;
}

void outputPattern(unsigned char *pattern) {
  for (int i=0;i<PATTERNSIZEX;i++) {
    for (int j=0;j<PATTERNSIZEY;j++) {
      frameBuffer[i][j] = *(pattern+i*PATTERNSIZEX+j);
    }
  }
}

void calcLEDBuffer(boolean color) {
  // color==1 green frame, color==0 red frame, orange == red and green
  unsigned char pixelLED = 0;
  unsigned char pixelColor;
  for (int i=0;i<PATTERNSIZEX;i++) {
    for (int j=0;j<PATTERNSIZEY;j++) {
      pixelColor = frameBuffer[i][j];
      if (color) { //green pixel
        switch (pixelColor) {
          case PIXELGREEN:
          case PIXELORANGE: pixelLED = 1; break;
          default: pixelLED = 0;
        }
      } else {     //red pixel
        switch (pixelColor) {
          case PIXELRED:
          case PIXELORANGE: pixelLED = 1; break;
          default: pixelLED = 0;
        }
      }
      LEDBuffer[i][j] = pixelLED;
    }
  }
}

void doMultiplexing(boolean colorGreen, unsigned char row) {
  boolean rowState;
  boolean colState;
  digitalWrite(PINOE, HIGH);        // disable outputs
  if (colorGreen) rowState = LOW;   // green color
  else rowState = HIGH;             // red color
  switchAllRowsHighZ();
  switchRowOutput(row, rowState);
  for (int j=0;j<PATTERNSIZEY;j++) {
    colState = LEDBuffer[row][j];
    if (!colorGreen) colState = !colState;  //for red output colState has to be inverted!
    switchColOutput(j, colState);
  }
  digitalWrite(PINOE, LOW);         // enable outputs
}

void switchAllRowsHighZ() {
  pinMode(PINROW0, INPUT);
  pinMode(PINROW1, INPUT);
  pinMode(PINROW2, INPUT);
  pinMode(PINROW3, INPUT);
  pinMode(PINROW4, INPUT);
}

void switchRowOutput(unsigned char row, boolean state) {
  unsigned char rowPin;
  switch (row) {
    case 0: rowPin = PINROW0; break;
    case 1: rowPin = PINROW1; break;
    case 2: rowPin = PINROW2; break;
    case 3: rowPin = PINROW3; break;
    case 4: rowPin = PINROW4; break;
    default: rowPin = PINROW0;
  }  
  pinMode(rowPin, OUTPUT);
  digitalWrite(rowPin, state);
}

void switchColOutput(unsigned char col, boolean state) {
  unsigned char colPin;
  switch (col) {
    case 0: colPin = PINCOL0; break;
    case 1: colPin = PINCOL1; break;
    case 2: colPin = PINCOL2; break;
    case 3: colPin = PINCOL3; break;
    case 4: colPin = PINCOL4; break;
    default: colPin = PINROW0;
  }
  digitalWrite(colPin, state);
}
