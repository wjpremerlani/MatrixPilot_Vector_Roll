// This file is part of the MatrixPilot LedTest demo.
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


////////////////////////////////////////////////////////////////////////////////
// options.h
// Bill Premerlani's UAV Dev Board
// 


// - First it tests all IO connections on the board.  This will only succeed
// if the board is sitting in SparkFun's green test rig.  If this fails (which
// it will for all normal users) the Red and Blue LEDs will flash 16 times.
// 
// - It then blinks the 4 LEDs in sequence 2 times, while calibrating the
// sensors and testing the EEPROM.  Keep the board flat and motionless
// until the this step is done.
// 
// - If the EEPROM test fails, it freaks out and flashes all 4 LEDs 16 times.
// 
// - If the EEPROM test succeeds, or after flashing if it failed, it switches
// over to alternately testing the accelerometers and testing the gyros for
// 15 seconds each.
// 
// - The Red LED turns on while testing the gyros, and off while
// testing the accelerometers.
// 
// - To test the gyros, rotate the board back and forth on one axis at
// a time and watch for the corresponding LED to flash.
// 
// - To test the accels, I like to let gravity do the work.  Point a
// short edge of the board up and see one LED go on, then point a long
// edge up, then sit it flat to test the Z accel.



////////////////////////////////////////////////////////////////////////////////
// Set Up Board Type
#define BOARD_TYPE 							AUAV3_BOARD


////////////////////////////////////////////////////////////////////////////////
// Use board orientation to change the mounting direction of the board.
// The following 4 orientations have the board parallel with the ground.
// ORIENTATION_FORWARDS:  Component-side up,   GPS connector front
// ORIENTATION_BACKWARDS: Component-side up,   GPS connector back
// ORIENTATION_INVERTED:  Component-side down, GPS connector front
// ORIENTATION_FLIPPED:   Component-side down, GPS connector back
// The following 2 orientations are "knife edge" mountings
// ORIENTATION_ROLLCW: Rick's picture #9, board rolled 90 degrees clockwise,
//		from point of view of the pilot
// ORIENTATION_ROLLCW180: Rick's pitcure #11, board rolled 90 degrees clockwise,
//		from point of view of the pilot, then rotate the board 180 around the Z axis of the plane,
//		so that the GPS connector points toward the tail of the plane
#define BOARD_ORIENTATION					ORIENTATION_FORWARDS


////////////////////////////////////////////////////////////////////////////////
// Set this value to your GPS type.  (Set to GPS_STD, GPS_UBX_2HZ, or GPS_UBX_4HZ)
#define GPS_TYPE							GPS_STD

// Note: As of MatrixPilot 3.0, Dead Reckoning and Wind Estimation are automatically enabled.

// Define MAG_YAW_DRIFT to be 1 to use magnetometer for yaw drift correction.
// Otherwise, if set to 0 the GPS will be used.
#define MAG_YAW_DRIFT 						0

// Set this to 1 if you want the UAV Dev Board to fly your plane without a radio transmitter or
// receiver. (Totally autonomous.)  This is just meant for debugging.  It is not recommended that
// you actually use this since there is no automatic landing code yet, and you'd have no manual
// control to fall back on if things go wrong.  It may not even be legal in your area.
#define NORADIO								1


////////////////////////////////////////////////////////////////////////////////
// Configure Input and Output Channels
//
// NUM_INPUTS: Set to 0-5 
//   1-4 enables only the first 1-4 of the 4 standard input channels
//   5 also enables E8 as the 5th input channel
#define NUM_INPUTS	0

// NUM_OUTPUTS: Set to 3, 4, 5, or 6
//   3 enables only the standard 3 output channels
//   4 also enables E0 as the 4th output channel
//   5 also enables E2 as the 5th output channel
//   6 also enables E4 as the 6th output channel
#define NUM_OUTPUTS	6

// Channel numbers for each output
// Use as is, or edit to match your setup.
//   - Only assign each channel to one output purpose
//   - If you don't want to use an output channel, set it to CHANNEL_UNUSED
// 
// NOTE: If your board is powered from your ESC through the throttle cable, make sure to
// connect THROTTLE_OUTPUT_CHANNEL to one of the built-in Outputs (1, 2, or 3) to make
// sure your board gets power.
// 
#define ROLL_OUTPUT_CHANNEL					CHANNEL_1
#define PITCH_OUTPUT_CHANNEL				CHANNEL_2
#define YAW_OUTPUT_CHANNEL					CHANNEL_3

#define X_ACCEL_OUTPUT_CHANNEL				CHANNEL_4
#define Y_ACCEL_OUTPUT_CHANNEL				CHANNEL_5
#define Z_ACCEL_OUTPUT_CHANNEL				CHANNEL_6


////////////////////////////////////////////////////////////////////////////////
// The Failsafe Channel is the RX channel that is monitored for loss of signal
// Make sure this is set to a channel you actually have plugged into the UAV Dev Board!
// 
// For a receiver that remembers a failsafe value for when it loses the transmitter signal,
// like the Spektrum AR6100, you can program the receiver's failsafe value to a value below
// the normal low value for that channel.  Then set the FAILSAFE_INPUT_MIN value to a value
// between the receiver's programmed failsafe value and the transmitter's normal lowest
// value for that channel.  This way the firmware can detect the difference between a normal
// signal, and a lost transmitter.
//
// FAILSAFE_INPUT_MIN and _MAX define the range within which we consider the radio on.
// Normal signals should fall within about 2000 - 4000.
#define FAILSAFE_INPUT_CHANNEL				THROTTLE_INPUT_CHANNEL
#define FAILSAFE_INPUT_MIN					1500
#define FAILSAFE_INPUT_MAX					4500


////////////////////////////////////////////////////////////////////////////////
// Control gains.
// All gains should be positive real numbers.

// SERVOSAT limits servo throw by controlling pulse width saturation.
// set it to 1.0 if you want full servo throw, otherwise set it to the portion that you want
#define SERVOSAT							1.0


////////////////////////////////////////////////////////////////////////////////
// Hardware In the Loop Simulation
// Only set this to 1 for testing in the simulator.  Do not try to fly with this set to 1!
// Requires setting GPS_TYPE to GPS_UBX_4HZ.
// See the MatrixPilot wiki for more info on using HILSIM.
#define HILSIM 								0


////////////////////////////////////////////////////////////////////////////////
// the following define is used to test the above gains and parameters.
// if you define TestGains, their functions will be enabled, even without GPS or Tx turned on.
// #define TestGains						// uncomment this line if you want to test your gains without using GPS
