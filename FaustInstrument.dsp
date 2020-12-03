// sine.dsp
// 
// First example for Sound_Synthesis seminar.
//
// Henrik von Coler
// 2020-04-21



import("stdfaust.lib");
import("noises.lib");

// input parameters with GUI elements
osc1G(x) = vgroup("Osc1", x);
osc2G(x) = vgroup("Osc2", x);
AmpEnv(x) = vgroup("AmpEnv", x);
PitchEnv(x) = vgroup("PitchEnv", x);
Filter(x) = vgroup("Filter", x);

light = nentry("lightSens[io: A10]",0, -1, 1, 0.1);
lightAmt = Filter(hslider("li2fc",0, 0, 2000, 100));

gyro = nentry("gyroSens[gyr: 1]",0, -1, 1, 0.1) : si.smoo;
gyroAmt = Filter(hslider("gy2fc",0, 0, 2000, 100));

freq  = hslider("freq",100, 10, 1000, 0.001);
gain  = hslider("gain", 1, 0, 1, 0.001) : si.smoo;
gate  = hslider("gate",0, 0, 1, 0.001);

osc_sel1  = osc1G(hslider("osz1[style:menu{'Sine':0;'Triangle':1;'Saw':2;'Square':3;'Noise':4}]", 0, 0, 4, 1));
osc_sel2  = osc2G(hslider("osz2[style:menu{'Sine':0;'Triangle':1;'Saw':2;'Square':3;'Noise':4}]", 0, 0, 4, 1));

freq_offset1  = osc1G(hslider("foff1[fine:0.01][unit:Hz]", 0, -12, 12, 1));
freq_offset2  = osc2G(hslider("foff2[fine:0.01][unit:Hz]", 0, -12, 12, 1));

vol1  = osc1G(hslider("vol1",0.5, 0, 1.5, 0.1));
vol2  = osc2G(hslider("vol2",0.5, 0, 1.5, 0.1));

a  = AmpEnv(hslider("attack[unit:s]", 0.01, 0, 5, 0.01));
d  = AmpEnv(hslider("decay[unit:s]", 0.01, 0, 5, 0.01));
s  = AmpEnv(hslider("sustain[unit:%]", 0.98, 0.001, 1, 0.01));
r  = AmpEnv(hslider("release[unit:s]", 0.01, 0, 5, 0.01));

aPitch  = PitchEnv(hslider("PiAtt[unit:s]", 0.01, 0, 5, 0.01));
dPitch  = PitchEnv(hslider("PiDec[unit:s]", 0.01, 0, 5, 0.01));
sPitch  = PitchEnv(hslider("PiSus[unit:%]", 0.98, 0.001, 1, 0.01));
rPitch  = PitchEnv(hslider("PiRe[unit:s]", 0.01, 0, 5, 0.01));
amtPitch  = PitchEnv(hslider("PiAmt[unit:Hz]", 0, 0, 2000, 100));

multiplier1 = 2^(freq_offset1/12);
freq1 = (multiplier1*freq)+(amtPitch*(gate : en.adsr(aPitch,dPitch,sPitch,rPitch)));
saw = os.sawtooth(freq1);
sine = os.osc(freq1);
square = os.square(freq1);
triangle = os.triangle(freq1);

multiplier2 = 2^(freq_offset2/12);
freq2 = (multiplier2*freq)+(amtPitch*(gate : en.adsr(aPitch,dPitch,sPitch,rPitch)));
saw2 = os.sawtooth(freq2);
sine2 = os.osc(freq2);
square2 = os.square(freq2);
triangle2 = os.triangle(freq2);

out_of_osc = sine, triangle, saw, square, noise : ba.selectn(5, osc_sel1) : *(vol1);
out_of_osc2 = sine2, triangle2, saw2, square2, noise : ba.selectn(5, osc_sel2) : *(vol2);

fc = Filter(hslider("FilterFreq[fine:10][unit:Hz]", 8000, 0, 20000, 1000))+(lightAmt*light)+(gyroAmt*gyro);
q = Filter(hslider("Q[fine:0.01]", 2, 0, 50, 1));
fgain = Filter(hslider("FilterGain", 0.2, 0, 1, 0.01));

// fb1 = hslider("feedback1",0.67,0,1,0.01);
// fb2 = hslider("feedback2",0.58,0,1,0.01);
// damp = hslider("damp",0.8,0,1,0.01);
// spread = hslider("spread",0,0,100,1);
// dry = hslider("dry_wet", 0.8, 0,1,0.01);

// reverb = *(dry), (re.mono_freeverb(fb1, fb2, damp, spread) : *(1-dry));

// process = out_of_osc, out_of_osc2 :> fi.resonlp(fc+freq, q, fgain) * (gate : en.adsr(a,d,s,r)) <: reverb :> *(gain);
process = out_of_osc, out_of_osc2 :> fi.resonlp(fc+freq, q, fgain) * (gate : en.adsr(a,d,s,r)) : *(gain);
