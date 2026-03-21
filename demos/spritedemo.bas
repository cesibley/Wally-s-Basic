10 REM ================================================
20 REM GET/PUT CIRCLE MOVE DEMO (KEYPRESS STEP)
30 REM Draw filled circle, GET it, wait key, erase, PUT elsewhere.
40 REM Repeat forever until STOP/BREAK.
50 REM ================================================
60 SCREEN 1
70 WIDTH 40
80 CLS
90 DIM SPR(10000)
100 R = 18
110 X = 70
120 Y = 90
130 DX = 36
140 DY = 0
150 COLOR 15,0
160 LOCATE 1,1: PRINT "GET/PUT circle move demo"
170 LOCATE 2,1: PRINT "Press any key to move the circle"
180 LOCATE 3,1: PRINT "Use STOP to exit"
190 GOSUB 500
200 GET (X-R,Y-R)-(X+R,Y+R),SPR
210 GOSUB 900
220 LINE (X-R,Y-R)-(X+R,Y+R),0,BF
230 X = X + DX
240 Y = Y + DY
250 IF X-R < 0 OR X+R > 319 THEN DX = -DX: X = X + DX
260 IF Y-R < 0 OR Y+R > 199 THEN DY = -DY: Y = Y + DY
270 PUT (X-R,Y-R),SPR
280 GOTO 210
500 CIRCLE (X,Y),R,3
510 PAINT (X,Y),3
520 RETURN
900 K$ = INKEY$
910 IF K$ = "" THEN 900
920 RETURN
