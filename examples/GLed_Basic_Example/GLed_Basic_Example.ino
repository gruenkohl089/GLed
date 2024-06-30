/////////////////////////////////////////////////////////////////////////////
//
//  Project:       LED control
//
//  Subcomponent:  ESP32 Led control
/////////////////////////////////////////////////////////////////////////////
//
// abstract:       led control
// premises:
// remarks:
// history:        10.04.2020, GJK, created.
// AUTHOR:         G.Kasper
// contact:        info@georgkasper.de
// copyright:      Georg Kasper
// file:           GLed_Example.ino
// language:       C++
// compiler:       g++ (i.e. Arduino IDE compiler)
//
// REVIEW:
/////////////////////////////////////////////////////////////////////////////

# include <Arduino.h>

# include <GLed.h>

// Note: 
// the programmable build in LED on the board do not have all the same gpio pin.
// So be sure to use your gpio number.
// For example:
//   Ezsbc: 5
//   Wemos s2 mini sesp32-s2: 15
//   Esp32 DevKitC V4: no user led.
//   ESP32 MH-ET LIVE: 2.
//   LOLIN32: 5.

GLed gled;								// default LED configured as (LED_BUILTIN, logic LOW, state off).
// GLed gled( 5, GLed::LOW_IS_ACTIVE);	// EZSBC
// GLed gled(15, GLed::LOW_IS_ACTIVE);	// wemos s2 mini esp32-s2.
// GLed gled( 5, GLed::HIGH_IS_ACTIVE);	// DIY board, rain gauge esp32.

void setup()
{
  // Serielle Schnittstelle aktivieren für Debugging
  Serial.begin(115200);
  delay(300);
  
  Serial.println( "BOOTING GLED Example - LED blinking" );
  Serial.printf( "  expect that the LED is controlled with pin %d \n", gled.get_pin() );
  Serial.printf( "  expect that the LED is ON if the pin level is %s\n",  
				 gled.get_logic_mode() == GLed::LOW_IS_ACTIVE ? "LOW" : "HIGH" );
  
  // activate the pin port for the GLed control.
  gled.begin(); 
}

void loop()
{
  Serial.println("the LED schould now be ON for 3 seconds - and then flash 20 times.");
  gled.on();
  delay(3000);
  
  gled.flash(20);

  Serial.println("the LED should now be OFF for 3 seconds");
  gled.off();
  delay(3000); 
    
  Serial.println("the LED schould now be flashing 10 times with a blinking periode of 0,5 second.");
  gled.flash(10, 500);

  Serial.println("the LED schould now be OFF for 5 second.");
  gled.off();
  
  delay(5000);  
}

// eof
