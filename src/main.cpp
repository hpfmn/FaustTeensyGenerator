#include <Arduino.h>
#include <menu.h>

#ifdef USECLICKENCODER
#include <ClickEncoder.h>
#include <menuIO/clickEncoderIn.h>
#endif

#ifdef USELCD
#include <menuIO/liquidCrystalOut.h>
#endif

#ifdef USEU8G2
#include <menuIO/u8g2Out.h>
#endif

#include <menuIO/keyIn.h>
#include <menuIO/chainStream.h>
#include <Wire.h>
#include <SD.h>

#include <Audio.h>
#include "FaustInstrument.h"

#include <MIDI.h>

#include <USBHost_t36.h>

#ifdef USBHOSTMIDI
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);
#endif

#ifdef HARDWAREMIDI
#define MIDIPORT Serial4
MIDI_CREATE_INSTANCE(HardwareSerial, MIDIPORT, MIDI);
#endif


result save_to_sdcard();
result load_from_sdcard();
const int chipSelect = BUILTIN_SDCARD;
result updateFParam();

const int VOICES = 8;

FaustInstrument *fi[VOICES] = {new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument(),
                               new FaustInstrument()};

AudioMixer4 mixer;
AudioMixer4 mixer2;
AudioMixer4 mixer3;

#ifdef SGTL5000
AudioOutputI2S i2s;
AudioConnection p1(mixer3, 0, i2s, 0);
AudioConnection p2(mixer3, 0, i2s, 1);
AudioControlSGTL5000 audioController;
#endif

#ifdef USESPDIF
AudioOutputSPDIF spdif;
AudioConnection p1(mixer3, 0, spdif, 0);
AudioConnection p2(mixer3, 0, spdif, 1);
#endif

AudioConnection p3(*fi[0], 0, mixer, 0);
AudioConnection p4(*fi[1], 0, mixer, 1);
AudioConnection p5(*fi[2], 0, mixer, 2);
AudioConnection p6(*fi[3], 0, mixer, 3);
AudioConnection p7(*fi[4], 0, mixer2, 0);
AudioConnection p8(*fi[5], 0, mixer2, 1);
AudioConnection p9(*fi[6], 0, mixer2, 2);
AudioConnection p10(*fi[7], 0, mixer2, 3);

AudioConnection p11(mixer, 0, mixer3, 0);
AudioConnection p12(mixer2, 0, mixer3, 1);

const int MPU_ADDR = 0x68;
uint16_t accelerometer_0, accelerometer_1, accelerometer_2; // variables for accelerometer raw data
uint16_t gyro_0, gyro_1, gyro_2; // variables for gyro raw data
int16_t temperature; // variables for temperature data
char tmp_str[7]; // temporary variable used in convert function
char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
   sprintf(tmp_str, "%6d", i);
   return tmp_str;
}

using namespace Menu;

#ifdef USEU8G2
// code for display
#define fontName u8g2_font_6x10_mf
#define fontX 5
#define fontY 12
#define offsetX 1
#define offsetY 2
#define U8_Width 128
#define U8_Height 64
#define USE_HWI2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// define menu colors
const colorDef<uint8_t> colors[6] MEMMODE={
   {{0,0},{0,1,1}},//bgColor
   {{1,1},{1,0,0}},//fgColor
   {{1,1},{1,0,0}},//valColor
   {{1,1},{1,0,0}},//unitColor
   {{0,1},{0,0,1}},//cursorColor
   {{1,1},{1,0,0}},//titleColor
};
#endif

#ifdef USELCD
// LCD /////////////////////////////////////////
LiquidCrystal lcd(27, 28, 29, 30, 31, 32);
#endif

#ifdef USECLICKENCODER
// Encoder /////////////////////////////////////
#define encA 2
#define encB 3
#define encBtn 5
#define encSteps 4

ClickEncoder clickEncoder = ClickEncoder(encA, encB, encBtn, encSteps);
ClickEncoderStream encStream(clickEncoder, 1);

IntervalTimer myTimer;

