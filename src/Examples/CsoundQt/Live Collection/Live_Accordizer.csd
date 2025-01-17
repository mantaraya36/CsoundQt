<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

/*

	LIVE ACCORDIZER
	
*/

;example for CsoundQt
;written by joachim heintz
;may 2011
;please send bug reports and suggestions
;to jh at joachimheintz.de

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gifftsiz  =         1024 ;fft size
giovlap   =         gifftsiz / 4 ;overlap
giwinsiz  =         gifftsiz * 2 ;window size
giwintyp  =         1 ;von hann window

  opcode	ShowLED_a, 0, Sakkk
;shows an audiosignal in an outvalue channel, in dB or raw amplitudes
;Soutchan: string as name of the outvalue channel
;asig: audio signal to be shown
;kdispfreq: refresh frequency of the display (Hz)
;kdb: 1 = show as db, 0 = show as raw amplitudes (both in the range 0-1)
;kdbrange: if idb=1: which dB range is shown
Soutchan, asig, ktrig, kdb, kdbrange	xin
kdispval	max_k	asig, ktrig, 1
	if kdb != 0 then
kdb 		= 		dbfsamp(kdispval)
kval 		= 		(kdbrange + kdb) / kdbrange
	else
kval		=		kdispval
	endif
	if ktrig == 1 then
		outvalue	Soutchan, kval
	endif
  endop

  opcode ShowOver_a, 0, Sakk
;shows if asig has been larger than 1 and stays khold seconds
;Soutchan: string as name of the outvalue channel
;kdispfreq: refresh frequency of the display (Hz)
Soutchan, asig, ktrig, khold	xin
kon		init		0
ktim		times
kstart		init		0
kend		init		0
khold		=		(khold < .01 ? .01 : khold); avoiding too short hold times
kmax		max_k		asig, ktrig, 1
	if kon == 0 && kmax > 1 && ktrig == 1 then
kstart		=		ktim
kend		=		kstart + khold
		outvalue	Soutchan, kmax
kon		=		1
	endif
	if kon == 1 && ktim > kend && ktrig == 1 then
		outvalue	Soutchan, 0
kon		=		0
	endif
  endop

  opcode Y_x4p_k, k, kkkkk
;calculates y from two given points in x dimension, two in y dimension, and an x input, at i-rate
kx, kx1, kx2, ky1, ky2 xin
kmm       =         (ky2-ky1) / (kx2-kx1)
knn       =         ky1 - kmm*kx1
ky        =         kmm*kx + knn
          xout      ky
  endop


instr 1; master instrument
;;get live input 
kinchnl	invalue	"inchnl"
aLiveInPre	inch		kinchnl
kingaindb	invalue	"ingaindb"
kLiveInGain	=		ampdb(kingaindb)
aLiveInPost	=		aLiveInPre * kLiveInGain

;;show live input
gkTrigDisp	metro		10
gkshowdb	invalue	"showdb"
gkdbrange	invalue	"dbrange"
		ShowLED_a	"LiveInPre", aLiveInPre, gkTrigDisp, gkshowdb, gkdbrange
		ShowOver_a	"LiveInPreOver", aLiveInPre, gkTrigDisp, 2
		ShowLED_a	"LiveInPost", aLiveInPost, gkTrigDisp, gkshowdb, gkdbrange
		ShowOver_a	"LiveInPostOver", aLiveInPost, gkTrigDisp, 2

;;get values for transpositions
krmstim	invalue	"rmstim" ;time in seconds for making rms measurements
kthreshup	invalue	"threshup" ;db threshold for upper values
kcentup	invalue	"centup" ;cent distance between single transpositions for upper threshold
kthreshlow	invalue	"threshlow" ;db threshold for lower values
kcentlow	invalue	"centlow" ;cent distance between single transpositions for lower threshold

;;analyze rms and show it
ktrig 		metro		1
krms		rms		aLiveInPost, 1
kdb		=		dbamp(krms)
kmeanamp	max_k		aLiveInPost, ktrig, 1
		outvalue	"rms", kdb

;;calculate transposition values from current rms
kcent		Y_x4p_k	kdb, kthreshlow, kthreshup, kcentlow, kcentup
if kcentup < kcentlow then ;change names if kcentlow is larger than kcentup
kcentupcp	=		kcentup
kcentup	=		kcentlow
kcentlow	=		kcentupcp
endif
kcent		=		(kcent < kcentlow ? kcentlow : (kcent > kcentup ? kcentup : kcent))

