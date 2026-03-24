REM ******************************************
REM *****         ADVENTURE XT          ******
REM *                                        *
REM * By Paul Allen Panks - 2007             *
REM *                                        *
REM * Ported to GW-Basic by D. RIOUAL - 2023 *
REM *                                        *
REM ******************************************
REM
CLEAR:COLOR 12,0:CLS:KEY OFF
HS$="Handy says,'Thank you for bringing Azrael to me! Here are several tools for you. I'll leave them here.'"
HS2$="Handy places some tools on the floor."
MS$="The beaker breaks. Azrael licks it up and suddenly falls to the floor! Checking him, he is fast asleep. You pick him up, carrying Azrael on your back. Back to Handy Smurf's!"
G=1000:IC=0:HP=192:RM=1:FB$=">>>>>>>>>>>>>>>>>>>> F I R E - B A L L ! ! !"
FB2$="The monster is burnt to a crisp!"
DIM NO$(60),LO(60),EX$(60),VB$(25),M(100,6),DE$(100),P(60),WD$(20)
FOR X=1 TO 56:READ NO$(X):NEXT
FOR X=1 TO 56:READ LO(X):NEXT
FOR X=1 TO 56:READ EX$(X):NEXT
FOR X=1 TO 19:READ VB$(X):NEXT
FOR X=1 TO 84:FOR Y=1 TO 6:READ M(X,Y):NEXT:NEXT
FOR X=1 TO 84:READ DE$(X):NEXT
FOR X=1 TO 56:READ P(X):NEXT
CLS:PRINT"@@@ Adventure XT @@@":PRINT"Written for the 13th annual interactive fiction contest":PRINT"By: Paul Panks (dunric@yahoo.com)":PRINT
PRINT"It has been four years since you last ventured into Blarg,"
PRINT"the land of might and magic. Mordimar, an evil wizard, found"
PRINT"the powerful Orb of Destiny. With it, he became nearly"
PRINT"invincible. As his power grew, the chi (life force)"
PRINT"from the surrounding forest was drained steadily, until"
PRINT"there was almost nothing left.":PRINT
PRINT"Determined to stop Mordimar, you set out on a quest"
PRINT"to re-acquire the orb and bring peace back to the"
PRINT"forest.":PRINT:PRINT"Your Quest Begins":PRINT
PRINT"@@@ Press any key to continue @@@"
L560: A$=INKEY$:IF A$="" THEN GOTO L560
CLS
PRINT"Are you playing a saved game (y/n)?";
L590: A$=INKEY$:IF A$="" THEN GOTO L590
IF A$="y" OR A$="Y" THEN PRINT A$:PRINT:GOTO L4330
IF A$="n" OR A$="N" THEN PRINT A$:PRINT:GOTO L1000
GOTO L590
REM Verb routines (generic)
L1000: NC=0:PRINT DE$(RM):IF LT=0 THEN IF RM>18 THEN PRINT"It is too dark to see much of anything!":GOTO L1030
IF RM < 21 THEN GOTO L1010
IF (RM >= 21) AND (RM < 41) THEN GOTO L1012
IF (RM >= 41) AND (RM < 61) THEN GOTO L1014
IF (RM >= 61) AND (RM < 81) THEN GOTO L1016
IF RM >= 81 THEN GOTO L1018
L1010: ON RM GOSUB L10000,L10040,L10080,L10120,L10160,L10210,L10250,L10290,L10330,L10360,L10400,L10440,L10480,L10520,L10560,L10600,L10640,L10670,L10700,L10740
GOTO L1030
L1012: ON (RM-20) GOSUB L10800,L10830,L10870,L10920,L10980,L11020,L11060,L11100,L11140,L11180,L11240,L11270,L11310,L11350,L11400,L11450,L11490,L11540,L11590,L11640
GOTO L1030
L1014: ON (RM-40) GOSUB L11680,L11710,L11730,L11750,L11800,L11840,L11890,L11940,L11990,L12030,L12070,L12130,L12180,L12220,L12250,L12300,L12340,L12380,L12430,L12470
GOTO L1030
L1016: ON (RM-60) GOSUB L12530,L12570,L12610,L12650,L12710,L12760,L12810,L12860,L12910,L12940,L12990,L13040,L13100,L13180,L13220,L13280,L13350,L13420,L13500,L13570
GOTO L1030
L1018: ON (RM-80) GOSUB L13680,L13730,L13800,L13870
L1030: GOSUB L4440
FOR X=7 TO 56:IF LO(X)=RM THEN PRINT NO$(X);"."
NEXT
L1100: IF LO(56)=0 AND RM=80 THEN LO(56)=-5:PRINT"You bring Azrael to Handy.":PRINT HS$:PRINT HS2$:LO(9)=RM:LO(22)=RM:LO(31)=RM:LO(32)=RM
CT=CT+1:V=0:N=0:NE$="":N$="":N2$="":V$="":V2$="":PR=0:PT=0:NM=0:BZ=0:FOR X=1 TO 10:WD$(X)="":NEXT X
INPUT ">",A$
REM Convert to lower case 
FOR X=1 TO LEN(A$):C=ASC(MID$(A$,X,1))
IF C>=65 AND C <=90  THEN C=C+32 : MID$(A$,X,1)= CHR$(C) 
NEXT X
GOSUB L4010
PT=1:NM=0:D$=A$:FOR A=1 TO LEN(D$):IF MID$(D$, A, 1)=" " THEN A$=MID$(D$,PT,A-PT): PT=A+1:NM=NM+1:WD$(NM)=A$
NEXT A:NM=NM+1:A$=MID$(D$,PT,A-PT):WD$(NM)=A$
V$=WD$(1):N$=WD$(2):IF WD$(3)="and" OR WD$(3)="then" THEN V2$=WD$(4):N2$=WD$(5):CO=1
IF WD$(3)="in" OR WD$(3)="from" OR WD$(3)="to" THEN V$=WD$(1):NE$=WD$(2):NE2$=WD$(4):PR=1:BZ=1
V=0:FOR X=1 TO 19:IF V$=VB$(X) THEN V=X
NEXT:IF V=0 THEN PRINT"What? Check your verb.":GOTO L1100
N=0:FOR X=1 TO 56:IF NE$=NO$(X) OR N$=NO$(X) THEN N=X
NEXT:IF N=0 THEN IF V<>10 AND V<>12 THEN PRINT"Huh? Check your noun.":GOTO L1100
L1200: ON V GOTO L2000,L2080,L2150,L2200,L2300,L2330,L2390,L2440,L2480,L2520,L2570,L2660,L2720,L2780,L2870,L2950,L3410,L3500,L3550
IF NC=0 THEN NC=1:GOTO L1030
GOTO L1100
REM ***** GO *****
L2000: :
IF RM=22 OR RM=81 THEN IF N=5 THEN IF LO(9)<>0 AND LO(9)<>RM THEN PRINT"You need a rope to climb up!":GOTO L1100
IF RM=65 THEN IF N=2 THEN IF LO(38)=RM THEN PRINT"The goblin blocks your path! He growls,'You cannot pass!'":GOTO L1100
IF RM=12 THEN IF N=2 THEN IF LO(39)=RM THEN PRINT"The hellhound slams you around! He screams,'DIE, KNAVE!'":TEMPO=1:GOSUB L30000:PRINT"You died.":TEMPO=2:GOSUB L30000:PRINT:GOTO L3820
IF RM=12 THEN IF N=2 OR N=3 THEN IF LT=0 THEN PRINT"It is much too dark to move in that direction!":PRINT"(You need a source of light)":GOTO L1100
IF N>6 OR N=0 OR M(RM,N)=0 THEN PRINT"You can't go that way.":GOTO L1100
RM=M(RM,N):GOTO L1000
REM ***** GET *****
L2080: :
IF N<7 OR N>35 THEN PRINT"You can't pick that up.":GOTO L1100
IF LO(N)<>0 AND LO(N)<>305 AND LO(N)<>405 AND LO(N)<>RM THEN PRINT"It's beyond your power to do that!":GOTO L1100
IF IC>8 THEN PRINT"You are carrying too much already!":GOTO L1100
IF N=20 AND LO(56)=RM THEN LO(20)=99:LO(56)=0:EX$(56)="azrael: a mischevious cat. He is asleep.":IC=IC+1:PRINT MS$:GOTO L1100
IC=IC+1:LO(N)=0:PRINT"Ok.":GOTO L1100
REM ***** DROP *****
L2150: :
IF N<7 THEN PRINT"You can't drop that.":GOTO L1100
IF LO(N)<>0 AND LO(N)<>105 AND LO(N)<>205 THEN PRINT"You don't have it.":GOTO L1100
IC=IC-1:LO(N)=RM:PRINT"Ok.":GOTO L1100
REM ***** INVENTORY *****
L2200: :
PRINT"You are carrying:"
SI=0:WD=0:AC=0:FOR X=7 TO 56:IF LO(X)=0 THEN SI=1:PRINT "   ";NO$(X);"."
IF LO(X)=105 THEN WD=X:SI=1:PRINT "   ";NO$(X);" (wielded)."
IF LO(X)=205 THEN SI=1:AC=AC+(X/4):AC=CINT(AC):PRINT "   ";NO$(X);" (worn)."
IF X=10 OR X=11 THEN IF LO(X)=0 THEN GOSUB L3320
IF X=8 AND LT=0 AND LO(8)=0 THEN PRINT"(the lantern is off)":ELSE IF X=8 AND LO(8)=0 THEN PRINT"(the lantern is aflame)"
NEXT:IF SI=0 THEN PRINT"Alas, you are empty-handed."
PRINT"(You have";HP;"hit points and";G;"gold coins).":GOTO L1100
REM ***** LOOK *****
L2300: :
GOTO L1000
REM ***** WIELD *****
L2330: :
IF N<7 OR N>35 OR N<29 THEN PRINT"You can't wield that.":GOTO L1100
IF LO(N)<>0 THEN PRINT"It's beyond your power to do that!":GOTO L1100
IF WD>0 THEN PRINT"You are already wielding something (";NO$(WD);").":GOTO L1100
WD=N:LO(N)=105:PRINT "Ok.":GOTO L1100
REM ***** UNWIELD *****
L2390: :
IF N<7 OR N>35 OR N<29 THEN PRINT"You can't unwield that.":GOTO L1100
IF LO(N)<>105 THEN PRINT"It's beyond your power to do that!":GOTO L1100
WD=0:LO(N)=0:PRINT "Ok.":GOTO L1100
REM ***** WEAR *****
L2440: :
IF N<7 OR N>35 OR LO(N)<>0 OR N<21 AND N>28 THEN PRINT"You can't wear that.":GOTO L1100
LO(N)=205:AC=AC+(N/4):AC=CINT(AC):PRINT"Ok.":GOTO L1100
REM ***** REMOVE *****
L2480: :
IF N<7 OR N>35 OR LO(N)<>0 OR N<21 AND N>28 THEN PRINT"You can't remove that.":GOTO L1100
LO(N)=0:AC=AC-(N/4):AC=CINT(AC):PRINT"Ok.":GOTO L1100
REM ***** EXAMINE *****
L2520: :
IF N=0 THEN PRINT"You notice nothing unusual about it.":GOTO L1100
IF LO(N)<>0 AND LO(N)<>RM AND LO(N)<>105 AND LO(N)<>205 THEN PRINT"That isn't here.":GOTO L1100
PRINT EX$(N):GOTO L1100
REM ***** USE *****
L2570: :
IF LO(N)<>0 AND LO(N)<>RM OR N<7 OR N>35 THEN PRINT"You can't use that!":GOTO L1100
IF N=7 THEN IF LO(8)<>0 AND LO(8)<>RM THEN PRINT"You need the lantern.":GOTO L1100
IF N=7 AND LT=1 THEN PRINT"The lantern is already on.":GOTO L1100
IF N=8 THEN IF LO(7)<>0 AND LO(7)<>RM THEN PRINT"You need the flask of oil.":GOTO L1100
IF N=8 AND LT=1 THEN PRINT"The lantern is already on.":GOTO L1100
IF N=8 THEN LT=1:PRINT"The lantern is now aflame.":GOTO L1100
GOTO L3860
REM ***** CLIMB *****
L2660: :
IF RM<>22 AND RM<>81 OR N>0 THEN PRINT"You can't climb that.":GOTO L1100
IF RM=22 THEN IF LO(9)=0 OR LO(9)=RM THEN RM=23:PRINT"You climb up...":GOTO L1000
IF RM=81 THEN IF LO(9)=0 OR LO(9)=RM THEN RM=82:PRINT"You climb up...":GOTO L1000
PRINT"You need the rope first.":GOTO L1100
REM ***** READ *****
L2720: :
IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"You can't read that!":GOTO L1100
IF N=17 THEN PRINT"The book reads (in part):":GOTO L3950
IF N=19 THEN PRINT"It is written in an unfamiliar tongue.":GOTO L1100
PRINT"You read it with little interest.":GOTO L1100
REM ***** BUY *****
L2780: :
IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO L1100
IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO L1100
IF LO(N)<>98 THEN PRINT"The clerk says,'We don't carry that.'":GOTO L1100
IF IC>=8 THEN PRINT"The clerk says,'You can't carry that.'":GOTO L1100
IF P(N)=99 THEN PRINT"The clerk says,'You can't buy that.'":GOTO L1100
IF P(N)>G THEN PRINT"The clerk says,'You don't have enough gold.'":GOTO L1100
G=G-P(N):LO(N)=0:IC=IC+1:PRINT"You hand clerk";P(N);"gold coins.":PRINT"He hands you the ";NO$(N);".":PRINT"He says,'Thank you for your business.'":GOTO L1100
REM ***** SELL *****
L2870: :
IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO L1100
IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO L1100
IF LO(N)=105 THEN PRINT"The clerk says,'You must unwield that first.'":GOTO L1100
IF LO(N)=205 THEN PRINT"The clerk says,'You must remove that first.'":GOTO L1100
IF LO(N)<>0 THEN PRINT"The clerk says,'I don't see you carrying that.'":GOTO L1100
LO(N)=98:CG=P(N)/2:G=G+CG:IC=IC-1:PRINT"You hand clerk the ";NO$(N);".":PRINT"He hands you";CG;"gold coins.":PRINT"He says,'Thank you for your business.'":GOTO L1100
REM ***** FIGHT *****
L2950: :
IF N<36 THEN PRINT"You can't fight that!":GOTO L1100
IF RM>72 AND RM<84 THEN PRINT"You can't fight here. This is a sacred place.":GOTO L1100
IF LO(N)<>RM THEN PRINT"The ";NO$(N);" isn't here.":GOTO L1100:ELSE RANDOMIZE TIMER:MH=INT(RND*150)+50:IF N=50 THEN MH=820
L2980: RANDOMIZE TIMER:I=INT(RND*35)+1:PRINT"You are fighting the "NO$(N);".":PRINT">"
DM=1:FOR X=7 TO 35:IF LO(X)=105 THEN DM=X
NEXT:IF DM=1 THEN PRINT"You are wielding nothing!":ELSE PRINT"You are wielding the ";NO$(DM);"."
PRINT">"
IF I<=5 THEN PRINT"You missed."
IF I>=5 AND I<=10 THEN PRINT"You hit ";NO$(N);".":MH=MH-10:ELSE IF I>=15 AND I<=20 OR I>=20 AND I<=25 THEN IF DM=31 OR DM>32 AND DM<36 THEN GOSUB L3250
IF I>=10 AND I<=15 THEN PRINT"You hit ";NO$(N);" very hard.":MH=MH-15:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-10
IF I>=15 AND I<=20 THEN PRINT"You smashed ";NO$(N);" with a bone-crushing sound.":MH=MH-30:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-15
IF I>=20 AND I<=25 THEN PRINT"You massacred ";NO$(N);" into small fragments.":MH=MH-40:IF DM=31 OR DM>32 AND DM<36 THEN MH=MH-DM
IF I=26 THEN IF DM=31 THEN PRINT"A bolt of lightning streaks down from above!":PRINT"Your BROADSWORD strikes ";NO$(N);" down!":MH=0
IF I=27 THEN IF DM=33 THEN PRINT"Your GLOWBALL strikes ";NO$(N);" very hard!":MH=MH-55
IF I=28 THEN IF DM=34 THEN PRINT"Your SCEPTER shoots flame at ";NO$(N);"!":MH=MH-65
IF I=29 THEN IF DM=35 THEN PRINT"Your SLAYER leaps from your hands! It impales ";NO$(N);"!":MH=MH-75
IF I>=30 THEN PRINT"Your attack was blocked by ";NO$(N);".":ELSE IF LO(19)=0 THEN IF I>=30 THEN PRINT"You cast HEAL...":PRINT"You are healed fully!":HP=192
TEMPO=1:GOSUB L30000:PRINT">":PRINT"The monster ";NO$(N);" is attacking.":TEMPO=1:GOSUB L30000:PRINT">":IF I=32 AND LO(19)=0 THEN PRINT"You cast FIRE-BALL...":PRINT FB$:PRINT FB2$:MH=0
RANDOMIZE TIMER:C=INT(RND*35)+1:IF C<=5 THEN PRINT"It missed you."
IF C>=5 AND C<=10 THEN PRINT"It hit you.":DT=7:HP=HP-DT:IF AC>DT THEN HP=HP+2
IF C>=10 AND C<=15 THEN PRINT"It hit you very hard.":DT=12:HP=HP-DT:IF AC>DT THEN HP=HP+4
IF C>=15 AND C<=20 THEN PRINT"It smashed you with a bone-crushing sound.":DT=20:HP=HP-DT:IF AC>DT THEN HP=HP+8
IF C>=20 AND C<=25 THEN PRINT"It massacred you into small fragments.":DT=40:HP=HP-DT:IF AC>DT THEN HP=HP+15
IF I=26 THEN IF N=50 THEN PRINT"A bolt of lightning streaks down from the heavens!":PRINT"You are fried to death!":TEMPO=1:GOSUB L30000:PRINT"You died.":TEMPO=1:GOSUB L30000:GOTO L3820
PRINT">":TEMPO=1:GOSUB L30000:IF MH<=0 THEN PRINT"The monster died.":PRINT"You killed ";NO$(N);".":TEMPO=1:GOSUB L30000:PRINT">":GOTO L3220
PRINT"Your HP:";HP:PRINT"Their HP:";MH:PRINT">":IF HP<=0 THEN PRINT"You died.":PRINT:TEMPO=1:GOSUB L30000:GOTO L3820
GOTO L2980
L3220: FOR X=7 TO 35:IF LO(X)=1000+N THEN PRINT"You found ";NO$(X);" on it!":LO(X)=RM
NEXT:PRINT"You gained";DT;"gold pieces and";N;"hit points.":HP=HP+N:G=G+DT:LO(N)=998:GOSUB L4850:GOTO L1100
END
L3250: IF DM=31 THEN PRINT"Your BROADSWORD glows!"
IF DM=33 THEN PRINT"Your GLOWBALL splits into eight pieces!"
IF DM=34 THEN PRINT"Your SCEPTER shoots lightning at ";NO$(N);"!"
IF DM=35 THEN PRINT"Your SLAYER vibrates!"
RETURN
END
REM ***** Check for items in knapsack/backpack *****
L3320: IF X=11 THEN GOTO L3370
PRINT"      The knapsack holds:"
S1=0:FOR Y=7 TO 35:IF LO(Y)=305 THEN S1=1:PRINT"        (";NO$(Y);")."
NEXT:IF S1=0 THEN PRINT"        (Nothing)"
RETURN
L3370: S1=0:PRINT"      The backpack holds:":FOR Y=7 TO 35:IF LO(Y)=405 THEN S1=1:PRINT"        (";NO$(Y);")."
NEXT:IF S1=0 THEN PRINT"        (Nothing)"
RETURN
REM ***** Put *****
L3410: :
IF LO(N)<>0 THEN PRINT"You must be carrying that first!":GOTO L1100
IF LO(10)<>0 AND LO(11)<>0 AND LO(10)<>RM AND LO(11)<>RM THEN PRINT"You aren't carrying the knapsack or backpack!":GOTO L1100
TL=0:IF NE2$="knapsack" THEN TL=10
IF NE2$="backpack" THEN TL=11
IF TL=0 THEN PRINT"You can't place it there.":GOTO L1100
IF TL=10 THEN LO(N)=305:IC=IC-1:PRINT"Ok.":GOTO L1100
LO(N)=405:IC=IC-1:PRINT"Ok.":GOTO L1100
REM ***** Eat *****
L3500: :
IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"That isn't here.":GOTO L1100
IF N=13 THEN HP=192:PRINT"You eat the food.":PRINT"You feel much better!":LO(13)=1:IC=IC-1:GOTO L1100
PRINT"You can't eat that.":GOTO L1100
REM ***** Drink *****
L3550: :
IF LO(N)<>0 AND LO(N)<>RM THEN PRINT"That isn't here.":GOTO L1100
IF N=14 THEN IF LO(16)<>0 AND LO(16)<>RM THEN PRINT"You need the glass bottle first.":GOTO L1100
IF N=14 THEN LO(14)=5:IC=IC-1:CT=0:PRINT"Ahhhh! Refreshing!":GOTO L1100
IF N=15 THEN LO(15)=1:LO(16)=0:PRINT"You drink the wine.":PRINT"It tastes great!":GOTO L1100
PRINT"You can't drink that.":GOTO L1100
REM ***** check for thirst *****
L3620: :
IF CT=50 THEN PRINT"You are thirsty."
IF CT=100 THEN PRINT"You are very thirsty."
IF CT=150 THEN PRINT"You have died of thirst.":TEMPO=1:GOSUB L30000:PRINT"You died.":TEMPO=2:GOSUB L30000:GOTO L3820
RETURN
L3670: PRINT"This is a text adventure. You play by entering in one or two"
PRINT"word commands (e.g. go north, get food, etc.). Valid commands"
PRINT"include:":PRINT
PRINT"1. go 2. get 3. drop 4. inventory 5. look 6. wield 7. unwield"
PRINT"8. wear 9. remove 10. examine 11. use 12. climb 13. read"
PRINT"14. buy 15. sell 16. kill 17. put 18. eat 19. drink"
PRINT"20. inventory 21. save game (or just 'save') 22. look"
PRINT"23. quit 24. help":PRINT:GOTO L1100
REM ***** list command *****
L3760: IF RM<>3 THEN PRINT"You are not in the village shop!":GOTO L1100
IF LO(37)<>RM THEN PRINT"The clerk isn't here.":GOTO L1100
PRINT"The clerk says,'Here is what we have in stock:"
SI=0:FOR X=7 TO 35:IF LO(X)=98 THEN SI=1:PRINT P(X);": ";NO$(X)
NEXT:IF SI=0 THEN PRINT"He scratches his head and says,'Alas, we have nothing in stock...'"
PRINT"Your gold: ";G:GOTO L1100
L3820: PRINT "@@@ Press any key @@@"
L3830: A$=INKEY$:IF A$="" THEN GOTO L3830
CLS:RUN
REM ***** Use (continued) *****
L3860: IF N=9 THEN IF RM=22 THEN RM=23:PRINT"You climb up...":GOTO L1000
IF N=9 THEN IF RM=81 THEN RM=82:PRINT"You climb up...":GOTO L1000
IF N=12 THEN IF RM=22 THEN RM=23:PRINT"You climb up (on the pole)...":GOTO L1000
IF N=12 THEN IF RM=81 THEN RM=82:PRINT"You climb up (on the pole)...":GOTO L1000
IF N=13 THEN PRINT"You must use 'eat' instead.":GOTO L1100
IF N=14 OR N=15 OR N=16 THEN PRINT"You must use 'drink' instead.":GOTO L1100
IF N=17 THEN PRINT"You must use 'read' instead.":GOTO L1100
IF N=19 THEN PRINT"You muse use 'read' instead.":GOTO L1100
PRINT"You can't use that here.":GOTO L1100
L3950: :
PRINT"'...to defeat mordimar, you must be wielding the broadsword. It is the only"
PRINT"weapon which can work against his black magic. The others will not damage"
PRINT"him enough. I have yet to acquire it, but someday I shall finally defeat"
PRINT"him! - Tursk'":GOTO L1100
REM ***** Check for other verbs *****
L4010: GOSUB L3620:IF A$="help" OR A$="hint" THEN GOSUB L3670:ELSE IF A$="list" THEN GOTO L3760
IF A$="go north" OR A$="n" OR A$="north" THEN V=1:N=1:CO=0:GOTO L1200
IF A$="go south" OR A$="s" OR A$="south" THEN V=1:N=2:CO=0:GOTO L1200
IF A$="go east" OR A$="e" OR A$="east" THEN V=1:N=3:CO=0:GOTO L1200
IF A$="go west" OR A$="w" OR A$="west" THEN V=1:N=4:CO=0:GOTO L1200
IF A$="go up" OR A$="u" OR A$="up" THEN V=1:N=5:CO=0:GOTO L1200
IF A$="go down" OR A$="d" OR A$="down" THEN V=1:N=6:CO=0:GOTO L1200
IF A$="inventory" OR A$="i" OR A$="inv" THEN V=4:N=56:CO=0:GOTO L1200
IF A$="save" OR A$="save game" THEN GOTO L4380
IF A$="quit" THEN PRINT:PRINT"Quit Game":PRINT:GOTO L4140
IF A$="look" OR A$="l" THEN V=5:N=56:CO=0:GOTO L1200
IF A$="map" THEN GOSUB L4520:GOTO L1200
RETURN
REM ***** QUIT GAME *****
L4140: PRINT"Are you sure (y/n)? ";
L4150: B$=INKEY$:IF B$="" THEN GOTO L4150
IF B$="y" THEN PRINT B$:PRINT:PRINT"Ok...thanks for playing!":TEMPO=2:GOSUB L30000:END
IF B$="n" THEN PRINT B$:PRINT:GOTO L1100
GOTO L4150
REM ***** LOAD A GAME *****
L4330: PRINT:PRINT"Loading...";:TEMPO=1:GOSUB L30000
ON ERROR GOTO L40000
OPEN "ADVXT.SAV" FOR INPUT AS #1
INPUT#1,G: INPUT#1,IC: INPUT#1,HP: INPUT#1,RM: INPUT#1,AC: INPUT#1,LT: INPUT#1,WD: INPUT #1,DM
FOR X=7 TO 56:INPUT#1,LO(X):NEXT:CLOSE #1:PRINT"Done.":TEMPO=1:GOSUB L30000
GOTO L1000
L4380: PRINT"Saving...";:TEMPO=1:GOSUB L30000
OPEN "ADVXT.SAV" FOR OUTPUT AS #1
PRINT#1,G: PRINT#1,IC: PRINT#1,HP: PRINT#1,RM: PRINT#1,AC: PRINT#1,LT: PRINT#1,WD: PRINT #1,DM
FOR X=7 TO 56:PRINT#1,LO(X):NEXT:CLOSE #1:PRINT"Done.":TEMPO=1:GOSUB L30000
GOTO L1100
END
L4440: PRINT"Obvious exits: < /";
IF M(RM,1)>0 THEN PRINT"north/ ";
IF M(RM,2)>0 THEN PRINT"south/ ";
IF M(RM,3)>0 THEN PRINT"east/ ";
IF M(RM,4)>0 THEN PRINT"west/ ";
IF M(RM,5)>0 THEN PRINT"up/ ";
IF M(RM,6)>0 THEN PRINT"down/ ";
PRINT">":RETURN
L4520: CLS:PRINT"Map of Adventure XT                    Hall2--Room4                            "
PRINT"                                      /                        23/branch--hut24"
PRINT"           8Toolshed--7Garden   Tavern1      26RWF---RWF25  22RWF              "
PRINT"                         |        |                   |        |               "
PRINT"                      6Church--Fountain5--Shop3    19RWF----20RWF--RWF21       "
PRINT"                                  |                            |     47        "
PRINT"                     27BF      Village Ent9                Clearing18 T-DE48   "
PRINT"                  28 29 |  30     |                   16    17 |   43 |44 45 46"
PRINT"                   BF--BF--BF     |  Pond15 River---River DE-T-T-Keep          "
PRINT"                    |      |      |10   13  14 |      |       39  40  |41      "
PRINT"             31BF--BF32    BF11--On--Meadow--Mead.  River  Tunnel--T--T-DE42   "
PRINT"                           |     |     34  35  36     | 37   |                 "
PRINT"                         33BF--Inter12-DF--DF--DF---River--Cave38              "
PRINT"                                 |         |   |51                             "
PRINT"                               Bridge49  50DF--DF---Alcove52  53Kitchen  RM54  "
PRINT"                                 |             |    57  58  59  60 |    61|62  "
PRINT"                               Bridge55      56DF---LF--LF--LF--Cottage-H-H-R63"
PRINT"                                 |                   |   |67 68                "
PRINT"      Mordimar64               Goblin65    [inset]  LF66LF--LF--LF69           "
PRINT"           |        71       72  |           |               |                 "
PRINT"    70Throne RM--Fountain--Shadow Castle   SNW73         Small Path74          "
PRINT"                                             |      76       |                 "
PRINT"                                         75SB P--SB Patch--Path77              "
L4745: A$=INKEY$:IF A$="" THEN GOTO L4745
PRINT"--------------------------*INSET*----------------------                        "
PRINT"|                Brainy82                             |                        "
PRINT"|                   |  81          83           84    |                        "
PRINT"|    79Jokey-----SM Forest--Outside G Castle--G Castle|                        "
PRINT"|        |                                            |                        "
PRINT"|    78Papa--Handy80                                  |                        "
PRINT"|        |                                            |                        "
PRINT"|    [to SNW]                                         |                        "
PRINT"-------------------------------------------------------                        "
GOTO L1100
L4850: IF N=50 THEN GOTO L4860:ELSE RETURN
L4860: PRINT:PRINT"@@@ You have defeated Mordimar!!! @@@":TEMPO=2:GOSUB L30000:PRINT
PRINT"Mordimar's crumbled body lay at your feet. As you"
PRINT"gaze into the Orb, you sense the world returning to"
PRINT"normal! The woodland creatures send great happiness"
PRINT"to your heart! It swells with joy!":PRINT
PRINT"'You have done a great deed!' their voices say."
PRINT"'Now we can come out and play!'":PRINT
PRINT"The world thanks you for your courage...":PRINT
PRINT"@@@ The End @@@":TEMPO=20:GOSUB L30000:PRINT:PRINT"@@@ Press any key @@@"
L4950: A$=INKEY$:IF A$="" THEN GOTO L4950
END
L10000: REM ***** ROOM 1 *****
PRINT"You are standing in a tavern with creaky wooden floors. A few tables"
PRINT"line the room, with flickering candles set in the middle, illuminating"
PRINT"the area. To the south (outside) stands a large fountain.":RETURN
L10040: REM ***** ROOM 2 *****
PRINT"You are standing on an upstairs hallway in the tavern. A lone room"
PRINT"stands to the east, unoccupied. The only exit is back down the"
PRINT"staircase.":RETURN
L10080: REM ***** ROOM 3 *****
PRINT"You are standing in the village shop. People come here to purchase"
PRINT"needed goods and sell unwanted items. Commands here include 'buy <item>',"
PRINT"'sell <item>' and 'list'. The shop is usually attended by a clerk.":RETURN
L10120: REM ***** ROOM 4 *****
PRINT"You are standing in a small room upstairs of the main tavern. A small"
PRINT"bed is visible here, allowing you to rest comfortably. The only exit"
PRINT"is back west.":RETURN
L10160: REM ***** ROOM 5 *****
PRINT"You are standing near a large fountain. Looking down at your reflection,"
PRINT"you notice the water appears mossy green. However, despite the color"
PRINT"it appears drinkable. To the east stands a small shop, while west of"
PRINT"here lies a church.":RETURN
L10210: REM ***** ROOM 6 *****
PRINT"You are standing in a small church. Pews adorn the room, with an altar"
PRINT"at the front. To the north (outside) a garden is visible. The only"
PRINT"other exit is back east.":RETURN
L10250: REM ***** ROOM 7 *****
PRINT"You are standing in a lovely garden. Several different flowers dot"
PRINT"the area, including lillies and begonias. To the west stands a"
PRINT"wooden toolshed.":RETURN
L10290: REM ***** ROOM 8 *****
PRINT"You are standing inside a darkened toolshed. A few items of interest"
PRINT"line the walls, but not much else is here. The only exit is back"
PRINT"east.":RETURN
L10330: REM ***** ROOM 9 *****
PRINT"You are standing at the entrance to a small village. A path heads south"
PRINT"from here into a well-lit forest.":RETURN
L10360: REM ***** ROOM 10 *****
PRINT"You are on a well-lit path walking through a forest. The area is"
PRINT"populated by many small creatures (scurrying about). The path"
PRINT"continues south.":RETURN
L10400: REM ***** ROOM 11 *****
PRINT"You are on a well-lit path walking through a forest. A lush green meadow"
PRINT"stands to the east (near a small pond). An intersection is south of"
PRINT"here.":RETURN
L10440: REM ***** ROOM 12 *****
PRINT"You are standing at an intersection in the forest. A dying woodland is"
PRINT"west of here, the trees charred black with soot. To the east stands a"
PRINT"darker section of the forest, while a long bridge is south.":RETURN
L10480: REM ***** ROOM 13 *****
PRINT"You are walking through a lush, green meadow. It opens up here and"
PRINT"continues for miles on end (to the east). To the west, a large path"
PRINT"cuts through a well-lit forest.":RETURN
L10520: REM ***** ROOM 14 *****
PRINT"You are in a lush, green meadow. To the north lies a small pond. Small"
PRINT"clouds drift on by, pushed by a gentle breeze. The only obvious exit"
PRINT"is back west.":RETURN
L10560: REM ***** ROOM 15 *****
PRINT"You are standing next to a small pond. The water appears mossy green"
PRINT"and devoid of life. Although you could try drinking the water, it does"
PRINT"not appear healthy. The only exit is back south.":RETURN
L10600: REM ***** ROOM 16 *****
PRINT"You are knee deep in a swiftly moving river. The water feels icy cold!"
PRINT"The river continues to the east, while less hurried currents are directly"
PRINT"south.":RETURN
L10640: REM ***** ROOM 17 *****
PRINT"You have reached the northern bank of a swiftly moving river. On the other"
PRINT"side of the water rises a tall Redwood forest (to the north).":RETURN
L10670: REM ***** ROOM 18 *****
PRINT"You are standing in a small clearing. There is nothing around but"
PRINT"blue sky.":RETURN
L10700: REM ***** ROOM 19 *****
PRINT"You are walking in a Redwood forest, the trees of which rise hundreds of"
PRINT"feet into the air. To the east stands a well-lit section of the forest,"
PRINT"while west of here the area becomes dark and oppressive.":RETURN
L10740: REM ***** ROOM 20  *****
PRINT"You are standing in a Redwood forest, the trees of which rise hundreds of"
PRINT"feet into the air. A large clearing is south, while one particular"
PRINT"tree -- with low-lying branches, stands to the north. It appears"
PRINT"climbable.":RETURN
L10800: REM ***** ROOM 21 *****
PRINT"The Redwood forest ends here next to a large, purple mountain range."
PRINT"The only obvious exit is back west.":RETURN
L10830: REM ***** ROOM 22 *****
PRINT"You are walking in a Redwood forest. A lone tree (with low-lying"
PRINT"branches) is here, as a poorly made ladder heads up into the treetops"
PRINT"above. The only obvious exit is back south.":RETURN
L10870: REM ***** ROOM 23 *****
PRINT"You are atop the forest, standing on a long branch. A small hut (with a"
PRINT"thatched roof) stands to the east near the top of the tree. Looking in,"
PRINT"you can see a small creature, apparently meditating. Perhaps it would"
PRINT"be best not to disturb him.":RETURN
L10920: REM ***** ROOM 24 *****
PRINT"You are standing inside a small hut. A small creature is here, apparently"
PRINT"meditating. He stops suddenly, noticing your presence, but this does"
PRINT"not seem to alarm him. He smiles at you, gestering to a nearby table"
PRINT"(next to a wooden bookcase). Perhaps you should sit down and"
PRINT"listen to what he has to say.":RETURN
L10980: REM ***** ROOM 25 *****
PRINT"You are walking in a Redwood forest. The trees thin out some to the"
PRINT"west. To the south, the trees are much taller, as the area is populated"
PRINT"with woodland creatures.":RETURN
L11020: REM ***** ROOM 26 *****
PRINT"You have reached the end of the Redwood forest. The only obvious exit"
PRINT"is back east. Further travel west is made impossible by majestic purple"
PRINT"mountains.":RETURN
L11060: REM ***** ROOM 27 *****
PRINT"You have reached the end of the burnt forest. The trees are charred"
PRINT"black with soot (no doubt the remnants from a once raging wildfire)."
PRINT"The only obvious exit is back south.":RETURN
L11100: REM ***** ROOM 28 *****
PRINT"You are walking in a burnt forest. The trees have been charred black"
PRINT"from a recent wildfire. East of here the trees appear larger and"
PRINT"more stable, while west of here the woods thin out somewhat.":RETURN
L11140: REM ***** ROOM 29 *****
PRINT"You are standing in the middle of a large, burnt forest. The trees"
PRINT"have been charred black from a recent fire. To the north, the forest"
PRINT"stops abruptly, while to the east and west the forest continues.":RETURN
L11180: REM ***** ROOM 30 *****
PRINT"You are walking in a burnt forest. The trees are particularly blackened"
PRINT"here. This must have been where the first started, as the surrounding"
PRINT"area is charred black with soot. Additionally, several telltale signs"
PRINT"of a lightning strike are evident nearby, suggesting the cause of"
PRINT"the blaze.":RETURN
L11240: REM ***** ROOM 31 *****
PRINT"You have reached the end of the burnt forest. Further travel west and"
PRINT"north becomes impossible by fallen trees.":RETURN
L11270: REM ***** ROOM 32 *****
PRINT"You are walking in a burnt forest, the trees of which appear blackened"
PRINT"by soot from a recent wildfire. To the north, the trees appear to thin"
PRINT"out somewhat, while fallen trees obstruct further travel to the west.":RETURN
L11310: REM ***** ROOM 33 *****
PRINT"You have reached a burnt forest, the trees of which appear blackened"
PRINT"with soot from a recent fire. To the north, the trees appear to thicken"
PRINT"considerably, while east of here lies an intersection.":RETURN
L11350: REM ***** ROOM 34 *****
PRINT"You have reached a dark forest. The trees appear to be evergreens,"
PRINT"shrouding most of the floor, while speckles of light filter down from"
PRINT"the thick canopy above. To the east, the forest appears to thin out"
PRINT"somewhat, while west of here lies an intersection.":RETURN
L11400: REM ***** ROOM 35 *****
PRINT"You are walking in a dark forest. The trees are particularly dense here, as"
PRINT"very little light filters down from below. There are several different"
PRINT"woodland creatures here (all scurrying about), including squirrels, rabbits,"
PRINT"foxes and birds. The forest continues east and south.":RETURN
L11450: REM ***** ROOM 36 *****
PRINT"You are walking in a dark forest. The trees thin out here near a large"
PRINT"river (to the east), while west of here appears to be the middle of"
PRINT"the forest. South of here the forest continues to thin out.":RETURN
L11490: REM ***** ROOM 37 *****
PRINT"You are standing at the bank of a swiftly moving river. Although it does"
PRINT"not appear too deep, caution should nonetheless be taken. The forest"
PRINT"continues west, while east of here a cave is visible, nestled near a"
PRINT"small alcove (to the southwest).":RETURN
L11540: REM ***** ROOM 38 *****
PRINT"You have entered into a dark cave. Very little is visible here, but you"
PRINT"do notice several glowing red lights, all wisping about endlessly. It is almost"
PRINT"as if the cave is haunted by evil spirits! The cave begins to widen some to"
PRINT"the north.":RETURN
L11590: REM ***** ROOM 39 *****
PRINT"You have reached a large tunnel within the cave, splitting off to the east"
PRINT"here (accessible through a hole). As the tunnel progresses, it appears to"
PRINT"narrow considerably. You might only be able to squeeze through by dropping"
PRINT"several items.":RETURN
L11640: REM ***** ROOM 40 *****
PRINT"You are crawling on your belly in a narrow tunnel. It heads to the east here,"
PRINT"turning sharply. Moans from disembodied spirits can be heard, getting louder"
PRINT"the deeper you go! The only obvious exit is back west.":RETURN
L11680: REM ***** ROOM 41 *****
PRINT"You have reached a small intersection within the cave. Tunnels head off in"
PRINT"all directions here, while the main tunnel continues north.":RETURN
L11710: REM ***** ROOM 42 *****
PRINT"The tunnel stops here near a large rock. The only exit is back west.":RETURN
L11730: REM ***** ROOM 43 *****
PRINT"You have reached a dead end within the cavern. The only exit is back east.":RETURN
L11750: REM ***** ROOM 44 *****
PRINT"You are crawling on your belly in the heart of the cave. The main tunnel"
PRINT"branches off here in four directions: north, south, east and west. One"
PRINT"particular tunnel (to the east) appears to glow softly with a pale"
PRINT"green hue. Perhaps it would be worth exploring further.":RETURN
L11800: REM ***** ROOM 45 *****
PRINT"You are crawling on your belly in a small tunnel. The area is surrounded"
PRINT"by pale green light, glowing softly. The air feels magical, as if the"
PRINT"light particles themselves are alive!":RETURN
L11840: REM ***** ROOM 46 *****
PRINT"You have reached a small keep within the cave. Several large candles,"
PRINT"glowing softly green, illuminate the darkness. This small room appears"
PRINT"to be a place of meditation and worship. Perhaps you should rest here"
PRINT"and learn more about this most unique room.":RETURN
L11890: REM ***** ROOM 47 *****
PRINT"You are crawling on your belly in the main tunnel within the cave. To the"
PRINT"south you notice a pale green light, glowing near a smaller tunnel towards"
PRINT"the southeast. To the east, the main tunnel forks off into a much smaller"
PRINT"one. It appears almost too narrow to pass through.":RETURN
L11940: REM ***** ROOM 48 *****
PRINT"You have reached a dead end within the cavern, as the main tunnel"
PRINT"forks off here into a very small passageway. However, a large"
PRINT"boulder has crashed through the tunnel from above, blocking further"
PRINT"passage east. The only exit is back west.":RETURN
L11990: REM ***** ROOM 49 *****
PRINT"You are standing on a wooden bridge just south of a large intersection."
PRINT"It appears to cross over a swiftly moving river (heading northeast). At"
PRINT"the end of the bridge you notice a large, stone castle.":RETURN
L12030: REM ***** ROOM 50 *****
PRINT"You are walking in a dark forest. The trees thicken somewhat to the east, as"
PRINT"very little light filters down from below. There are several woodland creatures"
PRINT"scurrying about, making you feel restless. The forest continues north and east.":RETURN
L12070: REM ***** ROOM 51 *****
PRINT"You are walking in the middle of a dark forest. The trees are particularly"
PRINT"thick here, as a plethora of woodland creatures scurry about the forest floor,"
PRINT"looking for food. To the east stands a secluded alcove, while north the forest"
PRINT"continues. Further travel south becomes increasingly difficult, but not"
PRINT"impossible.":RETURN
L12130: REM ***** ROOM 52 *****
PRINT"You have reached a secluded alcove, hidden within the forest. Several magical"
PRINT"creatures populate the area, including unicorns and one-eyed beasts. It appears"
PRINT"to be a land that time forgot, for these beings are only written about in"
PRINT"fairy tales! The only exit is back west.":RETURN
L12180: REM ***** ROOM 53 *****
PRINT"You are standing in a small kitchen within the cottage. A large wooden table"
PRINT"is set in the center, surrounded by chairs. To the south lies the main living"
PRINT"room, as a narrow hallway heads into several small rooms.":RETURN
L12220: REM ***** ROOM 54 *****
PRINT"You have entered into a small room within the cottage. Not much can be seen"
PRINT"here save for a bed and a small table. The only exit is back south.":RETURN
L12250: REM ***** ROOM 55 *****
PRINT"You are standing in the middle of a large bridge. Underneath, a river flows"
PRINT"to the northeast. To the south, the bridge stops abruptly near a large,"
PRINT"stone castle. Rumor has it that a goblin periodically appears, demanding"
PRINT"money by all who cross here. You wonder if the goblin will stop you, too!":RETURN
L12300: REM ***** ROOM 56 *****
PRINT"You have reached the limits of the dark forest. A well-lit section of"
PRINT"the forest opens up here, dramatically improving visibility. Far to the"
PRINT"east a cottage can be seen, smoke rising from a chimney high above.":RETURN
L12340: REM ***** ROOM 57 *****
PRINT"You are standing in a well-lit forest. Beams of light filter down from"
PRINT"the canopy above, dotting the forest floor. The forest continues to the"
PRINT"east and south, while west of here a dark forest is visible.":RETURN
L12380: REM ***** ROOM 58 *****
PRINT"You are standing in the middle of a well-lit forest. Beams of light filter down"
PRINT"from high above, dotting the forest floor. A particularly well-lit path, almost"
PRINT"magical in appearance, shimmers to the south, while east of here the forest"
PRINT"continues towards a large cottage.":RETURN
L12430: REM ***** ROOM 59 *****
PRINT"You are standing in a well-lit forest next to a large cottage (east). The"
PRINT"forest begins to thin out here somewhat, as the cottage dominates the eastern"
PRINT"edge of the woods (with the door wide open). Could someone be expecting you?":RETURN
L12470: REM ***** ROOM 60 *****
PRINT"You are standing inside a large cottage within the forest. A fireplace is here,"
PRINT"allowing you to warm your feet. Oddly enough, no one appears to have occupied"
PRINT"this abode for quite some time, even though a fire is ominously burning"
PRINT"neatly in a small fireplace. 'Could this place be haunted?' you wonder aloud."
PRINT"A hallway is east, while a kitchen is directly north.":RETURN
L12530: REM ***** ROOM 61 *****
PRINT"You are walking down a long hallway within the cottage. Several rooms are"
PRINT"visible here, some closed by locked doors (and others open). The hallway"
PRINT"continues east.":RETURN
L12570: REM ***** ROOM 62 *****
PRINT"You are walking down a long hallway within the cottage. Two rooms are"
PRINT"plainly visible up ahead (to the north and east), both of them left"
PRINT"wide open by the previous occupants.":RETURN
L12610: REM ***** ROOM 63 *****
PRINT"You have reached a small room within the cottage. Not much can be seen"
PRINT"here, save for a small bed next to a wooden dresser. The only exit is"
PRINT"back west.":RETURN
L12650: REM ***** ROOM 64 *****
PRINT"You have reached the inner sanctum of Mordimar, evil of the ancients! You"
PRINT"sense great power! He cackles at you with an insane laugh, yelling,'You"
PRINT"fool! You can't possibly hope to defeat me! I am Mordimar, master of the"
PRINT"ancient circle of wizards! I own this world! Who are YOU to challenge"
PRINT"otherwise?! Bah! So be it! I will crush you like a bug! Die, knave!'":RETURN
L12710: REM ***** ROOM 65 *****
PRINT"At the end of the bridge, a small goblin is said to often appear,"
PRINT"demanding gold. To the south, just beyond the bridge, stands a tall"
PRINT"stone castle. A pair of red flags, with gryphons emblazoned on"
PRINT"them, wave to you in a stiff breeze, somehow beckoning you inside!":RETURN
L12760: REM ***** ROOM 66 *****
PRINT"You are walking in a well-lit forest. Light filters down from the canopy"
PRINT"above, dotting the forest floor. Several woodland creatures scurry about,"
PRINT"some of them unaware of your presence. Fallen trees block passage to the"
PRINT"south. The only obvious exit is back north.":RETURN
L12810: REM ***** ROOM 67 *****
PRINT"You are standing in the middle of a well-lit forest. The path turns east"
PRINT"here near a large patch of strange red berries. They look too small for"
PRINT"humans to eat, but it looks as though the tiny woodland creatures eat"
PRINT"them regularly. To the north, the forest thickens somewhat.":RETURN
L12860: REM ***** ROOM 68 *****
PRINT"You have reached a particularly dark section of the forest, more secluded than"
PRINT"the rest. To the south, a small path heads past a series of red berry patches."
PRINT"Whatever lives in these woods must be fairly tiny, for the berries themselves"
PRINT"are all scattered about the forest floor.":RETURN
L12910: REM ***** ROOM 69 *****
PRINT"You have reached the limits of the forest, as storm-tossed trees block your"
PRINT"path to the east. The only obvious exit is back west.":RETURN
L12940: REM ***** ROOM 70 *****
PRINT"You have entered into the throne room of Mordimar. Strangely, he does not"
PRINT"appear to be here, although you DO sense a strong energy to the north! The"
PRINT"throne is made of solid gold and encrusted with several jewels. To the"
PRINT"east lies a stone fountain.":RETURN
L12990: REM ***** ROOM 71 *****
PRINT"You are standing in the castle courtyard. A large stone fountain, with the"
PRINT"statue of an angel, pours water down from an upheld jar into a motted basin"
PRINT"below. A throne room is visible to the west, while the castle entrance is"
PRINT"east.":RETURN
L13040: REM ***** ROOM 72 *****
PRINT"You have reached a tall, stone castle known as Shadow Castle. Legend has it"
PRINT"that the evil wizard Mordimar took the castle by storm many moons ago. High"
PRINT"above, a pair of flags, with gryphons emblazoned on them, flap endlessly in"
PRINT"a cool breeze. They appear to be beckoning you further into the castle! A"
PRINT"courtyard, complete with a fountain, stands to the west of here.":RETURN
L13100: REM ***** ROOM 73 *****
PRINT"You have somehow entered into a strange, new world just beyond the red berry"
PRINT"patch. Although it hardly seems possible, the entire area is populated by"
PRINT"dozens of tiny blue creatures not more than three apples high! You are"
PRINT"stunned by their speech, as they all seem to be speaking in a bizarre, elven"
PRINT"tongue, the likes of which you cannot make out. To the north, mushroom"
PRINT"houses are visible. This is where the blue creatures apparently live and"
PRINT"work.":RETURN
L13180: REM ***** ROOM 74 *****
PRINT"You have found a small path separate from the rest of the forest. It turns"
PRINT"to the south, heading west into a large red berry patch. A well-lit forest"
PRINT"is clearly visible (to the north).":RETURN
L13220: REM ***** ROOM 75 *****
PRINT"You are walking through a red berry patch. These berries are quite odd, as"
PRINT"only the bottom portion of the patch is scattered about the ground. It"
PRINT"appears as though whomever eats of these berries can't be more than"
PRINT"three apples high. To the north, a small village of mushroom houses"
PRINT"is plainly visible.":RETURN
L13280: REM ***** ROOM 76 *****
PRINT"You have reached a large red berry patch. Although it hardly seems"
PRINT"possible, only the bottom portion of the patch is scattered about the"
PRINT"ground, suggesting small woodland creatures, not more than three apples"
PRINT"high, eat from the patch. You pick a few berries from the tree and"
PRINT"taste them, but they are hardly edible, as the flavor is slightly"
PRINT"sour! Yuck! The red berry patch continues west.":RETURN
L13350: REM ***** ROOM 77 *****
PRINT"You have reached the end of the small path, stopping here at a"
PRINT"large red berry patch (to the west). Several woodland creatures"
PRINT"scurry about the floor, some of them blue! They appear to be"
PRINT"picking red berries from a large patch of berry bushes. Because"
PRINT"some of these creatures are so small, there is an abundance of"
PRINT"berries near the top, which is waist high for humans.":RETURN
L13420: REM ***** ROOM 78 *****
PRINT"As you walk about the village, trying not to step on the dozens of"
PRINT"curious blue creatures, one particular elf walks up to you. He has a"
PRINT"white beard and a red hat, which is distinct from all the others who"
PRINT"wear only white hats. He also appears to be much wiser than the rest,"
PRINT"if not older as well. He says to you,'You can walk around the village,"
PRINT"but please, be careful! If you need any help, ask Handy for tools or"
PRINT"other items. Have a smurfy day!' The path continues north and east.":RETURN
L13500: REM ***** ROOM 79 *****
PRINT"As you walk up to a goofy looking blue creature, he hands you a yellow"
PRINT"box, wrapped in a red bow. Carefully, you remove the wrappings, but suddenly"
PRINT"a blast of soot and ash explode in your face! Checking yourself over, you"
PRINT"do not appear to be harmed, just embarrassed! The blue creature laughs at"
PRINT"you, teasing,'You fell for my trick! BWHAHAHAHAHA!' You feel like stepping"
PRINT"on the blue creature! Thankfully, he walks away before you can smush him!":RETURN
L13570: REM ***** ROOM 80 *****
IF LO(56) = -5 THEN GOTO L13670
PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
PRINT"village. He appears to be working on several different projects as once, the"
PRINT"largest of which appears to be a wooden statue of appearance similar to"
PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
PRINT"to look around, but don't touch anything! I'm working on a robot to"
PRINT"help with the dam building next fall!' You wonder what he could be"
PRINT"talking about when it hits you: he's working on a robot! Handy looks"
PRINT"around the room and says,'What I really need is a nice, new rug! I"
PRINT"know! Bring me azrael and I'll give you some tools!'":RETURN
L13670: REM *****
PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
PRINT"village. He appears to be working on several different projects as once, the"
PRINT"largest of which appears to be a wooden statue of appearance similar to"
PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
PRINT"to look around, but don't touch anything! I'm working on a robot to"
PRINT"help with the dam building next fall!' You wonder what he could be"
PRINT"talking about when it hits you: he's working on a robot! Handy thanks"
PRINT"you for the nice rug. 'Thank you! It looks Smurfy!'":RETURN
L13680: REM ***** ROOM 81 *****
PRINT"You are walking in Smurf forest. On one particular tree you notice a"
PRINT"blue creature with glasses, reading from a book. He appears to be secluded"
PRINT"from the rest of the Smurfs, as he immerses himself in endless quotations"
PRINT"(apparently self-written). The path continues to the east near a hulking"
PRINT"castle.":RETURN
L13730: REM ***** ROOM 82 *****
PRINT"You are on a branch high atop Smurf forest. A lone blue creature is here,"
PRINT"wearing glasses and reading quotations from a large book. 'Famous quotations"
PRINT"from Brainy Smurf' he says to you. 'Quotation 36: He who seeks shall find,"
PRINT"and having seeked, shall be wiser the more!' You feel a sudden repulsiveness"
PRINT"towards this particular creature: no wonder he isn't liked! The only obvious"
PRINT"exit is back down the tree.":RETURN
L13800: REM ***** ROOM 83 *****
PRINT"You are standing outside Gargamel's castle, a hulking place with a large"
PRINT"bell at the top and several crows roosting on the straw-matted roof. From"
PRINT"inside, you can hear a man talking to no one in particular. You listen"
PRINT"for awhile until you hear him bellow,'I'll get you, you dispicable little"
PRINT"Smurfs!' You wonder why he could be so upset, but then you remember"
PRINT"how you were treated entering the village.":RETURN
L13870: REM ***** ROOM 84 *****
PRINT"You are standing inside Gargamel's castle. A tall man wearing a black"
PRINT"cloak is stirring a large pot, cackling in glee. 'Oh, the smurfs will"
PRINT"be mine, Azrael! And then after I've eaten them, I can turn the rest"
PRINT"into gold! Gold, Azrael! BWHAHAHAHAHAHA!' He appears to be talking to"
PRINT"his cat, scurrying about the floor in fear of his master! A few beakers"
PRINT"are here, as Gargamel appears to be mixing potions. There are also"
PRINT"a few gold bars laying on a wooden table near the back.":RETURN
RETURN
REM ***** DATA *****
DATA "north","south","east","west","up","down"
DATA "oil","lantern","rope","knapsack","backpack"
DATA "pole","food","water","wine","bottle"
DATA "book","gold","spellbook","beaker"
DATA "helmet","chainmail","boots","cloak"
DATA "ring","shield","amulet","gauntlets"
DATA "dagger","sword","broadsword","axe"
DATA "glowball","scepter","slayer"
DATA "villager","clerk","goblin","hellhound"
DATA "direwolf","skeleton","dragon","ghost"
DATA "wolf","warrior","tursk","troll","spider"
DATA "hobbit","mordimar"
DATA "papa","handy","jokey","brainy"
DATA "gargamel","azrael"
DATA 99,99,99,99,99,99
DATA 98,4,8,4,63,63,1,5,1,53,82,84,24,84
DATA 1038,1038,1038,1038,24,1041,1042,1045
DATA 4,98,1045,1038,24,1043,1042,5,3,65,12
DATA 41,45,38,35,10,20,72,24,47,68,64,78
DATA 80,79,82,84,84
REM Item descriptions
DATA "north: facing north."
DATA "south: facing south."
DATA "east: facing east."
DATA "west: facing west."
DATA "up: facing up."
DATA "down: facing down"
DATA "oil: a flask of oil. Requires the lantern."
DATA "lantern: a brass lantern stained with blood. Requires the oil."
DATA "rope: a coil of rope about 10' in length. It appears used."
DATA "knapsack: a leather knapsack. Can hold up to 4 items."
DATA "backpack: a large backpack. Can hold up to 6 items."
DATA "pole: a wooden pole about 6' in length."
DATA "food: these food rations can last up to two weeks."
DATA "water: the water appears murky green. It does not appear drinkable."
DATA "wine: a bottle of sparkling red wine. It looks drinkable."
DATA "bottle: an empty bottle."
DATA "book: a large leather book. It is written in an elvish tongue."
DATA "gold: several gold bars. They glitter beautifully!"
DATA "spellbook: an ancient spellbook. Useful in casting spells."
DATA "beaker: a glass beaker containing a red liquid."
DATA "helmet: an iron helmet. It has a few small dents."
DATA "chainmail: a chainlinked suit of armor. Offers good protection."
DATA "boots: a pair of leather boots. Offers minimal protection."
DATA "cloak: a large cloak. Offers fair protection."
DATA "ring: a ring of protection. It glows with a bluish hue."
DATA "shield: a large shield with a cross on the front. Offers great protection."
DATA "amulet: a magical amulet. It glows with a greenish hue."
DATA "gauntlets: a pair of metal gauntlets. Offers good protection."
DATA "dagger: a small dagger with a jeweled hilt. Effective at close range."
DATA "sword: a large two-handed sword. It has a ruby hilt. Effective at medium range."
DATA "broadsword: a large broadsword with a diamond hilt. Effective at long range."
DATA "axe: a large battle axe. It has a doubled edge. Effective at close range."
DATA "glowball: eight glowing balls wisping around like fireflies. Effective at long range."
DATA "scepter: a large, powerful scepter with elvish runes. It appears magical."
DATA "slayer: a large sword with a razor-sharp blade. Effective at medium range."
DATA "villager: a simple villager. He smiles at you briefly."
DATA "clerk: a stout clerk with glasses. You can 'buy' and 'sell' items here."
DATA "goblin: a menacing goblin with dark eyes and sharp teeth."
DATA "hellhound: a fiery hellhound raised from the depths of hell. He burns in a luminous flame."
DATA "direwolf: a direwolf wandering the caverns. He growls at you loudly."
DATA "skeleton: an undead zombie haunting the forest. He appears almost translucent."
DATA "dragon: a large dragon with tough scales and sharp claws. He bellows,'Be gone, knave!'"
DATA "ghost: a translucent ghost haunting the area where he died. Watch out!"
DATA "wolf: a friendly wolf with yellow eyes. He seems content on watching you."
DATA "warrior: an evil warrior bound by black magic. He looks tough indeed!"
DATA "tursk: a friendly adventurer with a trimmed beard and flowing hair."
DATA "troll: a troll. Nothing special."
DATA "spider: a black widow spider lurking in the shadows. Be wary of his bite!"
DATA "hobbit: a friendly hobbit. He has furry feet and stands less than 4 feet tall."
DATA "mordimar: the all-powerful wizard of legend! You sense great power flowing through him!"
DATA "papa: a small blue creature with a white beard and a red hat. He is mixing potions."
DATA "handy: a small blue creature wearing a white hat. He appears to be building something."
DATA "jokey: this mischevious blue creature appears to be up to no good. He carries around a box wrapped with a bow."
DATA "brainy: a small blue creature with glasses. He appears to be the least liked of his kind."
DATA "gargamel: an evil wizard trying to turn little blue creatures into gold. He has a black heart."
DATA "azrael: a mischevious cat. He appears somewhat malnourished."
REM Verbs
DATA"go","get","drop","inventory","look","wield","unwield","wear","remove","examine","use"
DATA"climb","read","buy","sell","kill","put","eat","drink"
REM map
REM   N, S, E, W, U, D
DATA 00,05,00,00,02,00
DATA 00,00,04,00,00,01
DATA 00,00,00,05,00,00
DATA 00,00,00,02,00,00
DATA 01,09,03,06,00,00
DATA 07,00,05,00,00,00
DATA 00,06,00,08,00,00
DATA 00,00,07,00,00,00
DATA 05,10,00,00,00,00
DATA 09,12,13,11,00,00
DATA 30,33,10,00,00,00
DATA 10,49,34,33,00,00
DATA 00,00,14,10,00,00
DATA 15,00,00,13,00,00
DATA 00,14,00,00,00,00
DATA 00,37,17,00,00,00
DATA 18,00,00,16,00,00
DATA 20,17,00,00,00,00
DATA 25,00,20,00,00,00
DATA 22,18,21,19,00,00
DATA 00,00,00,20,00,00
DATA 00,20,00,00,23,00
DATA 00,00,24,00,00,22
DATA 00,00,00,23,00,00
DATA 00,19,00,26,00,00
DATA 00,00,25,00,00,00
DATA 00,29,00,00,00,00
DATA 00,32,29,00,00,00
DATA 27,00,30,28,00,00
DATA 00,11,00,29,00,00
DATA 00,00,32,00,00,00
DATA 28,00,00,31,00,00
DATA 11,00,12,00,00,00
DATA 00,00,35,12,00,00
DATA 00,50,36,34,00,00
DATA 00,51,37,35,00,00
DATA 16,00,38,36,00,00
DATA 39,00,00,37,00,00
DATA 00,38,40,00,00,00
DATA 00,00,41,39,00,00
DATA 44,00,42,40,00,00
DATA 00,00,00,41,00,00
DATA 00,00,44,00,00,00
DATA 47,41,45,43,00,00
DATA 00,00,46,44,00,00
DATA 00,00,00,45,00,00
DATA 00,44,48,00,00,00
DATA 00,00,00,47,00,00
DATA 12,55,00,00,00,00
DATA 35,00,51,00,00,00
DATA 36,56,52,50,00,00
DATA 00,00,00,51,00,00
DATA 00,60,00,00,00,00
DATA 00,62,00,00,00,00
DATA 49,65,00,00,00,00
DATA 51,00,57,00,00,00
DATA 00,66,58,56,00,00
DATA 00,67,59,57,00,00
DATA 00,00,60,58,00,00
DATA 53,00,61,59,00,00
DATA 00,00,62,60,00,00
DATA 54,00,63,61,00,00
DATA 00,00,00,62,00,00
DATA 00,70,00,00,00,00
DATA 55,72,00,00,00,00
DATA 57,00,00,00,00,00
DATA 58,00,68,00,00,00
DATA 00,74,69,67,00,00
DATA 00,00,00,68,00,00
DATA 64,00,71,00,00,00
DATA 00,00,72,70,00,00
DATA 65,00,00,71,00,00
DATA 78,75,00,00,00,00
DATA 69,77,00,00,00,00
DATA 73,00,76,00,00,00
DATA 00,00,77,75,00,00
DATA 74,00,00,76,00,00
DATA 79,73,80,00,00,00
DATA 00,78,81,00,00,00
DATA 00,00,00,78,00,00
DATA 00,00,83,79,82,00
DATA 00,00,00,00,00,81
DATA 00,00,84,81,00,00
DATA 00,00,00,83,00,00
REM
DATA "Inside a Small Tavern"
DATA "Upstairs Hallway"
DATA "In the Village Shop"
DATA "Inside a Small Room"
DATA "By a large Fountain"
DATA "Inside the Village Church"
DATA "Within the Church Gargen"
DATA "Inside a Small Toolshed"
DATA "Entrance to Village"
DATA "On the Path"
DATA "In a Burnt Forest"
DATA "At an Intersection"
DATA "In a Large Meadow"
DATA "In a Large Meadow"
DATA "At a Small Pond"
DATA "In the River"
DATA "In the River"
DATA "In a Small Clearing"
DATA "Redwood Forest"
DATA "Redwood Forest"
DATA "Redwood Forest"
DATA "Redwood Forest"
DATA "High atop the forest"
DATA "Small Hut"
DATA "Redwood Forest"
DATA "Redwood Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "In a Burnt Forest"
DATA "Dark Forest"
DATA "Dark Forest"
DATA "Dark Forest"
DATA "In the River"
DATA "Large Cave"
DATA "In a Twisty Tunnel"
DATA "In a Twisty Tunnel"
DATA "In a Twisty Tunnel"
DATA "At a Dead End"
DATA "At a Dead End"
DATA "In a Twisty Tunnel"
DATA "In a Twisty Tunnel"
DATA "Small Keep"
DATA "In a Twisty Tunnel"
DATA "At a Dead End"
DATA "On the Bridge"
DATA "Dark Forest"
DATA "Dark Forest"
DATA "Small Alcove"
DATA "Kitchen"
DATA "Small Room"
DATA "On the Bridge"
DATA "Dark Forest"
DATA "Light Forest"
DATA "Light Forest"
DATA "Light Forest"
DATA "In a Small Cottage"
DATA "Hallway"
DATA "Hallway"
DATA "Small Room"
DATA "Mordimar"
DATA "On the Bridge"
DATA "Light Forest"
DATA "Light Forest"
DATA "Light Forest"
DATA "Light Forest"
DATA "Throne Room"
DATA "Fountain"
DATA "Shadow Castle"
DATA "Strange New World"
DATA "Small Path"
DATA "Smurf Berry Patch"
DATA "Smurf Berry Patch"
DATA "Smurf Berry Patch"
DATA "Papa Smurf"
DATA "Jokey Smurf"
DATA "Handy Smurf"
DATA "Smurf Forest"
DATA "High atop Smurf Forest"
DATA "Outside Gargamel's Castle"
DATA "Inside Gargamel's Castle"
REM item prices
DATA 99,99,99,99,99,99
DATA 12,26,8,40,200
DATA 42,30,16,50,20
DATA 100,1000,5000,28
DATA 200,800,120,80
DATA 300,150,380,400
DATA 50,480,750,320
DATA 2000,1000,3000
DATA 99,99,99,99
DATA 99,99,99,99
DATA 99,99,99,99,99
DATA 99,99
DATA 99,99,99,99
DATA 99,99
REM *****
L30000: REM ***** Wait for TEMPO seconds *****
T = TIMER
WHILE TIMER < T + TEMPO
WEND
RETURN
REM ***** ERROR HANDLING *****
L40000: PRINT :PRINT "Saved game is not accessible or doesn't exist.":PRINT"You have to start from the beginning."
END