void onTimer() {
  clickEncoder.service();
}
#endif

// using cog to generate variables and menu structure
/* [[[cog
from src.main import *

cog.outl(variables)
cog.outl(patch)
cog.outl(updateFunc)
cog.outl(menu)
cog.outl(updateAnalog)
cog.outl(updateGyro)
cog.outl(defines)
]]] */
float t_attack = 0.01;
float t_decay = 0.01;
float t_release = 0.01;
float t_sustain = 0.98;
float t_filterfreq = 8000;
float t_filtergain = 0.2;
float t_q = 2;
float t_gy2fc = 0;
float t_li2fc = 0;
float t_foff1 = 0;
float t_osz1 = 0;
prompt* osz1_data[]={
	new menuValue<float>("Sine", 0),
	new menuValue<float>("Noise", 4),
	new menuValue<float>("Square", 3),
	new menuValue<float>("Saw", 2),
	new menuValue<float>("Triangle", 1)
};
float t_vol1 = 0.5;
float t_foff2 = 0;
float t_osz2 = 0;
prompt* osz2_data[]={
	new menuValue<float>("Sine", 0),
	new menuValue<float>("Noise", 4),
	new menuValue<float>("Square", 3),
	new menuValue<float>("Saw", 2),
	new menuValue<float>("Triangle", 1)
};
float t_vol2 = 0.5;
float t_piamt = 0;
float t_piatt = 0.01;
float t_pidec = 0.01;
float t_pire = 0.01;
float t_pisus = 0.98;
int t_file_load = 0;
int t_file_save = 0;

float *Patch[] = {
	&t_attack,
	&t_decay,
	&t_release,
	&t_sustain,
	&t_filterfreq,
	&t_filtergain,
	&t_q,
	&t_gy2fc,
	&t_li2fc,
	&t_foff1,
	&t_osz1,
	&t_vol1,
	&t_foff2,
	&t_osz2,
	&t_vol2,
	&t_piamt,
	&t_piatt,
	&t_pidec,
	&t_pire,
	&t_pisus
};

