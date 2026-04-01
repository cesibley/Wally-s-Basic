10 ' MODULAR MULTIPLICATION CIRCLE - FULL SCREEN 12 (640x480)
20 ' by Kurt Moerman
30 ' https://www.facebook.com/profile.php?id=100010796895973
40 '
50 CLS
60 PRINT "Modular multiplication circle"
70 INPUT "Value of m (divisions, default 300): ", M%
80 IF M% = 0 THEN M% = 300
90 INPUT "Value of p (multiplier, default 77): ", P%
100 IF P% = 0 THEN P% = 77
110 SCREEN 12: CLS
120 PI! = 3.14159265
130 CX% = 320: CY% = 240
140 R% = 220
150 ASPECT! = 1
160 DIM X%(M% - 1), Y%(M% - 1)
170 FOR N% = 0 TO M% - 1
180 ALPHA! = PI! * (2 * N% / M% + .5)
190 X%(N%) = CX% - R% * COS(ALPHA!)
200 Y%(N%) = CY% - (R% * SIN(ALPHA!)) / ASPECT!
210 NEXT N%
220 KEY OFF
230 FOR N% = 0 TO M% - 1
240 K% = (N% * P%) MOD M%
250 LINE (X%(K%), Y%(K%))-(X%(N%), Y%(N%)), 15
260 NEXT N%
270 COLOR 14: LOCATE 1, 2: PRINT "Modular Multiplication Circle"
280 COLOR 11: LOCATE 2, 2: PRINT "m="; M%; "  p="; P%
290 COLOR 7
