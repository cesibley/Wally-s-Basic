DEFINT A-Z

DECLARE SUB ResetGameState ()
DECLARE SUB LoadStaticData ()
DECLARE SUB ShowTitle ()
DECLARE SUB StartSession
    CLS
    PRINT "Are you playing a saved game (y/n)?";
    DO
        DO
            A$ = INKEY$
        LOOP UNTIL A$ <> ""

        IF A$ = "y" OR A$ = "Y" THEN
            PRINT A$
            PRINT
            WantLoad = 1
            CALL LoadGame
            EXIT SUB
        ELSEIF A$ = "n" OR A$ = "N" THEN
            PRINT A$
            PRINT
            EXIT SUB
        END IF
    LOOP
END SUB

SUB LoadStaticData
    ' Placeholder so startup structure is explicit.
END SUB

SUB ShowTitle
    CLS
    COLOR 12, 0
    KEY OFF
    PRINT "@@@ Adventure XT @@@"
    PRINT "Written for the 13th annual interactive fiction contest"
    PRINT "By: Paul Panks (dunric@yahoo.com)"
    PRINT
    PRINT "It has been four years since you last ventured into Blarg,"
    PRINT "the land of might and magic. Mordimar, an evil wizard, found"
    PRINT "the powerful Orb of Destiny. With it, he became nearly"
    PRINT "invincible. As his power grew, the chi (life force)"
    PRINT "from the surrounding forest was drained steadily, until"
    PRINT "there was almost nothing left."
    PRINT
    PRINT "Determined to stop Mordimar, you set out on a quest"
    PRINT "to re-acquire the orb and bring peace back to the"
    PRINT "forest."
    PRINT
    PRINT "Your Quest Begins"
    PRINT
    PRINT "@@@ Press any key to continue @@@"
    DO: A$ = INKEY$: LOOP UNTIL A$ <> ""
END SUB

SUB StartSession
    CLS
    PRINT "Are you playing a saved game (y/n)?";
    DO
        A$ = INKEY$
    LOOP UNTIL A$ <> ""

    IF A$ = "y" OR A$ = "Y" THEN
        PRINT A$
        PRINT
        WantLoad = 1
        CALL LoadGame
    ELSEIF A$ = "n" OR A$ = "N" THEN
        PRINT A$
        PRINT
    ELSE
        CALL StartSession
    END IF
END SUB

SUB GameLoop
    DO WHILE QuitProgram = 0 AND ReturnToTitle = 0
        CALL ShowCurrentRoom
        CALL ReadCommand(A$)
        IF HandleMetaCommand(A$) = 0 THEN
            CALL ParseWords(A$)
            CALL ExecuteVerb
        END IF
    LOOP
END SUB

SUB ShowCurrentRoom
    NC = 0
    PRINT DE$(RM)
    IF LT = 0 AND RM > 18 THEN
        PRINT "It is too dark to see much of anything!"
    ELSE
        CALL ShowRoomDescription
    END IF
    CALL ShowObviousExits
    CALL ShowVisibleObjects
    CALL HandleHandyEvent
END SUB

