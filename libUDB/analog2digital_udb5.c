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


#include "libUDB_internal.h"
#include "oscillator.h"
#include "interrupt.h"

#if (BOARD_TYPE == UDB5_BOARD)

#define ALMOST_ENOUGH_SAMPLES 216 // there are 222 or 223 samples in a sum

#if (NUM_ANALOG_INPUTS >= 1)
struct ADchannel udb_analogInputs[NUM_ANALOG_INPUTS]; // 0-indexed, unlike servo pwIn/Out/Trim arrays
#endif
struct ADchannel udb_vcc;
struct ADchannel udb_5v;
struct ADchannel udb_vref; // reference voltage (deprecated, here for MAVLink compatibility)

// Number of locations for ADC buffer = 6 (AN0,15,16,17,18) x 1 = 6 words
// Align the buffer. This is needed for peripheral indirect mode
#define NUM_AD_CHAN 6
int16_t  BufferA[NUM_AD_CHAN] __attribute__((space(dma),aligned(32)));
int16_t  BufferB[NUM_AD_CHAN] __attribute__((space(dma),aligned(32)));

//int16_t vref_adj;
int16_t sample_count;
uint8_t DmaBuffer = 0;

#if (RECORD_FREE_STACK_SPACE == 1)
uint16_t maxstack = 0;
#endif

void udb_init_ADC(void)
{
	sample_count = 0;

	AD1CON1bits.FORM   = 3;		// Data Output Format: Signed Fraction (Q15 format)
	AD1CON1bits.SSRC   = 7;		// Sample Clock Source: Auto-conversion
	AD1CON1bits.ASAM   = 1;		// ADC Sample Control: Sampling begins immediately after conversion
	AD1CON1bits.AD12B  = 1;		// 12-bit ADC operation

	AD1CON2bits.CSCNA = 1;		// Scan Input Selections for CH0+ during Sample A bit
	AD1CON2bits.CHPS  = 0;		// Converts CH0

	AD1CON3bits.ADRC = 0;		// ADC Clock is derived from System Clock
	AD1CON3bits.ADCS = 11;		// ADC Conversion Clock Tad=Tcy*(ADCS+1)= (1/40M)*12 = 0.3us (3333.3Khz)
								// ADC Conversion Time for 12-bit Tc=14*Tad = 4.2us
	AD1CON3bits.SAMC = 1;		// No waiting between samples

	AD1CON2bits.VCFG = 0;		// use supply as reference voltage

	AD1CON1bits.ADDMABM = 1; 	// DMA buffers are built in sequential mode
	AD1CON2bits.SMPI    = (NUM_AD_CHAN-1);	
	AD1CON4bits.DMABL   = 0;	// Each buffer contains 1 word

	AD1CSSL = 0x0000;
	AD1CSSH = 0x0000;
	AD1PCFGL= 0xFFFF;
	AD1PCFGH= 0xFFFF;

//	include voltage monitor inputs
	_CSS0 = 1;		// Enable AN0 for channel scan
	_CSS1 = 1;		// Enable AN1 for channel scan
	_PCFG0 = 0;		// AN0 as Analog Input
	_PCFG1 = 0;		// AN1 as Analog Input

//  do not include the extra analog input pins
//	_CSS15 = 1;		// Enable AN15 for channel scan
//	_CSS16 = 1;		// Enable AN16 for channel scan
//	_CSS17 = 1;		// Enable AN17 for channel scan
//	_CSS18 = 1;		// Enable AN18 for channel scan

//	_PCFG15 = 0;	// AN15 as Analog Input 
//	_PCFG16 = 0;	// AN16 as Analog Input 
//	_PCFG17 = 0;	// AN17 as Analog Input 
//	_PCFG18 = 0;	// AN18 as Analog Input 

	_AD1IF = 0;				// Clear the A/D interrupt flag bit
	_AD1IP = INT_PRI_AD1;	// Set the interrupt priority
	_AD1IE = 0;				// Do Not Enable A/D interrupt
	AD1CON1bits.ADON = 1;	// Turn on the A/D converter

//  DMA Setup
	DMA0CONbits.AMODE = 2;			// Configure DMA for Peripheral indirect mode
	DMA0CONbits.MODE  = 2;			// Configure DMA for Continuous Ping-Pong mode
	DMA0PAD=(int16_t)&ADC1BUF0;
	DMA0CNT = NUM_AD_CHAN-1;
	DMA0REQ = 13;					// Select ADC1 as DMA Request source

	DMA0STA = __builtin_dmaoffset(BufferA);
	DMA0STB = __builtin_dmaoffset(BufferB);

	_DMA0IP = INT_PRI_DMA0;			// Set the DMA ISR priority
	_DMA0IF = 0;					// Clear the DMA interrupt flag bit
	_DMA0IE = 1;					// Set the DMA interrupt enable bit

	DMA0CONbits.CHEN = 1;			// Enable DMA
}

