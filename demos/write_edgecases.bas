10 REM WRITE/INPUT EDGE CASE COVERAGE
20 REM Verifies: empty string, commas inside strings, leading/trailing spaces, numeric formats.
30 ON ERROR GOTO 9000
40 F$="write_edgecases.tmp"
50 OPEN F$ FOR OUTPUT AS #1
60 WRITE #1, "", "A,B", "  spaced  ", -1, 1E-3, 1.2E+2, .5
70 CLOSE #1
80 OPEN F$ FOR INPUT AS #1
90 INPUT #1, A$, B$, C$, N1, N2, N3, N4
100 CLOSE #1
110 PRINT "A$=[";A$;"]"
120 PRINT "B$=[";B$;"]"
130 PRINT "C$=[";C$;"]"
140 PRINT "N1=[";N1;"]"
150 PRINT "N2=[";N2;"]"
160 PRINT "N3=[";N3;"]"
170 PRINT "N4=[";N4;"]"
180 REM Assertions
190 IF A$<>"" THEN PRINT "FAIL: empty string mismatch": END
200 IF B$<>"A,B" THEN PRINT "FAIL: comma-in-string mismatch": END
210 IF C$<>"  spaced  " THEN PRINT "FAIL: leading/trailing spaces mismatch": END
220 IF N1<>-1 THEN PRINT "FAIL: -1 mismatch": END
230 REM Numeric comparisons use a small tolerance because floating-point values may not round-trip exactly.
240 IF ABS(N2-0.001) > 1E-9 THEN PRINT "FAIL: 1E-3 mismatch": END
250 IF ABS(N3-120) > 1E-9 THEN PRINT "FAIL: 1.2E+2 mismatch": END
260 IF ABS(N4-0.5) > 1E-9 THEN PRINT "FAIL: .5 mismatch": END
270 PRINT "PASS: WRITE/INPUT edge cases OK"
280 END
9000 PRINT "ERROR: ";ERR;" AT ";ERL
9010 END
