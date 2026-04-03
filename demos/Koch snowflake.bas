10 SCREEN 12
20 CLS
30 KEY OFF
40 RANDOMIZE TIMER

50 LV = 5                 ' 3..5 recommended
60 CX = 320: CY = 250: SIDE = 340
70 H = SIDE * .8660254

80 X1 = CX - SIDE / 2: Y1 = CY + H / 3
90 X2 = CX + SIDE / 2: Y2 = Y1
100 X3 = CX: Y3 = CY - 2 * H / 3

110 DIM SX1(70000), SY1(70000), SX2(70000), SY2(70000), SL(70000)

120 C = 1
130 REM
140 COLR = C

150 XA = X1: YA = Y1: XB = X2: YB = Y2: GOSUB 1000
160 XA = X2: YA = Y2: XB = X3: YB = Y3: GOSUB 1000
170 XA = X3: YA = Y3: XB = X1: YB = Y1: GOSUB 1000

180 LOCATE 1, 1: COLOR 15
190 PRINT "Animated Koch Snowflake  Level:"; LV; "  Color:"; C; "  (press any key to stop)"

200 FOR D = 1 TO 100000: NEXT D     ' small delay; increase for slower animation

210 A$ = INKEY$
220 IF A$ <> "" THEN END

230 C = C + 1
240 IF C > 15 THEN C = 1
250 GOTO 130


1000 ' --- Draw one Koch side (iterative stack) ---
1010 SP = 1
1020 SX1(SP) = XA: SY1(SP) = YA
1030 SX2(SP) = XB: SY2(SP) = YB
1040 SL(SP) = LV

1050 IF SP = 0 THEN RETURN

1060 TX1 = SX1(SP): TY1 = SY1(SP)
1070 TX2 = SX2(SP): TY2 = SY2(SP)
1080 L = SL(SP)
1090 SP = SP - 1

1100 IF L <= 0 THEN LINE (TX1, TY1)-(TX2, TY2), COLR: GOTO 1050

1110 DX = (TX2 - TX1) / 3
1120 DY = (TY2 - TY1) / 3

1130 AX = TX1 + DX: AY = TY1 + DY
1140 CX2 = TX1 + 2 * DX: CY2 = TY1 + 2 * DY

1150 ' -60 degree rotation (screen Y grows downward)
1160 BX = AX + DX * .5 + DY * .8660254
1170 BY = AY - DX * .8660254 + DY * .5

1180 NL = L - 1

1190 ' push reverse order
1200 SP = SP + 1: SX1(SP) = CX2: SY1(SP) = CY2: SX2(SP) = TX2: SY2(SP) = TY2: SL(SP) = NL
1210 SP = SP + 1: SX1(SP) = BX:  SY1(SP) = BY:  SX2(SP) = CX2: SY2(SP) = CY2: SL(SP) = NL
1220 SP = SP + 1: SX1(SP) = AX:  SY1(SP) = AY:  SX2(SP) = BX:  SY2(SP) = BY:  SL(SP) = NL
1230 SP = SP + 1: SX1(SP) = TX1: SY1(SP) = TY1: SX2(SP) = AX:  SY2(SP) = AY:  SL(SP) = NL



1240 GOTO 1050