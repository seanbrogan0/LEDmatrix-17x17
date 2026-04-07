int h = 1;

#include <FastLED.h>

#include <AceButton.h>
using namespace ace_button;

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

const int btn1PIN = 8; //buttons
const int btn2PIN = 9;

AceButton btn1(btn1PIN);
AceButton btn2(btn2PIN);

//////////////////////////////////////////////////
////VARIABLES


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

byte MAXbright = 255;   //brightness - left pot


void handleEvent(AceButton*, uint8_t, uint8_t);


void setup() {
  // put your setup code here, to run once:

  //start delay
  delay(1000);
  Serial.begin(57600);
  
  // setup leds
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);

FastLED.clear(leds);

  //set pinmodes
  pinMode(aVARPIN, INPUT);
  pinMode(bVARPIN, INPUT);
  pinMode(btn1PIN, INPUT_PULLUP);
  pinMode(btn2PIN, INPUT_PULLUP);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);


}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // put your main code here, to run repeatedly:
Serial.print("Check");
  //look for button press
  btn1.check();
  btn2.check();

  READ_allpots();
  SHOW_Palette();
  SendColours();
Serial.println(h);
delay(20);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The event handler for both buttons.
void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {

Serial.print("btnCheck");
 switch (eventType) {
    case 1:
      Serial.println("Short2 Click");
      if (button->getPin() == btn1PIN) {
        h = h+1;
      }
      if (button->getPin() == btn2PIN) {
        h = h+1;
      }
      break;
    case 2:
      Serial.println("Short Click");
      if (button->getPin() == btn1PIN) {
        h = h+1;
      }
      if (button->getPin() == btn2PIN) {
        h = h+1;
      }
      break;
    case 3:
      Serial.println("Double Click");
      if (button->getPin() == btn1PIN) {
        h = h+1;
      }
      if (button->getPin() == btn2PIN) {
        h = h+1;
      }
      break;
    case 4:
      Serial.println("Long Click");
      break;
    case 5:
      break;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
  delay(1);

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

void SHOW_Palette(){

  
}