;;perform fft and create 6 transposition values
fbas		pvsanal	aLiveInPost, gifftsiz, giovlap, giwinsiz, giwintyp
ftrans1	pvscale	fbas, cent(kcent)
ftrans2	pvscale	fbas, cent(2*kcent)
ftrans3	pvscale	fbas, cent(3*kcent)
ftrans4	pvscale	fbas, cent(-kcent)
ftrans5	pvscale	fbas, cent(-2*kcent)
ftrans6	pvscale	fbas, cent(-3*kcent)

;;resynthesize and mix
atrans1	pvsynth	ftrans1
atrans2	pvsynth	ftrans2
atrans3	pvsynth	ftrans3
atrans4	pvsynth	ftrans4
atrans5	pvsynth	ftrans5
atrans6	pvsynth	ftrans6
amix		=		atrans1 + atrans2 + atrans3 + atrans4 + atrans5 + atrans6

;;output
koutgaindb	invalue	"outgaindb"
aout		=		amix* ampdb(koutgaindb)
		outch		1, aout, 2, aout

;;show output
		ShowLED_a	"out", aout, gkTrigDisp, gkshowdb, gkdbrange
		ShowOver_a	"outover", aout, gkTrigDisp, 2

endin


</CsInstruments>
<CsScore>
i 1 0 36000
e
</CsScore>
</CsoundSynthesizer>



