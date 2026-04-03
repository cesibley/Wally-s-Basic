10 REM ==========================================================
20 REM BARNSLEY FERN (DENSER + BETTER PALETTE) - SCREEN 12
30 REM GW-BASIC / WBASIC
40 REM - Warm-up iterations used before plotting for cleaner density
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

200 REM Warm-up + draw counts
210 WARM = 2000         ' settle attractor before plotting
220 N = 1000000         ' plotted iterations after warm-up

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

340 REM Warm-up pass (no plotting): increases visible fern density/shape quality
350 FOR I = 1 TO WARM
360   GOSUB 900
370 NEXT I

380 REM Plot pass
390 FOR I = 1 TO N
400   IF (I MOD 600) = 0 THEN IF INKEY$ <> "" THEN END
410
420   GOSUB 900
430
440   PX = INT(XO + X * SX)
450   PY = INT(YO - Y * SY)
460
470   IF PX >= 0 AND PX <= 639 AND PY >= 0 AND PY <= 479 THEN
480     REM Palette selection:
490     REM mostly green, occasional highlights based on iteration stride
500     K = (I MOD 120)
510     IF K = 0 THEN
520       C = PAL(8)          ' rare white sparkle
530     ELSEIF (K MOD 23) = 0 THEN
540       C = PAL(7)          ' yellow highlight
550     ELSE
560       C = PAL(1 + (K MOD 6))  ' green ramp 1..6
570     END IF
580     PSET (PX, PY), C
590   END IF
600 NEXT I

610 LOCATE 2,1: PRINT "Done. Press any key."
620 WHILE INKEY$ = "": WEND
630 END

900 REM One Barnsley transform step
910 R = RND
920 IF R < 0.01 THEN
930   X1 = 0
940   Y1 = 0.16 * Y
950 ELSEIF R < 0.86 THEN
960   X1 = 0.85 * X + 0.04 * Y
970   Y1 = -0.04 * X + 0.85 * Y + 1.6
980 ELSEIF R < 0.93 THEN
990   X1 = 0.2 * X - 0.26 * Y
1000   Y1 = 0.23 * X + 0.22 * Y + 1.6
1010 ELSE
1020   X1 = -0.15 * X + 0.28 * Y
1030   Y1 = 0.26 * X + 0.24 * Y + 0.44
1040 END IF
1050 X = X1: Y = Y1
1060 RETURN
