10 REM Sierpinski triangle for WBASIC (SCREEN 12 full screen)
20 RANDOMIZE TIMER
30 XMAX% = 639: YMAX% = 479: BORDER% = 10
40 SCREEN 12: CLS
50 LOCATE 2, 3: PRINT "Sierpinski triangle"
60 DIM CX%(2), CY%(2)   ' coordinates of triangle vertices
70 CX%(0) = XMAX% / 2: CY%(0) = BORDER%
80 CX%(1) = BORDER%: CY%(1) = YMAX% - BORDER%
90 CX%(2) = XMAX% - BORDER%: CY%(2) = YMAX% - BORDER%
100 PX% = XMAX% / 2
110 PY% = YMAX% / 2
120 COL% = 0: COL2% = 0: COL3% = 0
130 FOR K = 1 TO 90000
140 INDEX% = INT(RND * 3)
150 PX% = (PX% + CX%(INDEX%)) / 2
160 PY% = (PY% + CY%(INDEX%)) / 2
170 PSET (PX%, PY%), COL3%
180 COL3% = COL2%: COL2% = COL%: COL% = 13 + INDEX%
190 NEXT K
200 LOCATE 3, 3: PRINT "Drawing completed"
210 END