SUB ShowRoomDescription
    SELECT CASE RM
        CASE 1
            PRINT"You are standing in a tavern with creaky wooden floors. A few tables"
            PRINT"line the room, with flickering candles set in the middle, illuminating"
            PRINT"the area. To the south (outside) stands a large fountain."
        CASE 2
            PRINT"You are standing on an upstairs hallway in the tavern. A lone room"
            PRINT"stands to the east, unoccupied. The only exit is back down the"
            PRINT"staircase."
        CASE 3
            PRINT"You are standing in the village shop. People come here to purchase"
            PRINT"needed goods and sell unwanted items. Commands here include 'buy <item>',"
            PRINT"'sell <item>' and 'list'. The shop is usually attended by a clerk."
        CASE 4
            PRINT"You are standing in a small room upstairs of the main tavern. A small"
            PRINT"bed is visible here, allowing you to rest comfortably. The only exit"
            PRINT"is back west."
        CASE 5
            PRINT"You are standing near a large fountain. Looking down at your reflection,"
            PRINT"you notice the water appears mossy green. However, despite the color"
            PRINT"it appears drinkable. To the east stands a small shop, while west of"
            PRINT"here lies a church."
        CASE 6
            PRINT"You are standing in a small church. Pews adorn the room, with an altar"
            PRINT"at the front. To the north (outside) a garden is visible. The only"
            PRINT"other exit is back east."
        CASE 7
            PRINT"You are standing in a lovely garden. Several different flowers dot"
            PRINT"the area, including lillies and begonias. To the west stands a"
            PRINT"wooden toolshed."
        CASE 8
            PRINT"You are standing inside a darkened toolshed. A few items of interest"
            PRINT"line the walls, but not much else is here. The only exit is back"
            PRINT"east."
        CASE 9
            PRINT"You are standing at the entrance to a small village. A path heads south"
            PRINT"from here into a well-lit forest."
        CASE 10
            PRINT"You are on a well-lit path walking through a forest. The area is"
            PRINT"populated by many small creatures (scurrying about). The path"
            PRINT"continues south."
        CASE 11
            PRINT"You are on a well-lit path walking through a forest. A lush green meadow"
            PRINT"stands to the east (near a small pond). An intersection is south of"
            PRINT"here."
        CASE 12
            PRINT"You are standing at an intersection in the forest. A dying woodland is"
            PRINT"west of here, the trees charred black with soot. To the east stands a"
            PRINT"darker section of the forest, while a long bridge is south."
        CASE 13
            PRINT"You are walking through a lush, green meadow. It opens up here and"
            PRINT"continues for miles on end (to the east). To the west, a large path"
            PRINT"cuts through a well-lit forest."
        CASE 14
            PRINT"You are in a lush, green meadow. To the north lies a small pond. Small"
            PRINT"clouds drift on by, pushed by a gentle breeze. The only obvious exit"
            PRINT"is back west."
        CASE 15
            PRINT"You are standing next to a small pond. The water appears mossy green"
            PRINT"and devoid of life. Although you could try drinking the water, it does"
            PRINT"not appear healthy. The only exit is back south."
        CASE 16
            PRINT"You are knee deep in a swiftly moving river. The water feels icy cold!"
            PRINT"The river continues to the east, while less hurried currents are directly"
            PRINT"south."
        CASE 17
            PRINT"You have reached the northern bank of a swiftly moving river. On the other"
            PRINT"side of the water rises a tall Redwood forest (to the north)."
        CASE 18
            PRINT"You are standing in a small clearing. There is nothing around but"
            PRINT"blue sky."
        CASE 19
            PRINT"You are walking in a Redwood forest, the trees of which rise hundreds of"
            PRINT"feet into the air. To the east stands a well-lit section of the forest,"
            PRINT"while west of here the area becomes dark and oppressive."
        CASE 20
            PRINT"You are standing in a Redwood forest, the trees of which rise hundreds of"
            PRINT"feet into the air. A large clearing is south, while one particular"
            PRINT"tree -- with low-lying branches, stands to the north. It appears"
            PRINT"climbable."
        CASE 21
            PRINT"The Redwood forest ends here next to a large, purple mountain range."
            PRINT"The only obvious exit is back west."
        CASE 22
            PRINT"You are walking in a Redwood forest. A lone tree (with low-lying"
            PRINT"branches) is here, as a poorly made ladder heads up into the treetops"
            PRINT"above. The only obvious exit is back south."
        CASE 23
            PRINT"You are atop the forest, standing on a long branch. A small hut (with a"
            PRINT"thatched roof) stands to the east near the top of the tree. Looking in,"
            PRINT"you can see a small creature, apparently meditating. Perhaps it would"
            PRINT"be best not to disturb him."
        CASE 24
            PRINT"You are standing inside a small hut. A small creature is here, apparently"
            PRINT"meditating. He stops suddenly, noticing your presence, but this does"
            PRINT"not seem to alarm him. He smiles at you, gestering to a nearby table"
            PRINT"(next to a wooden bookcase). Perhaps you should sit down and"
            PRINT"listen to what he has to say."
        CASE 25
            PRINT"You are walking in a Redwood forest. The trees thin out some to the"
            PRINT"west. To the south, the trees are much taller, as the area is populated"
            PRINT"with woodland creatures."
        CASE 26
            PRINT"You have reached the end of the Redwood forest. The only obvious exit"
            PRINT"is back east. Further travel west is made impossible by majestic purple"
            PRINT"mountains."
        CASE 27
            PRINT"You have reached the end of the burnt forest. The trees are charred"
            PRINT"black with soot (no doubt the remnants from a once raging wildfire)."
            PRINT"The only obvious exit is back south."
        CASE 28
            PRINT"You are walking in a burnt forest. The trees have been charred black"
            PRINT"from a recent wildfire. East of here the trees appear larger and"
            PRINT"more stable, while west of here the woods thin out somewhat."
        CASE 29
            PRINT"You are standing in the middle of a large, burnt forest. The trees"
            PRINT"have been charred black from a recent fire. To the north, the forest"
            PRINT"stops abruptly, while to the east and west the forest continues."
        CASE 30
            PRINT"You are walking in a burnt forest. The trees are particularly blackened"
            PRINT"here. This must have been where the first started, as the surrounding"
            PRINT"area is charred black with soot. Additionally, several telltale signs"
            PRINT"of a lightning strike are evident nearby, suggesting the cause of"
            PRINT"the blaze."
        CASE 31
            PRINT"You have reached the end of the burnt forest. Further travel west and"
            PRINT"north becomes impossible by fallen trees."
        CASE 32
            PRINT"You are walking in a burnt forest, the trees of which appear blackened"
            PRINT"by soot from a recent wildfire. To the north, the trees appear to thin"
            PRINT"out somewhat, while fallen trees obstruct further travel to the west."
        CASE 33
            PRINT"You have reached a burnt forest, the trees of which appear blackened"
            PRINT"with soot from a recent fire. To the north, the trees appear to thicken"
            PRINT"considerably, while east of here lies an intersection."
        CASE 34
            PRINT"You have reached a dark forest. The trees appear to be evergreens,"
            PRINT"shrouding most of the floor, while speckles of light filter down from"
            PRINT"the thick canopy above. To the east, the forest appears to thin out"
            PRINT"somewhat, while west of here lies an intersection."
        CASE 35
            PRINT"You are walking in a dark forest. The trees are particularly dense here, as"
            PRINT"very little light filters down from below. There are several different"
            PRINT"woodland creatures here (all scurrying about), including squirrels, rabbits,"
            PRINT"foxes and birds. The forest continues east and south."
        CASE 36
            PRINT"You are walking in a dark forest. The trees thin out here near a large"
            PRINT"river (to the east), while west of here appears to be the middle of"
            PRINT"the forest. South of here the forest continues to thin out."
        CASE 37
            PRINT"You are standing at the bank of a swiftly moving river. Although it does"
            PRINT"not appear too deep, caution should nonetheless be taken. The forest"
            PRINT"continues west, while east of here a cave is visible, nestled near a"
            PRINT"small alcove (to the southwest)."
        CASE 38
            PRINT"You have entered into a dark cave. Very little is visible here, but you"
            PRINT"do notice several glowing red lights, all wisping about endlessly. It is almost"
            PRINT"as if the cave is haunted by evil spirits! The cave begins to widen some to"
            PRINT"the north."
        CASE 39
            PRINT"You have reached a large tunnel within the cave, splitting off to the east"
            PRINT"here (accessible through a hole). As the tunnel progresses, it appears to"
            PRINT"narrow considerably. You might only be able to squeeze through by dropping"
            PRINT"several items."
        CASE 40
            PRINT"You are crawling on your belly in a narrow tunnel. It heads to the east here,"
            PRINT"turning sharply. Moans from disembodied spirits can be heard, getting louder"
            PRINT"the deeper you go! The only obvious exit is back west."
        CASE 41
            PRINT"You have reached a small intersection within the cave. Tunnels head off in"
            PRINT"all directions here, while the main tunnel continues north."
        CASE 42
            PRINT"The tunnel stops here near a large rock. The only exit is back west."
        CASE 43
            PRINT"You have reached a dead end within the cavern. The only exit is back east."
        CASE 44
            PRINT"You are crawling on your belly in the heart of the cave. The main tunnel"
            PRINT"branches off here in four directions: north, south, east and west. One"
            PRINT"particular tunnel (to the east) appears to glow softly with a pale"
            PRINT"green hue. Perhaps it would be worth exploring further."
        CASE 45
            PRINT"You are crawling on your belly in a small tunnel. The area is surrounded"
            PRINT"by pale green light, glowing softly. The air feels magical, as if the"
            PRINT"light particles themselves are alive!"
        CASE 46
            PRINT"You have reached a small keep within the cave. Several large candles,"
            PRINT"glowing softly green, illuminate the darkness. This small room appears"
            PRINT"to be a place of meditation and worship. Perhaps you should rest here"
            PRINT"and learn more about this most unique room."
        CASE 47
            PRINT"You are crawling on your belly in the main tunnel within the cave. To the"
            PRINT"south you notice a pale green light, glowing near a smaller tunnel towards"
            PRINT"the southeast. To the east, the main tunnel forks off into a much smaller"
            PRINT"one. It appears almost too narrow to pass through."
        CASE 48
            PRINT"You have reached a dead end within the cavern, as the main tunnel"
            PRINT"forks off here into a very small passageway. However, a large"
            PRINT"boulder has crashed through the tunnel from above, blocking further"
            PRINT"passage east. The only exit is back west."
        CASE 49
            PRINT"You are standing on a wooden bridge just south of a large intersection."
            PRINT"It appears to cross over a swiftly moving river (heading northeast). At"
            PRINT"the end of the bridge you notice a large, stone castle."
        CASE 50
            PRINT"You are walking in a dark forest. The trees thicken somewhat to the east, as"
            PRINT"very little light filters down from below. There are several woodland creatures"
            PRINT"scurrying about, making you feel restless. The forest continues north and east."
        CASE 51
            PRINT"You are walking in the middle of a dark forest. The trees are particularly"
            PRINT"thick here, as a plethora of woodland creatures scurry about the forest floor,"
            PRINT"looking for food. To the east stands a secluded alcove, while north the forest"
            PRINT"continues. Further travel south becomes increasingly difficult, but not"
            PRINT"impossible."
        CASE 52
            PRINT"You have reached a secluded alcove, hidden within the forest. Several magical"
            PRINT"creatures populate the area, including unicorns and one-eyed beasts. It appears"
            PRINT"to be a land that time forgot, for these beings are only written about in"
            PRINT"fairy tales! The only exit is back west."
        CASE 53
            PRINT"You are standing in a small kitchen within the cottage. A large wooden table"
            PRINT"is set in the center, surrounded by chairs. To the south lies the main living"
            PRINT"room, as a narrow hallway heads into several small rooms."
        CASE 54
            PRINT"You have entered into a small room within the cottage. Not much can be seen"
            PRINT"here save for a bed and a small table. The only exit is back south."
        CASE 55
            PRINT"You are standing in the middle of a large bridge. Underneath, a river flows"
            PRINT"to the northeast. To the south, the bridge stops abruptly near a large,"
            PRINT"stone castle. Rumor has it that a goblin periodically appears, demanding"
            PRINT"money by all who cross here. You wonder if the goblin will stop you, too!"
        CASE 56
            PRINT"You have reached the limits of the dark forest. A well-lit section of"
            PRINT"the forest opens up here, dramatically improving visibility. Far to the"
            PRINT"east a cottage can be seen, smoke rising from a chimney high above."
        CASE 57
            PRINT"You are standing in a well-lit forest. Beams of light filter down from"
            PRINT"the canopy above, dotting the forest floor. The forest continues to the"
            PRINT"east and south, while west of here a dark forest is visible."
        CASE 58
            PRINT"You are standing in the middle of a well-lit forest. Beams of light filter down"
            PRINT"from high above, dotting the forest floor. A particularly well-lit path, almost"
            PRINT"magical in appearance, shimmers to the south, while east of here the forest"
            PRINT"continues towards a large cottage."
        CASE 59
            PRINT"You are standing in a well-lit forest next to a large cottage (east). The"
            PRINT"forest begins to thin out here somewhat, as the cottage dominates the eastern"
            PRINT"edge of the woods (with the door wide open). Could someone be expecting you?"
        CASE 60
            PRINT"You are standing inside a large cottage within the forest. A fireplace is here,"
            PRINT"allowing you to warm your feet. Oddly enough, no one appears to have occupied"
            PRINT"this abode for quite some time, even though a fire is ominously burning"
            PRINT"neatly in a small fireplace. 'Could this place be haunted?' you wonder aloud."
            PRINT"A hallway is east, while a kitchen is directly north."
        CASE 61
            PRINT"You are walking down a long hallway within the cottage. Several rooms are"
            PRINT"visible here, some closed by locked doors (and others open). The hallway"
            PRINT"continues east."
        CASE 62
            PRINT"You are walking down a long hallway within the cottage. Two rooms are"
            PRINT"plainly visible up ahead (to the north and east), both of them left"
            PRINT"wide open by the previous occupants."
        CASE 63
            PRINT"You have reached a small room within the cottage. Not much can be seen"
            PRINT"here, save for a small bed next to a wooden dresser. The only exit is"
            PRINT"back west."
        CASE 64
            PRINT"You have reached the inner sanctum of Mordimar, evil of the ancients! You"
            PRINT"sense great power! He cackles at you with an insane laugh, yelling,'You"
            PRINT"fool! You can't possibly hope to defeat me! I am Mordimar, master of the"
            PRINT"ancient circle of wizards! I own this world! Who are YOU to challenge"
            PRINT"otherwise?! Bah! So be it! I will crush you like a bug! Die, knave!'"
        CASE 65
            PRINT"At the end of the bridge, a small goblin is said to often appear,"
            PRINT"demanding gold. To the south, just beyond the bridge, stands a tall"
            PRINT"stone castle. A pair of red flags, with gryphons emblazoned on"
            PRINT"them, wave to you in a stiff breeze, somehow beckoning you inside!"
        CASE 66
            PRINT"You are walking in a well-lit forest. Light filters down from the canopy"
            PRINT"above, dotting the forest floor. Several woodland creatures scurry about,"
            PRINT"some of them unaware of your presence. Fallen trees block passage to the"
            PRINT"south. The only obvious exit is back north."
        CASE 67
            PRINT"You are standing in the middle of a well-lit forest. The path turns east"
            PRINT"here near a large patch of strange red berries. They look too small for"
            PRINT"humans to eat, but it looks as though the tiny woodland creatures eat"
            PRINT"them regularly. To the north, the forest thickens somewhat."
        CASE 68
            PRINT"You have reached a particularly dark section of the forest, more secluded than"
            PRINT"the rest. To the south, a small path heads past a series of red berry patches."
            PRINT"Whatever lives in these woods must be fairly tiny, for the berries themselves"
            PRINT"are all scattered about the forest floor."
        CASE 69
            PRINT"You have reached the limits of the forest, as storm-tossed trees block your"
            PRINT"path to the east. The only obvious exit is back west."
        CASE 70
            PRINT"You have entered into the throne room of Mordimar. Strangely, he does not"
            PRINT"appear to be here, although you DO sense a strong energy to the north! The"
            PRINT"throne is made of solid gold and encrusted with several jewels. To the"
            PRINT"east lies a stone fountain."
        CASE 71
            PRINT"You are standing in the castle courtyard. A large stone fountain, with the"
            PRINT"statue of an angel, pours water down from an upheld jar into a motted basin"
            PRINT"below. A throne room is visible to the west, while the castle entrance is"
            PRINT"east."
        CASE 72
            PRINT"You have reached a tall, stone castle known as Shadow Castle. Legend has it"
            PRINT"that the evil wizard Mordimar took the castle by storm many moons ago. High"
            PRINT"above, a pair of flags, with gryphons emblazoned on them, flap endlessly in"
            PRINT"a cool breeze. They appear to be beckoning you further into the castle! A"
            PRINT"courtyard, complete with a fountain, stands to the west of here."
        CASE 73
            PRINT"You have somehow entered into a strange, new world just beyond the red berry"
            PRINT"patch. Although it hardly seems possible, the entire area is populated by"
            PRINT"dozens of tiny blue creatures not more than three apples high! You are"
            PRINT"stunned by their speech, as they all seem to be speaking in a bizarre, elven"
            PRINT"tongue, the likes of which you cannot make out. To the north, mushroom"
            PRINT"houses are visible. This is where the blue creatures apparently live and"
            PRINT"work."
        CASE 74
            PRINT"You have found a small path separate from the rest of the forest. It turns"
            PRINT"to the south, heading west into a large red berry patch. A well-lit forest"
            PRINT"is clearly visible (to the north)."
        CASE 75
            PRINT"You are walking through a red berry patch. These berries are quite odd, as"
            PRINT"only the bottom portion of the patch is scattered about the ground. It"
            PRINT"appears as though whomever eats of these berries can't be more than"
            PRINT"three apples high. To the north, a small village of mushroom houses"
            PRINT"is plainly visible."
        CASE 76
            PRINT"You have reached a large red berry patch. Although it hardly seems"
            PRINT"possible, only the bottom portion of the patch is scattered about the"
            PRINT"ground, suggesting small woodland creatures, not more than three apples"
            PRINT"high, eat from the patch. You pick a few berries from the tree and"
            PRINT"taste them, but they are hardly edible, as the flavor is slightly"
            PRINT"sour! Yuck! The red berry patch continues west."
        CASE 77
            PRINT"You have reached the end of the small path, stopping here at a"
            PRINT"large red berry patch (to the west). Several woodland creatures"
            PRINT"scurry about the floor, some of them blue! They appear to be"
            PRINT"picking red berries from a large patch of berry bushes. Because"
            PRINT"some of these creatures are so small, there is an abundance of"
            PRINT"berries near the top, which is waist high for humans."
        CASE 78
            PRINT"As you walk about the village, trying not to step on the dozens of"
            PRINT"curious blue creatures, one particular elf walks up to you. He has a"
            PRINT"white beard and a red hat, which is distinct from all the others who"
            PRINT"wear only white hats. He also appears to be much wiser than the rest,"
            PRINT"if not older as well. He says to you,'You can walk around the village,"
            PRINT"but please, be careful! If you need any help, ask Handy for tools or"
            PRINT"other items. Have a smurfy day!' The path continues north and east."
        CASE 79
            PRINT"As you walk up to a goofy looking blue creature, he hands you a yellow"
            PRINT"box, wrapped in a red bow. Carefully, you remove the wrappings, but suddenly"
            PRINT"a blast of soot and ash explode in your face! Checking yourself over, you"
            PRINT"do not appear to be harmed, just embarrassed! The blue creature laughs at"
            PRINT"you, teasing,'You fell for my trick! BWHAHAHAHAHA!' You feel like stepping"
            PRINT"on the blue creature! Thankfully, he walks away before you can smush him!"
        CASE 80
            IF LO(56) = -5 THEN L13670
            PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
            PRINT"village. He appears to be working on several different projects as once, the"
            PRINT"largest of which appears to be a wooden statue of appearance similar to"
            PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
            PRINT"to look around, but don't touch anything! I'm working on a robot to"
            PRINT"help with the dam building next fall!' You wonder what he could be"
            PRINT"talking about when it hits you: he's working on a robot! Handy looks"
            PRINT"around the room and says,'What I really need is a nice, new rug! I"
            PRINT"know! Bring me azrael and I'll give you some tools!'"
            REM *****
            PRINT"You have entered into the mushroom house of Handy, a local 'handy' man in the"
            PRINT"village. He appears to be working on several different projects as once, the"
            PRINT"largest of which appears to be a wooden statue of appearance similar to"
            PRINT"his own! 'Welcome to my humble home!' he says to you. 'Please feel free"
            PRINT"to look around, but don't touch anything! I'm working on a robot to"
            PRINT"help with the dam building next fall!' You wonder what he could be"
            PRINT"talking about when it hits you: he's working on a robot! Handy thanks"
            PRINT"you for the nice rug. 'Thank you! It looks Smurfy!'"
        CASE 81
            PRINT"You are walking in Smurf forest. On one particular tree you notice a"
            PRINT"blue creature with glasses, reading from a book. He appears to be secluded"
            PRINT"from the rest of the Smurfs, as he immerses himself in endless quotations"
            PRINT"(apparently self-written). The path continues to the east near a hulking"
            PRINT"castle."
        CASE 82
            PRINT"You are on a branch high atop Smurf forest. A lone blue creature is here,"
            PRINT"wearing glasses and reading quotations from a large book. 'Famous quotations"
            PRINT"from Brainy Smurf' he says to you. 'Quotation 36: He who seeks shall find,"
            PRINT"and having seeked, shall be wiser the more!' You feel a sudden repulsiveness"
            PRINT"towards this particular creature: no wonder he isn't liked! The only obvious"
            PRINT"exit is back down the tree."
        CASE 83
            PRINT"You are standing outside Gargamel's castle, a hulking place with a large"
            PRINT"bell at the top and several crows roosting on the straw-matted roof. From"
            PRINT"inside, you can hear a man talking to no one in particular. You listen"
            PRINT"for awhile until you hear him bellow,'I'll get you, you dispicable little"
            PRINT"Smurfs!' You wonder why he could be so upset, but then you remember"
            PRINT"how you were treated entering the village."
        CASE 84
            PRINT"You are standing inside Gargamel's castle. A tall man wearing a black"
            PRINT"cloak is stirring a large pot, cackling in glee. 'Oh, the smurfs will"
            PRINT"be mine, Azrael! And then after I've eaten them, I can turn the rest"
            PRINT"into gold! Gold, Azrael! BWHAHAHAHAHAHA!' He appears to be talking to"
            PRINT"his cat, scurrying about the floor in fear of his master! A few beakers"
            PRINT"are here, as Gargamel appears to be mixing potions. There are also"
            PRINT"a few gold bars laying on a wooden table near the back."
    END SELECT
