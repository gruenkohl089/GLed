/////////////////////////////////////////////////////////////////////////////
//
//  Project:       ESP32 TOOLS
//
//  Subcomponent:  ESP32x Led Control
/////////////////////////////////////////////////////////////////////////////
//
// abstract:       The GLed class models an LED.
// premises:	   ESP32 or ESP32 variant.
// remarks:
// history:        10.04.2020, GJK, created.
// AUTHOR:         G.Kasper
// contact:        info@georgkasper.de
// copyright:      Georg Kasper
// file:           GLed.h
// language:       C++
// compiler:       g++ (for ex. the Arduino IDE compiler)
//
// REVIEW:
/////////////////////////////////////////////////////////////////////////////
//
#ifndef GLED_HEADER_H
#define GLED_HEADER_H

#include <Arduino.h>

// Here i follow the convention that GPIO 2 may control a build in LED.
// But be aware this is only a guess, many boards use a different gpio to control the LED.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_LED_RUNNING_CORE 0
#else
#define ARDUINO_LED_RUNNING_CORE 1
#endif

/**
 * The GLed class models an LED. It provides methods to manipulate the LED
 * and switch it on and off. This conceals the fact that the switching logic of the
 * LED can be positive or negative depending on the type of electronic circuit.
 * Some LEDs are switched on by switching the pin that controls the LED to HIGH (3.3V).
 * Others light up by switching the control pin to LOW (GND). By abstracting this detail,
 * code can be created whose on/off commands are identical for all boards.
 * To do this, a GLed object only needs to be told which pin is used and whether
 * the switching logic is positive (HIGH) or negative (GND) when it is created.
 * \n
 * This class is not thread save.
 */
class GLed {
public:
    enum gled_switching_logic_t { LOW_IS_ACTIVE, HIGH_IS_ACTIVE }; ///< switching logic selection type.

    static const int MY_LED_BUILDIN = LED_BUILTIN;                 ///< setup for NodeMCU v3 / Wemos d1 mini board & Co.
    static const int DEFAULT_DELAY_FLASH = 64;                     ///< default on time per flash
    static const int MAX_FLASH = 100;                              ///< truncate the number a flashes to this value.

    /**
     * the GLed standard constructor initializes the object
     * for use with the build in LED for the NodeMCU v3 board.
     * On the "NodeMUC v3" or "WEMOS d1 mini" the build in led is connected to VCC.
     * So a negative logic is used to turn on and off the LED.
     */
    GLed()
    	: pin(MY_LED_BUILDIN)
    	, state(0)
    	, activated(false)
    	, on_is_high_level(false)
    	, flash_count(0)
		, flash_dt(0)
    {};

    /**
     * if only a pin (GPIO number) to which the LED is connected is given it is assumed
     * that a positive logic is used to turn ON and OFF.\n
     * The LED is assumed to be connected to GND and gets it power from the GPIO port
     * which switches the LED.
     *  @param pin: the GPIO number to which the LED is connected.
     */
    GLed(int a_pin )
        : pin(a_pin)
        , state(0)
        , activated(false)
        , on_is_high_level(true)
    	, flash_count(0)
		, flash_dt(0)
		, flash_task_handle(0)
    {};

    /**
     * make a LED control object for a LED with a given GPIO and switching logic.
     * @param: pin: the GPIO number connected to the LED.
     * @param: a_switch_logic:
     *         if GLed::HIGH_IS_ACTIVE:
     *            the LED has to be ON if the GPIO port is HIGH and OFF on LOW.
     *         if GLed::LOW_IS_ACTIVE: the LED has to be OFF if the GPIO port is HIGH and ON on LOW.
     */
    GLed(int a_pin, gled_switching_logic_t a_switch_logic )
        : pin(a_pin)
        , state(0)
        , activated(false)
        , on_is_high_level(a_switch_logic == HIGH_IS_ACTIVE)
    	, flash_count(0)
		, flash_dt(0)
    	, flash_task_handle(0)
    {};


    /**
     * must be called before the led may be switched on or off.
     * This command sets the GPIO port into the correct mode.
     * If a LED is not activated the on/off/flash calls will be without any effects.</br>
     */
    void begin();

    /**
     * deactivate the LED and switch to off.
     * If called all output commands to the control pin gets suppressed.
     * But the status settings like switching logic or reassignments are still allowed.
     */
    void end();

