<CsVersion>
After 6.13
</CsVersion>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
nchnls = 8
ksmps = 128
A4 = 442
0dbfs = 1

; -------------------------------------------------

massign 0, 0
gaSpectrumOut init 0
gkSpectrumFFTSize init 4096

giFFTSizes[] fillarray 2048,4096,8192

chn_k "notein", "w"
chn_k "ccin", "w"

instr AudioTest
	; kon1, kon2, kon3, kon4, kon5,kon6, kon7, kon8 init 0
	ksignalFreq init A4
	ilevelExp = 0.33
	iuiRefreshRate = 24
	irange = 48
	iminAmp = ampdb(-60)
	imeterUpLag   = 0.01
	imeterDownLag = 0.1
	kdb init -999999
		
	kSpectrumSelect init 1
	
	;; ----- UI input -----
	ktrig metro iuiRefreshRate
	ksignal invalue "signaltype"
	kfftSizeIndex invalue "fftsize"
	kfftSize = giFFTSizes[kfftSizeIndex]
	if kfftSize != gkSpectrumFFTSize then
		gkSpectrumFFTSize = kfftSize
		reinit resetFFT
	endif
	
	kleveldelta  invalue "leveldelta"
	kdb = int(linlin(kleveldelta^ilevelExp, -60, 0, 0, 1))
	kamp = ampdb(kdb)
	ksignalOn = kamp > iminAmp ? 1 : 0
	if changed(kdb) == 1 then
		Sdisp = sprintfk("%d", kdb)
		outvalue "dbdisp", Sdisp
	endif
	
	if ksignalOn == 0 kgoto skipOutput
	
	if ktrig == 1 then
		kon1    invalue "on1"
		kon2    invalue "on2"
		kon3    invalue "on3"
		kon4    invalue "on4"
		kon5    invalue "on5"
		kon6    invalue "on6"
		kon7    invalue "on7"
		kon8    invalue "on8"
		kSpectrumSelect invalue "spmenu"
		ksignalFreq invalue "signalfreq"
	endif
	
	;; Output
	
	if ksignal == 0 then
		asig noise 1, 0
	elseif ksignal == 1 then
		asig pinker
	elseif ksignal == 2 then
		asig oscili 1, ksignalFreq
	elseif ksignal == 3 then
		asig vco2 1, ksignalFreq
	endif
	
	asig *= interp(sc_lag(kamp, 0.1))
	; output to graph
	display asig, ksmps/sr*2
	
	iswitchlag = 0.2
	
	aout1 = asig*interp(sc_lag(kon1, iswitchlag))
	koutamp1 max_k aout1, ktrig, 1
	outch 1, aout1
	
	aout2 = asig*interp(sc_lag(kon2, iswitchlag))
	koutamp2 max_k aout2, ktrig, 1
	outch 2, aout2
	
	if nchnls <= 2 goto skipOutput
	
	aout3 = asig*interp(sc_lag(kon3, iswitchlag))
	koutamp3 max_k aout3, ktrig, 1
	outch 3, aout3
	
	aout4 = asig*interp(sc_lag(kon4, iswitchlag))
	koutamp4 max_k aout4, ktrig, 1
	outch 4, aout4
	
	if nchnls <= 4 goto skipOutput
	
	aout5 = asig*interp(sc_lag(kon5, iswitchlag))
	koutamp5 max_k aout5, ktrig, 1
	outch 5, aout5
	
	aout6 = asig*interp(sc_lag(kon6, iswitchlag))
	koutamp6 max_k aout6, ktrig, 1
	outch 6, aout6
	
	if nchnls <= 6 goto skipOutput
	
	aout7 = asig*interp(sc_lag(kon7, iswitchlag))
	koutamp7 max_k aout7, ktrig, 1
	outch 7, aout7
	
	aout8 = asig*interp(sc_lag(kon8, iswitchlag))
	koutamp8 max_k aout8, ktrig, 1
	outch 8, aout8
	
	
	