void __attribute__((__interrupt__,__no_auto_psv__)) _DMA0Interrupt(void)
{
	indicate_loading_inter;
	interrupt_save_set_corcon;
	
#if (RECORD_FREE_STACK_SPACE == 1)
	uint16_t stack = SP_current();
	if (stack > maxstack)
	{
		maxstack = stack;
	}
#endif
	
#if (HILSIM != 1)
	int16_t *CurBuffer = (DmaBuffer == 0) ? BufferA : BufferB;
	udb_vcc.input = CurBuffer[A_VCC_BUFF-1];
	udb_5v.input =  CurBuffer[A_5V_BUFF-1];
	
#if (NUM_ANALOG_INPUTS >= 1)
	udb_analogInputs[0].input = CurBuffer[analogInput1BUFF-1];
#endif
#if (NUM_ANALOG_INPUTS >= 2)
	udb_analogInputs[1].input = CurBuffer[analogInput2BUFF-1];
#endif
#if (NUM_ANALOG_INPUTS >= 3)
	udb_analogInputs[2].input = CurBuffer[analogInput3BUFF-1];
#endif
#if (NUM_ANALOG_INPUTS >= 4)
	udb_analogInputs[3].input = CurBuffer[analogInput4BUFF-1];
#endif

#endif // HILSIM

	DmaBuffer ^= 1;				// Switch buffers
	IFS0bits.DMA0IF = 0;		// Clear the DMA0 Interrupt Flag

	if (udb_flags._.a2d_read == 1) // prepare for the next reading
	{
		udb_flags._.a2d_read = 0;
//		udb_xrate.sum = udb_yrate.sum = udb_zrate.sum = 0;
//		udb_xaccel.sum = udb_yaccel.sum = udb_zaccel.sum = 0;
#ifdef VREF
//		udb_vref.sum = 0;
#endif
	udb_vcc.sum = 0;
	udb_5v.sum = 0;
#if (NUM_ANALOG_INPUTS >= 1)
		udb_analogInputs[0].sum = 0;
#endif
#if (NUM_ANALOG_INPUTS >= 2)
		udb_analogInputs[1].sum = 0;
#endif
#if (NUM_ANALOG_INPUTS >= 3)
		udb_analogInputs[2].sum = 0;
#endif
#if (NUM_ANALOG_INPUTS >= 4)
		udb_analogInputs[3].sum = 0;
#endif
		sample_count = 0;
	}

	//	perform the integration:
	udb_vcc.sum += udb_vcc.input;
	udb_5v.sum +=  udb_5v.input;

#if (NUM_ANALOG_INPUTS >= 1)
	udb_analogInputs[0].sum += udb_analogInputs[0].input;
#endif
#if (NUM_ANALOG_INPUTS >= 2)
	udb_analogInputs[1].sum += udb_analogInputs[1].input;
#endif
#if (NUM_ANALOG_INPUTS >= 3)
	udb_analogInputs[2].sum += udb_analogInputs[2].input;
#endif
#if (NUM_ANALOG_INPUTS >= 4)
	udb_analogInputs[3].sum += udb_analogInputs[3].input;
#endif
	sample_count ++;

	// When there is a chance that data will be read soon,
	// have the new average values ready.
	if (sample_count > ALMOST_ENOUGH_SAMPLES)
	{
		udb_vcc.value = __builtin_divsd(udb_vcc.sum, sample_count);
		udb_5v.value = __builtin_divsd(udb_5v.sum, sample_count);
#if (NUM_ANALOG_INPUTS >= 1)
		udb_analogInputs[0].value = __builtin_divsd(udb_analogInputs[0].sum, sample_count);
#endif
#if (NUM_ANALOG_INPUTS >= 2)
		udb_analogInputs[1].value = __builtin_divsd(udb_analogInputs[1].sum, sample_count);
#endif
#if (NUM_ANALOG_INPUTS >= 3)
		udb_analogInputs[2].value = __builtin_divsd(udb_analogInputs[2].sum, sample_count);
#endif
#if (NUM_ANALOG_INPUTS >= 4)
		udb_analogInputs[3].value = __builtin_divsd(udb_analogInputs[3].sum, sample_count);
#endif
	}

	interrupt_restore_corcon;
}

#endif // BOARD_TYPE