END SUB

SUB ShowObviousExits
    PRINT "Obvious exits: < /";
    IF M(RM, 1) > 0 THEN PRINT "north/ ";
    IF M(RM, 2) > 0 THEN PRINT "south/ ";
    IF M(RM, 3) > 0 THEN PRINT "east/ ";
    IF M(RM, 4) > 0 THEN PRINT "west/ ";
    IF M(RM, 5) > 0 THEN PRINT "up/ ";
    IF M(RM, 6) > 0 THEN PRINT "down/ ";
    PRINT ">"
END SUB

SUB ShowVisibleObjects
    FOR X = 7 TO 56
        IF LO(X) = RM THEN PRINT NO$(X); "."
    NEXT X
END SUB

SUB HandleHandyEvent
    IF LO(56) = 0 AND RM = 80 THEN
        LO(56) = -5
        PRINT "You bring Azrael to Handy."
        PRINT HS$
        PRINT HS2$
        LO(9) = RM
        LO(22) = RM
        LO(31) = RM
        LO(32) = RM
    END IF
END SUB

SUB ReadCommand (A$)
    FOR X = 1 TO 10: WD$(X) = "": NEXT X
    V = 0: N = 0: NE$ = "": N$ = "": N2$ = "": V$ = "": V2$ = ""
    PR = 0: PT = 0: NM = 0: BZ = 0: CO = 0

    CT = CT + 1
    INPUT ">", A$
    FOR X = 1 TO LEN(A$)
        I = ASC(MID$(A$, X, 1))
        IF I >= 65 AND I <= 90 THEN MID$(A$, X, 1) = CHR$(I + 32)
    NEXT X
