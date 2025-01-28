#include <Adafruit_NeoPixel.h>
// Which pin on the Arduino is connected to the NeoPixels?
#define PIN  4
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS  24
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
float distance;

int delayval = 500; // delay for half a second


void setup() {
  strip.begin(); // This initializes the NeoPixel library.
  strip.clear();
  strip.show();   // make sure it is visible
   pinMode(3, OUTPUT); 
   pinMode(2, INPUT); 
   Serial.begin(9600); 
  pinMode(5,OUTPUT);

}

void loop() {
   digitalWrite(3, LOW);
   delayMicroseconds(2);
   digitalWrite(3, HIGH);
   delayMicroseconds(10);
   digitalWrite(3, LOW);
   distance = pulseIn(2, HIGH);
   distance= distance /58;
   Serial.println(distance);
  /*
  int red = random(0, 255);
  int gre = random(0, 255);
  int blue = random(0, 255);
  for(int i = 0; i < NUMPIXELS; i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    strip.setPixelColor(i, red, gre, blue);
    strip.show(); 
    // This sends the updated pixel color to the hardware.
    delay(delayval); 
    // Delay for a period of time (in milliseconds).
  }
/*
  digitalWrite(5,HIGH);
  delay(2000);
  digitalWrite(5,LOW);
  delay(2000);
  */
  
}
