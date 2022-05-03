
#include "../libUDB/libUDB.h"
#include "options.h"

#define CONFIRM_TIME ( 20 ) // 10 seconds to confirm cut down
#define CUT_DOWN_PULSE_WIDTH ( 120 ) // 60 seconds pulse

#define PARACHUTE_CONFIRM_TIME ( 8 ) // 4 seconds to confirm cut down
#define PARACHUTE_CUT_DOWN_PULSE_WIDTH ( 20 ) // 10 seconds pulse

#if (MINIUDB5 == 1)
#define CUTDOWN_LAT _LATC1
#define CUTDOWN_TRIS _TRISC1
#define PARACHUTE_LAT _LATC2
#define PARACHUTE_TRIS _TRISC2
#else
#define CUTDOWN_LAT _LATA1
#define CUTDOWN_TRIS _TRISA1
#define PARACHUTE_LAT _LATA5
#define PARACHUTE_TRIS _TRISA5
#endif
//below cutdown definitions for balloon cutdown
static int16_t cut_down_timer = CONFIRM_TIME ;

static int16_t cut_down_manual_input = 0 ;
static int16_t cut_down_trigger = 0 ;

static void ascent(void) ;
static void confirm(void) ;
static void start_cut_down(void) ;
static void end_cut_down(void) ;
static void wait(void) ;

static void (* cut_down_state) (void) = &ascent ;
//end of cutdown definitions for balloon cutdown

//======================================================

//below cutdown definitions for parachute deployment
static int16_t parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;

static int16_t parachute_cut_down_manual_input = 0 ;
static int16_t parachute_cut_down_trigger = 0 ;

static void parachute_ascent(void) ;
static void parachute_confirm(void) ;
static void parachute_start_cut_down(void) ;
static void parachute_end_cut_down(void) ;
static void parachute_wait(void) ;

static void (* parachute_cut_down_state) (void) = &parachute_ascent ;
//end of cutdown definitions for parachute deployment

//-====================================================================

//subroutines for cutdown for separation from the balloon
void init_cut_down(void)
{
	cut_down_timer = CONFIRM_TIME ;
	cut_down_state = &ascent ;
	CUTDOWN_LAT = 0 ; // initialize to be off
	CUTDOWN_TRIS = 0 ; // cut down output is on pin RA1
}

static void ascent(void)
{
	if ( cut_down_trigger == 1)
	{
		cut_down_state = &confirm ;
	}
	else
	{
		cut_down_timer = CONFIRM_TIME ;
	}
}

static void confirm(void)
{
	if ( cut_down_trigger == 1)
	{
		udb_led_toggle ( LED_ORANGE ) ;
		if ( cut_down_timer-- == 0 )
		{
			cut_down_timer = CUT_DOWN_PULSE_WIDTH ;
			cut_down_state = &start_cut_down ;
		}		
	}
	else
	{
		LED_ORANGE = LED_OFF ;
		cut_down_timer = CONFIRM_TIME ;
		cut_down_state = &ascent ;
	}
}

static void start_cut_down(void)
{
	LED_ORANGE = LED_ON ;
	CUTDOWN_LAT = 1 ;
	if ( cut_down_timer-- == 0 )
	{
		cut_down_state = &end_cut_down ;
	}
}

static void end_cut_down(void)
{
	LED_ORANGE = LED_OFF ;
	CUTDOWN_LAT = 0 ;
	if ( cut_down_trigger == 0 )
	{
		cut_down_timer = CONFIRM_TIME ;
		cut_down_state = &wait ;
	}
}

static void wait(void)
{
	if ( cut_down_manual_input == 1)
	{
		cut_down_state = &confirm ;
	}
	else
	{
		cut_down_timer = CONFIRM_TIME ;
	}
}