END SUB

FUNCTION HandleMetaCommand% (A$)
    HandleMetaCommand% = -1
    CALL CheckThirst

    IF A$ = "help" OR A$ = "hint" THEN
        CALL ShowHelp
        EXIT FUNCTION
    END IF

    IF A$ = "list" THEN
        CALL ShowShopList
        EXIT FUNCTION
    END IF

    IF A$ = "save" OR A$ = "save game" THEN
        CALL SaveGame
        EXIT FUNCTION
    END IF

    IF A$ = "quit" THEN
        CALL ConfirmQuit
        EXIT FUNCTION
    END IF

    IF A$ = "look" OR A$ = "l" THEN
        CALL DoLook
        EXIT FUNCTION
    END IF

    IF A$ = "map" THEN
        CALL ShowMap
        EXIT FUNCTION
    END IF

    IF IsDirectionShortcut%(A$, V, N) THEN
        CALL DoGo
        EXIT FUNCTION
    END IF

    IF A$ = "inventory" OR A$ = "i" OR A$ = "inv" THEN
        CALL DoInventory
        EXIT FUNCTION
    END IF

    HandleMetaCommand% = 0
END FUNCTION

FUNCTION IsDirectionShortcut% (A$, VerbOut, NounOut)
    IsDirectionShortcut% = 0
    SELECT CASE A$
        CASE "go north", "n", "north"
            VerbOut = 1: NounOut = 1
        CASE "go south", "s", "south"
            VerbOut = 1: NounOut = 2
        CASE "go east", "e", "east"
            VerbOut = 1: NounOut = 3
        CASE "go west", "w", "west"
            VerbOut = 1: NounOut = 4
        CASE "go up", "u", "up"
            VerbOut = 1: NounOut = 5
        CASE "go down", "d", "down"
            VerbOut = 1: NounOut = 6
        CASE ELSE
            EXIT FUNCTION
    END SELECT
    IsDirectionShortcut% = -1
END FUNCTION

SUB ParseWords (A$)
    D$ = A$
    PT = 1
    NM = 0
    FOR X = 1 TO LEN(D$)
        IF MID$(D$, X, 1) = " " THEN
            NM = NM + 1
            WD$(NM) = MID$(D$, PT, X - PT)
            PT = X + 1
        END IF
    NEXT X
    NM = NM + 1
    WD$(NM) = MID$(D$, PT)

    V$ = WD$(1)
    N$ = WD$(2)

    IF WD$(3) = "and" OR WD$(3) = "then" THEN
        V2$ = WD$(4)
        N2$ = WD$(5)
        CO = 1
    END IF

    IF WD$(3) = "in" OR WD$(3) = "from" OR WD$(3) = "to" THEN
        V$ = WD$(1)
        NE$ = WD$(2)
        NE2$ = WD$(4)
        PR = 1
        BZ = 1
    END IF
