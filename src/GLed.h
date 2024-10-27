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
#define FLASH_TASK_CORE 0
#else
#define FLASH_TASK_CORE tskNO_AFFINITY
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
    static const int   DEFAULT_FLASH_ON_TIME = 64;                 ///< default on time per flash [ms]
    static const int   DEFAULT_FLASH_OFF_TIME = 1000;              ///< default off time per flash [ms]
    static const int MAX_FLASH = 100;                              ///< syncron flash: truncate the number a flashes to this value.
    static const uint64_t FLASH_FOR_EVER = (uint64_t)(~0);         ///< number of blink sequences to be made.

    /**
     * the GLed standard constructor initializes the object
     * for use with the build in LED for the NodeMCU v3 board.
     * A negative logic is used to turn on and off the LED.
     * On the "NodeMUC v3" or "WEMOS d1 mini" the build in led is connected to VCC.
     */
    GLed()
    	: pin(MY_LED_BUILDIN)
    	, state(0)
    	, activated(false)
    	, on_is_high_level(false)
    	, flash_count(0)
		, flash_dt_on(0)
		, flash_dt_off(0)
		, flash_task_handle(nullptr)
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
		, flash_dt_on(0)
		, flash_dt_off(0)
		, flash_task_handle(nullptr)

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
		, flash_dt_on(0)
		, flash_dt_off(0)
    	, flash_task_handle(nullptr)
    { 
		set_logic_mode( a_switch_logic );
	};

	~GLed();

    /**
     * must be called before the led may be switched on or off.
     * This command sets the GPIO port into the correct mode.
     * If a LED is not activated the on/off/flash calls will be without any effects.</br>
     */
    void begin();

    /**
     * deactivate the LED and switch to off. A running flash task gets terminated.
     * All further LED switching commands to the control pin gets ignored.
     * But status settings like switching logic or reassignments are still allowed.
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
     * flash - start blinking of the activated LED for a given number of flashes.
     * Note: flash() conserves the lightening state as it was just when flash() gets called.
     *
     * If "count" is less than 1 no blinking is generated.
     * The call is synchronous and blocks until the blinking is done.
     * A flash() call will return after the blinking sequence has finished.
     * A single flash sequence consists always in the on and off time interval.
     * So the total amount of the delay is for count >= 1: count*(dt_on+dt_off)  [ms].
     * @param count: number of blinks. Count is truncated to maximal MAX_DELAY_FLASH.
     * @param dt_on: time during which the LED is ON when blinking (ms).
     * @param dt_off: time during which the LED is OFF when blinking (ms). If 0 then dt_on gets used.
     */
    void flash( unsigned count = 3, unsigned dt_on =   DEFAULT_FLASH_ON_TIME, unsigned dt_off = DEFAULT_FLASH_OFF_TIME );

    /** start blinking the activated LED like flash() but none blocking is done.
     *  A thread is started to perform the blinking.
     *  The call of async_flash() will return immediately
     *  and does not wait until the blinking sequence has completed.
     *  If a previous async_flash() task is already running when an new call is made
     *  the new count and time values gets set and used within the next blink sequence.
     *  No further thread is started in such a case.
     *  The count gets decreased at each blink sequence and the underling thread gets terminated if zero gets reached.  
	 *  @param count: number of flashes. Count is not truncated !
     *  @param dt_on: time during which the LED is ON when blinking (ms).
     *  @param dt_off: time during which the LED is OFF when blinking (ms). If 0 then dt_on gets used.
     *  @param core_no: core to run the flash thread.
     *  @return pdPASS, if the task was successfully created and added to a ready list,
     *                  otherwise an error code (see xTaskCreatePinnedToCore() for the code).
     */
    int async_flash( uint64_t count = FLASH_FOR_EVER, 
    				 unsigned dt_on = DEFAULT_FLASH_ON_TIME, 
    				 unsigned dt_off = DEFAULT_FLASH_OFF_TIME, 
    				 int core = FLASH_TASK_CORE );

	/*
	 * set the on and off time periods for the flash thread.
	 * If a flash tread is running the new setting gets used in the next flash period. 
	 * @param dt_on: time during which the LED is ON when blinking (ms).
     * @param dt_off: time during which the LED is OFF when blinking (ms). If 0 then dt_on gets used.
	*/
    void async_flash_set_time_regime( unsigned dt_on, unsigned dt_off );


    /**
     * Reassign the pin which is connected to the LED.
     * The object gets reinitialized as off and not activated.
     * You have to activate the LED again using begin() before she can be switched.
     * \note The gpio pin state of the previously controlled LED pin is set to led off and
     *       a running flash task gets terminated. But the gpio pin configuration itself is not changed.
     *
     *  @param pin   new controlling GPIO pin.
     *  @param logic if HIGH_IS_ACTIVE it is assumed that the LED is lightening if its gpio port ist HIGH.
     * And that the LED is off when the gpio pin is set to LOW. On LOW_IS_ACTIVE the switching logic is reverse.
     */
    void reconnect_to_pin( int pin, gled_switching_logic_t logic = GLed::HIGH_IS_ACTIVE  );

private:
    int pin;
    int state;
    volatile bool activated;
    bool on_is_high_level;
    volatile uint64_t flash_count;
    volatile unsigned flash_dt_on;
	volatile unsigned flash_dt_off;
    TaskHandle_t flash_task_handle;

friend
	void task_flash( void *pvParameters );
};

#endif

// eof