void cut_down_logic(void)
{
	if ( CUT_DOWN_CHANNEL_REVERSED )
	{
		if ( udb_pwIn[ CUT_DOWN_INPUT_CHANNEL ] > 3000 )
		{
			cut_down_manual_input = 0 ;
		}
		else
		{
			cut_down_manual_input = 1 ;
		}
	}
	else
	{
		if ( udb_pwIn[ CUT_DOWN_INPUT_CHANNEL ] > 3000 )
		{
			cut_down_manual_input = 1 ;
		}
		else
		{
			cut_down_manual_input = 0 ;
		}
	}
	if ( cut_down_manual_input || ( udb_flags._.radio_on != 1 ))
	{
		cut_down_trigger = 1 ;
	}
	else
	{
		cut_down_trigger = 0 ;
	}
	(* cut_down_state) () ; // execute state machine
}
//end of subroutines for cutdown for separation from the balloon

//================================================================================

//subroutines for cutdown for parachute deployment
void parachute_init_cut_down(void)
{
	parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;
	parachute_cut_down_state = &parachute_ascent ;
	PARACHUTE_LAT = 0 ; // initialize to be off
	PARACHUTE_TRIS = 0 ; // cut down output is on pin RA4 (pin available on the UDB5 board)
}

static void parachute_ascent(void)
{
	if ( parachute_cut_down_trigger == 1)
	{
		parachute_cut_down_state = &parachute_confirm ;
	}
	else
	{
		parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;
	}
}

static void parachute_confirm(void)
{
	if ( parachute_cut_down_trigger == 1)
	{
		udb_led_toggle ( LED_ORANGE ) ;
		if ( parachute_cut_down_timer-- == 0 )
		{
			parachute_cut_down_timer = PARACHUTE_CUT_DOWN_PULSE_WIDTH ;
			parachute_cut_down_state = &parachute_start_cut_down ;
		}		
	}
	else
	{
		LED_ORANGE = LED_OFF ;
		parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;
		parachute_cut_down_state = &parachute_ascent ;
	}
}

static void parachute_start_cut_down(void)
{
	LED_ORANGE = LED_ON ;
	PARACHUTE_LAT = 1 ;
	if ( parachute_cut_down_timer-- == 0 )
	{
		parachute_cut_down_state = &parachute_end_cut_down ;
	}
}

static void parachute_end_cut_down(void)
{
	LED_ORANGE = LED_OFF ;
	PARACHUTE_LAT = 0 ;
	if ( parachute_cut_down_trigger == 0 )
	{
		parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;
		parachute_cut_down_state = &parachute_wait ;
	}
}

static void parachute_wait(void)
{
	if ( parachute_cut_down_manual_input == 1)
	{
		parachute_cut_down_state = &parachute_confirm ;
	}
	else
	{
		parachute_cut_down_timer = PARACHUTE_CONFIRM_TIME ;
	}
}

void parachute_cut_down_logic(void)
{
	if ( PARACHUTE_CHANNEL_REVERSED )
	{
		if ( udb_pwIn[ PARACHUTE_INPUT_CHANNEL ] > 3000 )
		{
			parachute_cut_down_manual_input = 0 ;
		}
		else
		{
			parachute_cut_down_manual_input = 1 ;
		}
	}
	else
	{
		if ( udb_pwIn[ PARACHUTE_INPUT_CHANNEL ] > 3000 )
		{
			parachute_cut_down_manual_input = 1 ;
		}
		else
		{
			parachute_cut_down_manual_input = 0 ;
		}
	}
//ensures that the parachute is deployed only if the Tx signal is ON and if the parachute deployment command is received from the parachute channel
	if ( parachute_cut_down_manual_input && ( udb_flags._.radio_on == 1 ))
	{
		parachute_cut_down_trigger = 1 ;
	}
	else
	{
		parachute_cut_down_trigger = 0 ;
	}

// making sure that the parachute is certainly not deployed if Tx signal is loss	
//	if (udb_flags._.radio_on != 1)
//	{
//		parachute_cut_down_trigger = 0 ;
//	}
	(* parachute_cut_down_state) () ; // execute state machine
}
//end of subroutines for cutdown for parachute deployment