END SUB

FUNCTION FindVerbIndex% (VName$)
    FindVerbIndex% = 0
    FOR X = 1 TO 19
        IF VName$ = VB$(X) THEN
            FindVerbIndex% = X
            EXIT FUNCTION
        END IF
    NEXT X
END FUNCTION

FUNCTION FindNounIndex% (NName$, AltName$)
    FindNounIndex% = 0
    FOR X = 1 TO 56
        IF NName$ = NO$(X) OR AltName$ = NO$(X) THEN
            FindNounIndex% = X
            EXIT FUNCTION
        END IF
    NEXT X
END FUNCTION

SUB ExecuteVerb
    V = FindVerbIndex%(V$)
    IF V = 0 THEN
        PRINT "What? Check your verb."
        EXIT SUB
    END IF

    N = FindNounIndex%(N$, NE$)
    IF N = 0 THEN
        IF V <> 10 AND V <> 12 THEN
            PRINT "Huh? Check your noun."
            EXIT SUB
        END IF
    END IF

    SELECT CASE V
        CASE 1: CALL DoGo
        CASE 2: CALL DoGet
        CASE 3: CALL DoDrop
        CASE 4: CALL DoInventory
        CASE 5: CALL DoLook
        CASE 6: CALL DoWield
        CASE 7: CALL DoUnwield
        CASE 8: CALL DoWear
        CASE 9: CALL DoRemove
        CASE 10: CALL DoExamine
        CASE 11: CALL DoUse
        CASE 12: CALL DoClimb
        CASE 13: CALL DoRead
        CASE 14: CALL DoBuy
        CASE 15: CALL DoSell
        CASE 16: CALL DoFight
        CASE 17: CALL DoPut
        CASE 18: CALL DoEat
        CASE 19: CALL DoDrink
    END SELECT
END SUB

SUB DoGo
    IF (RM = 22 OR RM = 81) AND N = 5 THEN
        IF LO(9) <> 0 AND LO(9) <> RM THEN
            PRINT "You need a rope to climb up!"
            EXIT SUB
        END IF
    END IF

    IF RM = 65 AND N = 2 THEN
        IF LO(38) = RM THEN
            PRINT "The goblin blocks your path! He growls,'You cannot pass!'"
            EXIT SUB
        END IF
    END IF

    IF RM = 12 AND N = 2 THEN
        IF LO(39) = RM THEN
            PRINT "The hellhound slams you around! He screams,'DIE, KNAVE!'"
            TEMPO = 1: CALL PauseSeconds(TEMPO)
            PRINT "You died."
            TEMPO = 2: CALL PauseSeconds(TEMPO)
            ReturnToTitle = -1
            EXIT SUB
        END IF
    END IF

    IF RM = 12 AND (N = 2 OR N = 3) AND LT = 0 THEN
        PRINT "It is much too dark to move in that direction!"
        PRINT "(You need a source of light)"
        EXIT SUB
    END IF

    IF N > 6 OR N = 0 OR M(RM, N) = 0 THEN
        PRINT "You can't go that way."
        EXIT SUB
    END IF

    RM = M(RM, N)
END SUB

SUB DoGet
    IF N < 7 OR N > 35 THEN
        PRINT "You can't pick that up."
        EXIT SUB
    END IF

    IF LO(N) <> 0 AND LO(N) <> 305 AND LO(N) <> 405 AND LO(N) <> RM THEN
        PRINT "It's beyond your power to do that!"
        EXIT SUB
    END IF

    IF IC > 8 THEN
        PRINT "You are carrying too much already!"
        EXIT SUB
    END IF

    IF N = 20 AND LO(56) = RM THEN
        LO(20) = 99
        LO(56) = 0
        EX$(56) = "azrael: a mischevious cat. He is asleep."
        IC = IC + 1
        PRINT MS$
        EXIT SUB
    END IF

    IC = IC + 1
    LO(N) = 0
    PRINT "Ok."
END SUB

SUB DoDrop
    IF N < 7 THEN
        PRINT "You can't drop that."
        EXIT SUB
    END IF

    IF LO(N) <> 0 AND LO(N) <> 105 AND LO(N) <> 205 THEN
        PRINT "You don't have it."
        EXIT SUB
    END IF

    IC = IC - 1
    LO(N) = RM
    PRINT "Ok."
END SUB

FUNCTION WieldedItem%
    FOR X = 7 TO 56
        IF LO(X) = 105 THEN
            WieldedItem% = X
            EXIT FUNCTION
        END IF
    NEXT X
    WieldedItem% = 0
END FUNCTION

FUNCTION IsWeaponIndex% (ItemIndex)
    IsWeaponIndex% = 0
    IF ItemIndex >= 29 AND ItemIndex <= 35 THEN IsWeaponIndex% = -1
END FUNCTION

FUNCTION IsWearingIndex% (ItemIndex)
    IsWearingIndex% = 0
    IF ItemIndex >= 21 AND ItemIndex <= 28 THEN IsWearingIndex% = -1
END FUNCTION

SUB DoInventory
    PRINT "You are carrying:"
    SI = 0
    WD = 0
    AC = 0

    FOR X = 7 TO 56
        IF LO(X) = 0 THEN
            SI = 1
            PRINT "   "; NO$(X); "."
        END IF
        IF LO(X) = 105 THEN
            WD = X
            SI = 1
            PRINT "   "; NO$(X); " (wielded)."
        END IF
        IF LO(X) = 205 THEN
            SI = 1
            AC = AC + (X / 4)
            AC = CINT(AC)
            PRINT "   "; NO$(X); " (worn)."
        END IF
        IF X = 10 OR X = 11 THEN
            IF LO(X) = 0 THEN CALL ShowContainerContents(X)
        END IF
        IF X = 8 AND LO(8) = 0 THEN
            IF LT = 0 THEN
                PRINT "(the lantern is off)"
            ELSE
                PRINT "(the lantern is aflame)"
            END IF
        END IF
    NEXT X

    IF SI = 0 THEN PRINT "Alas, you are empty-handed."
    PRINT "(You have"; HP; "hit points and"; G; "gold coins)."
END SUB

SUB DoLook
    CALL ShowCurrentRoom
END SUB

SUB DoWield
    IF N < 29 OR N > 35 THEN
        PRINT "You can't wield that."
        EXIT SUB
    END IF

    IF LO(N) <> 0 THEN
        PRINT "It's beyond your power to do that!"
        EXIT SUB
    END IF

    IF WieldedItem% > 0 THEN
        PRINT "You are already wielding something ("; NO$(WieldedItem%); ")."
        EXIT SUB
    END IF

    WD = N
    LO(N) = 105
    PRINT "Ok."
END SUB

SUB DoUnwield
    IF N < 29 OR N > 35 THEN
        PRINT "You can't unwield that."
        EXIT SUB
    END IF

    IF LO(N) <> 105 THEN
        PRINT "It's beyond your power to do that!"
        EXIT SUB
    END IF

    WD = 0
    LO(N) = 0
    PRINT "Ok."
END SUB

SUB DoWear
    IF N < 21 OR N > 28 THEN
        PRINT "You can't wear that."
        EXIT SUB
    END IF

    IF LO(N) <> 0 THEN
        PRINT "You can't wear that."
        EXIT SUB
    END IF

    LO(N) = 205
    AC = AC + (N / 4)
    AC = CINT(AC)
    PRINT "Ok."