skipOutput:

	;; Audio Input
		
	imin = ampdbfs(-irange)
	
	
	kamp1 max_k inch(1), ktrig, 1
	kamp1 sc_lagud kamp1, imeterUpLag, imeterDownLag
	
	kamp2 max_k inch(2), ktrig, 1
	kamp2 sc_lagud kamp2, imeterUpLag, imeterDownLag
	
	kamp3 max_k inch(3), ktrig, 1
	kamp3 sc_lagud kamp3, imeterUpLag, imeterDownLag
	
	kamp4 max_k inch(4), ktrig, 1
	kamp4 sc_lagud kamp4, imeterUpLag, imeterDownLag
	
	kamp5 max_k inch(5), ktrig, 1
	kamp5 sc_lagud kamp5, imeterUpLag, imeterDownLag
	
	kamp6 max_k inch(6), ktrig, 1
	kamp6 sc_lagud kamp6, imeterUpLag, imeterDownLag
	
	kamp7 max_k inch(7), ktrig, 1
	kamp7 sc_lagud kamp7, imeterUpLag, imeterDownLag

	kamp8 max_k inch(8), ktrig, 1
	kamp8 sc_lagud kamp8, imeterUpLag, imeterDownLag
	
	if ktrig == 0 kgoto skipDisplay
	outvalue "in1", linlin(dbamp(kamp1), 0, 1, -irange, 0)
	outvalue "vu1", linlin(dbamp(koutamp1), 0, 1, -irange, 0)
	outvalue "in2", linlin(dbamp(kamp2), 0, 1, -irange, 0)
	outvalue "vu2", linlin(dbamp(koutamp2), 0, 1, -irange, 0)
	if nchnls <= 2 goto skipDisplay
	
	outvalue "in3", linlin(dbamp(kamp3), 0, 1, -irange, 0)
	outvalue "vu3", linlin(dbamp(koutamp3), 0, 1, -irange, 0)
	outvalue "in4", linlin(dbamp(kamp4), 0, 1, -irange, 0)
	outvalue "vu4", linlin(dbamp(koutamp4), 0, 1, -irange, 0)
	if nchnls <= 4 goto skipDisplay
	
	outvalue "in5", linlin(dbamp(kamp5), 0, 1, -irange, 0)
	outvalue "vu5", linlin(dbamp(koutamp5), 0, 1, -irange, 0)
	outvalue "in6", linlin(dbamp(kamp6), 0, 1, -irange, 0)
	outvalue "vu6", linlin(dbamp(koutamp6), 0, 1, -irange, 0)
	if nchnls <= 6 goto skipDisplay
	
	outvalue "in7", linlin(dbamp(kamp7), 0, 1, -irange, 0)
	outvalue "vu7", linlin(dbamp(koutamp7), 0, 1, -irange, 0)
	outvalue "in8", linlin(dbamp(kamp8), 0, 1, -irange, 0)
	outvalue "vu8", linlin(dbamp(koutamp8), 0, 1, -irange, 0)
	
skipDisplay:
	
	; update text display at slower rate
	ktrig2 metro iuiRefreshRate*0.33

	if ktrig2 == 1 then
		outvalue "indb1", round(dbfsamp(kamp1))
		outvalue "indb2", round(dbfsamp(kamp2))
		outvalue "indb3", round(dbfsamp(kamp3))
		outvalue "indb4", round(dbfsamp(kamp4))
		outvalue "indb5", round(dbfsamp(kamp5))
		outvalue "indb6", round(dbfsamp(kamp6))
		outvalue "indb7", round(dbfsamp(kamp7))
		outvalue "indb8", round(dbfsamp(kamp8))
	endif
	
	if kSpectrumSelect == 0 then
		aSpectrumOut = 0
	else
		aSpectrumOut inch kSpectrumSelect
		; low shelf
		aSpectrumOut pareq aSpectrumOut, 30, 0, 0.1, 1
	endif
	; without denorming the cpu usage shoots to 100% in some cases
	denorm aSpectrumOut
	
	; gaSpectrumOut += aSpectrumOut
resetFFT:
	prints "fft size: %d \n", i(gkSpectrumFFTSize)
	dispfft aSpectrumOut, 0.04, i(gkSpectrumFFTSize), 1
	
endin

instr led
	Schan = p4
	chnset 1, Schan
	if lastcycle() == 1 then
	  kOut = 0
		chnset kOut, Schan	
	endif
endin


instr MidiNote  ; midi note input
   ichn = p4
   ikey = p5
   ivel = p6
	chnset 1, "notein"
	schedule "led", 0, 0.1, "notein"
	if timeinstk() == 1 then
		Smsg sprintf "Note %d, vel=%d, chan=%d", ikey, ivel, ichn
		outvalue "display", Smsg
	endif
	
	iamp = ampdb(ivel/127 * 48 - 48)
	aenv linsegr 0, 0.05, 1, 0.05, 0
	aout oscili iamp*aenv, mtof(ikey)
	outch 1, aout
	
