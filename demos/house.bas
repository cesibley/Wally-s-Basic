10 REM -- WBASIC Picture Program: A Simple House --
20 SCREEN 1
30 COLOR 15, 1
40 CLS
50 REM -- Sky is background color set by COLOR --
60 REM -- Ground --
70 LINE (0, 150)-(319, 199), 2, BF
80 REM -- House body (brown) --
90 LINE (100, 100)-(220, 160), 6, BF
100 REM -- Roof (red) --
110 LINE (100, 100)-(160, 60), 4
120 LINE (160, 60)-(220, 100), 4
130 LINE (100, 100)-(220, 100), 4
140 PAINT (160, 70), 4, 4
150 REM -- Door (cyan) --
160 LINE (150, 130)-(170, 160), 3, BF
170 REM -- Window (white frame, cyan fill) --
180 LINE (110, 110)-(130, 125), 15, B
190 PAINT (111, 111), 3, 15
200 REM -- Sun (yellow) --
210 CIRCLE (280, 40), 20, 14
220 PAINT (280, 40), 14, 14
230 REM -- Wait for key --
240 Z$ = INKEY$ : IF Z$ = "" THEN 240