END SUB

SUB DoRemove
    IF N < 21 OR N > 28 THEN
        PRINT "You can't remove that."
        EXIT SUB
    END IF

    IF LO(N) <> 205 THEN
        PRINT "You can't remove that."
        EXIT SUB
    END IF

    LO(N) = 0
    AC = AC - (N / 4)
    AC = CINT(AC)
    PRINT "Ok."
END SUB

SUB DoExamine
    IF N = 0 THEN
        PRINT "You notice nothing unusual about it."
        EXIT SUB
    END IF

    IF LO(N) <> 0 AND LO(N) <> RM AND LO(N) <> 105 AND LO(N) <> 205 THEN
        PRINT "That isn't here."
        EXIT SUB
    END IF

    PRINT EX$(N)
END SUB

FUNCTION HasItemHereOrCarried% (ItemIndex)
    HasItemHereOrCarried% = 0
    IF LO(ItemIndex) = 0 OR LO(ItemIndex) = RM THEN HasItemHereOrCarried% = -1
END FUNCTION

SUB DoUse
    IF (LO(N) <> 0 AND LO(N) <> RM) OR N < 7 OR N > 35 THEN
        PRINT "You can't use that!"
        EXIT SUB
    END IF

    IF N = 7 THEN
        IF LO(8) <> 0 AND LO(8) <> RM THEN
            PRINT "You need the lantern."
            EXIT SUB
        END IF
        IF LT = 1 THEN
            PRINT "The lantern is already on."
            EXIT SUB
        END IF
    END IF

    IF N = 8 THEN
        IF LO(7) <> 0 AND LO(7) <> RM THEN
            PRINT "You need the flask of oil."
            EXIT SUB
        END IF
        IF LT = 1 THEN
            PRINT "The lantern is already on."
            EXIT SUB
        END IF
        LT = 1
        PRINT "The lantern is now aflame."
        EXIT SUB
    END IF

    CALL ContinueUse
END SUB

SUB ContinueUse
    IF N = 9 AND RM = 22 THEN
        RM = 23
        PRINT "You climb up..."
        EXIT SUB
    END IF
    IF N = 9 AND RM = 81 THEN
        RM = 82
        PRINT "You climb up..."
        EXIT SUB
    END IF
    IF N = 12 AND RM = 22 THEN
        RM = 23
        PRINT "You climb up (on the pole)..."
        EXIT SUB
    END IF
    IF N = 12 AND RM = 81 THEN
        RM = 82
        PRINT "You climb up (on the pole)..."
        EXIT SUB
    END IF
    IF N = 13 THEN
        PRINT "You must use 'eat' instead."
        EXIT SUB
    END IF
    IF N = 14 OR N = 15 OR N = 16 THEN
        PRINT "You must use 'drink' instead."
        EXIT SUB
    END IF
    IF N = 17 OR N = 19 THEN
        PRINT "You must use 'read' instead."
        EXIT SUB
    END IF
    PRINT "You can't use that here."
END SUB

SUB DoClimb
    IF (RM <> 22 AND RM <> 81) OR N > 0 THEN
        PRINT "You can't climb that."
        EXIT SUB
    END IF

    IF RM = 22 THEN
        IF LO(9) = 0 OR LO(9) = RM THEN
            RM = 23
            PRINT "You climb up..."
            EXIT SUB
        END IF
    END IF

    IF RM = 81 THEN
        IF LO(9) = 0 OR LO(9) = RM THEN
            RM = 82
            PRINT "You climb up..."
            EXIT SUB
        END IF
    END IF

    PRINT "You need the rope first."
END SUB

SUB DoRead
    IF LO(N) <> 0 AND LO(N) <> RM THEN
        PRINT "You can't read that!"
        EXIT SUB
    END IF

    IF N = 17 THEN
        PRINT "The book reads (in part):"
        PRINT "'...to defeat mordimar, you must be wielding the broadsword. It is the only"
        PRINT "weapon which can work against his black magic. The others will not damage"
        PRINT "him enough. I have yet to acquire it, but someday I shall finally defeat"
        PRINT "him! - Tursk'"
        EXIT SUB
    END IF

    IF N = 19 THEN
        PRINT "It is written in an unfamiliar tongue."
        EXIT SUB
    END IF

    PRINT "You read it with little interest."
END SUB

SUB DoBuy
    IF RM <> 3 THEN
        PRINT "You are not in the village shop!"
        EXIT SUB
    END IF

    IF LO(37) <> RM THEN
        PRINT "The clerk isn't here."
        EXIT SUB
    END IF

    IF LO(N) <> 98 THEN
        PRINT "The clerk says,'We don't carry that.'"
        EXIT SUB
    END IF

    IF IC >= 8 THEN
        PRINT "The clerk says,'You can't carry that.'"
        EXIT SUB
    END IF

    IF P(N) = 99 THEN
        PRINT "The clerk says,'You can't buy that.'"
        EXIT SUB
    END IF

    IF P(N) > G THEN
        PRINT "The clerk says,'You don't have enough gold.'"
        EXIT SUB
    END IF

    G = G - P(N)
    LO(N) = 0
    IC = IC + 1
    PRINT "You hand clerk"; P(N); "gold coins."
    PRINT "He hands you the "; NO$(N); "."
    PRINT "He says,'Thank you for your business.'"
END SUB

SUB DoSell
    IF RM <> 3 THEN
        PRINT "You are not in the village shop!"
        EXIT SUB
    END IF

    IF LO(37) <> RM THEN
        PRINT "The clerk isn't here."
        EXIT SUB
    END IF

    IF LO(N) = 105 THEN
        PRINT "The clerk says,'You must unwield that first.'"
        EXIT SUB
    END IF

    IF LO(N) = 205 THEN
        PRINT "The clerk says,'You must remove that first.'"
        EXIT SUB
    END IF

    IF LO(N) <> 0 THEN
        PRINT "The clerk says,'I don't see you carrying that.'"
        EXIT SUB
    END IF

    LO(N) = 98
    CG = P(N) / 2
    G = G + CG
    IC = IC - 1
    PRINT "You hand clerk the "; NO$(N); "."
    PRINT "He hands you"; CG; "gold coins."
    PRINT "He says,'Thank you for your business.'"
END SUB

