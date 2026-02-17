10 REM ==========================================================
20 REM SMOOTH SCROLLING SINE WAVE DEMO (GET/PUT + DRAW)
30 REM - Scrolls graph area left by 1 pixel via GET/PUT each frame
40 REM - Draws incoming sine sample on right edge using DRAW command
50 REM - Bridges successive samples so the wave is continuous
60 REM - Press any key to exit
70 REM ==========================================================
80 SCREEN 1
90 WIDTH 40
100 CLS
110 COLOR 15,0
120 LOCATE 1,1: PRINT "SMOOTH SINE SCROLL (GET/PUT + DRAW)"
130 LOCATE 2,1: PRINT "Press any key to exit"
140 TOP = 16
150 BOT = 199
160 MID = (TOP + BOT) \ 2
170 AMP = 70
180 PH = 0
190 DIM B(60000)
200 YP = MID
210 LINE (0,TOP)-(319,BOT),0,BF
220 GET (1,TOP)-(319,BOT),B
230 PUT (0,TOP),B
240 LINE (319,TOP)-(319,BOT),0
250 Y = MID + INT(AMP * SIN(PH))
260 IF Y < TOP + 1 THEN Y = TOP + 1
270 IF Y > BOT - 1 THEN Y = BOT - 1
280 IF Y >= YP THEN CMD$ = "BM319," + LTRIM$(STR$(YP)) + "C11D" + LTRIM$(STR$(Y-YP))
290 IF Y < YP THEN CMD$ = "BM319," + LTRIM$(STR$(YP)) + "C11U" + LTRIM$(STR$(YP-Y))
300 IF Y = YP THEN CMD$ = "BM319," + LTRIM$(STR$(Y)) + "C11U1D1"
310 DRAW CMD$
320 YP = Y
330 PH = PH + .05
340 K$ = INKEY$
350 IF K$ <> "" THEN SCREEN 0: WIDTH 80: END
360 GOTO 220
