;Written by Iain McCurdy, 2006

;Modified for QuteCsound by René, October 2010, updated Feb 2011
;Tested on Ubuntu 10.04 with csound-float 5.13.0 and QuteCsound svn rev 817

;Notes on modifications from original csd:
;	Add table(s) for exp slider
;	Add Browser for audio file and use of FilePlay2 udo, now accept mono or stereo wav files


;my flags on Ubuntu: -iadc -odac -b1024 -B2048 -+rtaudio=alsa -+rtmidi=null -m0
<CsoundSynthesizer>
<CsOptions>
--env:SSDIR+=../../SourceMaterials
</CsOptions>
<CsInstruments>
sr 		= 44100	;SAMPLE RATE
ksmps 	= 2		;NUMBER OF AUDIO SAMPLES IN EACH CONTROL CYCLE
nchnls 	= 2		;NUMBER OF CHANNELS (2=STEREO)


giExp20000	ftgen	0, 0, 129, -25, 0, 20.0, 128, 20000.0	;TABLE FOR EXP SLIDER


opcode FilePlay2, aa, Skoo		; Credit to Joachim Heintz
	;gives stereo output regardless your soundfile is mono or stereo
	Sfil, kspeed, iskip, iloop	xin
	ichn		filenchnls	Sfil
	if ichn == 1 then
		aL		diskin2	Sfil, kspeed, iskip, iloop
		aR		=		aL
	else
		aL, aR	diskin2	Sfil, kspeed, iskip, iloop
	endif
		xout		aL, aR
endop


instr	1	;GUI

	Sfile_new			strcpy	""						;INIT TO EMPTY STRING

	ktrig	metro	10
	if (ktrig == 1)	then
		kfco			invalue	"Filter_Cutoff"
		gkfco		tablei	kfco, giExp20000, 1
					outvalue	"Filter_Cutoff_Value", gkfco
		gkres		invalue	"Resonance"
		gkamp		invalue	"Amplitude"

		gkmode		invalue  "Type"
		gkskip		invalue	"Initialisation"

		gSfile		invalue	"_Browse"
		Sfile_old		strcpyk	Sfile_new
		Sfile_new		strcpyk	gSfile
		gkfile 		strcmpk	Sfile_new, Sfile_old
	endif
endin

instr	2
	kporttime		linseg	0,0.001,0.005,1,0.005							;CREATE A VARIABLE FUNCTION THAT RAPIDLY RAMPS UP TO A SET VALUE	
	kfco			portk	gkfco, kporttime								;SMOOTH MOVEMENT OF SLIDER VARIABLE

	kNew_file		changed	gkfile										;GENERATE A MOMENTARY '1' PULSE IN OUTPUT 'kSwitch' IF ANY OF THE SCANNED INPUT VARIABLES CHANGE. (OUTPUT 'kSwitch' IS NORMALLY ZERO)
	if	kNew_file=1	then												;IF I-RATE VARIABLE CHANGE TRIGGER IS '1'...
		reinit	NEW_FILE												;BEGIN A REINITIALISATION PASS FROM LABEL 'START'
	endif
	NEW_FILE:
	;OUTPUTS		OPCODE	FILE  | SPEED | INSKIP | LOOPING (0=OFF 1=ON)
	asigL, asigR	FilePlay2	gSfile,  1,      0,         1						;GENERATE 2 AUDIO SIGNALS FROM A STEREO SOUND FILE
				rireturn												;RETURN FROM REINITIALISATION PASS TO PERFORMANCE TIME PASSES

	kSwitch		changed	gkmode, gkskip									;GENERATE A MOMENTARY '1' PULSE IN OUTPUT 'kSwitch' IF ANY OF THE SCANNED INPUT VARIABLES CHANGE. (OUTPUT 'kSwitch' IS NORMALLY ZERO)
	if	kSwitch=1	then													;IF I-RATE VARIABLE CHANGE TRIGGER IS '1'...
		reinit	START												;BEGIN A REINITIALISATION PASS FROM LABEL 'START'
	endif
	START:
	aresL 		bqrez 	asigL, kfco, gkres , i(gkmode), i(gkskip)
	aresR 		bqrez 	asigR, kfco, gkres , i(gkmode), i(gkskip)
				rireturn												;RETURN FROM REINITIALISATION PASS TO PERFORMANCE TIME PASSES
				outs		aresL * gkamp, aresR * gkamp						;SEND FILTER OUTPUT TO THE AUDIO OUTPUTS, RESCALE USING SLIDER VARIABLE gkgain
endin
</CsInstruments>
<CsScore>
;INSTR | START | DURATION
i  1      0       3600		;INSTRUMENT 1 PLAYS FOR 1 HOUR
</CsScore>
</CsoundSynthesizer>