SUB DoFight
    DIM IHit AS INTEGER, CHit AS INTEGER, ThisWD AS INTEGER

    IF N < 36 THEN
        PRINT "You can't fight that!"
        EXIT SUB
    END IF

    IF RM > 72 AND RM < 84 THEN
        PRINT "You can't fight here. This is a sacred place."
        EXIT SUB
    END IF

    IF LO(N) <> RM THEN
        PRINT "The "; NO$(N); " isn't here."
        EXIT SUB
    END IF

    RANDOMIZE TIMER
    MH = INT(RND * 150) + 50
    IF N = 50 THEN MH = 820

    DO
        RANDOMIZE TIMER
        IHit = INT(RND * 35) + 1
        PRINT "You are fighting the "; NO$(N); "."
        PRINT ">"

        ThisWD = WieldedItem%
        DM = ThisWD
        IF ThisWD = 0 THEN
            PRINT "You are wielding nothing!"
        ELSE
            PRINT "You are wielding the "; NO$(ThisWD); "."
        END IF

        PRINT ">"

        IF IHit <= 5 THEN PRINT "You missed."
        IF IHit >= 5 AND IHit <= 10 THEN
            PRINT "You hit "; NO$(N); "."
            MH = MH - 10
        ELSEIF (IHit >= 15 AND IHit <= 20) OR (IHit >= 20 AND IHit <= 25) THEN
            IF DM = 31 OR (DM > 32 AND DM < 36) THEN
                IF DM = 31 THEN PRINT "Your BROADSWORD glows!"
                IF DM = 33 THEN PRINT "Your GLOWBALL splits into eight pieces!"
                IF DM = 34 THEN PRINT "Your SCEPTER shoots lightning at "; NO$(N); "!"
                IF DM = 35 THEN PRINT "Your SLAYER vibrates!"
            END IF
        END IF

        IF IHit >= 10 AND IHit <= 15 THEN
            PRINT "You hit "; NO$(N); " very hard."
            MH = MH - 15
            IF DM = 31 OR (DM > 32 AND DM < 36) THEN MH = MH - 10
        END IF

        IF IHit >= 15 AND IHit <= 20 THEN
            PRINT "You smashed "; NO$(N); " with a bone-crushing sound."
            MH = MH - 30
            IF DM = 31 OR (DM > 32 AND DM < 36) THEN MH = MH - 15
        END IF

        IF IHit >= 20 AND IHit <= 25 THEN
            PRINT "You massacred "; NO$(N); " into small fragments."
            MH = MH - 40
            IF DM = 31 OR (DM > 32 AND DM < 36) THEN MH = MH - DM
        END IF

        IF IHit = 26 AND DM = 31 THEN
            PRINT "A bolt of lightning streaks down from above!"
            PRINT "Your BROADSWORD strikes "; NO$(N); " down!"
            MH = 0
        END IF

        IF IHit = 27 AND DM = 33 THEN
            PRINT "Your GLOWBALL strikes "; NO$(N); " very hard!"
            MH = MH - 55
        END IF

        IF IHit = 28 AND DM = 34 THEN
            PRINT "Your SCEPTER shoots flame at "; NO$(N); "!"
            MH = MH - 65
        END IF

        IF IHit = 29 AND DM = 35 THEN
            PRINT "Your SLAYER leaps from your hands! It impales "; NO$(N); "!"
            MH = MH - 75
        END IF

        IF IHit >= 30 THEN
            PRINT "Your attack was blocked by "; NO$(N); "."
        ELSEIF LO(19) = 0 AND IHit >= 30 THEN
            PRINT "You cast HEAL..."
            PRINT "You are healed fully!"
            HP = 192
        END IF

        TEMPO = 1
        CALL PauseSeconds(TEMPO)
        PRINT ">"
        PRINT "The monster "; NO$(N); " is attacking."
        TEMPO = 1
        CALL PauseSeconds(TEMPO)
        PRINT ">"

        IF IHit = 32 AND LO(19) = 0 THEN
            PRINT "You cast FIRE-BALL..."
            PRINT FB$
            PRINT FB2$
            MH = 0
        END IF

        RANDOMIZE TIMER
        CHit = INT(RND * 35) + 1
        IF CHit <= 5 THEN PRINT "It missed you."
        IF CHit >= 5 AND CHit <= 10 THEN
            PRINT "It hit you."
            DT = 7
            HP = HP - DT
            IF AC > DT THEN HP = HP + 2
        END IF
        IF CHit >= 10 AND CHit <= 15 THEN
            PRINT "It hit you very hard."
            DT = 12
            HP = HP - DT
            IF AC > DT THEN HP = HP + 4
        END IF
        IF CHit >= 15 AND CHit <= 20 THEN
            PRINT "It smashed you with a bone-crushing sound."
            DT = 20
            HP = HP - DT
            IF AC > DT THEN HP = HP + 8
        END IF
        IF CHit >= 20 AND CHit <= 25 THEN
            PRINT "It massacred you into small fragments."
            DT = 40
            HP = HP - DT
            IF AC > DT THEN HP = HP + 15
        END IF

        IF IHit = 26 AND N = 50 THEN
            PRINT "A bolt of lightning streaks down from the heavens!"
            PRINT "You are fried to death!"
            TEMPO = 1: CALL PauseSeconds(TEMPO)
            PRINT "You died."
            TEMPO = 1: CALL PauseSeconds(TEMPO)
            ReturnToTitle = -1
            EXIT SUB
        END IF

        PRINT ">"
        TEMPO = 1
        CALL PauseSeconds(TEMPO)

        IF MH <= 0 THEN
            PRINT "The monster died."
            PRINT "You killed "; NO$(N); "."
            TEMPO = 1
            CALL PauseSeconds(TEMPO)
            PRINT ">"
            EXIT DO
        END IF

        PRINT "Your HP:"; HP
        PRINT "Their HP:"; MH
        PRINT ">"
        IF HP <= 0 THEN
            PRINT "You died."
            PRINT
            TEMPO = 1
            CALL PauseSeconds(TEMPO)
            ReturnToTitle = -1
            EXIT SUB
        END IF
    LOOP

    FOR X = 7 TO 35
        IF LO(X) = 1000 + N THEN
            PRINT "You found "; NO$(X); " on it!"
            LO(X) = RM
        END IF
    NEXT X

    PRINT "You gained"; DT; "gold pieces and"; N; "hit points."
    HP = HP + N
    G = G + DT
    LO(N) = 998
    CALL CheckVictory
END SUB

SUB ShowContainerContents (ItemIndex)
    IF ItemIndex = 10 THEN
        PRINT "      The knapsack holds:"
        S1 = 0
        FOR Y = 7 TO 35
            IF LO(Y) = 305 THEN
                S1 = 1
                PRINT "        ("; NO$(Y); ")."
            END IF
        NEXT Y
        IF S1 = 0 THEN PRINT "        (Nothing)"
    ELSEIF ItemIndex = 11 THEN
        S1 = 0
        PRINT "      The backpack holds:"
        FOR Y = 7 TO 35
            IF LO(Y) = 405 THEN
                S1 = 1
                PRINT "        ("; NO$(Y); ")."
            END IF
        NEXT Y
        IF S1 = 0 THEN PRINT "        (Nothing)"
    END IF
END SUB

SUB DoPut
    IF LO(N) <> 0 THEN
        PRINT "You must be carrying that first!"
        EXIT SUB
    END IF

    IF LO(10) <> 0 AND LO(11) <> 0 AND LO(10) <> RM AND LO(11) <> RM THEN
        PRINT "You aren't carrying the knapsack or backpack!"
        EXIT SUB
    END IF

    TL = 0
    IF NE2$ = "knapsack" THEN TL = 10
    IF NE2$ = "backpack" THEN TL = 11

    IF TL = 0 THEN
        PRINT "You can't place it there."
        EXIT SUB
    END IF

    IF TL = 10 THEN
        LO(N) = 305
        IC = IC - 1
        PRINT "Ok."
        EXIT SUB
    END IF

    LO(N) = 405
    IC = IC - 1
    PRINT "Ok."
END SUB

SUB DoEat
    IF LO(N) <> 0 AND LO(N) <> RM THEN
        PRINT "That isn't here."
        EXIT SUB
    END IF

    IF N = 13 THEN
        HP = 192
        PRINT "You eat the food."
        PRINT "You feel much better!"
        LO(13) = 1
        IC = IC - 1
        EXIT SUB
    END IF

    PRINT "You can't eat that."
END SUB

SUB DoDrink
    IF LO(N) <> 0 AND LO(N) <> RM THEN
        PRINT "That isn't here."
        EXIT SUB
    END IF

    IF N = 14 THEN
        IF LO(16) <> 0 AND LO(16) <> RM THEN
            PRINT "You need the glass bottle first."
            EXIT SUB
        END IF
        LO(14) = 5
        IC = IC - 1
        CT = 0
        PRINT "Ahhhh! Refreshing!"
        EXIT SUB
    END IF

    IF N = 15 THEN
        LO(15) = 1
        LO(16) = 0
        PRINT "You drink the wine."
        PRINT "It tastes great!"
        EXIT SUB
    END IF

    PRINT "You can't drink that."
