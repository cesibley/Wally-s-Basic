10 REM ============================================
20 REM WBASIC GRAPHICS COLOR PATTERN (SCREEN 1/2)
30 REM Draw filled rectangles for all palette colors
40 REM with overlay text labels per color block
50 REM ============================================
60
70 REM ---- SCREEN 1: 320x200 ----
80 SCREEN 1
90 CLS
100 BW = 80: BH = 50
110 FOR C = 0 TO 15
120   X = (C MOD 4) * BW
130   Y = (C \ 4) * BH
140   LINE (X,Y)-(X+BW-1,Y+BH-1), C, BF
150   IF C < 8 THEN FC = 15 ELSE FC = 0
160   COLOR FC, C
170   R = (C \ 4) * 6 + 3
180   K = (C MOD 4) * 20 + 3
190   LOCATE R, K
200   PRINT "COLOR"; C
210 NEXT C
220 COLOR 15,0
230 LOCATE 1,1
240 PRINT "SCREEN 1"
250
260 REM Keep visible until keypress
270 K$ = INKEY$: IF K$ = "" THEN 270
280
290 REM ---- SCREEN 2: 640x200 ----
300 SCREEN 2
310 CLS
320 BW = 160: BH = 50
330 FOR C = 0 TO 15
340   X = (C MOD 4) * BW
350   Y = (C \ 4) * BH
360   LINE (X,Y)-(X+BW-1,Y+BH-1), C, BF
370   IF C < 8 THEN FC = 15 ELSE FC = 0
380   COLOR FC, C
390   R = (C \ 4) * 6 + 3
400   K = (C MOD 4) * 20 + 3
410   LOCATE R, K
420   PRINT "COLOR"; C
430 NEXT C
440 COLOR 15,0
450 LOCATE 1,1
460 PRINT "SCREEN 2"
470
480 REM Keep visible until keypress
490 K$ = INKEY$: IF K$ = "" THEN 490
500 SCREEN 0
510 END