<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>374</x>
 <y>367</y>
 <width>785</width>
 <height>296</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>241</r>
  <g>226</g>
  <b>185</b>
 </bgcolor>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>2</x>
  <y>2</y>
  <width>512</width>
  <height>292</height>
  <uuid>{aa607456-d368-4d59-8497-d16d608404c3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>bqrez</label>
  <alignment>center</alignment>
  <font>Liberation Sans</font>
  <fontsize>18</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="background">
   <r>106</r>
   <g>117</g>
   <b>150</b>
  </bgcolor>
  <bordermode>border</bordermode>
  <borderradius>5</borderradius>
  <borderwidth>2</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>515</x>
  <y>2</y>
  <width>266</width>
  <height>292</height>
  <uuid>{74928ed2-b701-4668-9a11-74763d317e9b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>bqrez</label>
  <alignment>center</alignment>
  <font>Liberation Sans</font>
  <fontsize>18</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="background">
   <r>106</r>
   <g>117</g>
   <b>150</b>
  </bgcolor>
  <bordermode>border</bordermode>
  <borderradius>5</borderradius>
  <borderwidth>2</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>518</x>
  <y>21</y>
  <width>263</width>
  <height>270</height>
  <uuid>{d4bdb5ce-87d8-4c8c-9c64-40ec2eed6f5a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>--------------------------------------------------------------
bqrez is an implementation of a second-order multi-mode filter. Its input arguments are for filter cutoff frequency, amount of resonance, a five-way switch selecting filter mode and a 2-way switch selecting whether the data space for feedback within the filter is initially cleared or allowed to retain previously held information. Use of resonance can create large increases in amplitude in the output signal. In may be necessary, therefore, to rescale the amplitude of the output signal. In this example a slider has been provided to achieve this.</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBButton">
  <objectName/>
  <x>8</x>
  <y>8</y>
  <width>100</width>
  <height>30</height>
  <uuid>{24979132-c53f-4414-ac6b-6b4f503ecfe8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>  ON / OFF</text>
  <image>/</image>
  <eventLine>i 2 0 -1</eventLine>
  <latch>true</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject version="2" type="BSBDisplay">
  <objectName>Filter_Cutoff_Value</objectName>
  <x>448</x>
  <y>118</y>
  <width>60</width>
  <height>30</height>
  <uuid>{745d6bee-b951-4a03-9fe8-9e10d5ae4556}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>757.130</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>9</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>Filter_Cutoff</objectName>
  <x>8</x>
  <y>101</y>
  <width>500</width>
  <height>27</height>
  <uuid>{06814721-6151-4baa-84e2-8f39843b07a4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.52600000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>118</y>
  <width>180</width>
  <height>30</height>
  <uuid>{c6d7165c-6730-426f-b293-52b411bc73cf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Filter Cutoff</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBButton">
  <objectName>_Browse</objectName>
  <x>8</x>
  <y>254</y>
  <width>170</width>
  <height>30</height>
  <uuid>{b9431a61-61f7-432b-bf6f-c47ddc7f9050}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue>Seashore.wav</stringvalue>
  <text>Browse Audio File</text>
  <image>/</image>
  <eventLine/>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject version="2" type="BSBLineEdit">
  <objectName>_Browse</objectName>
  <x>179</x>
  <y>255</y>
  <width>330</width>
  <height>28</height>
  <uuid>{68b5f90b-b78e-4581-b434-232db5f4c40f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Seashore.wav</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>229</r>
   <g>229</g>
   <b>229</b>
  </bgcolor>
  <background>nobackground</background>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>233</y>
  <width>150</width>
  <height>30</height>
  <uuid>{936f6eb0-5225-4bd7-a5fe-a49df1c9c5ac}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Audio File</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>203</y>
  <width>180</width>
  <height>30</height>
  <uuid>{2e52a7a7-dc13-405e-896d-db2a7431d8dd}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Output Amplitude Scaling</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>Amplitude</objectName>
  <x>8</x>
  <y>186</y>
  <width>500</width>
  <height>27</height>
  <uuid>{6ab84e0f-82a7-4b9c-af23-35bd7d9b7fa2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.12600000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBDisplay">
  <objectName>Amplitude</objectName>
  <x>448</x>
  <y>203</y>
  <width>60</width>
  <height>30</height>
  <uuid>{99d6bf8b-3c85-4911-a29a-7dd678a940ed}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.126</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>9</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBButton">
  <objectName>Initialisation</objectName>
  <x>380</x>
  <y>52</y>
  <width>130</width>
  <height>30</height>
  <uuid>{b6345774-7938-4e62-9d8b-0ada81f7bcc5}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>  No Initialisation</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>170</x>
  <y>52</y>
  <width>84</width>
  <height>29</height>
  <uuid>{257d2fe0-c15d-4d4e-8f40-fc1ea010d5bf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Type</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBDropdown">
  <objectName>Type</objectName>
  <x>255</x>
  <y>52</y>
  <width>120</width>
  <height>30</height>
  <uuid>{e804f0ba-085b-49ac-8078-0b38a0fb498a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>Low-pass</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>High-pass</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Band-pass</name>
    <value>2</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Band-reject</name>
    <value>3</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>All-pass</name>
    <value>4</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>0</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>156</y>
  <width>180</width>
  <height>30</height>
  <uuid>{4b8cc27a-e102-4e54-abd9-b563c507ee44}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>Resonance</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>Resonance</objectName>
  <x>8</x>
  <y>139</y>
  <width>500</width>
  <height>27</height>
  <uuid>{eec983db-87f2-4ff0-b409-ed8ec573b9ae}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>1.00000000</minimum>
  <maximum>100.00000000</maximum>
  <value>16.24600000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBDisplay">
  <objectName>Resonance</objectName>
  <x>448</x>
  <y>156</y>
  <width>60</width>
  <height>30</height>
  <uuid>{bf739784-7c38-4ecb-90ac-9a603e010479}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>16.246</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>9</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<EventPanel name="" tempo="60.00000000" loop="8.00000000" x="913" y="162" width="655" height="346" visible="false" loopStart="0" loopEnd="0">    </EventPanel>
