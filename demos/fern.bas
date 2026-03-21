10 REM ==========================================================
20 REM BARNSLEY FERN (DENSER + BETTER PALETTE) - SCREEN 12
30 REM GW-BASIC / WBASIC
40 REM - Warm-up iterations removed for density
50 REM - Green palette ramp w/ subtle highlights
60 REM - Press any key to stop
70 REM ==========================================================
80 SCREEN 12
90 CLS
100 COLOR 10, 0
110 LOCATE 1,1: PRINT "BARNSLEY FERN (DENSE + PALETTE) - Press any key to stop"

120 RANDOMIZE TIMER

130 REM Fern state (x,y)
140 X = 0: Y = 0

150 REM Scaling to fit SCREEN 12 (tweak if desired)
160 SX = 80
170 SY = 45
180 XO = 320
190 YO = 470

200 REM How many points to draw + warm-up
210 N = 1000000        ' total iterations
220 WARM = 50         ' skip first points for density (try 50..200)

230 REM A nicer SCREEN 12 palette ramp (greens + highlight)
240 REM (indexes are standard 0..15 colors; 0 is black)
250 DIM PAL(8)
260 PAL(1) = 2   ' green
270 PAL(2) = 10  ' bright green
280 PAL(3) = 2
290 PAL(4) = 10
300 PAL(5) = 2
310 PAL(6) = 10
320 PAL(7) = 14  ' yellow highlight
330 PAL(8) = 15  ' white sparkle (rare)

340 FOR I = 1 TO N
350   IF (I MOD 600) = 0 THEN IF INKEY$ <> "" THEN END

360   R = RND

370   IF R < 0.01 THEN
380     X1 = 0
390     Y1 = 0.16 * Y

400   ELSEIF R < 0.86 THEN
410     X1 = 0.85 * X + 0.04 * Y
420     Y1 = -0.04 * X + 0.85 * Y + 1.6

430   ELSEIF R < 0.93 THEN
440     X1 = 0.2 * X - 0.26 * Y
450     Y1 = 0.23 * X + 0.22 * Y + 1.6

460   ELSE
470     X1 = -0.15 * X + 0.28 * Y
480     Y1 = 0.26 * X + 0.24 * Y + 0.44
490   END IF

500   X = X1: Y = Y1

510   IF I > WARM THEN
520     PX = INT(XO + X * SX)
530     PY = INT(YO - Y * SY)

540     IF PX >= 0 AND PX <= 639 AND PY >= 0 AND PY <= 479 THEN
550       REM Palette selection:
560       REM mostly green, occasional highlights based on iteration stride
570       K = (I MOD 120)
580       IF K = 0 THEN
590         C = PAL(8)          ' rare white sparkle
600       ELSEIF (K MOD 23) = 0 THEN
610         C = PAL(7)          ' yellow highlight
620       ELSE
630         C = PAL(1 + (K MOD 6))  ' green ramp 1..6
640       END IF
650       PSET (PX, PY), C
660     END IF
670   END IF
680 NEXT I

690 LOCATE 2,1: PRINT "Done. Press any key."
700 WHILE INKEY$ = "": WEND
710 END
