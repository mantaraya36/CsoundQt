A demo and "Sandbox" for getting started with scanned synthesis.

Many  comments from experiences are added to the code (see instrument "sound" and tables in the score.

Meant to be run in CsoundQt but the code can be used also in other ways.

By Tarmo Johannes trmjhnns@gmail.com 2013/2016

<CsoundSynthesizer>
<CsOptions>
-odac
;-d ; ucomment if you don't want to have output about tables and fft analyse
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

chn_k "preset",3

giTest filelen "../../SourceMaterials/circularstring-128"; to check if it is found, otherwise it will crash scanu in runtime

; presets: every preset consist  of 5 parameters: fninit, scanning rate,  fnmass,   fncentr,   fn_scanning_trajectory (all of then are table numbers, expect scanning rate)
 
giPresets ftgen 0,0,256,-2, \
  	18, 0.03, 22, 40, 71,  \ ; preset no 1 for instr preset_call, index 0 in table
  	10, 0.02, 20, 40, 70, \
	16, 0.01, 21, 40, 71, \
	12, 0.2, 21, 40, 72, \
	12, 0.2, 21, 40, 72, \  
	14, 0.002, 22, 42, 72, \ 
	19, 0.12, 20, 40, 73, \ 
	11, 0.01, 23, 42, 71, \ 
	14, 0.003, 22, 43, 74, \ 
	19, 0.1, 25, 41, 70, \	
	13, 0.05, 20, 41, 70, \ ; preset 11
	11, 0.1, 23, 42, 72, \ 
	12, 0.3, 25, 42, 70,\ 
	14,   0.001, 22, 43, 74, \ 
	12, 0.005, 20, 40, 74,\ 
	10, 0.01, 22, 41, 74, \
	12, 0.008, 21, 42, 72, \
	14, 0.002, 21, 42, 72, \
	14, 0.002, 21, 40, 72,\  
	14, 0.1, 20, 40, 72,\ 
	17, 0.02, 21, 40,72 ,\ ; preset  21
	15, 0.012, 22,43,70,\
	18, 0.43, 20, 40, 72,\
	10, 0.02, 20, 40, 70, \
	16, 0.015, 22, 41, 71, \
	12, 0.235, 20, 40, 73, \
	13, 0.188, 21, 43, 70, \  
	14, 0.0025, 21, 42, 73, \ 
	15, 0.128, 20, 41, 73, \ 
	18, 0.122, 20, 42, 73 ; 30


; helping instrument to enable to change preset by midi CC 1
alwayson "midireader" ; comment out, if you don't use MIDI control
instr midireader
	kvalue ctrl7 1,1,1,30
	kvalue = int(kvalue)
	if (changed(kvalue)==1) then 
		;printk2 kvalue
		chnset kvalue, "preset"
	endif
endin
	
; instrument for generating wav files from scanned sounds
; schedule "make_files",0,10,11, 7.07	; vali 5.07, 7.02
; 10 5.00 5
instr make_files
	icount = 10
	idur = p3
	ifreq = p5
	ibase = p4 ;1 ; 11, 21
	index = 0
looppoint:
	schedule "preset_call", index*idur+1, idur,  -10,  ifreq, index+ibase, 21+index
	loop_lt index, 1, icount, looppoint   	
endin	
	

	
instr preset_call, 1 ; instrument to read data from presets and start sounding instrument. Can be played also from MIDI keyboard

	iamp = (p4<0) ? p4 :  dbfsamp( ampmidi(0.6)) ; in dbFS (NB!if given by parameter, should be <0, otherwise MIDI velocity does not work)
	ifreq =  (p5>0) ? cpspch(p5) : cpsmidi() ; given as 8.05 etc. Or from Midi
	inr = (p6==0) ? chnget:i("preset")-1 : p6-1 ; preset numbers -> index 
	ifileindex = p7 ; if set, forfard it to sound instrument to record the sound with name sounds/soundin.<index> 
	print iamp, ifreq, inr
	ip1 tab_i inr*5+0, giPresets
	ip2 tab_i inr*5+1, giPresets
	ip3 tab_i inr*5+2, giPresets
	ip4 tab_i inr*5+3, giPresets
	ip5 tab_i inr*5+4, giPresets
	
	;event_i "i","sound",0,p3,iamp,ip1,ip2,ip3,ip4,ip5,ifreq, ifileindex
	ascanned subinstr "sound", iamp,ip1,ip2,ip3,ip4,ip5,ifreq, ifileindex ; subinstr to be able to play from MIDI
	outs ascanned, ascanned
	dispfft ascanned, 0.1, 1024;, 0, 0
endin

instr sound
	iamp= ampdbfs(p4)	
	; see manual for scanu about the parameters	 
	; see below by the table deifinitions for comments
	
	iinit = p5
	irate = (p6 == 0) ? 0.01 : p6 ; do not allow 0!
	; irate - essential influence to the soud. If bigger value, less ondulations, more stable sound. Smaller value - often more interesting (try something like 0.001..0.3)

	ifnvel = 60 
	ifnmass = p7
	ifnstif = 30 ; matrix of the
	ifncentr = p8
	ifndamp = 50
	
	kmass = 1  ; k-parameters influence the sound supprosingly little (whwn changed in runtime. The most influence I heard by changing kstif
	kstif = .1 
	kcentr = .1 
	kdamp = -0.001 ; you can make the sound to grow -small positive values (!avoid exploding)- or dampen ( use small negative values)
	ileft = 0.1  ; did not make so much difference for me
	iright = 0.5 
	kpos = 0  
	kstrngth = 0

	idisp = 0
	id = 2		
	itraj = p9
	
	ifreq = p10
	ain = 0 ; a nice effect to use some band filtered white noise as ain- works a little bit like EQ boost, but more interesting. Other "beautiful" signals got too cracling for me and the mixture was not so interesting.
	
	scanu iinit, irate, ifnvel, ifnmass, ifnstif, ifncentr, ifndamp, kmass, kstif, kcentr, kdamp, ileft, iright, kpos, kstrngth, ain, idisp, id
	
	kenv madsr 0.1,0.1,0.8,0.5 
	a1  scans iamp*kenv, ifreq, itraj, id 
	
	ifileindex = p11
	if (ifileindex>0) then
		Sfilename sprintf "sounds/soundin.%d",ifileindex
		fout Sfilename,4,a1
	endif 	
	
	
	outs a1,a1
endin


</CsInstruments>
<CsScore>

; Initial condition, sometimes said as the shape of "hammerhead" striking  against the string - MAJOR INFLUENCE TO SOUND
f 10 0 128 7 1 128 0.001 ; With gen7 cleaner beginning than gen5. Gen5 - more overtones
f 11 0 128 10 1 0.3 0.2 0.1 ; sine- quite interesting,  a lot of overtones, use irate something like 0.05 
f 12 0 128 20 4 5 7 ;blackman window, intresting also with other forms of gen20  
f 13 0 128 7 0 40 0.75 42 0.28 36 0.66 10 1 ; different zig-zagd  
f 14 0 128 7 0.99 35 0.02 27 0.99 20 0.03 46 1 
f 15 0 128 7 0.99 33 1 28 0.99 20 0.03 15 1 31 1 
f 16 0 128 7 0 52 0.3 44 0.65 28 0.01 4 1 
f 17 0 128 7 0 15 0.57 37 0.3 5 0.68 38 0.65 3 0.95 19 0.72 10 1 
f 18 0 128 7 0 6 0 6 0.51 6 0 40 0 8 0.99 5 0 22 0 10 0.58 6 0 18 0 
f 19 0 128 5 0.001 128 1 ; exponent up
 
; Masses 
f 20 0 128 -7 1 128 1 ; OK, if all equal. More interesting if first masses smaller. For example 0.3..0.8 
; If masses over 1, the sound is denser and more harmonics.; ; interesting also like 10..12 
f 21 0 128 -7 0.3 128 0.8 
f 22 0 128 -7 1 128 1.5 
f 23 0 128 -7 10 128 12 
f 24 0 128 -7 1 128 0.2
f 25 0 128 -7 10 128 1
 
; Spring matrices 
f 30 0 16384 -23 "../../SourceMaterials/circularstring-128" 
; original: "string-128.matrix" 
; grid - quite interesting; string and torus - not big difference 
; full - does not function well
; circular - more overtones, spectrum more even that with string 
; spiral - quite interesting, buzz-like, many samples out of range 
 
; Centering force 
f 40  0 128 -7 0.01 128 2 ;quite interesting when decreasing - 1..0 - a lot of harmoncis. 0..1 - cleaner sound; all 0 -  relatively; kui 0..0.1, more subtle, higher sounding    
; quite interesting very small: 0..0.25 
f 41 0 128 -7 1 128 0 
f 42 0 128 -7 0 128 0.1
f 43 0 128 -5 1 64 0.5  64 1 ; exponent curve down
f 44 0 128 -5 0.1 64 0.5 64 0.1 ; eksponent wave

 
; Damping 
f 50 0 128 -7 1 128 1 ;64 1 0 0 64 0     ;1 128 1      ; interesting, if some parts 0    ; does not incluence  much 1..1 OK 
 
; Initial velocity 
f 60 0 128 -7 0 128 0 ; 0 128 20 1     ;0 128 -7 0 64 0.5 64 0     ;128 0.8      ; dangerous - can explode. ; gen20 - interesting ;everything 0 - is OK, 
 
; Scanning trajectories ; the numbers may not exeed of the number of pints in matrix
f 70 0 128 -5 1 64 127 64 1 ; forst half back and forth
f 71 0 128 -7 1 128 127 ; mas by mass
f 72 0 128 -20 1 127 ; Hanning - interesting
f 73 0 128  -7 1 40 92 42 20 36 120 10 127 ; zigzag
f 74 0 128  -7 127 40 32 42 64 36 16 10 1 

  
f0 3600 ; wait for live events

; preset_call start dur amp freq preset_no					
;i "preset_call" 0 4.5 -10 6.00 1
;i . 4 4.5 . 6.02 2
;i . 8 4.5 . 6.00 6
;i . 11 4.5 . 6.04 3
;i . 14 18.5 . 6.03  4


;i . 			+ . .	. 	2
;i . 			+ . .	. 	3
;i . 			+ . .	. 	4
;etc


</CsScore>


</CsoundSynthesizer>


































<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>493</width>
 <height>594</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>80</r>
  <g>80</g>
  <b>80</b>
 </bgcolor>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>6</x>
  <y>11</y>
  <width>479</width>
  <height>250</height>
  <uuid>{631d7941-597e-4fe7-b562-bde75ce8db38}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Scanned synthesis sandbox for experiments

Press Live Events for tweaking different parameteres

You can also choose preset from widget and press "On/Off"  button to hear the sound.

Or play using MIDI keyboard or Virtual MIDI keyboard

See csd for comments about different parameters.

Have fun!

			tarmo</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>14</fontsize>
  <precision>3</precision>
  <color>
   <r>235</r>
   <g>235</g>
   <b>235</b>
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
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>preset</objectName>
  <x>78</x>
  <y>368</y>
  <width>54</width>
  <height>25</height>
  <uuid>{8bab02d9-3429-4e68-bbc2-c5636353bc1f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>left</alignment>
  <font>Liberation Sans</font>
  <fontsize>10</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="background">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>1.00000000</resolution>
  <minimum>1</minimum>
  <maximum>30</maximum>
  <randomizable group="0">false</randomizable>
  <value>30</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>13</x>
  <y>367</y>
  <width>58</width>
  <height>25</height>
  <uuid>{f99002b6-bed6-4f13-bb57-b9b80b2c9da0}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Preset: </label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>235</r>
   <g>235</g>
   <b>235</b>
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
 <bsbObject version="2" type="BSBButton">
  <objectName>button3</objectName>
  <x>146</x>
  <y>366</y>
  <width>100</width>
  <height>30</height>
  <uuid>{b7ed5a76-2c06-4e35-988e-630c6e7c38d8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>Note On/Off</text>
  <image>/</image>
  <eventLine>i1 0 -1  -10  8.00 0 0</eventLine>
  <latch>true</latch>
  <momentaryMidiButton>false</momentaryMidiButton>
  <latched>false</latched>
  <fontsize>10</fontsize>
 </bsbObject>
 <bsbObject version="2" type="BSBGraph">
  <objectName/>
  <x>14</x>
  <y>401</y>
  <width>369</width>
  <height>193</height>
  <uuid>{9d6c9171-d93b-4630-8cd4-ee4992293e89}</uuid>
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
  <showSelector>true</showSelector>
  <showGrid>true</showGrid>
  <showTableInfo>true</showTableInfo>
  <showScrollbars>true</showScrollbars>
  <enableTables>true</enableTables>
  <enableDisplays>true</enableDisplays>
  <all>true</all>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>271</y>
  <width>480</width>
  <height>86</height>
  <uuid>{be249736-2965-4218-b915-49f2cad28998}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Change preset by the spinbox below or (Virtual) Midi controller Channel 1, CC 1 (mostly modulator wheel).

New presets apply to new notes, no infulence during playing time.</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>235</r>
   <g>235</g>
   <b>235</b>
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
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>394</x>
  <y>398</y>
  <width>99</width>
  <height>179</height>
  <uuid>{6a631529-6fa3-4414-8c12-4d6cd39b1320}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Have a look at the FFT display (last one in menu)</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>235</r>
   <g>235</g>
   <b>235</b>
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
</bsbPanel>
<bsbPresets>
</bsbPresets>
<EventPanel name="tweak parameters of isntr sound" tempo="60.00000000" loop="8.00000000" x="442" y="697" width="655" height="346" visible="false" loopStart="0" loopEnd="0">;               ;                ;                ;                ;amp dbFS               ;init               ;irate               ;massed               ;centering               ;scanning trajectory               ;freq 
i "sound" 0 5 -10 18 0.43 20 40 72 261 
i . 1 . . . . . . . 100 
i . 2 . . . . . . . 200 
i . 3 . . . . . . . 400 
i . 4 . . . . . . . 800 
i . 5 . . . . . . . 2000 </EventPanel>
<EventPanel name="" tempo="60.00000000" loop="8.00000000" x="120" y="455" width="655" height="346" visible="false" loopStart="0" loopEnd="0">;               ;                ;                ;                ;amp               ;freq               ;preset             ;save file 
i "preset_call" 0 4 -10 8.07 3 0 
; viimane oli preset 8    </EventPanel>
