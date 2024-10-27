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
// history:        10.10.2024, GJK, created.
// AUTHOR:         G.Kasper
// contact:        info@georgkasper.de
// copyright:      Georg Kasper
// file:           GLed_Async_Blinking_Example.ino
// language:       C++
// compiler:       g++ (i.e. Arduino IDE compiler)
//
// REVIEW:
/////////////////////////////////////////////////////////////////////////////

# include <Arduino.h>

# include <GLed.h>

// Note:
// the programmable build-in LED on the different board do not have all the same gpio pin and switching logic.
// For example:
//   Ezsbc: 5
//   Wemos s2 mini sesp32-s2: 15
//   Esp32 DevKitC V4: no user led.
//   ESP32 MH-ET LIVE: 2.
//   LOLIN32: 5.

int led_gpio_num                              = 18;                  // <<< ADJUST according to your board.
GLed::gled_switching_logic_t  switching_logic = GLed::LOW_IS_ACTIVE;  // <<< ADJUST according to your board, else the on/off commands are interchanged.

// GLed gled_defualt;					// default LED configured as (LED_BUILTIN, logic LOW, state off).
// GLed gled(18, GLed::LOW_IS_ACTIVE);	// EZSBC, blue
// GLed gled(15, GLed::LOW_IS_ACTIVE);	// wemos s2 mini esp32-s2.
// GLed gled( 5, GLed::HIGH_IS_ACTIVE);	// DIY board, rain gauge esp32.

GLed gled( led_gpio_num, switching_logic );

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
  gled.async_flash();
}

void loop()
{
  static int loop_cnt = 1;
  static unsigned on_time = GLed::DEFAULT_FLASH_ON_TIME;
  static unsigned off_time = GLed::DEFAULT_FLASH_OFF_TIME;
  
  Serial.println( "the LED schould now be blinking for ever." );
  Serial.printf( "On: %d ms, Off: %d ms\n", on_time, off_time );
  delay(3000);

  Serial.printf( "%d: The loop does its job and forgets the flashing LED.\n", loop_cnt );
  delay(5000);
 
  // change the blinking timing - signal something has changed during work:
  if( loop_cnt % 2 == 0 )
    // default blinking:
    on_time = GLed::DEFAULT_FLASH_ON_TIME,
    off_time = GLed::DEFAULT_FLASH_OFF_TIME;
  else	
    // long on (2500ms), interrupted by a shorter off time (700ms). 
	on_time = 2500,
    off_time = 700;
	
  gled.async_flash_set_time_regime( on_time, off_time );
  
  loop_cnt ++;
  Serial.println();
}

// eof