	/**
	 * deactivate the led. Alias for end().
	 */
    void deactivate() { end(); }

    /**
     * set the switching logic.
     *  @param logic if HIGH_IS_ACTIVE it is assumed that the LED is lightening
     *              if its gpio port is HIGH.
     * And that the LED is off when the gpio pin is set to LOW.
     * \n
     * If set to LOW_IS_ACTIVE: the logic is just the inverse as before.
     * The LED is lightening if the gpio is set to LOW.
     */
    void set_logic_mode( gled_switching_logic_t logic );

    /**
     * set the switching logic.
     *  @param on_is_high_level, if true it is assumed that the LED is lightening if its gpio port is HIGH.
     * And that the LED is off when the gpio pin is set to LOW.
     * \n
     * If set to false: the logic is just the inverse as before.
     * The LED is lightening if the gpio is set to LOW.
     */
    void set_logic_mode( bool on_is_high_level = true );

	/**
     * get the switching logic.
	 * @returns switching logic of type gled_switching_logic_t,
	 *  returns one of the both values:  LOW_IS_ACTIVE, HIGH_IS_ACTIVE.
	 */
    gled_switching_logic_t get_logic_mode( void ) const { return on_is_high_level ? HIGH_IS_ACTIVE : LOW_IS_ACTIVE; }

    /**
     * make the led lightening.
     */
    void on();

    /**
     * switch the led off.
     */
    void off();

    /**
     * switch led on or off.
     * @param mode  if true let the LED shine, else shut it off.
     */
    void switch_lightening( bool mode ) { if(mode) on(); else off(); }

    /**
     * check if the LED is lightening.
     * @returns true if on, false if off.
     */
    bool is_on() const
    {
        return state != 0;
    }

    /** change the lightening state of the LED.
     * If the LED is lightening then a toggle() call will switch off the LED and visa versa.
     */
    void toggle();

    /**
     * get the pin used to control the LED of this object.
     * @parmas pin: pin number
     */
    int get_pin() const { return pin; }

    /**
     * flash - blinking the activated LED.
     * Note: flash() conserves the lightening state as it was just when flash() gets called.
     *
     * If "count" is less than 1 no blinking is generated.
     * The call is synchronous and blocks until the blinking is done.
     * A flash() call will return after the blinking sequence has finished.
     * So the total amount of the delay is for count >= 1:  (2*count-1)*dt  [ms].
     *  @param count: number of blinks. Count is truncated to maximal MAX_DELAY_FLASH.
     *  @param dt:    time during which the LED is ON (also OFF) when blinking (ms).
     */
    void flash( unsigned int count = 1, unsigned int dt = DEFAULT_DELAY_FLASH);

    /** blinking the activated LED like flash() but none blocking is done.
     *  The call of async_flash() will return immediately
     *  and does not wait until the blinking sequence has completed.
	 *  @param count: number of flashes. Count is not truncated !
     *  @param dt:    time during which the LED is ON (also OFF) when blinking (ms).
     *  @param core_no: core to run the flash thread.
     *  @return pdPASS, if the task was successfully created and added to a ready list,
     *                  otherwise an error code (see xTaskCreatePinnedToCore() for the code).
     */
    int async_flash( unsigned int count = 13, unsigned int dt = DEFAULT_DELAY_FLASH, int core = ARDUINO_LED_RUNNING_CORE );

    /**
     * Reassign the pin which is connected to the LED.
     * The object gets reinitialized as off and not activated.
     * You have to activate the LED again using begin() before she can be switched.
     * \note The (pin) state of the previously controlled LED port remains unchanged.
     *  You may want to set the LED to off before calling reconnect_to_pin().
     *
     *  @param pin   new controlling GPIO pin.
     *  @param logic if HIGH_IS_ACTIVE it is assumed that the LED is lightening if its gpio port ist HIGH.
     * And that the LED is off when the gpio pin is set to LOW. On LOW_IS_ACTIVE the switching logic is reverse.
     */
    void reconnect_to_pin( int pin, gled_switching_logic_t logic = GLed::HIGH_IS_ACTIVE  );

private:
    int pin;
    int state;
    bool activated;
    bool on_is_high_level;
    unsigned flash_count;
    unsigned flash_dt;
    TaskHandle_t flash_task_handle;

friend
	void task_flash( void *pvParameters );
};

#endif

// eof
