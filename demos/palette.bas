10 SCREEN 1
20 COLOR 0, 1 : REM Black background, Palette 1
30 PALETTE 3, 12: REM Change text attribute (3) to Bright Red (12)
40 PRINT "This text is now Bright Red!"
45 if inkey$="" then 45
50 PALETTE 3, 9 : REM Instantly changes all text to Bright Blue (9)
60 PRINT "This text is now Bright Blue!"