END SUB

SUB CheckThirst
    IF CT = 50 THEN PRINT "You are thirsty."
    IF CT = 100 THEN PRINT "You are very thirsty."
    IF CT = 150 THEN
        PRINT "You have died of thirst."
        TEMPO = 1
        CALL PauseSeconds(TEMPO)
        PRINT "You died."
        TEMPO = 2
        CALL PauseSeconds(TEMPO)
        ReturnToTitle = -1
        EXIT SUB
    END IF
END SUB

SUB ShowHelp
    PRINT "This is a text adventure. You play by entering in one or two"
    PRINT "word commands (e.g. go north, get food, etc.). Valid commands"
    PRINT "include:"
    PRINT
    PRINT "1. go 2. get 3. drop 4. inventory 5. look 6. wield 7. unwield"
    PRINT "8. wear 9. remove 10. examine 11. use 12. climb 13. read"
    PRINT "14. buy 15. sell 16. kill 17. put 18. eat 19. drink"
    PRINT "20. inventory 21. save game (or just 'save') 22. look"
    PRINT "23. quit 24. help"
    PRINT
END SUB

SUB ShowShopList
    IF RM <> 3 THEN
        PRINT "You are not in the village shop!"
        EXIT SUB
    END IF

    IF LO(37) <> RM THEN
        PRINT "The clerk isn't here."
        EXIT SUB
    END IF

    PRINT "The clerk says,'Here is what we have in stock:"
    SI = 0
    FOR X = 7 TO 35
        IF LO(X) = 98 THEN
            SI = 1
            PRINT P(X); ": "; NO$(X)
        END IF
    NEXT X
    IF SI = 0 THEN PRINT "He scratches his head and says,'Alas, we have nothing in stock...'"
    PRINT "Your gold: "; G
END SUB

SUB ConfirmQuit
    PRINT
    PRINT "Quit Game"
    PRINT
    PRINT "Are you sure (y/n)? ";
    DO
        DO
            B$ = INKEY$
        LOOP UNTIL B$ <> ""

        IF B$ = "y" THEN
            PRINT B$
            PRINT
            PRINT "Ok...thanks for playing!"
            TEMPO = 2
            CALL PauseSeconds(TEMPO)
            QuitProgram = -1
            EXIT SUB
        ELSEIF B$ = "n" THEN
            PRINT B$
            PRINT
            EXIT SUB
        END IF
    LOOP
END SUB

SUB LoadGame
    PRINT
    PRINT "Loading...";
    TEMPO = 1
    CALL PauseSeconds(TEMPO)

    ON ERROR GOTO LoadFailed
    OPEN "ADVXT.SAV" FOR INPUT AS #1
    INPUT #1, G
    INPUT #1, IC
    INPUT #1, HP
    INPUT #1, RM
    INPUT #1, AC
    INPUT #1, LT
    INPUT #1, WD
    INPUT #1, DM
    FOR X = 7 TO 56
        INPUT #1, LO(X)
    NEXT X
    CLOSE #1
    ON ERROR GOTO 0
    PRINT "Done."
    TEMPO = 1
    CALL PauseSeconds(TEMPO)
    EXIT SUB

LoadFailed:
    ON ERROR GOTO 0
    PRINT
    PRINT "Saved game is not accessible or doesn't exist."
    PRINT "You have to start from the beginning."
END SUB

SUB SaveGame
    PRINT "Saving...";
    TEMPO = 1
    CALL PauseSeconds(TEMPO)
    OPEN "ADVXT.SAV" FOR OUTPUT AS #1
    PRINT #1, G
    PRINT #1, IC
    PRINT #1, HP
    PRINT #1, RM
    PRINT #1, AC
    PRINT #1, LT
    PRINT #1, WD
    PRINT #1, DM
    FOR X = 7 TO 56
        PRINT #1, LO(X)
    NEXT X
    CLOSE #1
    PRINT "Done."
    TEMPO = 1
    CALL PauseSeconds(TEMPO)
END SUB

SUB ShowMap
    CLS
    PRINT "Map of Adventure XT                    Hall2--Room4                            "
    PRINT "                                      /                        23/branch--hut24"
    PRINT "           8Toolshed--7Garden   Tavern1      26RWF---RWF25  22RWF              "
    PRINT "                         |        |                   |        |               "
    PRINT "                      6Church--Fountain5--Shop3    19RWF----20RWF--RWF21       "
    PRINT "                                  |                            |     47        "
    PRINT "                     27BF      Village Ent9                Clearing18 T-DE48   "
    PRINT "                  28 29 |  30     |                   16    17 |   43 |44 45 46"
    PRINT "                   BF--BF--BF     |  Pond15 River---River DE-T-T-Keep          "
    PRINT "                    |      |      |10   13  14 |      |       39  40  |41      "
    PRINT "             31BF--BF32    BF11--On--Meadow--Mead.  River  Tunnel--T--T-DE42   "
    PRINT "                           |     |     34  35  36     | 37   |                 "
    PRINT "                         33BF--Inter12-DF--DF--DF---River--Cave38              "
    PRINT "                                 |         |   |51                             "
    PRINT "                               Bridge49  50DF--DF---Alcove52  53Kitchen  RM54  "
    PRINT "                                 |             |    57  58  59  60 |    61|62  "
    PRINT "                               Bridge55      56DF---LF--LF--LF--Cottage-H-H-R63"
    PRINT "                                 |                   |   |67 68                "
    PRINT "      Mordimar64               Goblin65    [inset]  LF66LF--LF--LF69           "
    PRINT "           |        71       72  |           |               |                 "
    PRINT "    70Throne RM--Fountain--Shadow Castle   SNW73         Small Path74          "
    PRINT "                                             |      76       |                 "
    PRINT "                                         75SB P--SB Patch--Path77              "
    DO: A$ = INKEY$: LOOP UNTIL A$ <> ""
    PRINT "--------------------------*INSET*----------------------                        "
    PRINT "|                Brainy82                             |                        "
    PRINT "|                   |  81          83           84    |                        "
    PRINT "|    79Jokey-----SM Forest--Outside G Castle--G Castle|                        "
    PRINT "|        |                                            |                        "
    PRINT "|    78Papa--Handy80                                  |                        "
    PRINT "|        |                                            |                        "
    PRINT "|    [to SNW]                                         |                        "
    PRINT "-------------------------------------------------------                        "
END SUB

SUB CheckVictory
    IF N <> 50 THEN EXIT SUB
    PRINT
    PRINT "@@@ You have defeated Mordimar!!! @@@"
    TEMPO = 2
    CALL PauseSeconds(TEMPO)
    PRINT
    PRINT "Mordimar's crumbled body lay at your feet. As you"
    PRINT "gaze into the Orb, you sense the world returning to"
    PRINT "normal! The woodland creatures send great happiness"
    PRINT "to your heart! It swells with joy!"
    PRINT
    PRINT "'You have done a great deed!' their voices say."
    PRINT "'Now we can come out and play!'"
    PRINT
    PRINT "The world thanks you for your courage..."
    PRINT
    PRINT "@@@ The End @@@"
    TEMPO = 20
    CALL PauseSeconds(TEMPO)
    PRINT
    PRINT "@@@ Press any key @@@"
    DO: A$ = INKEY$: LOOP UNTIL A$ <> ""
    QuitProgram = -1
END SUB

SUB PauseSeconds (Tempo)
    DIM T AS SINGLE
    T = TIMER
    WHILE TIMER < T + Tempo
    WEND
END SUB

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