endin

instr MidiCC
	ichan = p4
	icc = p5
	ival = p6
	schedule "led", 0, 0.1, "ccin"
	Smsg sprintf "CC %d: %d (chan=%d)", icc, ival, ichan
	if timeinstk() == 1 then
		outvalue "displaycc", Smsg
	endif
	
endin

instr 3
	; generate midi note
	noteondur 1, 60, 100, p3
endin

instr MidiIn
	iMidiNote = nstrnum("MidiNote")
	kstatus init 0
	kstatus, kchan, kdata1, kdata2 midiin
	if kstatus == 144 && kdata1 > 0 then
		schedulek iMidiNote + kdata1/1000, 0, -1, kchan, kdata1, kdata2
	elseif kstatus == 128 || (kstatus == 144 && kdata1 == 0) then
		turnoff2 iMidiNote+kdata1/1000, 4, 1
	elseif kstatus == 176 then
		schedulek "MidiCC", 0, 0.1, kchan, kdata1, kdata2
	endif
endin

instr Setup
	outvalue "graph-index", 0
	outvalue "spectrum-index", 1
	outvalue "display", "..."
	outvalue "displaycc", "..."
	outvalue "notein", 0
	outvalue "ccin", 0
	
	outvalue "vu1",0
	outvalue "vu2",0
	outvalue "vu3",0
	outvalue "vu4",0
	outvalue "vu5",0
	outvalue "vu6",0
	outvalue "vu7",0
	outvalue "vu8",0
	
	outvalue "in1", 0
	outvalue "in2", 0
	outvalue "in3", 0
	outvalue "in4", 0
	outvalue "in5", 0
	outvalue "in6", 0
	outvalue "in7", 0
	outvalue "in8", 0
	
	outvalue "samplerate", sprintf("%d", sr)
	outvalue "nchnls", sprintf("%d", nchnls)
	
	turnoff
endin


</CsInstruments>
<CsScore>
i "AudioTest"   0.0  3600
i "MidiIn"      0.02 3600
i "Setup"       0.2  -1
e
</CsScore>
</CsoundSynthesizer>




