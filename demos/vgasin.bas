10 REM ==========================================================
20 REM SINE WAVE SCROLLER USING GET/PUT STRIP BLITS
30 REM Similar to VGASIN.BAS, but scrolls the wave band with GET/PUT
40 REM and draws new samples as connected line segments.
50 REM Press any key to exit.
60 REM ==========================================================
70 SCREEN 12
80 CLS
90 AMP = 100
100 FREQ! = .04
110 YOFF = 240
120 STEP! = .2
130 SHIFT = 5
140 TOPY = YOFF - AMP - 2
150 BOTY = YOFF + AMP + 2
160 CHUNK = 16
170 PHASE! = 0
180 WAVEC = 11
190 DIM BUF%(20000)
200 LOCATE 1,1: PRINT "GET/PUT sine scroller - press any key to exit"
210 GOSUB 1000
220 IF INKEY$ <> "" THEN END
230 GOSUB 2000
240 PHASE! = PHASE! + STEP!
250 GOSUB 3000
260 T! = TIMER
270 WHILE TIMER < T! + .01: WEND
280 GOTO 220
1000 REM draw initial full wave with connected line segments
1010 COLOR WAVEC
1020 X1 = 0
1030 Y1 = INT(AMP * SIN(PHASE!) + YOFF + .5)
1040 FOR X2 = 1 TO 639
1050   A! = (X2 * FREQ!) + PHASE!
1060   Y2 = INT(AMP * SIN(A!) + YOFF + .5)
1070   LINE (X1,Y1)-(X2,Y2)
1080   X1 = X2
1090   Y1 = Y2
1100 NEXT X2
1110 RETURN
2000 REM scroll the wave band left using GET/PUT strip copies
2010 FOR SX = SHIFT TO 639 STEP CHUNK
2020   EX = SX + CHUNK - 1
2030   IF EX > 639 THEN EX = 639
2040   GET (SX,TOPY)-(EX,BOTY), BUF%
2050   PUT (SX - SHIFT,TOPY), BUF%, PSET
2060 NEXT SX
2070 LINE (640 - SHIFT,TOPY)-(639,BOTY),0,BF
2080 RETURN
3000 REM draw the fresh right-edge segment(s) for the new phase
3010 COLOR WAVEC
3020 X1 = 639 - SHIFT - 1
3030 A! = (X1 * FREQ!) + PHASE!
3040 Y1 = INT(AMP * SIN(A!) + YOFF + .5)
3050 FOR X2 = 640 - SHIFT TO 639
3060   A! = (X2 * FREQ!) + PHASE!
3070   Y2 = INT(AMP * SIN(A!) + YOFF + .5)
3080   LINE (X1,Y1)-(X2,Y2)
3090   X1 = X2
3100   Y1 = Y2
3110 NEXT X2
3120 RETURN
