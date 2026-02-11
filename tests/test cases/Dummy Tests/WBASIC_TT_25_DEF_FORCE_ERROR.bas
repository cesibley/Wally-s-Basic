10 REM ======================================================
20 REM WBASIC TORTURE TEST – FORCED FAILURE (INTENTIONAL)
30 REM This test is designed to ALWAYS FAIL.
40 REM Used to validate test harness failure handling.
50 REM ======================================================

100 PRINT "FORCED FAILURE TEST: triggering intentional error"
110 PRINT "This is expected behavior."

200 REM Trigger a guaranteed runtime error
210 X = 1 / 0

999 END
