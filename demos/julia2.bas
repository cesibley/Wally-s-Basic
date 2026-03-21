10 REM ==========================================================
20 REM JULIA SET FRACTAL (GW-BASIC / WBASIC) - SCREEN 12
30 REM Fixed parameter C = -0.8 + 0.156i
40 REM - Press any key to stop
50 REM - Renders every other scanline for speed (change STEPY)
60 REM ==========================================================
70 SCREEN 12
80 CLS
90 COLOR 15,0
100 LOCATE 1,1: PRINT "JULIA SET (SCREEN 12)  -  Press any key to stop"
110 LOCATE 2,1: PRINT "C = -0.8 + 0.156I"

120 MAXIT = 60
130 STEPY = 1      ' 1 = full resolution (slower), 2 = faster, 3+ even faster

140 CR = -0.8
150 CI = 0.156
160 XMIN = -1.8
170 XMAX =  1.8
180 YMIN = -1.35
190 YMAX =  1.35

200 XW = 639
210 YH = 479

220 FOR PY = 0 TO YH STEP STEPY
230   IF INKEY$ <> "" THEN END
240   ZY0 = YMIN + (YMAX - YMIN) * (PY / YH)

250   FOR PX = 0 TO XW
260     ZX0 = XMIN + (XMAX - XMIN) * (PX / XW)
270     ZX = ZX0
280     ZY = ZY0
290     I = 0

300     WHILE (ZX*ZX + ZY*ZY) < 4 AND I < MAXIT
310       XT = ZX*ZX - ZY*ZY + CR
320       ZY = 2 * ZX * ZY + CI
330       ZX = XT
340       I = I + 1
350     WEND

360     IF I >= MAXIT THEN
370       C = 0
380     ELSE
390       C = (I MOD 15) + 1     ' cycle 1..15
400     END IF

410     PSET (PX, PY), C
420     IF STEPY > 1 AND PY < YH THEN PSET (PX, PY + 1), C
430   NEXT PX
440 NEXT PY

450 LOCATE 3,1: PRINT "Done. Press any key."
460 WHILE INKEY$ = "": WEND
470 END
