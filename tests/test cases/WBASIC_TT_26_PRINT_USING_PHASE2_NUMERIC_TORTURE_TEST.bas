10 REM ============================================================
20 REM PRINT USING PHASE 2 NUMERIC TORTURE TEST
30 REM Covers , + - $ * ^^^^ numeric mask support and interactions.
40 REM ============================================================
50 DEFINT A-Z
60 PASS=0:FAIL=0
70 ON ERROR GOTO 9000

100 PRINT "Running PRINT USING phase 2 numeric torture test..."
110 PRINT

200 PRINT "TEST1: grouping mask #,###.##"
210 PRINT USING "#,###.##"; 1234.56
220 PRINT USING "#,###.##"; 12.3
230 PASS=PASS+1

300 PRINT "TEST2: leading plus +###.##"
310 PRINT USING "+###.##"; 12.34
320 PRINT USING "+###.##"; -12.34
330 PASS=PASS+1

400 PRINT "TEST3: trailing plus ###.##+"
410 PRINT USING "###.##+"; 12.34
420 PRINT USING "###.##+"; -12.34
430 PASS=PASS+1

500 PRINT "TEST4: trailing minus ###.##-"
510 PRINT USING "###.##-"; 12.34
520 PRINT USING "###.##-"; -12.34
530 PASS=PASS+1

600 PRINT "TEST5: currency $$###.##"
610 PRINT USING "$$###.##"; 12.34
620 PRINT USING "$$###.##"; -12.34
630 PASS=PASS+1

700 PRINT "TEST6: star fill **###.##"
710 PRINT USING "**###.##"; 12.34
720 PRINT USING "**###.##"; -12.34
730 PASS=PASS+1

800 PRINT "TEST7: scientific ##.##^^^^"
810 PRINT USING "##.##^^^^"; 12345.6
820 PRINT USING "##.##^^^^"; -0.01234
830 PASS=PASS+1

900 PRINT "TEST8: mixed symbols +$*#,###.##"
910 PRINT USING "+$*#,###.##"; 1234.56
920 PRINT USING "+$*#,###.##"; -1234.56
930 PASS=PASS+1

1000 PRINT
1010 PRINT "=================================="
1020 PRINT "PRINT USING PHASE 2 NUMERIC TEST COMPLETE"
1030 PRINT "PASS=";PASS;" FAIL=";FAIL
1040 PRINT "=================================="
1050 END

9000 FAIL=FAIL+1
9010 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9020 PRINT "PASS=";PASS;" FAIL=";FAIL
9030 END
