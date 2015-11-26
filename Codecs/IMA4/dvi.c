/*	Copyright © 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
    #include <CoreFoundation/CFBase.h>
#else
	#include "CFBase.h"
#endif
#include "dvi.h"

void
adpcm_decoder(
	unsigned char *inp,
	short *outp,
    int len,
    struct adpcm_state *state,
    const short *indexTable,
    const short *stepsizeTable,
    Boolean outputIs16bit,
	Boolean isStereo)
{
    long delta;				/* Current adpcm output value */
    long step;				/* Stepsize */
    long valprev;			/* virtual previous output value */
    long vpdiff;			/* Current change to valprev */
    long index;				/* Current step change index */
    long inputbuffer = 0;	/* place to keep next 4-bit value -- initialized to fix a false warning*/
    long bufferstep = 0;	/* toggle between inputbuffer/input */
    char *out8bit = (char*)outp;
	unsigned char *nextInpJump;

    valprev = state->valprev;
    index = state->index;
 
	// validate the index, just in case
    if ( index < 0 ) index = 0;
    else if ( index > 88 ) index = 88;

	// must output the first sample as is, first
	if (outputIs16bit) {
		*outp++ = valprev;
		if (isStereo)
			outp++;
	}
	else {
		*out8bit++ = (valprev >> 8) ^ 0x80;
		if (isStereo)
			out8bit++;
	}

	len--;

	nextInpJump = inp + 4;

	// loop on those nibbles
    for ( ; len > 0 ; len-- ) {

		/* Step 1 - get the delta value and compute next index */
		if ( bufferstep ) {
		    delta = (inputbuffer >> 4) & 0x0f;
		} else {
			if (isStereo) {
				if (inp == nextInpJump) {
					inp += 4;
					nextInpJump = inp + 4;
				}
			}

		    inputbuffer = *inp++;
		    delta = inputbuffer & 0x0f;
		}
		bufferstep = !bufferstep;

		/* Step 4 - update output value */
	    step = stepsizeTable[index];
		vpdiff = 0;
		if (delta & 4) vpdiff += step;
		if (delta & 2) vpdiff += (step >> 1);
		if (delta & 1) vpdiff += (step >> 2);
		vpdiff += (step >> 3);

		if ( delta & 8 )
			vpdiff = -vpdiff;

		valprev += vpdiff;

		/* Step 5 - clamp output value */
		if ( valprev > 32767 )
		  valprev = 32767;
		else if ( valprev < -32768 )
		  valprev = -32768;

		/* Step 7 - Output value */
		if (outputIs16bit) {
			*outp++ = valprev;
			if (isStereo)
				outp++;
		}
		else {
			*out8bit++ = (valprev >> 8) ^ 0x80;
			if (isStereo)
				out8bit++;
		}

		/* Step 2 - Find new index value */ 
		index += indexTable[delta];
		if ( index < 0 ) index = 0;
		else if ( index > 88 ) index = 88;
	}

	// return updated state... that we will never use
    state->valprev = valprev;
    state->index = index;
}