result updateFParam() {
	for(int v = 0; v < VOICES; v++) {
		fi[v]->setParamValue("attack", t_attack);
		fi[v]->setParamValue("decay", t_decay);
		fi[v]->setParamValue("release", t_release);
		fi[v]->setParamValue("sustain", t_sustain);
		fi[v]->setParamValue("FilterFreq", t_filterfreq);
		fi[v]->setParamValue("FilterGain", t_filtergain);
		fi[v]->setParamValue("Q", t_q);
		fi[v]->setParamValue("gy2fc", t_gy2fc);
		fi[v]->setParamValue("li2fc", t_li2fc);
		fi[v]->setParamValue("foff1", t_foff1);
		fi[v]->setParamValue("osz1", t_osz1);
		fi[v]->setParamValue("vol1", t_vol1);
		fi[v]->setParamValue("foff2", t_foff2);
		fi[v]->setParamValue("osz2", t_osz2);
		fi[v]->setParamValue("vol2", t_vol2);
		fi[v]->setParamValue("PiAmt", t_piamt);
		fi[v]->setParamValue("PiAtt", t_piatt);
		fi[v]->setParamValue("PiDec", t_pidec);
		fi[v]->setParamValue("PiRe", t_pire);
		fi[v]->setParamValue("PiSus", t_pisus);
	}
	return proceed;
}
prompt* p_AmpEnv[] = {
	new menuField<float>(t_attack, "attack", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle),
	new Exit("<Back"),
	new menuField<float>(t_sustain, "sustain", "%", 0.001, 1, 0.01, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_release, "release", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_decay, "decay", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle)
};
menuNode &m_AmpEnv = *new menuNode("AmpEnv", sizeof(p_AmpEnv)/sizeof(prompt*), p_AmpEnv);
prompt* p_Filter[] = {
	new menuField<float>(t_filterfreq, "FilterFreq", "Hz", 0, 20000, 1000, 10, updateFParam,enterEvent, noStyle),
	new Exit("<Back"),
	new menuField<float>(t_li2fc, "li2fc", "", 0, 2000, 100, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_gy2fc, "gy2fc", "", 0, 2000, 100, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_q, "Q", "", 0, 50, 1, 0.01, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_filtergain, "FilterGain", "", 0, 1, 0.01, 0, updateFParam,enterEvent, noStyle)
};
menuNode &m_Filter = *new menuNode("Filter", sizeof(p_Filter)/sizeof(prompt*), p_Filter);
prompt* p_Osc1[] = {
	new menuField<float>(t_foff1, "foff1", "Hz", -12, 12, 1, 0.01, updateFParam,enterEvent, noStyle),
	new Exit("<Back"),
	new menuField<float>(t_vol1, "vol1", "", 0, 1.5, 0.1, 0, updateFParam,enterEvent, noStyle),
	new Menu::select<float>("osz1", t_osz1, sizeof(osz1_data)/sizeof(prompt*), osz1_data,updateFParam, exitEvent, wrapStyle)
};
menuNode &m_Osc1 = *new menuNode("Osc1", sizeof(p_Osc1)/sizeof(prompt*), p_Osc1);
prompt* p_Osc2[] = {
	new menuField<float>(t_foff2, "foff2", "Hz", -12, 12, 1, 0.01, updateFParam,enterEvent, noStyle),
	new Exit("<Back"),
	new menuField<float>(t_vol2, "vol2", "", 0, 1.5, 0.1, 0, updateFParam,enterEvent, noStyle),
	new Menu::select<float>("osz2", t_osz2, sizeof(osz2_data)/sizeof(prompt*), osz2_data,updateFParam, exitEvent, wrapStyle)
};
menuNode &m_Osc2 = *new menuNode("Osc2", sizeof(p_Osc2)/sizeof(prompt*), p_Osc2);
prompt* p_PitchEnv[] = {
	new menuField<float>(t_piamt, "PiAmt", "Hz", 0, 2000, 100, 0, updateFParam,enterEvent, noStyle),
	new Exit("<Back"),
	new menuField<float>(t_pisus, "PiSus", "%", 0.001, 1, 0.01, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_pire, "PiRe", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_pidec, "PiDec", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle),
	new menuField<float>(t_piatt, "PiAtt", "s", 0, 5, 0.01, 0, updateFParam,enterEvent, noStyle)
};
menuNode &m_PitchEnv = *new menuNode("PitchEnv", sizeof(p_PitchEnv)/sizeof(prompt*), p_PitchEnv);
prompt* p_FaustInstrument[] = {
	&m_AmpEnv,
	new menuField<int>(t_file_save, "save", "", 0, 127, 1, 0, save_to_sdcard, exitEvent, noStyle),
	new menuField<int>(t_file_load, "load", "", 0, 127, 1, 0, load_from_sdcard, exitEvent, noStyle),
	&m_PitchEnv,
	&m_Osc2,
	&m_Osc1,
	&m_Filter
};
menuNode &myMenu = *new menuNode("FaustInstrument", sizeof(p_FaustInstrument)/sizeof(prompt*), p_FaustInstrument);

void update_analog() {
	for(int v = 0; v < VOICES; v++) {
		fi[v]->setParamValue("lightSens", (analogRead(A10)/(float) 1024)*(((float) 1)-((float) -1))+((float) -1));
	}
}
void update_gyro() {
	Wire1.beginTransmission(MPU_ADDR);
	Wire1.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
	Wire1.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
	Wire1.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
	
	// "Wire1.read()<<8 | Wire1.read();" means two registers are read and stored in the same variable
	accelerometer_0 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
	accelerometer_1 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
	accelerometer_2 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
	temperature = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
	gyro_0 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
	gyro_1 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
	gyro_2 = Wire1.read()<<8 | Wire1.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

	for(int v = 0; v < VOICES; v++) {
		fi[v]->setParamValue("gyroSens", (gyro_1/(float) 0xFFFF)*(((float) 1)-((float) -1))+((float) -1));
	}
}
#define MAX_DEPTH 3
#define USEANALOG
#define USEGYRO

