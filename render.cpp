/*		ks-mini		
 *
 *	TODO:
 *		-button toggles between waveforms
 *		-scope outputting both waveforms + combined waveform
 *		-convert wave making into wavetables for efficiency
 *		-use neon math library 
 *		-filters && delays
 */

#include <Bela.h>
#include <cmath>
#include <Scope.h>
#include <Stk.h>
#include <BiQuad.h>
#include <Mu45FilterCalc.h>

//setup global vars needed for dsp
float phase, inverseSampleRate;
int audioFramesPerAnalogFrame;
//set the analog channels to read from
// int sensorInputFrequency = 0, sensorInputAmplitude = 1, sensorInputFilter = 2;
//wav making
float frequency = 440.0, amplitude = 0.8, crusher = 0, out, out_L, out_R, sawtooth, sinewave, squarewave, triwave;
//wav selector
bool isSquare = true;
//filter variables
stk::BiQuad lpf_R, lpf_L, hpf_R, hpf_L;
float loCoeffs[5], hiCoeffs[5];
//control number for printline debugging
int ctrlNum = 0;
//scope variables
Scope scope;

void calculateWaves(float amplitude, float phase) {
	//calculate sinewave
	sinewave = amplitude * sinf(phase);
	//calculate squarewave
	if (phase < M_PI)	squarewave = amplitude;
		else squarewave = -amplitude;
	//calculate sawtooth
	sawtooth = amplitude - (amplitude / M_PI * phase);
	//calculate triwave
	if (phase < M_PI) triwave = -amplitude + (2 * amplitude / M_PI) * phase;
		else triwave = (3*amplitude) - (2 * amplitude / M_PI) * phase;
}

bool setup(BelaContext *context, void *userData){
	//setup scope
	scope.setup(3, context->audioSampleRate);
	//establish digital input (buttons)
	pinMode(context, 0, P8_09, INPUT);
	pinMode(context, 0, P8_10, INPUT);
	//establish digital outputs (led);
	pinMode(context, 0, P8_08, OUTPUT);
	pinMode(context, 0, P8_07, OUTPUT);
	//check if analog channels are enabled
	if(context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
		rt_printf("Error: this example needs analog enabled, with 4 or 8 channels\n");
		return false;
	}
	//check that we have the same number of inputs and outputs.
	if(context->audioInChannels != context->audioOutChannels ||
			context->analogInChannels != context-> analogOutChannels){
		printf("Error: for this project, you need the same number of input and output channels.\n");
		return false;
	}
	//useful calculations for global vars
	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	return true;
}

void render(BelaContext *context, void *userData) {
	//read digital input
	int button_ONE = digitalRead(context, 0, P8_09), //read the value of the LEFT button
		 button_TWO = digitalRead(context, 0, P8_10); //read the value of the RIGHT button

	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		digitalWrite(context, n, P8_08, GPIO_LOW);
		digitalWrite(context, n, P8_07, GPIO_LOW);

		if(ctrlNum == 1000) rt_printf("button_ONE: %i	button_TWO: %i \n", button_ONE, button_TWO);
		
		//on even samples read analog input
		if(!(n % audioFramesPerAnalogFrame)) { 
			frequency = map(analogRead(context, n/audioFramesPerAnalogFrame, 3), 0, 1, 40, 1000);
			amplitude = analogRead(context, n/audioFramesPerAnalogFrame, 0);
			crusher =  analogRead(context, n/audioFramesPerAnalogFrame, 2);//map(analogRead(context, n/audioFramesPerAnalogFrame, 2), 0, 1, 100, 1000);
		}

		/* start filter */
  // 	Mu45FilterCalc::calcCoeffsLPF(loCoeffs, crusher, .8, context->audioSampleRate);
  //  	lpf_R.setCoefficients(loCoeffs[0], loCoeffs[1], loCoeffs[2], loCoeffs[3], loCoeffs[4]);
  //  	lpf_L.setCoefficients(loCoeffs[0], loCoeffs[1], loCoeffs[2], loCoeffs[3], loCoeffs[4]);
    	
    	//if(ctrlNum == 1000)
    	//rt_printf("coeffs = %5.5f, %5.5f, %5.5f, %5.5f, %5.5f\n",loCoeffs[0], loCoeffs[1], loCoeffs[2], loCoeffs[3], loCoeffs[4]);
    	/* end filter */

		calculateWaves(amplitude, phase);

		out = sinewave;
		out_L = lpf_L.tick(sinewave);
		out_R = lpf_R.tick(sinewave);
		
		//log waveform to scope
        scope.log(out, out, out); //only show the outputted wave
		
		//bitcrusher
		if(crusher < .2){
			audioWrite(context, n, 0, out_L);
		    audioWrite(context, n, 1, out_R);
		} else if(crusher < .4) {
			if( ctrlNum % 5 == 0 ) {
				audioWrite(context, n, 0, out_L);
		    	audioWrite(context, n, 1, out_R);	
			}
		} else if(crusher < .6) {
			if( ctrlNum % 10 == 0 ) {
				audioWrite(context, n, 0, out_L);
		    	audioWrite(context, n, 1, out_R);	
			}
		} else if(crusher < .8) {
			if( ctrlNum % 20 == 0 ) {
				audioWrite(context, n, 0, out_L);
		    	audioWrite(context, n, 1, out_R);	
			}
		} else if(crusher < .99) {
			if( ctrlNum % 40 == 0 ) {
				audioWrite(context, n, 0, out_L);
		    	audioWrite(context, n, 1, out_R);	
			}
		}

		//update & wrap phase of sine tone
		phase += 2.0 * M_PI * frequency * inverseSampleRate;
		if(phase > 2.0 * M_PI) 
			phase -= 2.0 * M_PI;
	}
	
	//debug console prints 
	if(ctrlNum == 1000){
		rt_printf("frequency: %7.4fhz   amplitude: %7.4f   crusher: %7.4f\n", frequency,amplitude,crusher);
		ctrlNum = 0;
	} ctrlNum++;
} //END RENDER LOOP

void cleanup(BelaContext *context, void *userData) {
}