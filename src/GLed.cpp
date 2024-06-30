/////////////////////////////////////////////////////////////////////////////
//
//  Project:       ESP32
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
// file:           GLed.cpp
// language:       C++
// compiler:       g++ (Arduino IDE)
//
// REVIEW:
/////////////////////////////////////////////////////////////////////////////
//

//
//  LED control
//

#include <Arduino.h>

#include "GLed.h"

typedef struct { GLed * pGLed; } task_args_t;

void task_flash( void *pvParameters );

static const char* TAG = "GLED";

void GLed::begin()
{
	ESP_LOGW( TAG, "LED (%d) activated", pin );
    pinMode(pin, OUTPUT);
    activated = true;
    off();
}

void GLed::end()
{
	ESP_LOGW( TAG, "LED (%d) disabled", pin );
    off();
    activated = false;
}

void GLed::set_logic_mode( gled_switching_logic_t logic )
{
    if( logic == LOW_IS_ACTIVE )
        on_is_high_level = false;
    else if( logic == HIGH_IS_ACTIVE )
        on_is_high_level = true;
    else
        /*not reached*/
        on_is_high_level = true;
}

void GLed::set_logic_mode( bool a_on_is_high_level )
{
    on_is_high_level = a_on_is_high_level;
}


void GLed::on()
{
    if( activated ) {
        state = 1;
        digitalWrite(pin, on_is_high_level ? HIGH : LOW);
    }
}

void GLed::off()
{
    if( activated ) {
        state = 0;
        digitalWrite(pin, on_is_high_level ? LOW : HIGH);
    }
}

void GLed::toggle()
{
    if( is_on() )
        off();
    else
        on();
}

void GLed::flash( unsigned int count, unsigned int dt )
{
    // delay time: count >=1;  (2*count-1)*dt
    if( activated ) {
    	bool led_mode = is_on();

        if( count > MAX_FLASH ) // avoid an almost endless blocking loop
            count = MAX_FLASH;

        for( unsigned int i = 0; i < count; i++ ) {
            if( i > 0 )
                delay(dt);
            on();
            delay(dt);
            off();
        }

        switch_lightening( led_mode );
    }
}

int GLed::async_flash( unsigned int count, unsigned int dt, int core )
{
	// run time: count >=1;  (2*count-1)*dt  [ms].

    int rc = 0;
    task_args_t task_args;

    if( flash_task_handle != nullptr ) {
    	ESP_LOGW( TAG, "task_flash already running. Reject the new setting (%u, %u, %d)", count, dt, activated );
    	return 0;
    }

    if( activated ) {
		#if CONFIG_FREERTOS_UNICORE
			core = 0;
		#endif

		if( core != 0 && core != 1 )
			core = ARDUINO_LED_RUNNING_CORE;

    	task_args.pGLed = this;
    	flash_count = count;
    	flash_dt = dt;

		rc = xTaskCreatePinnedToCore(
			task_flash
			,  "task_flash"
			,  2048  // Stack size
			,  & task_args  // When no parameter is used, simply pass NULL
			,  2  	 // Priority
			,  & flash_task_handle // With task handle we will be able to manipulate with this task.
			,  core // Core on which the task will run
		);
    }

    return rc;
}

void task_flash( void *pvParameters ){
	GLed * pGLed = ((task_args_t*)(pvParameters)) -> pGLed;
	const unsigned int count = pGLed -> flash_count;
	const unsigned int dt =    pGLed -> flash_dt;

	ESP_LOGW( TAG, "task_flash started (%u, %u, %d)", count, dt, (int) pGLed->activated );

	const TickType_t xDelay = dt / portTICK_PERIOD_MS;
	const bool led_mode = pGLed->is_on();

    for( unsigned int i = 0; i < count && pGLed->activated; i++ ) {
            if( i > 0 )
                vTaskDelay( xDelay );
            pGLed->on();
    		vTaskDelay( xDelay );
            pGLed->off();
            pGLed -> flash_count --; // inform the GLed object about the remaining pulses count.
    }
    ESP_LOGW( TAG, "task_flash terminated (%u, %u, %d)", count, dt, (int) pGLed->activated );
    pGLed->switch_lightening( led_mode );
    pGLed -> flash_task_handle = nullptr;
	vTaskDelete(NULL);
}


void GLed::reconnect_to_pin( int a_pin, gled_switching_logic_t logic )
{
    on_is_high_level = logic == HIGH_IS_ACTIVE;
    pin = a_pin;
    state = 0;
}
// ---------------------------------------------------------------------------

// eof
