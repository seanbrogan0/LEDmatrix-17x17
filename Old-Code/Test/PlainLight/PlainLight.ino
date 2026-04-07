
#include <FastLED.h>

#define DATA_PIN 3

//////////////////////////////////////////////////
// DEFINE LED Matrix

// Params for width and height
const uint8_t MatWidth = 17;
const uint8_t MatHeight = 17;
#define NUM_LEDS (MatWidth * MatHeight)

CRGB leds[NUM_LEDS];

// XY as LED number from x,y co-ordinates
//for serpentine wiring pattern

uint8_t x; 
uint8_t y;

uint16_t XY(uint8_t x,uint8_t y )
{
  uint16_t z;
    if( y & 0x01) {
      uint8_t reverseX = (MatWidth - 1) - x;  // Odd rows run backwards
      z = (y * MatWidth) + reverseX;
    } else {
      z = (y * MatWidth) + x;                 // Even rows run forwards
    } 
  return z;     // z returns LED number within matrix
}

//////////////////////////////////////////////////
////
////DEFINE PINS (OTHER THAN LED STRIP)

const int aVARPIN = A1;   //left pot
const int bVARPIN = A2;   //right pot


//Variables and start level that will be adjusted by potentiometer
//arrays and values for smoothing
#define numReadings 10

int aRAY [numReadings];
byte aRAYdex = 0;
int aRAYtot = 0;
int aRAYavg = 0;

int bRAY [numReadings];
byte bRAYdex = 0;
int bRAYtot = 0;
int bRAYavg = 0;


byte MAXbright = 255;   //brightness - left pot, or slider if I can get it to work


void setup() {
  // put your setup code here, to run once:
  
  //start delay
  delay(1000);
  Serial.begin(57600);
  
  // setup leds
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);

FastLED.clear(leds);

}

void loop() {
  // put your main code here, to run repeatedly:

  // put your main code here, to run repeatedly:

    READ_allpots();

    Pick_Colour();

  SendColours();
  
delay(20);

}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void SendColours() {
  //////////
  //sub to set brightness and show leds

    if (aRAYavg > 1017) {
    MAXbright = 255;
  }
  else {
    MAXbright = map(aRAYavg, 0, 1023, 15, 255);
  }
  FastLED.setBrightness(MAXbright);
  FastLED.show();
  delay(20);

}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


void READ_allpots(){

  aRAYtot = aRAYtot - aRAY[aRAYdex];
  aRAY[aRAYdex] = analogRead(aVARPIN);     //pot smoothing
  aRAYtot = aRAYtot + aRAY[aRAYdex];      //
  aRAYdex = aRAYdex + 1;                  //

  if (aRAYdex >= numReadings) {           //
    aRAYdex = 0;
  }
  aRAYavg = aRAYtot / numReadings;        //

  bRAYtot = bRAYtot - bRAY[bRAYdex];
  bRAY[bRAYdex] = analogRead(bVARPIN);     //pot smoothing
  bRAYtot = bRAYtot + bRAY[bRAYdex];      //
  bRAYdex = bRAYdex + 1;                  //

  if (bRAYdex >= numReadings) {           //
    bRAYdex = 0;
  }
  bRAYavg = bRAYtot / numReadings;        //

}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void Pick_Colour(){

    for( int j = 2; j < MatHeight-2; j++) {
          for (x = 2; x < MatWidth-2; x++){
        leds[XY(x,j)] = CRGB(255,255,255);
    }
  }
}
