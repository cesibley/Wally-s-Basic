10 REM ============================================
20 REM WBASIC GRAPHICS COLOR PATTERN (SCREEN 1/2)
30 REM Draw filled rectangles for all palette colors
40 REM ============================================
50
60 REM ---- SCREEN 1: 320x200 ----
70 SCREEN 1
80 CLS
90 BW = 80: BH = 50
100 FOR C = 0 TO 15
110   X = (C MOD 4) * BW
120   Y = (C \ 4) * BH
130   LINE (X,Y)-(X+BW-1,Y+BH-1), C, BF
140 NEXT C
150
160 REM Keep visible until keypress
170 K$ = INKEY$: IF K$ = "" THEN 170
180
190 REM ---- SCREEN 2: 640x200 ----
200 SCREEN 2
210 CLS
220 BW = 160: BH = 50
230 FOR C = 0 TO 15
240   X = (C MOD 4) * BW
250   Y = (C \ 4) * BH
260   LINE (X,Y)-(X+BW-1,Y+BH-1), C, BF
270 NEXT C
280
290 REM Keep visible until keypress
300 K$ = INKEY$: IF K$ = "" THEN 300
310 SCREEN 0
320 END
