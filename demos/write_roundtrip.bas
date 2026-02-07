10 CLS
20 PRINT "WRITE / INPUT round-trip demo"
30 PRINT
40 A$ = "HELLO"
50 B$ = "HE said ""WOW"""   ' string contains embedded quotes
60 WRITE A$, 12, 3.25, B$
70 PRINT
80 OPEN "tmp_write.csv" FOR OUTPUT AS #1
90 WRITE #1, A$, 12, 3.25, B$
100 CLOSE #1
110 OPEN "tmp_write.csv" FOR INPUT AS #1
120 INPUT #1, A2$, N1, N2, B2$
130 CLOSE #1
140 PRINT "A2$=["; A2$; "]"
150 PRINT "N1=["; N1; "]"
160 PRINT "N2=["; N2; "]"
170 PRINT "B2$=["; B2$; "]"
180 PRINT
190 PRINT "Done."