<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>704</width>
 <height>590</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>65</r>
  <g>65</g>
  <b>65</b>
 </bgcolor>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>331</x>
  <y>59</y>
  <width>373</width>
  <height>328</height>
  <uuid>{f046b35f-13ea-4c0f-9282-b4b6007fc90e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Audio Output</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>240</r>
   <g>240</g>
   <b>240</b>
  </color>
  <bgcolor mode="background">
   <r>99</r>
   <g>99</g>
   <b>99</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>dbdisp</objectName>
  <x>604</x>
  <y>124</y>
  <width>36</width>
  <height>28</height>
  <uuid>{cd057454-88c9-4ff0-8146-afe05c81f45d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>-20</label>
  <alignment>center</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>235</r>
   <g>157</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>6</x>
  <y>282</y>
  <width>322</width>
  <height>105</height>
  <uuid>{d25e4c33-85be-48ad-a3f4-b071e3baa5fe}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>MIDI I/O</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="background">
   <r>99</r>
   <g>99</g>
   <b>99</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>6</x>
  <y>9</y>
  <width>698</width>
  <height>48</height>
  <uuid>{dc5604fd-41c0-4189-abe2-55c72f10172b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Audio / MIDI Test</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>20</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="background">
   <r>99</r>
   <g>99</g>
   <b>99</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>6</x>
  <y>59</y>
  <width>322</width>
  <height>220</height>
  <uuid>{6b0524fe-c62b-443e-a7cc-2786168abc62}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Audio Input</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>243</r>
   <g>243</g>
   <b>243</b>
  </color>
  <bgcolor mode="background">
   <r>99</r>
   <g>99</g>
   <b>99</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>62</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{9144966e-0db1-4ae8-a1ad-3fb19f21a144}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in2</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>0.85638393</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>23</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{9b761a44-1f92-480b-9656-ffdbfa02e0ed}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in1</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>0.86168943</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>19</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{cf92f08d-bb58-4a3d-acda-479165db96d4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>1</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb1</objectName>
  <x>12</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{557e7fb7-3fab-45bd-b4ea-cbe3fd20a92a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>-5.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb2</objectName>
  <x>51</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{38bd1f5e-dcf7-4897-a5c6-565e19983e63}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>-6.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>button1</objectName>
  <x>206</x>
  <y>327</y>
  <width>100</width>
  <height>48</height>
  <uuid>{a3545b15-5470-4efb-9dfd-eee56bc47d2a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>Generate note</text>
  <image>/</image>
  <eventLine>i3 0 0.5</eventLine>
  <latch>false</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>10</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>20</x>
  <y>311</y>
  <width>25</width>
  <height>25</height>
  <uuid>{a4ab00b5-62ab-45b1-9b1f-2978742d4057}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>notein</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.00000000</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>76</g>
   <b>17</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>25</r>
   <g>25</g>
   <b>25</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBDropdown" version="2">
  <objectName>signaltype</objectName>
  <x>549</x>
  <y>247</y>
  <width>110</width>
  <height>28</height>
  <uuid>{6d7e91db-0e40-4c78-8ff3-e01a436b45af}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>whitenoise</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>pinknoise</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>sine</name>
    <value>2</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>saw</name>
    <value>3</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>2</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>58</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{00820929-c5f2-4b95-857d-8f65bc444510}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>2</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>140</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{da5af5b6-02b8-49a3-b786-9b9d9cb6a8c0}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in4</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>101</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{bbbbb404-dee3-4f9a-8b33-ee195f268fd9}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in3</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#262626</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>97</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{731bae72-8739-4e2d-bcf8-dac396e37ccc}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>3</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb3</objectName>
  <x>90</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{0dea1661-8fb5-4157-8e62-59c9d9ab05a1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb4</objectName>
  <x>129</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{fa018fbc-6904-481b-9ab5-765ad0da46ca}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>136</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{727c9851-0e1b-4fc9-8afc-fd6c62f253be}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>4</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>218</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{e26d498d-3c52-4e91-83a3-1a269ef06cb8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in6</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#262626</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>179</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{01c6f2e2-d06d-4bc4-9dbf-6917d96e9e98}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in5</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>175</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{2846ddf1-0e08-457a-8d28-e6dd839a2eb8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>5</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb5</objectName>
  <x>168</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{5abc28b7-4a2d-472d-9b00-4bca52043588}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb6</objectName>
  <x>207</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{577dd7c9-26ee-41c7-b301-6f7b95b7dd6a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>214</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{f55bd16e-cd68-44a7-8b08-2f9e1183ab42}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>6</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>295</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{0de4815b-bd0e-4a3e-abd9-4fd53ca0079a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in8</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>257</x>
  <y>85</y>
  <width>16</width>
  <height>140</height>
  <uuid>{d0bca9d8-5b3a-4d17-bda8-da14e12ceaf0}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>in7</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#272727</borderColor>
  <color>
   <r>85</r>
   <g>255</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>253</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{34ecffc5-5d86-422e-97fc-e5c9fd0e0aa7}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>7</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb7</objectName>
  <x>246</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{e2d590fc-d377-4d40-bd2e-9f8ff3af5bc5}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>indb8</objectName>
  <x>285</x>
  <y>250</y>
  <width>36</width>
  <height>21</height>
  <uuid>{3da9b4f6-fe00-4b13-a271-976525c17057}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>125</r>
   <g>255</g>
   <b>60</b>
  </color>
  <bgcolor mode="background">
   <r>20</r>
   <g>20</g>
   <b>20</b>
  </bgcolor>
  <value>0.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>-120.00000000</minimum>
  <maximum>24.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>292</x>
  <y>227</y>
  <width>23</width>
  <height>25</height>
  <uuid>{4c976814-f547-431d-ab74-e9225f3aff12}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>8</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on1</objectName>
  <x>341</x>
  <y>170</y>
  <width>48</width>
  <height>48</height>
  <uuid>{55747531-f2e1-4e50-9839-3e788d995014}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>1</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>566</x>
  <y>174</y>
  <width>113</width>
  <height>35</height>
  <uuid>{a498f284-9d66-4ec8-9c40-07632ace83b4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Level (dB)</label>
  <alignment>center</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>188</g>
   <b>53</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>357</x>
  <y>87</y>
  <width>16</width>
  <height>80</height>
  <uuid>{738f62ff-0256-428d-826b-2e96fb28c483}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu1</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>3</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#262626</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on2</objectName>
  <x>391</x>
  <y>170</y>
  <width>48</width>
  <height>48</height>
  <uuid>{6fa42161-ada8-4ca1-a785-00e8d9ca3d0c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>2</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>406</x>
  <y>87</y>
  <width>16</width>
  <height>80</height>
  <uuid>{81589d71-6606-4fa8-b0fa-daeaf64b35ac}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu2</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on3</objectName>
  <x>440</x>
  <y>170</y>
  <width>48</width>
  <height>48</height>
  <uuid>{aeb8b307-415d-4a1f-83a6-edd57e6e525d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>3</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>456</x>
  <y>87</y>
  <width>16</width>
  <height>80</height>
  <uuid>{47096117-b4df-4a57-9f63-43e4433b93d2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu3</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on4</objectName>
  <x>490</x>
  <y>170</y>
  <width>48</width>
  <height>48</height>
  <uuid>{28590fdc-f133-43b3-99b8-7d7c379d5f79}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>4</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>506</x>
  <y>88</y>
  <width>16</width>
  <height>80</height>
  <uuid>{1c265b1b-8455-4c2c-96e6-e6f5d5d97caf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu4</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on5</objectName>
  <x>343</x>
  <y>327</y>
  <width>48</width>
  <height>48</height>
  <uuid>{7a266b0a-9e69-4579-b3d5-bf911c9ae383}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>5</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>358</x>
  <y>243</y>
  <width>16</width>
  <height>80</height>
  <uuid>{122c6052-83a6-4992-86e3-a44e37e9d19d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu5</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on6</objectName>
  <x>393</x>
  <y>327</y>
  <width>48</width>
  <height>48</height>
  <uuid>{e04bef86-57d9-46fc-8e12-508e8b42f03f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>6</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>407</x>
  <y>243</y>
  <width>16</width>
  <height>80</height>
  <uuid>{ad1f2483-c122-4501-8e77-b1795091923d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu6</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on7</objectName>
  <x>442</x>
  <y>327</y>
  <width>48</width>
  <height>48</height>
  <uuid>{987ad683-ffda-4c02-ba76-fba80eef6f99}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>7</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>457</x>
  <y>243</y>
  <width>16</width>
  <height>80</height>
  <uuid>{831662dd-60f9-46a5-a8b2-6335223bf3ab}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu7</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>on8</objectName>
  <x>492</x>
  <y>327</y>
  <width>48</width>
  <height>48</height>
  <uuid>{eb86882f-def6-42b6-90ae-22f49f88a876}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>8</text>
  <image>/</image>
  <eventLine/>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>24</fontsize>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>507</x>
  <y>243</y>
  <width>16</width>
  <height>80</height>
  <uuid>{2816aa9b-fa21-4b9c-a8ae-c29e6509d5b7}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>vu8</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.23529400</xValue>
  <yValue>-inf</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>16</x>
  <y>287</y>
  <width>82</width>
  <height>26</height>
  <uuid>{3112a9ff-3ad0-4640-be66-9c8f659b03c2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Note received</label>
  <alignment>left</alignment>
  <valignment>bottom</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>240</r>
   <g>240</g>
   <b>240</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBGraph" version="2">
  <objectName>graph-index</objectName>
  <x>550</x>
  <y>276</y>
  <width>142</width>
  <height>98</height>
  <uuid>{c1ed5763-b6f3-49fa-a32b-01e54082a604}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>0</value>
  <objectName2/>
  <zoomx>1.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <modex>lin</modex>
  <modey>lin</modey>
  <showSelector>false</showSelector>
  <showGrid>true</showGrid>
  <showTableInfo>false</showTableInfo>
  <showScrollbars>false</showScrollbars>
  <enableTables>false</enableTables>
  <enableDisplays>true</enableDisplays>
  <all>true</all>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>display</objectName>
  <x>46</x>
  <y>311</y>
  <width>140</width>
  <height>25</height>
  <uuid>{23d21873-0a2e-49e7-826a-9c6e4613e89a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>...</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>243</r>
   <g>243</g>
   <b>243</b>
  </color>
  <bgcolor mode="background">
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>0</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName/>
  <x>19</x>
  <y>355</y>
  <width>25</width>
  <height>25</height>
  <uuid>{4226d3d7-ef78-4a0f-8d15-8cd322f71634}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>ccin</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.00000000</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>0</r>
   <g>154</g>
   <b>231</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>25</r>
   <g>25</g>
   <b>25</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>16</x>
  <y>332</y>
  <width>82</width>
  <height>26</height>
  <uuid>{67bb9df3-11b5-4f60-a18d-7f0dd3cb6dd2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>CC received</label>
  <alignment>left</alignment>
  <valignment>bottom</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>240</r>
   <g>240</g>
   <b>240</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>displaycc</objectName>
  <x>45</x>
  <y>355</y>
  <width>140</width>
  <height>25</height>
  <uuid>{37470029-ec0e-4126-921e-eb82afe59857}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>...</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>243</r>
   <g>243</g>
   <b>243</b>
  </color>
  <bgcolor mode="background">
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>0</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBKnob" version="2">
  <objectName>leveldelta</objectName>
  <x>572</x>
  <y>88</y>
  <width>100</width>
  <height>100</height>
  <uuid>{f8080f9c-7129-4dca-882f-9a3d2607106b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.29000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f57c00</textcolor>
  <border>0</border>
  <borderColor>#512900</borderColor>
  <showvalue>false</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>samplerate</objectName>
  <x>508</x>
  <y>20</y>
  <width>60</width>
  <height>26</height>
  <uuid>{650b8432-c87b-4a3c-8b0e-cbb2b482c252}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>48000</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>18</fontsize>
  <precision>3</precision>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>nchnls</objectName>
  <x>660</x>
  <y>20</y>
  <width>30</width>
  <height>26</height>
  <uuid>{4fe7d33b-9e2f-41c1-9a8e-ef1d0f723b31}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>8</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>18</fontsize>
  <precision>3</precision>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>428</x>
  <y>21</y>
  <width>80</width>
  <height>25</height>
  <uuid>{9abf6954-2dc5-492c-bfb6-d4d40850f9f9}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Sample Rate</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>205</r>
   <g>205</g>
   <b>205</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>580</x>
  <y>21</y>
  <width>80</width>
  <height>25</height>
  <uuid>{eaacf58e-e1b3-4a36-8cc7-18bc45a9d98f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Channels</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>205</r>
   <g>205</g>
   <b>205</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBGraph" version="2">
  <objectName>spectrum-index</objectName>
  <x>6</x>
  <y>390</y>
  <width>698</width>
  <height>200</height>
  <uuid>{a99a03fb-0336-41b5-99f4-bc12ec8036c3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>1</value>
  <objectName2/>
  <zoomx>1.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <modex>lin</modex>
  <modey>lin</modey>
  <showSelector>false</showSelector>
  <showGrid>true</showGrid>
  <showTableInfo>true</showTableInfo>
  <showScrollbars>false</showScrollbars>
  <enableTables>false</enableTables>
  <enableDisplays>true</enableDisplays>
  <all>true</all>
 </bsbObject>
 <bsbObject type="BSBDropdown" version="2">
  <objectName>spmenu</objectName>
  <x>614</x>
  <y>391</y>
  <width>90</width>
  <height>28</height>
  <uuid>{a8934883-bf45-4155-be13-834e1b5f9a9b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>None</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 1</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 2</name>
    <value>2</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 3</name>
    <value>3</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 4</name>
    <value>4</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 5</name>
    <value>5</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 6</name>
    <value>6</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 7</name>
    <value>7</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>Input 8</name>
    <value>8</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>1</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>547</x>
  <y>226</y>
  <width>97</width>
  <height>24</height>
  <uuid>{8afe76f5-61f5-4669-b921-f9520406a099}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Signal Type</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>171</r>
   <g>171</g>
   <b>171</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScrollNumber" version="2">
  <objectName>signalfreq</objectName>
  <x>660</x>
  <y>248</y>
  <width>32</width>
  <height>26</height>
  <uuid>{53268cff-714e-423a-902b-7351141be7ec}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>11</fontsize>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <bgcolor mode="background">
   <r>39</r>
   <g>39</g>
   <b>39</b>
  </bgcolor>
  <value>1000.00000000</value>
  <resolution>1.00000000</resolution>
  <minimum>50.00000000</minimum>
  <maximum>4000.00000000</maximum>
  <bordermode>false</bordermode>
  <borderradius>3</borderradius>
  <borderwidth>0</borderwidth>
  <randomizable group="0">false</randomizable>
  <mouseControl act=""/>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>659</x>
  <y>226</y>
  <width>30</width>
  <height>24</height>
  <uuid>{ae79f074-43a6-4d48-965c-5f07fcc4d3ac}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Hz</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Arial</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>164</r>
   <g>164</g>
   <b>164</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDropdown" version="2">
  <objectName>fftsize</objectName>
  <x>614</x>
  <y>420</y>
  <width>90</width>
  <height>28</height>
  <uuid>{13bbe66e-eb51-4cd5-b44e-59eb1ee3e6dd}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description>FFT size</description>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>2048</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>4096</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>8192</name>
    <value>2</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>0</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
