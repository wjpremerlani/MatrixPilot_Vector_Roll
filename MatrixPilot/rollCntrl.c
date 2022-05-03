// This file is part of MatrixPilot.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009-2011 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "defines.h"

#define MAX_INPUT            (1000) // maximum input in pwm units
#define ACCEL_OFFSET_MAXIMUM ((int32_t)(ROLL_OFFSET_MAXIMUM*(0.25*RMAX/57.3)))

#if (USE_CONFIGFILE == 1)
#error "USE_CONFIGFILE == 1 is not supported in this branch"
#include "config.h"
#include "redef.h"

	uint16_t yawkdail;
	uint16_t rollkp;
	uint16_t rollkd;
#elif ((SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK) || (GAINS_VARIABLE == 1))
	uint16_t yawkdail			= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	uint16_t rollkp				= (uint16_t)(ROLLKP*RMAX);
	uint16_t rollkd				= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);
#else 
	const uint16_t yawkdail		= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	const uint16_t rollkp		= (uint16_t)(ROLLKP*RMAX);
	const uint16_t rollkd		= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);
#endif	

#if (USE_CONFIGFILE == 1)
	uint16_t hoverrollkp;
	uint16_t hoverrollkd;
#elif ((SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK) || (GAINS_VARIABLE == 1))
	uint16_t hoverrollkp		= (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	uint16_t hoverrollkd		= (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
#else
	const uint16_t hoverrollkp	= (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	const uint16_t hoverrollkd	= (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
#endif

void normalRollCntrl(void);
void hoverRollCntrl(void);

#if (USE_CONFIGFILE == 1)
#error "USE_CONFIGFILE == 1 is not supported in this branch"
void init_rollCntrl(void)
{
	yawkdail 	= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	rollkp 		= (uint16_t)(ROLLKP*RMAX);
	rollkd 		= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);

	hoverrollkp = (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	hoverrollkd = (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
}
#endif

#if  ( STABILIZE_HOVER == 1 )
#error "stabilized hover is not supported in this branch, contact Bill"
#endif

void rollCntrl(void)
{
	if (canStabilizeHover() && current_orientation == F_HOVER)
	{
		hoverRollCntrl();
	}
	else
	{
		normalRollCntrl();
	}
}

#if (( INVERTED_FLIGHT_STABILIZED_MODE == 1 ) || ( INVERTED_FLIGHT_WAYPOINT_MODE == 1))
#error "inverted flight is not yet supported in this branch, contact Bill"
#endif // INVERTED FLIGHT CHECK

void normalRollCntrl(void)
{
	union longww rollAccum = { 0 };
	union longww gyroRollFeedback;
	union longww gyroYawFeedback;

	fractional rmat6;
	fractional omegaAccum2;
	int16_t steeringInput = 0 ; 
	int16_t steeringAccel = 0 ;
	int16_t desired_roll[2] ;
	int16_t actual_roll[2] ;
	int16_t roll_error ;
	int16_t roll_offset ;	

/*	//TODO: support inverted flight
	if (!canStabilizeInverted() || !desired_behavior._.inverted)
	{
		rmat6 = rmat[6];
		omegaAccum2 = omegaAccum[2];
	}
	else
	{
		rmat6 = -rmat[6];
		omegaAccum2 = -omegaAccum[2];
	}
*/
	rmat6 = rmat[6];
	omegaAccum2 = omegaAccum[2];
		
	// determine fly-by-wire pilot inputs for use in stabilized control mode
	steeringInput = 0 ; // just in case no airframe type is specified or radio is off
	if (udb_flags._.radio_on == 1)
	{
#if ( (AIRFRAME_TYPE == AIRFRAME_STANDARD) || (AIRFRAME_TYPE == AIRFRAME_GLIDER) )
		if (AILERON_INPUT_CHANNEL != CHANNEL_UNUSED)  // compiler is smart about this
		{
			steeringInput = udb_pwIn[ AILERON_INPUT_CHANNEL ] - udb_pwTrim[ AILERON_INPUT_CHANNEL ];
			steeringInput = REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, steeringInput);
		}
		else if (RUDDER_INPUT_CHANNEL != CHANNEL_UNUSED)
		{
			steeringInput = udb_pwIn[ RUDDER_INPUT_CHANNEL ] - udb_pwTrim[ RUDDER_INPUT_CHANNEL ];
			steeringInput = REVERSE_IF_NEEDED(RUDDER_CHANNEL_REVERSED, steeringInput);
		}
		else
		{
			steeringInput = 0;
		}
#endif // AIRFRAME_STANDARD

#if (AIRFRAME_TYPE == AIRFRAME_VTAIL)
#error "vtail airframe is not supported in this branch"
#endif // AIRFRAME_VTAIL

#if (AIRFRAME_TYPE == AIRFRAME_DELTA)
		// delta wing must have an aileron input, so use that
		// unmix the elevons
		int16_t aileronInput  = REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, (udb_pwIn[AILERON_INPUT_CHANNEL] - udb_pwTrim[AILERON_INPUT_CHANNEL]));
		int16_t elevatorInput = REVERSE_IF_NEEDED(ELEVATOR_CHANNEL_REVERSED, (udb_pwIn[ELEVATOR_INPUT_CHANNEL] - udb_pwTrim[ELEVATOR_INPUT_CHANNEL]));
		steeringInput = REVERSE_IF_NEEDED(ELEVON_VTAIL_SURFACES_REVERSED, ((elevatorInput - aileronInput)));
#endif // AIRFRAME_DELTA
	}

	if (steeringInput > MAX_INPUT -1) steeringInput = MAX_INPUT-1;
	if (steeringInput < -MAX_INPUT +1 ) steeringInput = -MAX_INPUT+1;

	steeringAccel = __builtin_divsd( __builtin_mulus((uint16_t)(MAXIMUM_LATERAL_ACCELERATION/4.0*RMAX), steeringInput), MAX_INPUT) ;

#ifdef TestGains
	flags._.GPS_steering = 0; // turn off navigation
#endif
	if (AILERON_NAVIGATION && flags._.GPS_steering)
	{
		rollAccum.WW = determine_navigation_deflection('a');
	}
	else
	{
		rollAccum.WW = 0 ;
	}
	
	rollAccum.WW += steeringAccel ;

//if condition that veries if arduino sends valid pwm pulses and if roll channel is defined 
// UDB PWM values are double the width in ms of a normal PWM value

	if ((udb_flags._.arduino_on) && ( ROLL_OFFSET_INPUT_CHANNEL > 0 ))
		{

//roll_offset_input stores the offset for the roll axis as determined by the pwm values sent by Arduino
			int16_t roll_offset_input ;

#if ( ROLL_OFFSET_CHANNEL_REVERSED == 0 )
//
			{
//if Arduino sends a pwm value less than 1 ms width then the roll offset is ignored
			  if (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]<2000)
				{
					roll_offset_input = 0 ;
				}
//if Arduino sends a pwm value larger than 2 ms width then the roll offset is ignored
			  if (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]>4000)
				{
				    roll_offset_input = 0 ;
				}
//if Arduino sends sends a pwm value larger than 1 ms and smaller than 2 ms then the roll offset is computed as a difference between the roll offset pwm and the neutral point (1.5 ms)
			   if ((udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]>=2000) && (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]<=4000))
				 {
					roll_offset_input = udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL] - 3000 ;
				 }
			}
