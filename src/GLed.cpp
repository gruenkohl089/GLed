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

GLed::~GLed()
{
	end();
}

void GLed::begin()
{
	ESP_LOGW( TAG, "LED (%d) activated, lights up if gpio%d is %d", pin, pin, get_logic_mode() == HIGH_IS_ACTIVE );
    pinMode(pin, OUTPUT);
    activated = true;
    off();
}

void GLed::end()
{
	ESP_LOGW( TAG, "LED (%d) disabled", pin );
	if( flash_task_handle != nullptr ) {
		vTaskDelete(flash_task_handle );
	}
	flash_task_handle = nullptr;
	off();
	activated = false;   // This will also terminate the flash thread if running.
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

void GLed::flash( unsigned count, unsigned dt_on, unsigned dt_off )
{
    // delay time: count >=1;  count*(dt_on+dt_off) - dt_off
    if( activated ) {
    	bool led_mode = is_on();

        if( count > MAX_FLASH ) // avoid an almost endless blocking loop
            count = MAX_FLASH;

        for( unsigned int i = 0; i < count; i++ ) {
            if( i > 0 )
                delay(dt_off);
            on();
            delay(dt_on);
            off();
        }

        switch_lightening( led_mode );
    }
}

void GLed::async_flash_set_time_regime( unsigned dt_on, unsigned dt_off )
{
	ESP_LOGI( TAG, "async_flash_set_time_regime: old on=%u off=%u ms", flash_dt_on,  flash_dt_off );
    flash_dt_on = dt_on;
	flash_dt_off = dt_off == 0 ? dt_on : dt_off;
	ESP_LOGI( TAG, "                             new on=%u off=%u ms", flash_dt_on,  flash_dt_off );
}

int GLed::async_flash( uint64_t count, unsigned dt_on, unsigned dt_off, int core_num )
{
	// run time: for count >=1: (2*count-1)*dt  [ms].

	// NOTE: changing the blinking parameters is still in development.
	//       The current implementation may be critical and needs a redesign due to a thread race condition.
	
    int rc = 0;
    static task_args_t task_args;

	if( flash_task_handle != nullptr )    // thread running, observe the RACE CONDITION 
    { 
    	// reset running thread parameters:
		ESP_LOGI( TAG, "task_flash already running remaining: flash_count=%" PRIu64 ", flash_dt=(%u,%u)", flash_count, flash_dt_on,  flash_dt_off );
		// set the new parameters:
		flash_count = count;
		async_flash_set_time_regime( dt_on, dt_off );
		ESP_LOGI( TAG, "reset with new param's: flash_count=%" PRIu64 ", flash_dt=(%u,%u), core=%x, led activated=%d", flash_count, flash_dt_on, flash_dt_off, core_num, activated );
    }
	else
	{
		// start a flash thread:
		ESP_LOGI( TAG, "async_flash: start a flash thread: flash_count=%" PRIu64 ", flash_dt=(%u,%u), core=%x, led activated=%d", flash_count, flash_dt_on, flash_dt_off, core_num, activated );

    	flash_count = count;
    	async_flash_set_time_regime( dt_on, dt_off );

    	if( activated )
    	{
    		task_args.pGLed = this;

			rc = xTaskCreatePinnedToCore(
					task_flash
					,  "task_flash"
					,  2048
					,  & task_args
					,  2
					,  & flash_task_handle
					,  core_num
			);
		}
	}

    return rc;
}

void task_flash( void *pvParameters ){
	GLed * pGLed = ((task_args_t*)(pvParameters)) -> pGLed;

	ESP_LOGI( TAG, "task_flash started (#=%" PRIu64 ", dt=(%u,%u) activated=%d)", 
					pGLed -> flash_count, pGLed -> flash_dt_on, pGLed -> flash_dt_off, (int) pGLed->activated );

	const bool start_lightening = pGLed->is_on();

	// terminate the thread if the LED gets de-activated or the counter countdown reaches 0:
    while( pGLed -> flash_count && pGLed->activated  ) {
    	pGLed->on();
		vTaskDelay( pGLed -> flash_dt_on / portTICK_PERIOD_MS  );
        pGLed->off();
        vTaskDelay( pGLed -> flash_dt_off / portTICK_PERIOD_MS );
        // just be sure: no overflow: never go from count = 0 to count = -1 == =xFFFF...FFFFF
        if( pGLed -> flash_count > 0 )
        {
        	const uint64_t c = pGLed -> flash_count - 1;
        	pGLed -> flash_count = c; // inform the GLed object about the remaining pulse count.
        }
    }

    ESP_LOGW( TAG, "task_flash terminating (#=%" PRIu64 ", activated=%d)", 
    				pGLed -> flash_count, (int) pGLed->activated );
    pGLed->switch_lightening( start_lightening );
    pGLed -> flash_task_handle = nullptr;
	vTaskDelete(NULL);
}

void GLed::reconnect_to_pin( int a_pin, gled_switching_logic_t logic )
{
	end();

    state = 0;
    activated = 0;
    set_logic_mode( logic );
    pin = a_pin;
}
// ---------------------------------------------------------------------------

// eof