// [[[end]]]

#ifdef USEBUTTON
// navigation buttons
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_SEL 4
#define BTN_ESC 5

keyMap btn_map[] = {
   {-BTN_SEL, defaultNavCodes[enterCmd].ch},
   {-BTN_UP, defaultNavCodes[upCmd].ch},
   {-BTN_DOWN, defaultNavCodes[downCmd].ch},
   {-BTN_ESC, defaultNavCodes[escCmd].ch}
};

keyIn<4> btn(btn_map);
#endif

MENU_OUTPUTS(out,MAX_DEPTH
#ifdef USEU8G2
   ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
#endif
#ifdef USELCD
   ,LIQUIDCRYSTAL_OUT(lcd,{0,0,16,2})
#endif
   ,NONE
);

#ifdef USECLICKENCODER
MENU_INPUTS(in, &encStream);
#endif

navNode path[MAX_DEPTH];
navRoot nav(myMenu, path, MAX_DEPTH, in, out);


result alert(menuOut& o,idleEvent e) {
   if (e==idling) {
      o.setCursor(0,0);
      o.print("alert test");
      o.setCursor(0,1);
      o.print("press [select]");
      o.setCursor(0,2);
      o.print("to continue...");
   }
   return proceed;
}

result doAlert(eventMask e, prompt &item) {
   nav.idleOn(alert);
   return proceed;
}

//when menu is suspended
result idle(menuOut& o,idleEvent e) {
   o.clear();
   switch(e) {
      case idleStart:o.println("suspending menu!");break;
      case idling:o.println("suspended...");break;
      case idleEnd:o.println("resuming menu.");break;
   }
   return proceed;
}

byte note2voice[128];
byte suspendedVoice[VOICES];
byte voice2note[VOICES];
byte voices = 0;
byte pedal = 0;
byte VoiceUsed[VOICES];
double pitch_offset = 0;

// MIDI handlers
void setFreq(byte voice) {
	byte note = voice2note[voice];
	fi[voice]->setParamValue("freq", pow(2.0, ((note-69)+pitch_offset)/12.0)*440.0);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
	for(byte v = 0; v<VOICES; v++) {
		if(!VoiceUsed[v]) {
			fi[v]->setParamValue("gate", 1);
			fi[v]->setParamValue("gain", velocity/127.0);
			note2voice[note] = v;
			VoiceUsed[v] = 1;
			voice2note[v] = note;
			setFreq(v);
			break;
		}
	}
}

void handleNoteOff(byte channel, byte note, byte velocity) {
	if(pedal) {
		suspendedVoice[note2voice[note]] = 1;
	} else {
		fi[note2voice[note]]->setParamValue("gate", 0);
		VoiceUsed[note2voice[note]] = 0;
	}
}

void myPitchChange(byte channel, int pitch) {
	const int PITCH_SEMITONES = 12;
	pitch_offset = ((pitch+0.5)/8191.5)*PITCH_SEMITONES;
	for(byte v = 0; v<VOICES; v++) {
		if(VoiceUsed[v])
			setFreq(v);
	}
}

void myControlChange(byte channel, byte control, byte value) {
	// Sustain Pedal
	if (control == 64) {
		if (value > 0)
			pedal = 1;
		else {
			pedal = 0;
			for(int v = 0; v < VOICES; v++) {
				if(suspendedVoice[v]) {
					fi[v]->setParamValue("gate", 0);
					suspendedVoice[v] = 0;
				}
			}
		}
	}

	if (control == 55) {
		int sel = int(value/127.0*4.0);
		for(int v = 0; v < VOICES; v++) {
			fi[v]->setParamValue("osc_select", sel);
		}
	}

}