#else
			{

//if Arduino sends a pwm value less than 1 ms width then the roll offset is ignored
			 if (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]<2000)
				{
					roll_offset_input = 0 ;
				}
//if Arduino sends a pwm value larger than 2 ms width then the roll offset is ignored
			  if (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]>4000)
				{
				    roll_offset_input = 0 ;
				}
//if Arduino sends sends a pwm value larger than 1 ms and smaller than 2 ms then the roll offset is computed as a difference between the roll offset pwm and the neutral point (1.5 ms)
			  if ((udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]>=2000) && (udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL]<=4000))
				 {
				roll_offset_input = 3000 - udb_pwIn[ROLL_OFFSET_INPUT_CHANNEL] ;
				 }
			}
#endif  // ROLL_OFFSET_CHANNEL_REVERSED

			roll_offset = __builtin_divsd(__builtin_mulss( roll_offset_input , ACCEL_OFFSET_MAXIMUM), 1000);
		}
		else
		{
			roll_offset = 0;
		}
//adds roll offset to roll computed value	
	rollAccum.WW += roll_offset ;


//limits roll to equivalent roll for MAXIMUM_LATERAL_ACCELERATION set in options.h file
	if (rollAccum.WW > ( MAXIMUM_LATERAL_ACCELERATION/4.0 )*RMAX )
	{
		rollAccum.WW = ( MAXIMUM_LATERAL_ACCELERATION/4.0 )*RMAX ;
	}

//limits roll to equivalent roll for MAXIMUM_LATERAL_ACCELERATION set in options.h file
	if (rollAccum.WW < - ( MAXIMUM_LATERAL_ACCELERATION/4.0 )*RMAX )
	{
		rollAccum.WW = - ( MAXIMUM_LATERAL_ACCELERATION/4.0 )*RMAX ;
	}
	
	desired_roll[0] = - rollAccum._.W0  ; // minus sign is because of UDB coordinate system
	desired_roll[1] = RMAX/4 ;
	
	actual_roll[0] = rmat[6] ;
	actual_roll[1] = rmat[8] ;
	
//normalization of the vector desired roll that has two components: - rollAccum._.W0 and RMAX/4
	vector2_normalize(desired_roll ,desired_roll ) ;

//roll error through cross product between actual_roll and desired_roll
	roll_error =  Cross2D(actual_roll,desired_roll) ;
	
#ifdef TestGains
	flags._.pitch_feedback = 1;
#endif

	if (ROLL_STABILIZATION_AILERONS && flags._.pitch_feedback)
	{
		gyroRollFeedback.WW = __builtin_mulus(rollkd , omegaAccum[1]);
		rollAccum.WW = __builtin_mulsu(roll_error , rollkp);
	}
	else
	{
		gyroRollFeedback.WW = 0;
		rollAccum.WW = 0 ;
	}

	if (YAW_STABILIZATION_AILERON && flags._.pitch_feedback)
	{
		gyroYawFeedback.WW = __builtin_mulus(yawkdail, omegaAccum2);
	}
	else
	{
		gyroYawFeedback.WW = 0;
	}

	roll_control = (int32_t)rollAccum._.W1 - (int32_t)gyroRollFeedback._.W1 - (int32_t)gyroYawFeedback._.W1;
	// Servo reversing is handled in servoMix.c
}


void hoverRollCntrl(void)
{
	int16_t rollNavDeflection;
	union longww gyroRollFeedback;

	if (flags._.pitch_feedback)
	{
		if (AILERON_NAVIGATION && flags._.GPS_steering)
		{
			rollNavDeflection = (tofinish_line > HOVER_NAV_MAX_PITCH_RADIUS/2) ? determine_navigation_deflection('h') : 0;
		}
		else
		{
			rollNavDeflection = 0;
		}
		
		gyroRollFeedback.WW = __builtin_mulus(hoverrollkd , omegaAccum[1]);
	}
	else
	{
		rollNavDeflection = 0;
		gyroRollFeedback.WW = 0;
	}

	roll_control = rollNavDeflection -(int32_t)gyroRollFeedback._.W1;
}
