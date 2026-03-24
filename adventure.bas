10 REM ******************************************
11 REM *****         ADVENTURE XT          ******
12 REM *                                        *
13 REM * By Paul Allen Panks - 2007             *
14 REM *                                        *
15 REM * Ported to GW-Basic by D. RIOUAL - 2023 *
16 REM *                                        *
17 REM ******************************************
100 REM
110 CLEAR:COLOR 12,0:CLS:KEY OFF
120 HS$="Handy says,'Thank you for bringing Azrael to me! Here are several tools for you. I'll leave them here.'"
130 HS2$="Handy places some tools on the floor."
140 MS$="The beaker breaks. Azrael licks it up and suddenly falls to the floor! Checking him, he is fast asleep. You pick him up, carrying Azrael on your back. Back to Handy Smurf's!"
150 G=1000:IC=0:HP=192:RM=1:FB$=">>>>>>>>>>>>>>>>>>>> F I R E - B A L L ! ! !"
160 FB2$="The monster is burnt to a crisp!"
170 DIM NO$(60),LO(60),EX$(60),VB$(25),M(100,6),DE$(100),P(60),WD$(20)
180 FOR X=1 TO 56:READ NO$(X):NEXT
190 FOR X=1 TO 56:READ LO(X):NEXT
200 FOR X=1 TO 56:READ EX$(X):NEXT
210 FOR X=1 TO 19:READ VB$(X):NEXT
220 FOR X=1 TO 84:FOR Y=1 TO 6:READ M(X,Y):NEXT:NEXT
230 FOR X=1 TO 84:READ DE$(X):NEXT
240 FOR X=1 TO 56:READ P(X):NEXT
450 CLS:PRINT"@@@ Adventure XT @@@":PRINT"Written for the 13th annual interactive fiction contest":PRINT"By: Paul Panks (dunric@yahoo.com)":PRINT
460 PRINT"It has been four years since you last ventured into Blarg,"
470 PRINT"the land of might and magic. Mordimar, an evil wizard, found"
480 PRINT"the powerful Orb of Destiny. With it, he became nearly"
490 PRINT"invincible. As his power grew, the chi (life force)"
500 PRINT"from the surrounding forest was drained steadily, until"
510 PRINT"there was almost nothing left.":PRINT
520 PRINT"Determined to stop Mordimar, you set out on a quest"
530 PRINT"to re-acquire the orb and bring peace back to the"
540 PRINT"forest.":PRINT:PRINT"Your Quest Begins":PRINT
550 PRINT"@@@ Press any key to continue @@@"
560 A$=INKEY$:IF A$="" THEN 560
570 CLS
580 PRINT"Are you playing a saved game (y/n)?";
590 A$=INKEY$:IF A$="" THEN 590
600 IF A$="y" OR A$="Y" THEN PRINT A$:PRINT:GOTO 4330
610 IF A$="n" OR A$="N" THEN PRINT A$:PRINT:GOTO 1000
620 GOTO 590
999 REM Verb routines (generic)
1000 NC=0:PRINT DE$(RM):IF LT=0 THEN IF RM>18 THEN PRINT"It is too dark to see much of anything!":GOTO 1030
1001 IF RM < 21 THEN 1010
1002 IF (RM >= 21) AND (RM < 41) THEN 1012
1003 IF (RM >= 41) AND (RM < 61) THEN 1014
1004 IF (RM >= 61) AND (RM < 81) THEN 1016
1005 IF RM >= 81 THEN 1018
1010 ON RM GOSUB 10000,10040,10080,10120,10160,10210,10250,10290,10330,10360,10400,10440,10480,10520,10560,10600,10640,10670,10700,10740
1011 GOTO 1030
1012 ON (RM-20) GOSUB 10800,10830,10870,10920,10980,11020,11060,11100,11140,11180,11240,11270,11310,11350,11400,11450,11490,11540,11590,11640
1013 GOTO 1030
1014 ON (RM-40) GOSUB 11680,11710,11730,11750,11800,11840,11890,11940,11990,12030,12070,12130,12180,12220,12250,12300,12340,12380,12430,12470
1015 GOTO 1030
1016 ON (RM-60) GOSUB 12530,12570,12610,12650,12710,12760,12810,12860,12910,12940,12990,13040,13100,13180,13220,13280,13350,13420,13500,13570
1017 GOTO 1030
1018 ON (RM-80) GOSUB 13680,13730,13800,13870
1030 GOSUB 4440
1040 FOR X=7 TO 56:IF LO(X)=RM THEN PRINT NO$(X);"."
1050 NEXT
1100 IF LO(56)=0 AND RM=80 THEN LO(56)=-5:PRINT"You bring Azrael to Handy.":PRINT HS$:PRINT HS2$:LO(9)=RM:LO(22)=RM:LO(31)=RM:LO(32)=RM
1105 CT=CT+1:V=0:N=0:NE$="":N$="":N2$="":V$="":V2$="":PR=0:PT=0:NM=0:BZ=0:FOR X=1 TO 10:WD$(X)="":NEXT X
1110 INPUT ">",A$
1111 REM Convert to lower case 
1112 FOR X=1 TO LEN(A$):C=ASC(MID$(A$,X,1))
1113 IF C>=65 AND C <=90  THEN C=C+32 : MID$(A$,X,1)= CHR$(C) 
1114 NEXT X
1118 GOSUB 4010
1120 PT=1:NM=0:D$=A$:FOR A=1 TO LEN(D$):IF MID$(D$, A, 1)=" " THEN A$=MID$(D$,PT,A-PT): PT=A+1:NM=NM+1:WD$(NM)=A$
1130 NEXT A:NM=NM+1:A$=MID$(D$,PT,A-PT):WD$(NM)=A$
1140 V$=WD$(1):N$=WD$(2):IF WD$(3)="and" OR WD$(3)="then" THEN V2$=WD$(4):N2$=WD$(5):CO=1
1150 IF WD$(3)="in" OR WD$(3)="from" OR WD$(3)="to" THEN V$=WD$(1):NE$=WD$(2):NE2$=WD$(4):PR=1:BZ=1
1160 V=0:FOR X=1 TO 19:IF V$=VB$(X) THEN V=X
1170 NEXT:IF V=0 THEN PRINT"What? Check your verb.":GOTO 1100
1180 N=0:FOR X=1 TO 56:IF NE$=NO$(X) OR N$=NO$(X) THEN N=X
1190 NEXT:IF N=0 THEN IF V<>10 AND V<>12 THEN PRINT"Huh? Check your noun.":GOTO 1100
1200 ON V GOTO 2000,2080,2150,2200,2300,2330,2390,2440,2480,2520,2570,2660,2720,2780,2870,2950,3410,3500,3550
1205 IF NC=0 THEN NC=1:GOTO 1030
1210 GOTO 1100
1999 REM ***** GO *****
2000 :
2010 IF RM=22 OR RM=81 THEN IF N=5 THEN IF LO(9)<>0 AND LO(9)<>RM THEN PRINT"You need a rope to climb up!":GOTO 1100
2020 IF RM=65 THEN IF N=2 THEN IF LO(38)=RM THEN PRINT"The goblin blocks your path! He growls,'You cannot pass!'":GOTO 1100
2030 IF RM=12 THEN IF N=2 THEN IF LO(39)=RM THEN PRINT"The hellhound slams you around! He screams,'DIE, KNAVE!'":TEMPO=1:GOSUB 30000:PRINT"You died.":TEMPO=2:GOSUB 30000:PRINT:GOTO 3820
2040 IF RM=12 THEN IF N=2 OR N=3 THEN IF LT=0 THEN PRINT"It is much too dark to move in that direction!":PRINT"(You need a source of light)":GOTO 1100
2050 IF N>6 OR N=0 OR M(RM,N)=0 THEN PRINT"You can't go that way.":GOTO 1100
2060 RM=M(RM,N):GOTO 1000
2070 REM ***** GET *****
2080 :
2090 IF N<7 OR N>35 THEN PRINT"You can't pick that up.":GOTO 1100
2100 IF LO(N)<>0 AND LO(N)<>305 AND LO(N)<>405 AND LO(N)<>RM THEN PRINT"It's beyond your power to do that!":GOTO 1100
2110 IF IC>8 THEN PRINT"You are carrying too much already!":GOTO 1100
2120 IF N=20 AND LO(56)=RM THEN LO(20)=99:LO(56)=0:EX$(56)="azrael: a mischevious cat. He is asleep.":IC=IC+1:PRINT MS$:GOTO 1100
2130 IC=IC+1:LO(N)=0:PRINT"Ok.":GOTO 1100
2140 REM ***** DROP *****
2150 :
2160 IF N<7 THEN PRINT"You can't drop that.":GOTO 1100
2170 IF LO(N)<>0 AND LO(N)<>105 AND LO(N)<>205 THEN PRINT"You don't have it.":GOTO 1100
2180 IC=IC-1:LO(N)=RM:PRINT"Ok.":GOTO 1100
2190 REM ***** INVENTORY *****
2200 :
2210 PRINT"You are carrying:"
2220 SI=0:WD=0:AC=0:FOR X=7 TO 56:IF LO(X)=0 THEN SI=1:PRINT "   ";NO$(X);"."
2230 IF LO(X)=105 THEN WD=X:SI=1:PRINT "   ";NO$(X);" (wielded)."
2240 IF LO(X)=205 THEN SI=1:AC=AC+(X/4):AC=CINT(AC):PRINT "   ";NO$(X);" (worn)."
2250 IF X=10 OR X=11 THEN IF LO(X)=0 THEN GOSUB 3320
2260 IF X=8 AND LT=0 AND LO(8)=0 THEN PRINT"(the lantern is off)":ELSE IF X=8 AND LO(8)=0 THEN PRINT"(the lantern is aflame)"
2270 NEXT:IF SI=0 THEN PRINT"Alas, you are empty-handed."
2280 PRINT"(You have";HP;"hit points and";G;"gold coins).":GOTO 1100
2290 REM ***** LOOK *****
2300 :
2310 GOTO 1000
2320 REM ***** WIELD *****
2330 :
2340 IF N<7 OR N>35 OR N<29 THEN PRINT"You can't wield that.":GOTO 1100
2350 IF LO(N)<>0 THEN PRINT"It's beyond your power to do that!":GOTO 1100
2360 IF WD>0 THEN PRINT"You are already wielding something (";NO$(WD);").":GOTO 1100
2370 WD=N:LO(N)=105:PRINT "Ok.":GOTO 1100
2380 REM ***** UNWIELD *****
2390 :
2400 IF N<7 OR N>35 OR N<29 THEN PRINT"You can't unwield that.":GOTO 1100
2410 IF LO(N)<>105 THEN PRINT"It's beyond your power to do that!":GOTO 1100
2420 WD=0:LO(N)=0:PRINT "Ok.":GOTO 1100
2430 REM ***** WEAR *****
2440 :
2450 IF N<7 OR N>35 OR LO(N)<>0 OR N<21 AND N>28 THEN PRINT"You can't wear that.":GOTO 1100
2460 LO(N)=205:AC=AC+(N/4):AC=CINT(AC):PRINT"Ok.":GOTO 1100
2470 REM ***** REMOVE *****
2480 :
2490 IF N<7 OR N>35 OR LO(N)<>0 OR N<21 AND N>28 THEN PRINT"You can't remove that.":GOTO 1100
2500 LO(N)=0:AC=AC-(N/4):AC=CINT(AC):PRINT"Ok.":GOTO 1100
2510 REM ***** EXAMINE *****
2520 :
2530 IF N=0 THEN PRINT"You notice nothing unusual about it.":GOTO 1100
2540 IF LO(N)<>0 AND LO(N)<>RM AND LO(N)<>105 AND LO(N)<>205 THEN PRINT"That isn't here.":GOTO 1100
2550 PRINT EX$(N):GOTO 1100
2560 REM ***** USE *****
2570 :
2580 IF LO(N)<>0 AND LO(N)<>RM OR N<7 OR N>35 THEN PRINT"You can't use that!":GOTO 1100
2590 IF N=7 THEN IF LO(8)<>0 AND LO(8)<>RM THEN PRINT"You need the lantern.":GOTO 1100
2600 IF N=7 AND LT=1 THEN PRINT"The lantern is already on.":GOTO 1100
2610 IF N=8 THEN IF LO(7)<>0 AND LO(7)<>RM THEN PRINT"You need the flask of oil.":GOTO 1100
2620 IF N=8 AND LT=1 THEN PRINT"The lantern is already on.":GOTO 1100
2630 IF N=8 THEN LT=1:PRINT"The lantern is now aflame.":GOTO 1100
2640 GOTO 3860
2650 REM ***** CLIMB *****
2660 :
2670 IF RM<>22 AND RM<>81 OR N>0 THEN PRINT"You can't climb that.":GOTO 1100
2680 IF RM=22 THEN IF LO(9)=0 OR LO(9)=RM THEN RM=23:PRINT"You climb up...":GOTO 1000
2690 IF RM=81 THEN IF LO(9)=0 OR LO(9)=RM THEN RM=82:PRINT"You climb up...":GOTO 1000
2700 PRINT"You need the rope first.":GOTO 1100
2710 REM ***** READ *****
2720 :
2730 IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"You can't read that!":GOTO 1100
2740 IF N=17 THEN PRINT"The book reads (in part):":GOTO 3950
2750 IF N=19 THEN PRINT"It is written in an unfamiliar tongue.":GOTO 1100
2760 PRINT"You read it with little interest.":GOTO 1100
2770 REM ***** BUY *****
2780 :
2790 IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO 1100
2800 IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO 1100
2810 IF LO(N)<>98 THEN PRINT"The clerk says,'We don't carry that.'":GOTO 1100
2820 IF IC>=8 THEN PRINT"The clerk says,'You can't carry that.'":GOTO 1100
2830 IF P(N)=99 THEN PRINT"The clerk says,'You can't buy that.'":GOTO 1100
2840 IF P(N)>G THEN PRINT"The clerk says,'You don't have enough gold.'":GOTO 1100
2850 G=G-P(N):LO(N)=0:IC=IC+1:PRINT"You hand clerk";P(N);"gold coins.":PRINT"He hands you the ";NO$(N);".":PRINT"He says,'Thank you for your business.'":GOTO 1100
2860 REM ***** SELL *****
2870 :
2880 IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO 1100
2890 IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO 1100
2900 IF LO(N)=105 THEN PRINT"The clerk says,'You must unwield that first.'":GOTO 1100
2910 IF LO(N)=205 THEN PRINT"The clerk says,'You must remove that first.'":GOTO 1100
2920 IF LO(N)<>0 THEN PRINT"The clerk says,'I don't see you carrying that.'":GOTO 1100
2930 LO(N)=98:CG=P(N)/2:G=G+CG:IC=IC-1:PRINT"You hand clerk the ";NO$(N);".":PRINT"He hands you";CG;"gold coins.":PRINT"He says,'Thank you for your business.'":GOTO 1100
2940 REM ***** FIGHT *****
2950 :
2960 IF N<36 THEN PRINT"You can't fight that!":GOTO 1100
2965 IF RM>72 AND RM<84 THEN PRINT"You can't fight here. This is a sacred place.":GOTO 1100
2970 IF LO(N)<>RM THEN PRINT"The ";NO$(N);" isn't here.":GOTO 1100:ELSE RANDOMIZE TIMER:MH=INT(RND*150)+50:IF N=50 THEN MH=820
2980 RANDOMIZE TIMER:I=INT(RND*35)+1:PRINT"You are fighting the "NO$(N);".":PRINT">"
2990 DM=1:FOR X=7 TO 35:IF LO(X)=105 THEN DM=X
3000 NEXT:IF DM=1 THEN PRINT"You are wielding nothing!":ELSE PRINT"You are wielding the ";NO$(DM);"."
3010 PRINT">"
3020 IF I<=5 THEN PRINT"You missed."
3030 IF I>=5 AND I<=10 THEN PRINT"You hit ";NO$(N);".":MH=MH-10:ELSE IF I>=15 AND I<=20 OR I>=20 AND I<=25 THEN IF DM=31 OR DM>32 AND DM<36 THEN GOSUB 3250
3040 IF I>=10 AND I<=15 THEN PRINT"You hit ";NO$(N);" very hard.":MH=MH-15:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-10
3050 IF I>=15 AND I<=20 THEN PRINT"You smashed ";NO$(N);" with a bone-crushing sound.":MH=MH-30:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-15
3060 IF I>=20 AND I<=25 THEN PRINT"You massacred ";NO$(N);" into small fragments.":MH=MH-40:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-DM
3070 IF I=26 THEN IF DM=31 THEN PRINT"A bolt of lightning streaks down from above!":PRINT"Your BROADSWORD strikes ";NO$(N);" down!":MH=0
3080 IF I=27 THEN IF DM=33 THEN PRINT"Your GLOWBALL strikes ";NO$(N);" very hard!":MH=MH-55
3090 IF I=28 THEN IF DM=34 THEN PRINT"Your SCEPTER shoots flame at ";NO$(N);"!":MH=MH-65
3100 IF I=29 THEN IF DM=35 THEN PRINT"Your SLAYER leaps from your hands! It impales ";NO$(N);"!":MH=MH-75
3110 IF I>=30 THEN PRINT"Your attack was blocked by ";NO$(N);".":ELSE IF LO(19)=0 THEN IF I>=30 THEN PRINT"You cast HEAL...":PRINT"You are healed fully!":HP=192
3120 TEMPO=1:GOSUB 30000:PRINT">":PRINT"The monster ";NO$(N);" is attacking.":TEMPO=1:GOSUB 30000:PRINT">":IF I=32 AND LO(19)=0 THEN PRINT"You cast FIRE-BALL...":PRINT FB$:PRINT FB2$:MH=0
3130 RANDOMIZE TIMER:C=INT(RND*35)+1:IF C<=5 THEN PRINT"It missed you."
3140 IF C>=5 AND C<=10 THEN PRINT"It hit you.":DT=7:HP=HP-DT:IF AC>DT THEN HP=HP+2
3150 IF C>=10 AND C<=15 THEN PRINT"It hit you very hard.":DT=12:HP=HP-DT:IF AC>DT THEN HP=HP+4
3160 IF C>=15 AND C<=20 THEN PRINT"It smashed you with a bone-crushing sound.":DT=20:HP=HP-DT:IF AC>DT THEN HP=HP+8
3170 IF C>=20 AND C<=25 THEN PRINT"It massacred you into small fragments.":DT=40:HP=HP-DT:IF AC>DT THEN HP=HP+15
3180 IF I=26 THEN IF N=50 THEN PRINT"A bolt of lightning streaks down from the heavens!":PRINT"You are fried to death!":TEMPO=1:GOSUB 30000:PRINT"You died.":TEMPO=1:GOSUB 30000:GOTO 3820
3190 PRINT">":TEMPO=1:GOSUB 30000:IF MH<=0 THEN PRINT"The monster died.":PRINT"You killed ";NO$(N);".":TEMPO=1:GOSUB 30000:PRINT">":GOTO 3220
3200 PRINT"Your HP:";HP:PRINT"Their HP:";MH:PRINT">":IF HP<=0 THEN PRINT"You died.":PRINT:TEMPO=1:GOSUB 30000:GOTO 3820
3210 GOTO 2980
3220 FOR X=7 TO 35:IF LO(X)=1000+N THEN PRINT"You found ";NO$(X);" on it!":LO(X)=RM
3230 NEXT:PRINT"You gained";DT;"gold pieces and";N;"hit points.":HP=HP+N:G=G+DT:LO(N)=998:GOSUB 4850:GOTO 1100
3240 END
3250 IF DM=31 THEN PRINT"Your BROADSWORD glows!"
3260 IF DM=33 THEN PRINT"Your GLOWBALL splits into eight pieces!"
3270 IF DM=34 THEN PRINT"Your SCEPTER shoots lightning at ";NO$(N);"!"
3280 IF DM=35 THEN PRINT"Your SLAYER vibrates!"
3290 RETURN
3300 END
3310 REM ***** Check for items in knapsack/backpack *****
3320 IF X=11 THEN GOTO 3370
3330 PRINT"      The knapsack holds:"
3340 S1=0:FOR Y=7 TO 35:IF LO(Y)=305 THEN S1=1:PRINT"        (";NO$(Y);")."
3350 NEXT:IF S1=0 THEN PRINT"        (Nothing)"
3360 RETURN
3370 S1=0:PRINT"      The backpack holds:":FOR Y=7 TO 35:IF LO(Y)=405 THEN S1=1:PRINT"        (";NO$(Y);")."
3380 NEXT:IF S1=0 THEN PRINT"        (Nothing)"
3390 RETURN
3400 REM ***** Put *****
3410 :
3420 IF LO(N)<>0 THEN PRINT"You must be carrying that first!":GOTO 1100
3430 IF LO(10)<>0 AND LO(11)<>0 AND LO(10)<>RM AND LO(11)<>RM THEN PRINT"You aren't carrying the knapsack or backpack!":GOTO 1100
3440 TL=0:IF NE2$="knapsack" THEN TL=10
3450 IF NE2$="backpack" THEN TL=11
3460 IF TL=0 THEN PRINT"You can't place it there.":GOTO 1100
3470 IF TL=10 THEN LO(N)=305:IC=IC-1:PRINT"Ok.":GOTO 1100
3480 LO(N)=405:IC=IC-1:PRINT"Ok.":GOTO 1100
3490 REM ***** Eat *****
3500 :
3510 IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"That isn't here.":GOTO 1100
3520 IF N=13 THEN HP=192:PRINT"You eat the food.":PRINT"You feel much better!":LO(13)=1:IC=IC-1:GOTO 1100
3530 PRINT"You can't eat that.":GOTO 1100
3540 REM ***** Drink *****
3550 :
3560 IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"That isn't here.":GOTO 1100
3570 IF N=14 THEN IF LO(16)<>0 AND LO(16)<>RM THEN PRINT"You need the glass bottle first.":GOTO 1100
3580 IF N=14 THEN LO(14)=5:IC=IC-1:CT=0:PRINT"Ahhhh! Refreshing!":GOTO 1100
3590 IF N=15 THEN LO(15)=1:LO(16)=0:PRINT"You drink the wine.":PRINT"It tastes great!":GOTO 1100
3600 PRINT"You can't drink that.":GOTO 1100
3610 REM ***** check for thirst *****
3620 :
3630 IF CT=50 THEN PRINT"You are thirsty."
3640 IF CT=100 THEN PRINT"You are very thirsty."
3650 IF CT=150 THEN PRINT"You have died of thirst.":TEMPO=1:GOSUB 30000:PRINT"You died.":TEMPO=2:GOSUB 30000:GOTO 3820
3660 RETURN
3670 PRINT"This is a text adventure. You play by entering in one or two"
3680 PRINT"word commands (e.g. go north, get food, etc.). Valid commands"
3690 PRINT"include:":PRINT
3700 PRINT"1. go 2. get 3. drop 4. inventory 5. look 6. wield 7. unwield"
3710 PRINT"8. wear 9. remove 10. examine 11. use 12. climb 13. read"
3720 PRINT"14. buy 15. sell 16. kill 17. put 18. eat 19. drink"
3730 PRINT"20. inventory 21. save game (or just 'save') 22. look"
3740 PRINT"23. quit 24. help":PRINT:GOTO 1100
3750 REM ***** list command *****
3760 IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO 1100
3770 IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO 1100
3780 PRINT"The clerk says,'Here is what we have in stock:"
3790 SI=0:FOR X=7 TO 35:IF LO(X)=98 THEN SI=1:PRINT P(X);": ";NO$(X)
3800 NEXT:IF SI=0 THEN PRINT"He scratches his head and says,'Alas, we have nothing in stock...'"
3810 PRINT"Your gold: ";G:GOTO 1100
3820 PRINT "@@@ Press any key @@@"
3830 A$=INKEY$:IF A$="" THEN GOTO 3830
3840 CLS:RUN
3850 REM ***** Use (continued) *****
3860 IF N=9 THEN IF RM=22 THEN RM=23:PRINT"You climb up...":GOTO 1000
3870 IF N=9 THEN IF RM=81 THEN RM=82:PRINT"You climb up...":GOTO 1000
3880 IF N=12 THEN IF RM=22 THEN RM=23:PRINT"You climb up (on the pole)...":GOTO 1000
3890 IF N=12 THEN IF RM=81 THEN RM=82:PRINT"You climb up (on the pole)...":GOTO 1000
3900 IF N=13 THEN PRINT"You must use 'eat' instead.":GOTO 1100
3910 IF N=14 OR N=15 OR N=16 THEN PRINT"You must use 'drink' instead.":GOTO 1100
3920 IF N=17 THEN PRINT"You must use 'read' instead.":GOTO 1100
3930 IF N=19 THEN PRINT"You muse use 'read' instead.":GOTO 1100
3940 PRINT"You can't use that here.":GOTO 1100
3950 :
3960 PRINT"'...to defeat mordimar, you must be wielding the broadsword. It is the only"
3970 PRINT"weapon which can work against his black magic. The others will not damage"
3980 PRINT"him enough. I have yet to acquire it, but someday I shall finally defeat"
3990 PRINT"him! - Tursk'":GOTO 1100
4000 REM ***** Check for other verbs *****
4010 GOSUB 3620:IF A$="help" OR A$="hint" THEN GOSUB 3670:ELSE IF A$="list" THEN GOTO 3760
4020 IF A$="go north" OR A$="n" OR A$="north" THEN V=1:N=1:CO=0:GOTO 1200
4030 IF A$="go south" OR A$="s" OR A$="south" THEN V=1:N=2:CO=0:GOTO 1200
4040 IF A$="go east" OR A$="e" OR A$="east" THEN V=1:N=3:CO=0:GOTO 1200
4050 IF A$="go west" OR A$="w" OR A$="west" THEN V=1:N=4:CO=0:GOTO 1200
4060 IF A$="go up" OR A$="u" OR A$="up" THEN V=1:N=5:CO=0:GOTO 1200
4070 IF A$="go down" OR A$="d" OR A$="down" THEN V=1:N=6:CO=0:GOTO 1200
4080 IF A$="inventory" OR A$="i" OR A$="inv" THEN V=4:N=56:CO=0:GOTO 1200
4090 IF A$="save" OR A$="save game" THEN GOTO 4380
4100 IF A$="quit" THEN PRINT:PRINT"Quit Game":PRINT:GOTO 4140
4110 IF A$="look" OR A$="l" THEN V=5:N=56:CO=0:GOTO 1200
4115 IF A$="map" THEN GOSUB 4520:GOTO 1200
4120 RETURN
4130 REM ***** QUIT GAME *****
4140 PRINT"Are you sure (y/n)? ";
4150 B$=INKEY$:IF B$="" THEN GOTO 4150
4160 IF B$="y" THEN PRINT B$:PRINT:PRINT"Ok...thanks for playing!":TEMPO=2:GOSUB 30000:END
4170 IF B$="n" THEN PRINT B$:PRINT:GOTO 1100
4180 GOTO 4150
4320 REM ***** LOAD A GAME *****
4330 PRINT:PRINT"Loading...";:TEMPO=1:GOSUB 30000
4335 ON ERROR GOTO 40000
4340 OPEN "ADVXT.SAV" FOR INPUT AS #1
4350 INPUT#1,G: INPUT#1,IC: INPUT#1,HP: INPUT#1,RM: INPUT#1,AC: INPUT#1,LT: INPUT#1,WD: INPUT #1,DM
4360 FOR X=7 TO 56:INPUT#1,LO(X):NEXT:CLOSE #1:PRINT"Done.":TEMPO=1:GOSUB 30000
4370 GOTO 1000
4380 PRINT"Saving...";:TEMPO=1:GOSUB 30000
4390 OPEN "ADVXT.SAV" FOR OUTPUT AS #1
4400 PRINT#1,G: PRINT#1,IC: PRINT#1,HP: PRINT#1,RM: PRINT#1,AC: PRINT#1,LT: PRINT#1,WD: PRINT #1,DM
4410 FOR X=7 TO 56:PRINT#1,LO(X):NEXT:CLOSE #1:PRINT"Done.":TEMPO=1:GOSUB 30000
4420 GOTO 1100
4430 END
4440 PRINT"Obvious exits: < /";
4450 IF M(RM,1)>0 THEN PRINT"north/ ";
4460 IF M(RM,2)>0 THEN PRINT"south/ ";
4470 IF M(RM,3)>0 THEN PRINT"east/ ";
4480 IF M(RM,4)>0 THEN PRINT"west/ ";
4490 IF M(RM,5)>0 THEN PRINT"up/ ";
4500 IF M(RM,6)>0 THEN PRINT"down/ ";
4510 PRINT">":RETURN
4520 CLS:PRINT"Map of Adventure XT                    Hall2--Room4                            "
4530 PRINT"                                      /                        23/branch--hut24"
4540 PRINT"           8Toolshed--7Garden   Tavern1      26RWF---RWF25  22RWF              "
4550 PRINT"                         |        |                   |        |               "
4560 PRINT"                      6Church--Fountain5--Shop3    19RWF----20RWF--RWF21       "
4570 PRINT"                                  |                            |     47        "
4580 PRINT"                     27BF      Village Ent9                Clearing18 T-DE48   "
4590 PRINT"                  28 29 |  30     |                   16    17 |   43 |44 45 46"
4600 PRINT"                   BF--BF--BF     |  Pond15 River---River DE-T-T-Keep          "
4610 PRINT"                    |      |      |10   13  14 |      |       39  40  |41      "
4620 PRINT"             31BF--BF32    BF11--On--Meadow--Mead.  River  Tunnel--T--T-DE42   "
4630 PRINT"                           |     |     34  35  36     | 37   |                 "
4640 PRINT"                         33BF--Inter12-DF--DF--DF---River--Cave38              "
4650 PRINT"                                 |         |   |51                             "
4660 PRINT"                               Bridge49  50DF--DF---Alcove52  53Kitchen  RM54  "
4670 PRINT"                                 |             |    57  58  59  60 |    61|62  "
4680 PRINT"                               Bridge55      56DF---LF--LF--LF--Cottage-H-H-R63"
4690 PRINT"                                 |                   |   |67 68                "
4700 PRINT"      Mordimar64               Goblin65    [inset]  LF66LF--LF--LF69           "
4710 PRINT"           |        71       72  |           |               |                 "
4720 PRINT"    70Throne RM--Fountain--Shadow Castle   SNW73         Small Path74          "
4730 PRINT"                                             |      76       |                 "
4740 PRINT"                                         75SB P--SB Patch--Path77              "
4745 A$=INKEY$:IF A$="" THEN 4745
4750 PRINT"--------------------------*INSET*----------------------                        "
4760 PRINT"|                Brainy82                             |                        "
4770 PRINT"|                   |  81          83           84    |                        "
4780 PRINT"|    79Jokey-----SM Forest--Outside G Castle--G Castle|                        "
4790 PRINT"|        |                                            |                        "
4800 PRINT"|    78Papa--Handy80                                  |                        "
4810 PRINT"|        |                                            |                        "
4820 PRINT"|    [to SNW]                                         |                        "
4830 PRINT"-------------------------------------------------------                        "
4840 GOTO 1100
4850 IF N=50 THEN GOTO 4860:ELSE RETURN
4860 PRINT:PRINT"@@@ You have defeated Mordimar!!! @@@":TEMPO=2:GOSUB 30000:PRINT
4870 PRINT"Mordimar's crumbled body lay at your feet. As you"
4880 PRINT"gaze into the Orb, you sense the world returning to"
4890 PRINT"normal! The woodland creatures send great happiness"
4900 PRINT"to your heart! It swells with joy!":PRINT
4910 PRINT"'You have done a great deed!' their voices say."
4920 PRINT"'Now we can come out and play!'":PRINT
4930 PRINT"The world thanks you for your courage...":PRINT
4940 PRINT"@@@ The End @@@":TEMPO=20:GOSUB 30000:PRINT:PRINT"@@@ Press any key @@@"
4950 A$=INKEY$:IF A$="" THEN GOTO 4950
4960 END
10000 REM ***** ROOM 1 *****
10010 PRINT"You are standing in a tavern with creaky wooden floors. A few tables"
10020 PRINT"line the room, with flickering candles set in the middle, illuminating"
10030 PRINT"the area. To the south (outside) stands a large fountain.":RETURN
10040 REM ***** ROOM 2 *****
10050 PRINT"You are standing on an upstairs hallway in the tavern. A lone room"
10060 PRINT"stands to the east, unoccupied. The only exit is back down the"
10070 PRINT"staircase.":RETURN
10080 REM ***** ROOM 3 *****
10090 PRINT"You are standing in the village shop. People come here to purchase"
10100 PRINT"needed goods and sell unwanted items. Commands here include 'buy <item>',"
10110 PRINT"'sell <item>' and 'list'. The shop is usually attended by a clerk.":RETURN
10120 REM ***** ROOM 4 *****
10130 PRINT"You are standing in a small room upstairs of the main tavern. A small"
10140 PRINT"bed is visible here, allowing you to rest comfortably. The only exit"
10150 PRINT"is back west.":RETURN
10160 REM ***** ROOM 5 *****
10170 PRINT"You are standing near a large fountain. Looking down at your reflection,"
10180 PRINT"you notice the water appears mossy green. However, despite the color"
10190 PRINT"it appears drinkable. To the east stands a small shop, while west of"
10200 PRINT"here lies a church.":RETURN
10210 REM ***** ROOM 6 *****
10220 PRINT"You are standing in a small church. Pews adorn the room, with an altar"
10230 PRINT"at the front. To the north (outside) a garden is visible. The only"
10240 PRINT"other exit is back east.":RETURN
10250 REM ***** ROOM 7 *****
10260 PRINT"You are standing in a lovely garden. Several different flowers dot"
10270 PRINT"the area, including lillies and begonias. To the west stands a"
10280 PRINT"wooden toolshed.":RETURN
10290 REM ***** ROOM 8 *****
10300 PRINT"You are standing inside a darkened toolshed. A few items of interest"
10310 PRINT"line the walls, but not much else is here. The only exit is back"
10320 PRINT"east.":RETURN
10330 REM ***** ROOM 9 *****
10340 PRINT"You are standing at the entrance to a small village. A path heads south"
10350 PRINT"from here into a well-lit forest.":RETURN
10360 REM ***** ROOM 10 *****
10370 PRINT"You are on a well-lit path walking through a forest. The area is"
10380 PRINT"populated by many small creatures (scurrying about). The path"
10390 PRINT"continues south.":RETURN
10400 REM ***** ROOM 11 *****
10410 PRINT"You are on a well-lit path walking through a forest. A lush green meadow"
10420 PRINT"stands to the east (near a small pond). An intersection is south of"
10430 PRINT"here.":RETURN
10440 REM ***** ROOM 12 *****
10450 PRINT"You are standing at an intersection in the forest. A dying woodland is"
10460 PRINT"west of here, the trees charred black with soot. To the east stands a"
10470 PRINT"darker section of the forest, while a long bridge is south.":RETURN
10480 REM ***** ROOM 13 *****
10490 PRINT"You are walking through a lush, green meadow. It opens up here and"
10500 PRINT"continues for miles on end (to the east). To the west, a large path"
10510 PRINT"cuts through a well-lit forest.":RETURN
10520 REM ***** ROOM 14 *****
10530 PRINT"You are in a lush, green meadow. To the north lies a small pond. Small"
10540 PRINT"clouds drift on by, pushed by a gentle breeze. The only obvious exit"
10550 PRINT"is back west.":RETURN
10560 REM ***** ROOM 15 *****
10570 PRINT"You are standing next to a small pond. The water appears mossy green"
10580 PRINT"and devoid of life. Although you could try drinking the water, it does"
10590 PRINT"not appear healthy. The only exit is back south.":RETURN
10600 REM ***** ROOM 16 *****
10610 PRINT"You are knee deep in a swiftly moving river. The water feels icy cold!"
10620 PRINT"The river continues to the east, while less hurried currents are directly"
10630 PRINT"south.":RETURN
10640 REM ***** ROOM 17 *****
10650 PRINT"You have reached the northern bank of a swiftly moving river. On the other"
10660 PRINT"side of the water rises a tall Redwood forest (to the north).":RETURN
10670 REM ***** ROOM 18 *****
10680 PRINT"You are standing in a small clearing. There is nothing around but"
10690 PRINT"blue sky.":RETURN
10700 REM ***** ROOM 19 *****
10710 PRINT"You are walking in a Redwood forest, the trees of which rise hundreds of"
10720 PRINT"feet into the air. To the east stands a well-lit section of the forest,"
10730 PRINT"while west of here the area becomes dark and oppressive.":RETURN
10740 REM ***** ROOM 20  *****
10750 PRINT"You are standing in a Redwood forest, the trees of which rise hundreds of"
10760 PRINT"feet into the air. A large clearing is south, while one particular"
10770 PRINT"tree -- with low-lying branches, stands to the north. It appears"
10780 PRINT"climbable.":RETURN
10800 REM ***** ROOM 21 *****
10810 PRINT"The Redwood forest ends here next to a large, purple mountain range."
10820 PRINT"The only obvious exit is back west.":RETURN
10830 REM ***** ROOM 22 *****
10840 PRINT"You are walking in a Redwood forest. A lone tree (with low-lying"
10850 PRINT"branches) is here, as a poorly made ladder heads up into the treetops"
10860 PRINT"above. The only obvious exit is back south.":RETURN
10870 REM ***** ROOM 23 *****
10880 PRINT"You are atop the forest, standing on a long branch. A small hut (with a"
10890 PRINT"thatched roof) stands to the east near the top of the tree. Looking in,"
10900 PRINT"you can see a small creature, apparently meditating. Perhaps it would"
10910 PRINT"be best not to disturb him.":RETURN
10920 REM ***** ROOM 24 *****
10930 PRINT"You are standing inside a small hut. A small creature is here, apparently"
10940 PRINT"meditating. He stops suddenly, noticing your presence, but this does"
10950 PRINT"not seem to alarm him. He smiles at you, gestering to a nearby table"
10960 PRINT"(next to a wooden bookcase). Perhaps you should sit down and"
10970 PRINT"listen to what he has to say.":RETURN
10980 REM ***** ROOM 25 *****
10990 PRINT"You are walking in a Redwood forest. The trees thin out some to the"
11000 PRINT"west. To the south, the trees are much taller, as the area is populated"
11010 PRINT"with woodland creatures.":RETURN
11020 REM ***** ROOM 26 *****
11030 PRINT"You have reached the end of the Redwood forest. The only obvious exit"
11040 PRINT"is back east. Further travel west is made impossible by majestic purple"
11050 PRINT"mountains.":RETURN
11060 REM ***** ROOM 27 *****
11070 PRINT"You have reached the end of the burnt forest. The trees are charred"
11080 PRINT"black with soot (no doubt the remnants from a once raging wildfire)."
11090 PRINT"The only obvious exit is back south.":RETURN
11100 REM ***** ROOM 28 *****
11110 PRINT"You are walking in a burnt forest. The trees have been charred black"
11120 PRINT"from a recent wildfire. East of here the trees appear larger and"
11130 PRINT"more stable, while west of here the woods thin out somewhat.":RETURN
11140 REM ***** ROOM 29 *****
11150 PRINT"You are standing in the middle of a large, burnt forest. The trees"
11160 PRINT"have been charred black from a recent fire. To the north, the forest"
11170 PRINT"stops abruptly, while to the east and west the forest continues.":RETURN
11180 REM ***** ROOM 30 *****
11190 PRINT"You are walking in a burnt forest. The trees are particularly blackened"
11200 PRINT"here. This must have been where the first started, as the surrounding"
11210 PRINT"area is charred black with soot. Additionally, several telltale signs"
11220 PRINT"of a lightning strike are evident nearby, suggesting the cause of"
11230 PRINT"the blaze.":RETURN
11240 REM ***** ROOM 31 *****
11250 PRINT"You have reached the end of the burnt forest. Further travel west and"
11260 PRINT"north becomes impossible by fallen trees.":RETURN
11270 REM ***** ROOM 32 *****
11280 PRINT"You are walking in a burnt forest, the trees of which appear blackened"
11290 PRINT"by soot from a recent wildfire. To the north, the trees appear to thin"
11300 PRINT"out somewhat, while fallen trees obstruct further travel to the west.":RETURN
11310 REM ***** ROOM 33 *****
11320 PRINT"You have reached a burnt forest, the trees of which appear blackened"
11330 PRINT"with soot from a recent fire. To the north, the trees appear to thicken"
11340 PRINT"considerably, while east of here lies an intersection.":RETURN
11350 REM ***** ROOM 34 *****
11360 PRINT"You have reached a dark forest. The trees appear to be evergreens,"
11370 PRINT"shrouding most of the floor, while speckles of light filter down from"
11380 PRINT"the thick canopy above. To the east, the forest appears to thin out"
11390 PRINT"somewhat, while west of here lies an intersection.":RETURN
11400 REM ***** ROOM 35 *****
11410 PRINT"You are walking in a dark forest. The trees are particularly dense here, as"
11420 PRINT"very little light filters down from below. There are several different"
11430 PRINT"woodland creatures here (all scurrying about), including squirrels, rabbits,"
11440 PRINT"foxes and birds. The forest continues east and south.":RETURN
11450 REM ***** ROOM 36 *****
11460 PRINT"You are walking in a dark forest. The trees thin out here near a large"
11470 PRINT"river (to the east), while west of here appears to be the middle of"
11480 PRINT"the forest. South of here the forest continues to thin out.":RETURN
11490 REM ***** ROOM 37 *****
11500 PRINT"You are standing at the bank of a swiftly moving river. Although it does"
11510 PRINT"not appear too deep, caution should nonetheless be taken. The forest"
11520 PRINT"continues west, while east of here a cave is visible, nestled near a"
11530 PRINT"small alcove (to the southwest).":RETURN
11540 REM ***** ROOM 38 *****
11550 PRINT"You have entered into a dark cave. Very little is visible here, but you"
11560 PRINT"do notice several glowing red lights, all wisping about endlessly. It is almost"
11570 PRINT"as if the cave is haunted by evil spirits! The cave begins to widen some to"
11580 PRINT"the north.":RETURN
11590 REM ***** ROOM 39 *****
11600 PRINT"You have reached a large tunnel within the cave, splitting off to the east"
11610 PRINT"here (accessible through a hole). As the tunnel progresses, it appears to"
11620 PRINT"narrow considerably. You might only be able to squeeze through by dropping"
11630 PRINT"several items.":RETURN
11640 REM ***** ROOM 40 *****
11650 PRINT"You are crawling on your belly in a narrow tunnel. It heads to the east here,"
11660 PRINT"turning sharply. Moans from disembodied spirits can be heard, getting louder"
11670 PRINT"the deeper you go! The only obvious exit is back west.":RETURN
11680 REM ***** ROOM 41 *****
11690 PRINT"You have reached a small intersection within the cave. Tunnels head off in"
11700 PRINT"all directions here, while the main tunnel continues north.":RETURN
11710 REM ***** ROOM 42 *****
11720 PRINT"The tunnel stops here near a large rock. The only exit is back west.":RETURN
11730 REM ***** ROOM 43 *****
11740 PRINT"You have reached a dead end within the cavern. The only exit is back east.":RETURN
11750 REM ***** ROOM 44 *****
11760 PRINT"You are crawling on your belly in the heart of the cave. The main tunnel"
11770 PRINT"branches off here in four directions: north, south, east and west. One"
11780 PRINT"particular tunnel (to the east) appears to glow softly with a pale"
11790 PRINT"green hue. Perhaps it would be worth exploring further.":RETURN
11800 REM ***** ROOM 45 *****
11810 PRINT"You are crawling on your belly in a small tunnel. The area is surrounded"
11820 PRINT"by pale green light, glowing softly. The air feels magical, as if the"
11830 PRINT"light particles themselves are alive!":RETURN
11840 REM ***** ROOM 46 *****
11850 PRINT"You have reached a small keep within the cave. Several large candles,"
11860 PRINT"glowing softly green, illuminate the darkness. This small room appears"
11870 PRINT"to be a place of meditation and worship. Perhaps you should rest here"
11880 PRINT"and learn more about this most unique room.":RETURN
11890 REM ***** ROOM 47 *****
11900 PRINT"You are crawling on your belly in the main tunnel within the cave. To the"
11910 PRINT"south you notice a pale green light, glowing near a smaller tunnel towards"
11920 PRINT"the southeast. To the east, the main tunnel forks off into a much smaller"
11930 PRINT"one. It appears almost too narrow to pass through.":RETURN
11940 REM ***** ROOM 48 *****
11950 PRINT"You have reached a dead end within the cavern, as the main tunnel"
11960 PRINT"forks off here into a very small passageway. However, a large"
11970 PRINT"boulder has crashed through the tunnel from above, blocking further"
11980 PRINT"passage east. The only exit is back west.":RETURN
11990 REM ***** ROOM 49 *****
12000 PRINT"You are standing on a wooden bridge just south of a large intersection."
12010 PRINT"It appears to cross over a swiftly moving river (heading northeast). At"
12020 PRINT"the end of the bridge you notice a large, stone castle.":RETURN
12030 REM ***** ROOM 50 *****
12040 PRINT"You are walking in a dark forest. The trees thicken somewhat to the east, as"
12050 PRINT"very little light filters down from below. There are several woodland creatures"
12060 PRINT"scurrying about, making you feel restless. The forest continues north and east.":RETURN
12070 REM ***** ROOM 51 *****
12080 PRINT"You are walking in the middle of a dark forest. The trees are particularly"
12090 PRINT"thick here, as a plethora of woodland creatures scurry about the forest floor,"
12100 PRINT"looking for food. To the east stands a secluded alcove, while north the forest"
12110 PRINT"continues. Further travel south becomes increasingly difficult, but not"
12120 PRINT"impossible.":RETURN
12130 REM ***** ROOM 52 *****
12140 PRINT"You have reached a secluded alcove, hidden within the forest. Several magical"
12150 PRINT"creatures populate the area, including unicorns and one-eyed beasts. It appears"
12160 PRINT"to be a land that time forgot, for these beings are only written about in"
12170 PRINT"fairy tales! The only exit is back west.":RETURN
12180 REM ***** ROOM 53 *****
12190 PRINT"You are standing in a small kitchen within the cottage. A large wooden table"
12200 PRINT"is set in the center, surrounded by chairs. To the south lies the main living"
12210 PRINT"room, as a narrow hallway heads into several small rooms.":RETURN
12220 REM ***** ROOM 54 *****
12230 PRINT"You have entered into a small room within the cottage. Not much can be seen"
12240 PRINT"here save for a bed and a small table. The only exit is back south.":RETURN
12250 REM ***** ROOM 55 *****
12260 PRINT"You are standing in the middle of a large bridge. Underneath, a river flows"
12270 PRINT"to the northeast. To the south, the bridge stops abruptly near a large,"
12280 PRINT"stone castle. Rumor has it that a goblin periodically appears, demanding"
12290 PRINT"money by all who cross here. You wonder if the goblin will stop you, too!":RETURN
12300 REM ***** ROOM 56 *****
12310 PRINT"You have reached the limits of the dark forest. A well-lit section of"
12320 PRINT"the forest opens up here, dramatically improving visibility. Far to the"
12330 PRINT"east a cottage can be seen, smoke rising from a chimney high above.":RETURN
12340 REM ***** ROOM 57 *****
12350 PRINT"You are standing in a well-lit forest. Beams of light filter down from"
12360 PRINT"the canopy above, dotting the forest floor. The forest continues to the"
12370 PRINT"east and south, while west of here a dark forest is visible.":RETURN
12380 REM ***** ROOM 58 *****
12390 PRINT"You are standing in the middle of a well-lit forest. Beams of light filter down"
12400 PRINT"from high above, dotting the forest floor. A particularly well-lit path, almost"
12410 PRINT"magical in appearance, shimmers to the south, while east of here the forest"
12420 PRINT"continues towards a large cottage.":RETURN
12430 REM ***** ROOM 59 *****
12440 PRINT"You are standing in a well-lit forest next to a large cottage (east). The"
12450 PRINT"forest begins to thin out here somewhat, as the cottage dominates the eastern"
12460 PRINT"edge of the woods (with the door wide open). Could someone be expecting you?":RETURN
12470 REM ***** ROOM 60 *****
12480 PRINT"You are standing inside a large cottage within the forest. A fireplace is here,"
12490 PRINT"allowing you to warm your feet. Oddly enough, no one appears to have occupied"
12500 PRINT"this abode for quite some time, even though a fire is ominously burning"
12510 PRINT"neatly in a small fireplace. 'Could this place be haunted?' you wonder aloud."
12520 PRINT"A hallway is east, while a kitchen is directly north.":RETURN
12530 REM ***** ROOM 61 *****
12540 PRINT"You are walking down a long hallway within the cottage. Several rooms are"
12550 PRINT"visible here, some closed by locked doors (and others open). The hallway"
12560 PRINT"continues east.":RETURN
12570 REM ***** ROOM 62 *****
12580 PRINT"You are walking down a long hallway within the cottage. Two rooms are"
12590 PRINT"plainly visible up ahead (to the north and east), both of them left"
12600 PRINT"wide open by the previous occupants.":RETURN
12610 REM ***** ROOM 63 *****
12620 PRINT"You have reached a small room within the cottage. Not much can be seen"
12630 PRINT"here, save for a small bed next to a wooden dresser. The only exit is"
12640 PRINT"back west.":RETURN
12650 REM ***** ROOM 64 *****
12660 PRINT"You have reached the inner sanctum of Mordimar, evil of the ancients! You"
12670 PRINT"sense great power! He cackles at you with an insane laugh, yelling,'You"
12680 PRINT"fool! You can't possibly hope to defeat me! I am Mordimar, master of the"
12690 PRINT"ancient circle of wizards! I own this world! Who are YOU to challenge"
12700 PRINT"otherwise?! Bah! So be it! I will crush you like a bug! Die, knave!'":RETURN
12710 REM ***** ROOM 65 *****
12720 PRINT"At the end of the bridge, a small goblin is said to often appear,"
12730 PRINT"demanding gold. To the south, just beyond the bridge, stands a tall"
12740 PRINT"stone castle. A pair of red flags, with gryphons emblazoned on"
12750 PRINT"them, wave to you in a stiff breeze, somehow beckoning you inside!":RETURN
12760 REM ***** ROOM 66 *****
12770 PRINT"You are walking in a well-lit forest. Light filters down from the canopy"
12780 PRINT"above, dotting the forest floor. Several woodland creatures scurry about,"
12790 PRINT"some of them unaware of your presence. Fallen trees block passage to the"
12800 PRINT"south. The only obvious exit is back north.":RETURN
12810 REM ***** ROOM 67 *****
12820 PRINT"You are standing in the middle of a well-lit forest. The path turns east"
12830 PRINT"here near a large patch of strange red berries. They look too small for"
12840 PRINT"humans to eat, but it looks as though the tiny woodland creatures eat"
12850 PRINT"them regularly. To the north, the forest thickens somewhat.":RETURN
12860 REM ***** ROOM 68 *****
12870 PRINT"You have reached a particularly dark section of the forest, more secluded than"
12880 PRINT"the rest. To the south, a small path heads past a series of red berry patches."
12890 PRINT"Whatever lives in these woods must be fairly tiny, for the berries themselves"
12900 PRINT"are all scattered about the forest floor.":RETURN
12910 REM ***** ROOM 69 *****
12920 PRINT"You have reached the limits of the forest, as storm-tossed trees block your"
12930 PRINT"path to the east. The only obvious exit is back west.":RETURN
12940 REM ***** ROOM 70 *****
12950 PRINT"You have entered into the throne room of Mordimar. Strangely, he does not"
12960 PRINT"appear to be here, although you DO sense a strong energy to the north! The"
12970 PRINT"throne is made of solid gold and encrusted with several jewels. To the"
12980 PRINT"east lies a stone fountain.":RETURN
12990 REM ***** ROOM 71 *****
13000 PRINT"You are standing in the castle courtyard. A large stone fountain, with the"
13010 PRINT"statue of an angel, pours water down from an upheld jar into a motted basin"
13020 PRINT"below. A throne room is visible to the west, while the castle entrance is"
13030 PRINT"east.":RETURN
13040 REM ***** ROOM 72 *****
13050 PRINT"You have reached a tall, stone castle known as Shadow Castle. Legend has it"
13060 PRINT"that the evil wizard Mordimar took the castle by storm many moons ago. High"
13070 PRINT"above, a pair of flags, with gryphons emblazoned on them, flap endlessly in"
13080 PRINT"a cool breeze. They appear to be beckoning you further into the castle! A"
13090 PRINT"courtyard, complete with a fountain, stands to the west of here.":RETURN
13100 REM ***** ROOM 73 *****
13110 PRINT"You have somehow entered into a strange, new world just beyond the red berry"
13120 PRINT"patch. Although it hardly seems possible, the entire area is populated by"
13130 PRINT"dozens of tiny blue creatures not more than three apples high! You are"
13140 PRINT"stunned by their speech, as they all seem to be speaking in a bizarre, elven"
13150 PRINT"tongue, the likes of which you cannot make out. To the north, mushroom"
13160 PRINT"houses are visible. This is where the blue creatures apparently live and"
13170 PRINT"work.":RETURN
13180 REM ***** ROOM 74 *****
13190 PRINT"You have found a small path separate from the rest of the forest. It turns"
13200 PRINT"to the south, heading west into a large red berry patch. A well-lit forest"
13210 PRINT"is clearly visible (to the north).":RETURN
13220 REM ***** ROOM 75 *****
13230 PRINT"You are walking through a red berry patch. These berries are quite odd, as"
13240 PRINT"only the bottom portion of the patch is scattered about the ground. It"
13250 PRINT"appears as though whomever eats of these berries can't be more than"
13260 PRINT"three apples high. To the north, a small village of mushroom houses"
13270 PRINT"is plainly visible.":RETURN
13280 REM ***** ROOM 76 *****
13290 PRINT"You have reached a large red berry patch. Although it hardly seems"
13300 PRINT"possible, only the bottom portion of the patch is scattered about the"
13310 PRINT"ground, suggesting small woodland creatures, not more than three apples"
13320 PRINT"high, eat from the patch. You pick a few berries from the tree and"
13330 PRINT"taste them, but they are hardly edible, as the flavor is slightly"
13340 PRINT"sour! Yuck! The red berry patch continues west.":RETURN
13350 REM ***** ROOM 77 *****
13360 PRINT"You have reached the end of the small path, stopping here at a"
13370 PRINT"large red berry patch (to the west). Several woodland creatures"
13380 PRINT"scurry about the floor, some of them blue! They appear to be"
13390 PRINT"picking red berries from a large patch of berry bushes. Because"
13400 PRINT"some of these creatures are so small, there is an abundance of"
13410 PRINT"berries near the top, which is waist high for humans.":RETURN
13420 REM ***** ROOM 78 *****
13430 PRINT"As you walk about the village, trying not to step on the dozens of"
13440 PRINT"curious blue creatures, one particular elf walks up to you. He has a"
13450 PRINT"white beard and a red hat, which is distinct from all the others who"
13460 PRINT"wear only white hats. He also appears to be much wiser than the rest,"
13470 PRINT"if not older as well. He says to you,'You can walk around the village,"
13480 PRINT"but please, be careful! If you need any help, ask Handy for tools or"
13490 PRINT"other items. Have a smurfy day!' The path continues north and east.":RETURN
13500 REM ***** ROOM 79 *****
13510 PRINT"As you walk up to a goofy looking blue creature, he hands you a yellow"
13520 PRINT"box, wrapped in a red bow. Carefully, you remove the wrappings, but suddenly"
13530 PRINT"a blast of soot and ash explode in your face! Checking yourself over, you"
13540 PRINT"do not appear to be harmed, just embarrassed! The blue creature laughs at"
13550 PRINT"you, teasing,'You fell for my trick! BWHAHAHAHAHA!' You feel like stepping"
13560 PRINT"on the blue creature! Thankfully, he walks away before you can smush him!":RETURN
13570 REM ***** ROOM 80 *****
13571 IF LO(56) = -5 THEN 13670
13580 PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
13590 PRINT"village. He appears to be working on several different projects as once, the"
13600 PRINT"largest of which appears to be a wooden statue of appearance similar to"
13610 PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
13620 PRINT"to look around, but don't touch anything! I'm working on a robot to"
13630 PRINT"help with the dam building next fall!' You wonder what he could be"
13640 PRINT"talking about when it hits you: he's working on a robot! Handy looks"
13650 PRINT"around the room and says,'What I really need is a nice, new rug! I"
13660 PRINT"know! Bring me azrael and I'll give you some tools!'":RETURN
13670 REM *****
13671 PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
13672 PRINT"village. He appears to be working on several different projects as once, the"
13673 PRINT"largest of which appears to be a wooden statue of appearance similar to"
13674 PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
13675 PRINT"to look around, but don't touch anything! I'm working on a robot to"
13676 PRINT"help with the dam building next fall!' You wonder what he could be"
13677 PRINT"talking about when it hits you: he's working on a robot! Handy thanks"
13678 PRINT"you for the nice rug. 'Thank you! It looks Smurfy!'":RETURN
13680 REM ***** ROOM 81 *****
13685 PRINT"You are walking in Smurf forest. On one particular tree you notice a"
13690 PRINT"blue creature with glasses, reading from a book. He appears to be secluded"
13700 PRINT"from the rest of the Smurfs, as he immerses himself in endless quotations"
13710 PRINT"(apparently self-written). The path continues to the east near a hulking"
13720 PRINT"castle.":RETURN
13730 REM ***** ROOM 82 *****
13740 PRINT"You are on a branch high atop Smurf forest. A lone blue creature is here,"
13750 PRINT"wearing glasses and reading quotations from a large book. 'Famous quotations"
13760 PRINT"from Brainy Smurf' he says to you. 'Quotation 36: He who seeks shall find,"
13770 PRINT"and having seeked, shall be wiser the more!' You feel a sudden repulsiveness"
13780 PRINT"towards this particular creature: no wonder he isn't liked! The only obvious"
13790 PRINT"exit is back down the tree.":RETURN
13800 REM ***** ROOM 83 *****
13810 PRINT"You are standing outside Gargamel's castle, a hulking place with a large"
13820 PRINT"bell at the top and several crows roosting on the straw-matted roof. From"
13830 PRINT"inside, you can hear a man talking to no one in particular. You listen"
13840 PRINT"for awhile until you hear him bellow,'I'll get you, you dispicable little"
13850 PRINT"Smurfs!' You wonder why he could be so upset, but then you remember"
13860 PRINT"how you were treated entering the village.":RETURN
13870 REM ***** ROOM 84 *****
13880 PRINT"You are standing inside Gargamel's castle. A tall man wearing a black"
13890 PRINT"cloak is stirring a large pot, cackling in glee. 'Oh, the smurfs will"
13900 PRINT"be mine, Azrael! And then after I've eaten them, I can turn the rest"
13910 PRINT"into gold! Gold, Azrael! BWHAHAHAHAHAHA!' He appears to be talking to"
13920 PRINT"his cat, scurrying about the floor in fear of his master! A few beakers"
13930 PRINT"are here, as Gargamel appears to be mixing potions. There are also"
13940 PRINT"a few gold bars laying on a wooden table near the back.":RETURN
13950 RETURN
20000 REM ***** DATA *****
20010 DATA "north","south","east","west","up","down"
20020 DATA "oil","lantern","rope","knapsack","backpack"
20030 DATA "pole","food","water","wine","bottle"
20040 DATA "book","gold","spellbook","beaker"
20050 DATA "helmet","chainmail","boots","cloak"
20060 DATA "ring","shield","amulet","gauntlets"
20070 DATA "dagger","sword","broadsword","axe"
20080 DATA "glowball","scepter","slayer"
20090 DATA "villager","clerk","goblin","hellhound"
20100 DATA "direwolf","skeleton","dragon","ghost"
20110 DATA "wolf","warrior","tursk","troll","spider"
20120 DATA "hobbit","mordimar"
20130 DATA "papa","handy","jokey","brainy"
20140 DATA "gargamel","azrael"
20150 DATA 99,99,99,99,99,99
20160 DATA 98,4,8,4,63,63,1,5,1,53,82,84,24,84
20170 DATA 1038,1038,1038,1038,24,1041,1042,1045
20180 DATA 4,98,1045,1038,24,1043,1042,5,3,65,12
20190 DATA 41,45,38,35,10,20,72,24,47,68,64,78
20200 DATA 80,79,82,84,84
20210 REM Item descriptions
20220 DATA "north: facing north."
20230 DATA "south: facing south."
20240 DATA "east: facing east."
20250 DATA "west: facing west."
20260 DATA "up: facing up."
20270 DATA "down: facing down"
20280 DATA "oil: a flask of oil. Requires the lantern."
20290 DATA "lantern: a brass lantern stained with blood. Requires the oil."
20300 DATA "rope: a coil of rope about 10' in length. It appears used."
20310 DATA "knapsack: a leather knapsack. Can hold up to 4 items."
20320 DATA "backpack: a large backpack. Can hold up to 6 items."
20330 DATA "pole: a wooden pole about 6' in length."
20340 DATA "food: these food rations can last up to two weeks."
20350 DATA "water: the water appears murky green. It does not appear drinkable."
20360 DATA "wine: a bottle of sparkling red wine. It looks drinkable."
20370 DATA "bottle: an empty bottle."
20380 DATA "book: a large leather book. It is written in an elvish tongue."
20390 DATA "gold: several gold bars. They glitter beautifully!"
20400 DATA "spellbook: an ancient spellbook. Useful in casting spells."
20410 DATA "beaker: a glass beaker containing a red liquid."
20420 DATA "helmet: an iron helmet. It has a few small dents."
20430 DATA "chainmail: a chainlinked suit of armor. Offers good protection."
20440 DATA "boots: a pair of leather boots. Offers minimal protection."
20450 DATA "cloak: a large cloak. Offers fair protection."
20460 DATA "ring: a ring of protection. It glows with a bluish hue."
20470 DATA "shield: a large shield with a cross on the front. Offers great protection."
20480 DATA "amulet: a magical amulet. It glows with a greenish hue."
20490 DATA "gauntlets: a pair of metal gauntlets. Offers good protection."
20500 DATA "dagger: a small dagger with a jeweled hilt. Effective at close range."
20510 DATA "sword: a large two-handed sword. It has a ruby hilt. Effective at medium range."
20520 DATA "broadsword: a large broadsword with a diamond hilt. Effective at long range."
20530 DATA "axe: a large battle axe. It has a doubled edge. Effective at close range."
20540 DATA "glowball: eight glowing balls wisping around like fireflies. Effective at long range."
20550 DATA "scepter: a large, powerful scepter with elvish runes. It appears magical."
20560 DATA "slayer: a large sword with a razor-sharp blade. Effective at medium range."
20570 DATA "villager: a simple villager. He smiles at you briefly."
20580 DATA "clerk: a stout clerk with glasses. You can 'buy' and 'sell' items here."
20590 DATA "goblin: a menacing goblin with dark eyes and sharp teeth."
20600 DATA "hellhound: a fiery hellhound raised from the depths of hell. He burns in a luminous flame."
20610 DATA "direwolf: a direwolf wandering the caverns. He growls at you loudly."
20620 DATA "skeleton: an undead zombie haunting the forest. He appears almost translucent."
20630 DATA "dragon: a large dragon with tough scales and sharp claws. He bellows,'Be gone, knave!'"
20640 DATA "ghost: a translucent ghost haunting the area where he died. Watch out!"
20650 DATA "wolf: a friendly wolf with yellow eyes. He seems content on watching you."
20660 DATA "warrior: an evil warrior bound by black magic. He looks tough indeed!"
20670 DATA "tursk: a friendly adventurer with a trimmed beard and flowing hair."
20680 DATA "troll: a troll. Nothing special."
20690 DATA "spider: a black widow spider lurking in the shadows. Be wary of his bite!"
20700 DATA "hobbit: a friendly hobbit. He has furry feet and stands less than 4 feet tall."
20710 DATA "mordimar: the all-powerful wizard of legend! You sense great power flowing through him!"
20720 DATA "papa: a small blue creature with a white beard and a red hat. He is mixing potions."
20730 DATA "handy: a small blue creature wearing a white hat. He appears to be building something."
20740 DATA "jokey: this mischevious blue creature appears to be up to no good. He carries around a box wrapped with a bow."
20750 DATA "brainy: a small blue creature with glasses. He appears to be the least liked of his kind."
20760 DATA "gargamel: an evil wizard trying to turn little blue creatures into gold. He has a black heart."
20770 DATA "azrael: a mischevious cat. He appears somewhat malnourished."
20780 REM Verbs
20790 DATA"go","get","drop","inventory","look","wield","unwield","wear","remove","examine","use"
20800 DATA"climb","read","buy","sell","kill","put","eat","drink"
20810 REM map
20820 REM   N, S, E, W, U, D
20830 DATA 00,05,00,00,02,00
20840 DATA 00,00,04,00,00,01
20850 DATA 00,00,00,05,00,00
20860 DATA 00,00,00,02,00,00
20870 DATA 01,09,03,06,00,00
20880 DATA 07,00,05,00,00,00
20890 DATA 00,06,00,08,00,00
20900 DATA 00,00,07,00,00,00
20910 DATA 05,10,00,00,00,00
20920 DATA 09,12,13,11,00,00
20930 DATA 30,33,10,00,00,00
20940 DATA 10,49,34,33,00,00
20950 DATA 00,00,14,10,00,00
20960 DATA 15,00,00,13,00,00
20970 DATA 00,14,00,00,00,00
20980 DATA 00,37,17,00,00,00
20990 DATA 18,00,00,16,00,00
21000 DATA 20,17,00,00,00,00
21010 DATA 25,00,20,00,00,00
21020 DATA 22,18,21,19,00,00
21030 DATA 00,00,00,20,00,00
21040 DATA 00,20,00,00,23,00
21050 DATA 00,00,24,00,00,22
21060 DATA 00,00,00,23,00,00
21070 DATA 00,19,00,26,00,00
21080 DATA 00,00,25,00,00,00
21090 DATA 00,29,00,00,00,00
21100 DATA 00,32,29,00,00,00
21110 DATA 27,00,30,28,00,00
21120 DATA 00,11,00,29,00,00
21130 DATA 00,00,32,00,00,00
21140 DATA 28,00,00,31,00,00
21150 DATA 11,00,12,00,00,00
21160 DATA 00,00,35,12,00,00
21170 DATA 00,50,36,34,00,00
21180 DATA 00,51,37,35,00,00
21190 DATA 16,00,38,36,00,00
21200 DATA 39,00,00,37,00,00
21210 DATA 00,38,40,00,00,00
21220 DATA 00,00,41,39,00,00
21230 DATA 44,00,42,40,00,00
21240 DATA 00,00,00,41,00,00
21250 DATA 00,00,44,00,00,00
21260 DATA 47,41,45,43,00,00
21270 DATA 00,00,46,44,00,00
21280 DATA 00,00,00,45,00,00
21290 DATA 00,44,48,00,00,00
21300 DATA 00,00,00,47,00,00
21310 DATA 12,55,00,00,00,00
21320 DATA 35,00,51,00,00,00
21330 DATA 36,56,52,50,00,00
21340 DATA 00,00,00,51,00,00
21350 DATA 00,60,00,00,00,00
21360 DATA 00,62,00,00,00,00
21370 DATA 49,65,00,00,00,00
21380 DATA 51,00,57,00,00,00
21390 DATA 00,66,58,56,00,00
21400 DATA 00,67,59,57,00,00
21410 DATA 00,00,60,58,00,00
21420 DATA 53,00,61,59,00,00
21430 DATA 00,00,62,60,00,00
21440 DATA 54,00,63,61,00,00
21450 DATA 00,00,00,62,00,00
21460 DATA 00,70,00,00,00,00
21470 DATA 55,72,00,00,00,00
21480 DATA 57,00,00,00,00,00
21490 DATA 58,00,68,00,00,00
21500 DATA 00,74,69,67,00,00
21510 DATA 00,00,00,68,00,00
21520 DATA 64,00,71,00,00,00
21530 DATA 00,00,72,70,00,00
21540 DATA 65,00,00,71,00,00
21550 DATA 78,75,00,00,00,00
21560 DATA 69,77,00,00,00,00
21570 DATA 73,00,76,00,00,00
21580 DATA 00,00,77,75,00,00
21590 DATA 74,00,00,76,00,00
21600 DATA 79,73,80,00,00,00
21610 DATA 00,78,81,00,00,00
21620 DATA 00,00,00,78,00,00
21630 DATA 00,00,83,79,82,00
21640 DATA 00,00,00,00,00,81
21650 DATA 00,00,84,81,00,00
21660 DATA 00,00,00,83,00,00
21670 REM
21680 DATA "Inside a Small Tavern"
21690 DATA "Upstairs Hallway"
21700 DATA "In the Village Shop"
21710 DATA "Inside a Small Room"
21720 DATA "By a large Fountain"
21730 DATA "Inside the Village Church"
21740 DATA "Within the Church Gargen"
21750 DATA "Inside a Small Toolshed"
21760 DATA "Entrance to Village"
21770 DATA "On the Path"
21780 DATA "In a Burnt Forest"
21790 DATA "At an Intersection"
21800 DATA "In a Large Meadow"
21810 DATA "In a Large Meadow"
21820 DATA "At a Small Pond"
21830 DATA "In the River"
21840 DATA "In the River"
21850 DATA "In a Small Clearing"
21860 DATA "Redwood Forest"
21870 DATA "Redwood Forest"
21880 DATA "Redwood Forest"
21890 DATA "Redwood Forest"
21900 DATA "High atop the forest"
21910 DATA "Small Hut"
21920 DATA "Redwood Forest"
21930 DATA "Redwood Forest"
21940 DATA "In a Burnt Forest"
21950 DATA "In a Burnt Forest"
21960 DATA "In a Burnt Forest"
21970 DATA "In a Burnt Forest"
21980 DATA "In a Burnt Forest"
21990 DATA "In a Burnt Forest"
22000 DATA "In a Burnt Forest"
22010 DATA "Dark Forest"
22020 DATA "Dark Forest"
22030 DATA "Dark Forest"
22040 DATA "In the River"
22050 DATA "Large Cave"
22060 DATA "In a Twisty Tunnel"
22070 DATA "In a Twisty Tunnel"
22080 DATA "In a Twisty Tunnel"
22090 DATA "At a Dead End"
22100 DATA "At a Dead End"
22110 DATA "In a Twisty Tunnel"
22120 DATA "In a Twisty Tunnel"
22130 DATA "Small Keep"
22140 DATA "In a Twisty Tunnel"
22150 DATA "At a Dead End"
22160 DATA "On the Bridge"
22170 DATA "Dark Forest"
22180 DATA "Dark Forest"
22190 DATA "Small Alcove"
22200 DATA "Kitchen"
22210 DATA "Small Room"
22220 DATA "On the Bridge"
22230 DATA "Dark Forest"
22240 DATA "Light Forest"
22250 DATA "Light Forest"
22260 DATA "Light Forest"
22270 DATA "In a Small Cottage"
22280 DATA "Hallway"
22290 DATA "Hallway"
22300 DATA "Small Room"
22310 DATA "Mordimar"
22320 DATA "On the Bridge"
22330 DATA "Light Forest"
22340 DATA "Light Forest"
22350 DATA "Light Forest"
22360 DATA "Light Forest"
22370 DATA "Throne Room"
22380 DATA "Fountain"
22390 DATA "Shadow Castle"
22400 DATA "Strange New World"
22410 DATA "Small Path"
22420 DATA "Smurf Berry Patch"
22430 DATA "Smurf Berry Patch"
22440 DATA "Smurf Berry Patch"
22450 DATA "Papa Smurf"
22460 DATA "Jokey Smurf"
22470 DATA "Handy Smurf"
22480 DATA "Smurf Forest"
22490 DATA "High atop Smurf Forest"
22500 DATA "Outside Gargamel's Castle"
22510 DATA "Inside Gargamel's Castle"
22520 REM item prices
22530 DATA 99,99,99,99,99,99
22540 DATA 12,26,8,40,200
22550 DATA 42,30,16,50,20
22560 DATA 100,1000,5000,28
22570 DATA 200,800,120,80
22580 DATA 300,150,380,400
22590 DATA 50,480,750,320
22600 DATA 2000,1000,3000
22610 DATA 99,99,99,99
22620 DATA 99,99,99,99
22630 DATA 99,99,99,99,99
22640 DATA 99,99
22650 DATA 99,99,99,99
22660 DATA 99,99
29999 REM *****
30000 REM ***** Wait for TEMPO seconds *****
30010 T = TIMER
30020 WHILE TIMER < T + TEMPO
30030 WEND
30040 RETURN
39999 REM ***** ERROR HANDLING *****
40000 PRINT :PRINT "Saved game is not accessible or doesn't exist.":PRINT"You have to start from the beginning."
40020 END
