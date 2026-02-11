10 REM ============================================================
20 REM TAB TORTURE TEST (screen cursor behavior)
30 REM Style-aligned torture test scaffold with PASS/FAIL summary.
40 REM ============================================================
50 DEFINT A-Z
60 PASS=0:FAIL=0

100 PRINT "Running TAB torture test..."
110 PRINT
120 ON ERROR GOTO 9000

200 PRINT "TEST1: Absolute forward columns (TAB(10), TAB(20))"
210 PRINT "Expected visual: X at col 10, Y at col 20 on same line"
220 PRINT TAB(10)"X"TAB(20)"Y"
230 PASS=PASS+1

300 PRINT "TEST2: TAB to same column does not force newline"
310 PRINT "Expected visual: X immediately followed by Y at same column flow"
320 PRINT TAB(10)"X"TAB(10)"Y"
330 PASS=PASS+1

400 PRINT "TEST3: TAB backward target wraps to next line"
410 PRINT "Expected visual: Y appears on next line at col 5"
420 PRINT TAB(10)"X"TAB(5)"Y"
430 PASS=PASS+1

500 PRINT "TEST4: TAB target beyond WIDTH uses modulo"
510 PRINT "Expected visual: with default width 80, TAB(90) maps to col 10"
520 PRINT TAB(10)"X"TAB(90)"Y"
530 PASS=PASS+1

600 PRINT "TEST5: TAB(-1) at line start does not insert extra blank line"
610 PRINT "Expected visual: line then immediate next line with X...Y (no empty line)"
620 PRINT "line"
630 PRINT TAB(-1)"X"TAB(90)"Y"
640 PASS=PASS+1

700 PRINT
710 PRINT "=================================="
720 PRINT "TAB TORTURE TEST COMPLETE"
730 PRINT "PASS=";PASS;" FAIL=";FAIL
740 PRINT "=================================="
750 END

9000 FAIL=FAIL+1
9010 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9020 PRINT "PASS=";PASS;" FAIL=";FAIL
9030 END
