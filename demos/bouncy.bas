10 REM ==========================================================
20 REM BOUNCY BALL - SCREEN 12
30 REM Speed + Angle physics + Screen Border
40 REM Press any key to exit
50 REM ==========================================================

60 DEFINT A-Z
70 SCREEN 12
80 WIDTH 80
90 CLS
100 RANDOMIZE TIMER

110 PI! = 3.14159265

120 REM ---- DRAW SCREEN BORDER ----
130 LINE (0,0)-(639,479), 15, B      ' outer white border
140 LINE (1,1)-(638,478), 8, B       ' inner gray border

150 REM ---- BALL SIZE ----
160 R = 30
170 DIAM = R * 2
180 SPRW = DIAM + 1
190 SPRH = DIAM + 1

200 DIM B%(20000)

210 REM ---- INITIAL POSITION ----
220 X! = 200
230 Y! = 150

240 REM ---- SPEED + ANGLE ----
250 SPEEED! = 4.5
260 ANGLE! = RND * 2 * PI!

270 BALLC = 14
280 OUTLINEC = 15
290 BGC = 0

300 REM ---- CREATE SPRITE ----
310 TMPX = 10: TMPY = 10
320 LINE (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), BGC, BF
330 CIRCLE (TMPX + R, TMPY + R), R, OUTLINEC
340 PAINT (TMPX + R, TMPY + R), BALLC, OUTLINEC
350 GET (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), B%
360 LINE (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), BGC, BF

370 OLDX = INT(X!)
380 OLDY = INT(Y!)
390 PUT (OLDX, OLDY), B%, XOR

400 DO
410   IF INKEY$ <> "" THEN EXIT DO

420   PUT (OLDX, OLDY), B%, XOR

430   VX! = SPEEED! * COS(ANGLE!)
440   VY! = SPEEED! * SIN(ANGLE!)

450   X! = X! + VX!
460   Y! = Y! + VY!

470   REM ---- LEFT / RIGHT WALL ----
480   IF X! < 1 THEN
490       X! = 1
500       ANGLE! = PI! - ANGLE!
510       ANGLE! = ANGLE! + (RND - .5) * .2
520   END IF

530   IF X! > 638 - (SPRW - 1) THEN
540       X! = 638 - (SPRW - 1)
550       ANGLE! = PI! - ANGLE!
560       ANGLE! = ANGLE! + (RND - .5) * .2
570   END IF

580   REM ---- TOP / BOTTOM WALL ----
590   IF Y! < 1 THEN
600       Y! = 1
610       ANGLE! = -ANGLE!
620       ANGLE! = ANGLE! + (RND - .5) * .2
630   END IF

640   IF Y! > 478 - (SPRH - 1) THEN
650       Y! = 478 - (SPRH - 1)
660       ANGLE! = -ANGLE!
670       ANGLE! = ANGLE! + (RND - .5) * .2
680   END IF

690   NEWX = INT(X!)
700   NEWY = INT(Y!)

710   PUT (NEWX, NEWY), B%, XOR
720   OLDX = NEWX
730   OLDY = NEWY

740   FOR D = 1 TO 2000: NEXT D
750 LOOP

760 PUT (OLDX, OLDY), B%, XOR
770 LOCATE 1,1: PRINT "Done."
780 END