void setup() {
#ifdef DEBUGOUT
	Serial.begin(9600);
	while (!Serial) {
		;// wait for serial port to connect.
	}
#endif

	if (!SD.begin(chipSelect)) {
#ifdef DEBUGOUT
		Serial.println("initialization failed!");
#endif
		return;
	}
#ifdef DEBUGOUT
	Serial.println("initialization done.");
#endif

#ifdef USEGYRO
   Wire1.begin();
   Wire1.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
   Wire1.write(0x6B); // PWR_MGMT_1 register
   Wire1.write(0); // set to zero (wakes up the MPU-6050)
   Wire1.endTransmission(true);
#endif

   AudioMemory((VOICES*2) + 2);

#ifdef SGTL5000
   audioController.enable();
   audioController.volume(0.4);
#endif

   // MIDI.setHandleNoteOn(handleNoteOn);
   // MIDI.setHandleNoteOff(handleNoteOff);
   // MIDI.begin(MIDI_CHANNEL_OMNI);

   myusb.begin();
   midi1.setHandleNoteOn(handleNoteOn);
   midi1.setHandleNoteOff(handleNoteOff);
   midi1.setHandlePitchChange(myPitchChange);
   midi1.setHandleControlChange(myControlChange);


#ifdef USECLICKENCODER
  clickEncoder.setAccelerationEnabled(true);
  clickEncoder.setDoubleClickEnabled(false); // Disable doubleclicks makes the response faster.  See: https://github.com/soligen2010/encoder/issues/6

  myTimer.begin(onTimer, 1000);
#endif

#ifdef USELCD
  lcd.begin(16,2);
#endif

#ifdef USEBUTTON
   btn.begin();
#endif

#ifdef USEU8G2
   Wire.begin();
   u8g2.begin();
   u8g2.setFont(fontName);
#endif

   nav.idleTask=idle; //point a function to be used when menu is suspended
   options->invertFieldKeys = true;
}

void loop() { // Main loop

#ifdef USEANALOG
   if (micros() % 200  == 0) {
      update_analog();
   }
#endif

#ifdef USEGYRO
   if (micros() % 200  == 0) {
      update_gyro();
   }
#endif

   myusb.Task();
   midi1.read();
   // nav.doInput();
   nav.poll();

   // if (nav.changed(0)) {  //only draw if menu changed for gfx device
   //    //change checking leaves more time for other tasks
   //    u8g2.firstPage();
   //    do nav.doOutput(); while(u8g2.nextPage());
   // }
}

result load_from_sdcard() {
	char filename[15];
	snprintf(filename, sizeof(filename), "%d.ptc", (int) t_file_load);
	File myFile;
	int patchEntries = sizeof(Patch)/sizeof(float **);
#ifdef DEBUGOUT
		Serial.printf("try to load from file %s\n", filename);
#endif

	myFile = SD.open(filename);
	if (myFile) {
		// read from the file until there's nothing else in it:
		for(int i = 0; i< patchEntries; i++) {
			myFile.read((char *) Patch[i], sizeof(float));
		}
		// close the file:
		myFile.close();

		/*for (int i = 0; i<PARAMS; i++) {
			synth.setParamValue(params[i].name, Patch[i]);
		}*/
		updateFParam();

	} else {
		// error
#ifdef DEBUG
		Serial.printf("error opening file %s for reading\n", filename);
#endif
	}

	return proceed;
}

result save_to_sdcard() {
	char filename[15];
	snprintf(filename, sizeof(filename), "%d.ptc", (int) t_file_save);
	File myFile;
	int patchEntries = sizeof(Patch)/sizeof(float **);
#ifdef DEBUGOUT
		Serial.printf("try save to file %s\n", filename);
#endif

	/*for (int i = 0; i<PARAMS; i++) {
		Patch[i] = synth.getParamValue(params[i].name);
	}*/

	if (SD.exists(filename)) {
		SD.remove(filename);
	}

	myFile = SD.open(filename, FILE_WRITE);
	if (myFile) {
		for(int i = 0; i< patchEntries; i++) {
			myFile.write((char *) Patch[i], sizeof(float));
		}

		// close the file:
		myFile.close();
	} else {
		// error
#ifdef DEBUGOUT
		Serial.printf("error opening file %s for reading\n", filename);
#endif
	}

	return proceed;
}