<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>203</x>
 <y>102</y>
 <width>913</width>
 <height>622</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>220</r>
  <g>220</g>
  <b>220</b>
 </bgcolor>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>9</x>
  <y>9</y>
  <width>886</width>
  <height>44</height>
  <uuid>{2a56f609-3f3a-49a2-8ef2-04e1041ed49f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>LIVE ACCORDIZER</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>26</fontsize>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>121</y>
  <width>891</width>
  <height>50</height>
  <uuid>{49b99e1e-2b4b-4aef-b413-95171c64b350}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Note that the latency depends on the ksmps value in the orchestra header, the software (csound flag -b) and the hardware (-B) buffer size.
</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>12</fontsize>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>173</y>
  <width>484</width>
  <height>148</height>
  <uuid>{e39c91d6-a0bd-4681-8dbc-e384493e578c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label> INPUT</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Lucida Grande</font>
  <fontsize>18</fontsize>
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
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>LiveInPre</objectName>
  <x>117</x>
  <y>205</y>
  <width>336</width>
  <height>22</height>
  <uuid>{7286a833-c0b0-4f55-8ecc-837453106e15}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>out1_post</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.09974708</xValue>
  <yValue>0.73076900</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>LiveInPreOver</objectName>
  <x>451</x>
  <y>205</y>
  <width>27</width>
  <height>22</height>
  <uuid>{42e31476-6cb3-42b4-a1fd-8f48e76892ac}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>outLover</objectName2>
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
   <r>196</r>
   <g>14</g>
   <b>12</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>LiveInPost</objectName>
  <x>113</x>
  <y>281</y>
  <width>336</width>
  <height>22</height>
  <uuid>{77fcb44e-b9a2-423d-9764-52f01754d96d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>out2_post</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.10528557</xValue>
  <yValue>0.59090900</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>LiveInPostOver</objectName>
  <x>447</x>
  <y>281</y>
  <width>27</width>
  <height>22</height>
  <uuid>{3e4c2001-fd4f-4b10-bd75-1766b5d1bbde}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>outRover</objectName2>
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
   <r>196</r>
   <g>14</g>
   <b>12</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>18</x>
  <y>205</y>
  <width>93</width>
  <height>28</height>
  <uuid>{9dd7688d-d7d4-4863-bbe8-44c1e622f2bd}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Live in Pre</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Lucida Grande</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>16</x>
  <y>281</y>
  <width>97</width>
  <height>29</height>
  <uuid>{a6c9d0da-7821-4528-b193-9ee9af3d441f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Live in Post</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Lucida Grande</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>17</x>
  <y>243</y>
  <width>93</width>
  <height>31</height>
  <uuid>{bdebe2b5-07c1-4f1f-b7cb-d2f29a26a5f8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Input Gain</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Lucida Grande</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>518</x>
  <y>172</y>
  <width>380</width>
  <height>217</height>
  <uuid>{e748e0c8-197c-4d4d-b893-9501e50ae02b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label> OUTPUT</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>18</fontsize>
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
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>out</objectName>
  <x>525</x>
  <y>238</y>
  <width>338</width>
  <height>24</height>
  <uuid>{133bd5e9-655f-4ba7-895d-45c9c8e54eb3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>out</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.25955719</xValue>
  <yValue>0.25955719</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>noborder</bordermode>
  <borderColor>#00FF00</borderColor>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>outover</objectName>
  <x>860</x>
  <y>238</y>
  <width>26</width>
  <height>24</height>
  <uuid>{dc5f0c73-a680-4e93-9855-6706fbd5180d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <objectName2>outRover</objectName2>
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
   <r>196</r>
   <g>14</g>
   <b>12</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>dbrange</objectName>
  <x>413</x>
  <y>333</y>
  <width>79</width>
  <height>27</height>
  <uuid>{81528aba-64e8-4dc7-b127-daa1434195a2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <alignment>left</alignment>
  <font>Lucida Grande</font>
  <fontsize>14</fontsize>
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
  <resolution>0.10000000</resolution>
  <minimum>1</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>50</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>326</x>
  <y>332</y>
  <width>90</width>
  <height>29</height>
  <uuid>{cc0e0187-e5c9-4b44-b8c0-28d00d45cb89}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>dB-Range</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>137</x>
  <y>333</y>
  <width>110</width>
  <height>27</height>
  <uuid>{df57c2f7-4a74-460f-9f7f-b3efb8106306}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Show LED's as</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBDropdown">
  <objectName>showdb</objectName>
  <x>246</x>
  <y>333</y>
  <width>80</width>
  <height>28</height>
  <uuid>{d09ab184-f4b1-4340-8730-e73b870bcf9a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>raw amps</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>dB</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>1</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBScope">
  <objectName/>
  <x>522</x>
  <y>269</y>
  <width>368</width>
  <height>105</height>
  <uuid>{c385ef49-3efa-4ec3-806a-ff883343cb9c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>-255.00000000</value>
  <type>scope</type>
  <zoomx>2.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
  <triggermode>NoTrigger</triggermode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>8</x>
  <y>400</y>
  <width>888</width>
  <height>198</height>
  <uuid>{f470cb16-0510-44e6-b6e0-c532a68e3409}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>ACCORDIZER</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
  <fontsize>18</fontsize>
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
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>526</x>
  <y>205</y>
  <width>95</width>
  <height>27</height>
  <uuid>{dcc76929-4dec-4912-b228-fcab4a27e75f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Output Gain</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>Lucida Grande</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>inchnl</objectName>
  <x>97</x>
  <y>334</y>
  <width>47</width>
  <height>27</height>
  <uuid>{2fce6993-35f8-4dd7-bedd-f809c0803989}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>DejaVu Sans</font>
  <fontsize>14</fontsize>
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
  <resolution>1.00000000</resolution>
  <minimum>1</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>1</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>-3</x>
  <y>333</y>
  <width>102</width>
  <height>29</height>
  <uuid>{df02308f-b978-4275-817e-1ad38c5b30d5}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Input Channel</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>391</x>
  <y>452</y>
  <width>266</width>
  <height>30</height>
  <uuid>{0d90798d-f4cd-4370-baf7-d117997cae4e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Accord Structure at high Threshold</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>365</x>
  <y>491</y>
  <width>235</width>
  <height>24</height>
  <uuid>{90dc5bd9-22fd-4aab-ac2a-a6e7cf34778a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Distance between adjacent tones (Cent)</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>centup</objectName>
  <x>614</x>
  <y>490</y>
  <width>64</width>
  <height>27</height>
  <uuid>{de281b04-9f42-41a7-9ad2-75515d6cd2e3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>1.00000000</resolution>
  <minimum>0</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>50</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>373</x>
  <y>518</y>
  <width>213</width>
  <height>42</height>
  <uuid>{dc562970-3547-4613-b423-f980f4a82ccb}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Threshold (= at which dB level or higher to reach his situation)</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>threshup</objectName>
  <x>614</x>
  <y>526</y>
  <width>64</width>
  <height>27</height>
  <uuid>{323951d3-56d2-48d5-afdc-c1eb8f54d4f5}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>1.00000000</resolution>
  <minimum>-50</minimum>
  <maximum>0</maximum>
  <randomizable group="0">false</randomizable>
  <value>-20</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>44</x>
  <y>450</y>
  <width>266</width>
  <height>30</height>
  <uuid>{30f7237e-42cb-49f9-9b89-0896da60f44c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Accord Structure at low Threshold</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>18</x>
  <y>489</y>
  <width>235</width>
  <height>24</height>
  <uuid>{635ff1ef-bd61-4e26-9c16-f01fd8ec1a59}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Distance between adjacent tones (Cent)</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>centlow</objectName>
  <x>267</x>
  <y>488</y>
  <width>64</width>
  <height>27</height>
  <uuid>{618669e3-8f70-44e5-aeb5-89bd7647b97e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>1.00000000</resolution>
  <minimum>0</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>450</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>26</x>
  <y>516</y>
  <width>213</width>
  <height>42</height>
  <uuid>{545f1c7f-6f98-4c47-9e3c-a07a88ce7a0f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Threshold (= at which dB level or lower to reach his situation)</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>Nimbus Sans L</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>threshlow</objectName>
  <x>267</x>
  <y>524</y>
  <width>64</width>
  <height>27</height>
  <uuid>{e2c3db92-0dd4-4a90-8819-43b132af448c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>1.00000000</resolution>
  <minimum>-50</minimum>
  <maximum>0</maximum>
  <randomizable group="0">false</randomizable>
  <value>-40</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>702</x>
  <y>415</y>
  <width>136</width>
  <height>42</height>
  <uuid>{d4ae25c0-9a81-4fd8-afce-694d22d3a04f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Time to determine an rms value (sec)</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>rmstim</objectName>
  <x>740</x>
  <y>462</y>
  <width>64</width>
  <height>27</height>
  <uuid>{e459468b-1ea5-4944-a962-65cc28b63eaf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>0.00100000</resolution>
  <minimum>0.001</minimum>
  <maximum>1</maximum>
  <randomizable group="0">false</randomizable>
  <value>0.001</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>707</x>
  <y>496</y>
  <width>136</width>
  <height>30</height>
  <uuid>{ccf95bda-000e-42c9-903d-c7d6d0ce0315}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>Current RMS</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBDisplay">
  <objectName>rms</objectName>
  <x>740</x>
  <y>527</y>
  <width>68</width>
  <height>29</height>
  <uuid>{0d761b7a-0eec-4bd7-a464-fe07f49af4dd}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>-54.061</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
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
  <bordermode>border</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>9</x>
  <y>63</y>
  <width>891</width>
  <height>50</height>
  <uuid>{c365e002-1222-4401-bf75-06202dd0bca6}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>The Live Input is transformed to an accord of six tones: three above and three below the live input. The distance of the notes depends on the rms level of the live input. You can set the values in the ACCORDIZER section.</label>
  <alignment>left</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>ingaindb</objectName>
  <x>115</x>
  <y>244</y>
  <width>260</width>
  <height>26</height>
  <uuid>{64de8264-0345-4faf-b16c-4af284a95268}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>-12.00000000</minimum>
  <maximum>12.00000000</maximum>
  <value>0.27692308</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>453</x>
  <y>243</y>
  <width>30</width>
  <height>31</height>
  <uuid>{eecbf48f-983f-4ce5-bdcc-e6b1ae657b20}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>dB</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>ingaindb</objectName>
  <x>375</x>
  <y>245</y>
  <width>76</width>
  <height>27</height>
  <uuid>{a44b45c8-8c22-4c53-9442-75339608f708}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>0.00100000</resolution>
  <minimum>-1e+12</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>0.276923</value>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>outgaindb</objectName>
  <x>622</x>
  <y>205</y>
  <width>155</width>
  <height>24</height>
  <uuid>{2735c656-a8f6-429d-acc3-2fe0db7e078a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>-12.00000000</minimum>
  <maximum>12.00000000</maximum>
  <value>-0.69677419</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>858</x>
  <y>203</y>
  <width>30</width>
  <height>31</height>
  <uuid>{a5947f34-1e11-490f-bd5d-c543d6e4494e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <label>dB</label>
  <alignment>right</alignment>
  <valignment>top</valignment>
  <font>DejaVu Sans</font>
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
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>outgaindb</objectName>
  <x>778</x>
  <y>204</y>
  <width>76</width>
  <height>27</height>
  <uuid>{52bb78bb-8e63-44a1-a86a-5def867d5311}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
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
  <resolution>0.00100000</resolution>
  <minimum>-1e+12</minimum>
  <maximum>1e+12</maximum>
  <randomizable group="0">false</randomizable>
  <value>-0.696774</value>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<EventPanel name="" tempo="60.00000000" loop="8.00000000" x="360" y="248" width="612" height="322" visible="false" loopStart="0" loopEnd="0">    </EventPanel>
