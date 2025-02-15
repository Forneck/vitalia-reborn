#0
Non-attachable trigger~
0 g 100
~
* You can't attach trigger 0!
~
#1
Mob Tutorial Example Quest Offer - M14~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A very basic 3 trigger receive quest. Trigs 1-3.
* Make sure the actor is a player first.
if %actor.is_pc% && %direction% == south
  * only greet players coming from the south.
  * wait 1 second, always give the player time before you start sending text.
  wait 1 sec
  say Can you help me, %actor.name%?
  wait 1 sec
  say An ogre has something of mine.
  wait 1 sec
  say If you slay him I'll give you all the coins I can spare.
  wait 1 sec
  say Please, bring me the wings he has stolen.
end
~
#2
Mob Tutorial Example Kill Ogre - 16~
0 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A very basic 3 trigger receive quest. Trigs 1-3.
say You got the best of me, %actor.name%. But I'll be back.
* Load the wings to be returned to the questmaster.
%load% obj 1
* Reload the mob for the next questor.
%load% mob %self.vnum%
~
#3
Mob Tutorial Example Completion - 14~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A very basic 3 trigger receive quest. Trigs 1-3.
* Check if this was indeed the right object.
if %object.vnum% == 1
  wait 1 sec
  say Thank you, %actor.name%
  %send% %actor% %self.name% gives you a gold piece.
%echoaround% %actor% %actor.name% is rewarded for %hisher% valor.
  * Reward the actor with an entire gold coin!
  nop %actor.gold(1)%
  wait 5 sec
  %purge% %object%
else
  * This isn't the right object - don't accept it.
  say I don't want that - bring me back my wings.
  return 0
end
~
#4
Tutorial II Guard Greet - 24~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A basic guard bribe trigger. Trigs 4, 5, 7, 8.
* This is a N S road so only greet players arriving from the south.
if %direction% == south && %actor.is_pc%
  wait 1 sec
  emote snaps to attention.
  wait 1 sec
  say Admittance to the city is 10 coins.
end
~
#5
Tutorial II Guard Bribe 10 - 24~
0 m 1
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A basic guard bribe trigger. Trigs 4, 5, 7, 8.
* This is a comment. Always use amplifying comments to explain your Triggers!
* If actor gives 10 coins or more.
if %amount% >= 10
  * If more than 10 give them change.
  if %amount% > 10
    eval change %amount% - 10
    give %change% coin %actor.name%
  end
  * Otherwise they must have given exactly 10 coins, open the gate.
  say Thank you.
  wait 1 sec
  unlock gateway
  wait 1 sec
  open gateway
  wait 10 sec
  close gateway
  wait 1 sec
  lock gateway
  * Else they gave too few! be nice and refund them.
else
  say only %amount% coins, I require 10.
  give %amount% coin %actor.name%
end
~
#6
Obj Command Magic Eight Ball - O47~
1 c 2
shake~
* By Rumble of The Builder Academy    tbamud.com 9091
* Numeric Arg: 2 means in character's carried inventory.
* Command trigs do not work for level 33 and above.
* There are 20 possible answers that the Magic Eight Ball can give.
* Of these, nine are full positive, two are full negative, one is
* mostly positive, three are mostly negative, and five are abstentions.
*
* Check arguments if they match. /= checks abbreviations.
set argcheck 'ball 'eightball
if %argcheck.contains('%arg%)%
  * Echo text to everyone else in the room and the actor.
  %echoaround% %actor% %actor.name% shakes %self.shortdesc% vigorously.
  %send% %actor% You shake %self.shortdesc% vigorously.
  * Use a switch to choose a random response (1-20).
  switch %random.20%
    * Send the answer! %self% is the 8ball, or whatever the trig is attached to.
    * Only the actor sees the answer.
    * Case is what we are trying to match. Does %random.20% == 1?
    case 1
      %send% %actor% %self.shortdesc% reveals the answer: Outlook Good
      * We are done with this case so check the next one.
    break
    case 2
      %send% %actor% %self.shortdesc% reveals the answer: Outlook Not So Good
    break
    case 3
      %send% %actor% %self.shortdesc% reveals the answer: My Reply Is No
    break
    case 4
      %send% %actor% %self.shortdesc% reveals the answer: Don't Count On It
    break
    case 5
      %send% %actor% %self.shortdesc% reveals the answer: You May Rely On It
    break
    case 6
      %send% %actor% %self.shortdesc% reveals the answer: Ask Again Later
    break
    case 7
      %send% %actor% %self.shortdesc% reveals the answer: Most Likely
    break
    case 8
      %send% %actor% %self.shortdesc% reveals the answer: Cannot Predict Now
    break
    case 9
      %send% %actor% %self.shortdesc% reveals the answer: Yes
    break
    case 10
      %send% %actor% %self.shortdesc% reveals the answer: Yes, definitely
    break
    case 11
      %send% %actor% %self.shortdesc% reveals the answer: Better Not Tell You Now
    break
    case 12
      %send% %actor% %self.shortdesc% reveals the answer: It Is Certain
    break
    case 13
      %send% %actor% %self.shortdesc% reveals the answer: Very Doubtful
    break
    case 14
      %send% %actor% %self.shortdesc% reveals the answer: It Is Decidedly So
    break
    case 15
      %send% %actor% %self.shortdesc% reveals the answer: Concentrate And Ask Again
    break
    case 16
      %send% %actor% %self.shortdesc% reveals the answer: Signs Point To Yes
    break
    case 17
      %send% %actor% %self.shortdesc% reveals the answer: My Sources Say No
    break
    case 18
      %send% %actor% %self.shortdesc% reveals the answer: Without A Doubt
    break
    case 19
      %send% %actor% %self.shortdesc% reveals the answer: Reply Hazy, Try Again
    break
    case 20
      %send% %actor% %self.shortdesc% reveals the answer: As I See It, Yes
    break
    * Every switch should have a default. A catch-all if the cases do not match.
    default
      %send% %actor% %self.shortdesc% explodes since your question is unanswerable.
    break
    * Every switch must have a done! Just like every if needs an end!
  done
  * The actor didn't use the command shake with arg ball or eightball.
else
  * Return 0 allows the command to continue through to the MUD. The player will
  * get the Huh!?! response or the shake social if you have one.
  return 0
end
~
#7
Tutorial II Guard Closes Gate - 24~
0 e 0
The gate is opened from~
* By Rumble of The Builder Academy    tbamud.com 9091
* A basic guard bribe trigger. Trigs 4, 5, 7, 8.
* This is required to close the gate after someone opens it from the other
* side.
wait 5 sec
close gate
wait 1 sec
lock gate
~
#8
Tutorial II Guard Closes Gate 2 - 24~
0 e 0
leaves north.~
* By Rumble of The Builder Academy    tbamud.com 9091
* A basic guard bribe trigger. Trigs 4, 5, 7, 8.
* This is required to close the gate after the guard is bribed and someone
* leaves to the north.
wait 5 sec
close gate
wait 1 sec
lock gate
~
#9
Tutorial Quest 1317 - Starter~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Quest Trigs 9-12. If the player comes from the south and has not done the quest.
if %direction% == south && %actor.varexists(solved_tutorial_quest_zone_0)%
  wait 1 sec
  say you have already completed this quest.
  wait 2 sec
  say but you can try it again if you so desire. Would you like to find the magic eight ball again?
  rdelete solved_tutorial_quest_zone_0 %actor.id%
else
  wait 1 sec
  say Hello, %actor.name%. Could you find me the magic eight ball?
  say Please say yes, %actor.name%.
end
~
#10
Tutorial Quest 1317 - Accept~
0 d 1
yes~
* By Rumble of The Builder Academy    tbamud.com 9091
* Quest Trigs 9-12. If the player has not done the quest give them a hint.
if !%actor.varexists(solved_tutorial_quest_zone_0)%
  wait 1 sec
  say Perfect, %actor.name%. I'll make this easy. It is to the east.
  wait 3 sec
  say I'd go get it myself, but I'm lazy and you need the exercise.
  wait 1 sec
end
~
#11
Tutorial Quest 1317 - Completion~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Quest Trigs 9-12. If the player returns the right object reward them.
if !%actor.varexists(solved_tutorial_quest_zone_0)%  && %object.vnum% == 47
  set solved_tutorial_quest_zone_0 1
  remote solved_tutorial_quest_zone_0 %actor.id%
  %purge% %object%
  dance
  wait 1 sec
  say Thank you, %actor.name%.
  nop %actor.exp(50)%
  nop %actor.gold(50)%
  say finally, now I can get some answers.
  wait 1 sec
  emote shakes the magic eight ball vigorously.
  wait 1 sec
  emote does not seem too pleased with his answer.
else
  say I don't want that!
  junk %object.name%
end
~
#12
Tutorial Quest 1441 - Load 8ball~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Quest Trigs 9-12. Drop the 8ball on their head when they enter.
wait 1 sec
%echoaround% %actor% A magic eight ball drops from the sky striking %actor.name% on the head.
%send% %actor% A magic eight ball drops from the sky striking you on the head.
%load% obj 1394
%damage% %actor% %random.5%
~
#13
Restorative Comfy Bed 1401 - Sleep~
1 c 4
sl~
set argcheck 'bed
if %cmd.mudcommand% == sleep && %argcheck.contains('%arg%)%
  %force% %actor% sleep
  set laying_in_comfy_bed_14 1
  remote laying_in_comfy_bed_14 %actor.id%
end
~
#14
Restorative Comfy Bed 1401 - Heal~
1 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Healing Bed Trigs 13-15. Heals those who sleep on it.
set room_var %actor.room%
set target_char %room_var.people%
while %target_char%
  set tmp_target %target_char.next_in_room%
  if %target_char.varexists(laying_in_comfy_bed_14)%
    %damage% %target_char% -10
    %send% %target_char% You feel refreshed from sleeping in the comfy bed.
  end
  set target_char %tmp_target%
done
~
#15
Restorative Comfy Bed 1401 - Wake~
1 c 100
wa~
* does not work for level 33 and above.
if %cmd.mudcommand% == wake
  %force% %actor% wake
  rdelete laying_in_comfy_bed_14 %actor.id%
end
~
#16
Damage Example~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* First put a wait in so the echo waits for the player to get into the room.
wait 1 sec
%echo% As %actor.name% enters the room a bolt of lightning smites %actor.himher%.
* Take away 10 hitpoints
%damage% %actor% 10
wait 5 sec
%echo% The Gods feel sorry for %actor.name% and restore him to full health.
* Restore 10 hitpoints (-# heals the actor).
%damage% %actor% -10
* Another Example that damages actor to 1 hitpoint
wait 2 sec
%echo% hp: %actor.hitp%
eval all_but_one_hitp %actor.hitp% - 1
%echo% all_but_one_hitp: %all_but_one_hitp%
if %all_but_one_hitp% > 0
  %damage% %actor% %all_but_one_hitp%
  %echo% hp: %actor.hitp%
end
~
#17
Door Example~
2 c 100
move~
* Example by Falstar for room 14520
* The following trigger is designed to reveal a trapdoor leading down when
* Player types 'Move Chest'
*
* The following ARGument determines what can be MOVEd ('move' Command inserted
* in Arg List of trigger)
if %arg% == chest
  *Send text to player to show effect of command
  %send% %actor% You move the chest across the floor, revealing a trapdoor underneath!!
  *Send text to other players in room to inform them of Player's discovery
  %echoaround% %actor% %actor.name% has discovered a hidden trapdoor under a chest!
  * Set door flags to 'ab' - Exit is a door that can be opened and closed, then close door
  %door% 14520 down flags ab
  * Change door name to 'trapdoor' (used in door commands, eg open trapdoor, lock trapdoor)
  %door% 14520 down name trapdoor
  * Set room that trapdoor exits leads to (Room 14521 in this case)
  %door% 14520 down room 14521
  * Set Exit desc for the wary adventure who decides to 'look down'
  %door% 14520 down description A wooden ladder leads down into the darkness.
  * Set the Vnum of the KEY OBJECT which will lock/unlock door
  %door% 14520 down key 14500
  * Now for Continuity, door commands must be set for reverse direction FROM 14521
  %door% 14521 up flags ab
  %door% 14521 up name trapdoor
  %door% 14521 up room 14520
  %door% 14521 up description A wooden ladder leads up into a storeroom.
  %door% 14521 up key 14500
  * IMPORTANT: Note reverse of direction in both the commands and extra
  * descriptions and room numbers it can be very easy to get lost here and
  * probably get your adventurer lost too. Finally set up what happens when
  * adventurer tries to move anything else and end the trigger
else
  %send% %actor% Move what ?!
end
* Another example by Welcor
* if %arg% == tapestries
*   %send% %actor% You pull the tapestries down, and reveal a secret vault!
*   %echoaround% %actor% As %actor.name% pulls the tapestries down a secret vault is revealed.
*   %door% 23667 east flags abcd
*   %door% 23667 east key 23634
*   %door% 23667 east name vault
*   %door% 23667 east room 23668
* else
*   %send% %actor% Pull what ?!
* end
~
#18
Switch Echo Example~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Since this is an ENTER trigger we must put a wait in here
* so it doesn't fire before the player enters the room
wait 1
switch %random.4%
  case 1
    * only the person (actor) entering the room will see this.
    %send% %actor% You trip over a root as you walk into the room.
    * everyone in the room except the actor will see this.
    %echoaround% %actor% %actor.name% trips on a root while walking into the room.
    * everyone in the room will see this.
    %echo% The root suddenly springs to life and attacks!
    * let everyone in the zone hear about this.
    %zoneecho% %self.vnum% %actor.name% is a clutz.
  break
  case 2
    %send% %actor% You strut into the room.
    %echoaround% %actor% %actor.name% Seems to have a big head..
    %echo% A strong breeze kicks some leaves up into the air.
  break
  case 3
    %echo% A light breeze picks up, causing the leaves to rustle quietly.
  break
  case 4
    * echoes go to everyone, even sleeping players. To prevent this try:
    set target_char %self.people%
    while %target_char%
      set tmp_target %target_char.next_in_room%
      if %target_char.pos% != sleeping
        %send% %target_char% You are not sleeping
      else
        %send% %target_char% You are sleeping
      end
      set target_char %tmp_target%
    done
  break
  default
    * this should be here, even if it's never reached
    %echo% This is the default case, just in case I missed something. Get it? Just in case!
  break
done
~
#19
AT Example~
2 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
%at% 33 %echo% at'd to a room
%at% Rumble %echo% at'd to Rumble
~
#20
Rumble's Spy~
0 d 100
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* Arguments: * means all speech will trigger this.
* This will echo all speech to Rumble.
%at% rumble say %actor.name% says, '%speech%' 1
* doesn't work:
%at% rumble mecho %actor.name% says, '%speech%' 2
~
#21
Transform Example~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* As a greet trigger, entering the room will cause this mob to transform into
* a random mob vnum 100-199.
emote starts to twist and contort into something different.
eval number %random.100% + 99
%transform% %number%
say Transform complete to mob %number%.
~
#22
IF Example~
0 g 100
~
* By Rumble & Relsqui of The Builder Academy    tbamud.com 9091
* First we set %anumber% to some number between 1 and 100.
set anumber %random.100%
* Then the beginning of the if-block.
if %anumber% == 5
  * The following commands are only executed if the above condition is true.
  * That is, if %anumber% is equal to 5.
  clap
  say It's your lucky day!
  say You picked my favorite number!
  * If those commands were executed, the program skips to the next end.
  * Otherwise, it looks for an elseif.
elseif %anumber% > 90
  * To read the following commands, the program must have determined the
  * following: The first condition %anumber% == 5 is false.
  * The second condition %anumber% > 90 is true.
  emote shrinks down to the size of a mouse.
  say Squeak squeak squeak!
  * If the first elseif condition was also false, the program looks for the
  * next one.
elseif %anumber% < 10
  * Here's a tricky one. If %anumber% equals 5, the program will already have
  * run the commands immediately after the if-statement at the top. Therefore,
  * the following commands only run when: %anumber% is not five (failing the;* first check) %anumber% is not greater than 90 (failing the second check)
  * %anumber% is less than 10 (passing the third check.)
  emote grows an elephant's nose.
  emote blows a mighty blast with %self.hisher% trunk.
  * And now, the default, when %anumber% fails all the above checks.
else
  * Note that else has no condition.
  emote disappears in a cloud of blue smoke.
  %purge% %self%
  * For that to happen, all of this must be true: %anumber% is not five.
  * %anumber% is not greater than 90. %anumber% is not less than 10. If all
  * those conditions are met, the mob will disappear in a cloud of blue smoke.
  * Finally, please don't forget the all-important...
end
* Ends are good. They tell the program when the if-block is over. After any of
* the above sets of commands runs, the program skips to the next end.
*
* We could have omitted the else. If we had, and none of the conditions had
* been met, no commands in the if-block would run. However, there can never be
* more than one else. There is only one default. There must always be the same
* number of if's and end's. We could have had any number of ifelses, from zero
* on up. To summarize, here is the basic format of an if-block:
* if (condition)
*   (commands)
* elseif (condition) <---optional. 0 or more
*   (commands)
* else <---optional. 1 or 0
*   (commands)
* end
*
* So the simplest possible if-block would be:
* if (condition)
*   (commands)
* end
~
#23
While Damage Example - Grenade O1301~
1 h 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A small script to make a grenade go off 3 seconds after it is dropped.
* Set the rooms ID to a variable.
set room_var %actor.room%
wait 3 s
* Send a message when the bomb goes off.
%echo% The Grenade blasts into smithereens, striking everyone here.
* Target the first char.
set target_char %room_var.people%
* Now loop through everyone in the room and hurt them.
while %target_char%
  * Set the next target before this one perhaps dies.
  set tmp_target %target_char.next_in_room%
  * This is where the good/bad things are supposed to happen.
  %send% %target_char% The explosion hurts you.
  * Damage them... 30 hitpoints. To heal use -#.
  %damage% %target_char% 30
  * Set the next target.
  set target_char %tmp_target%
  * Loop back.
done
* After we go through everyone get rid of the grenade.
%purge% %self%
~
#24
Room While Teleport Example~
2 bg 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Target the first person or mob in the room.
eval person %self.people%
wait 1 sec
*While there are still mobs or people in the room.
while %person%
  %send% %person% You are next!
  %echo% I am targetting %person.name%.
  %echoaround% %person% %person.name% is struck by a bolt of lightning. Leaving only a pile of ash.
  %teleport% %person% 3
  %force% %person% look
  eval person %self.people%
done
~
#25
Command Example~
2 c 100
l~
* By Rumble of The Builder Academy    tbamud.com 9091
* A command trigger to demonstrate basic usage. The Arg list 'l' will capture
* any 'l' word. We are looking specifically for the MUD command (all commands
* are listed under COMMANDS) 'look' with the abbreviated argument of 'window.'
if %cmd.mudcommand% == look && window /= %arg%
  %echo% %actor.name% typed %cmd% to %cmd.mudcommand% at the %arg%.
else
  * Without this return zero a player would be unable to look at anything else
  * in the room, leave, list, etc.
  return 0
end
~
#26
Blocks Mobs Not Following~
2 g 100
~
Name: 'Blocks Mobs Not Following',  VNum: [   26], RNum: [   26]
Trigger Intended Assignment: Rooms
Trigger Type: Enter , Numeric Arg: 100, Arg list: None
Commands:
* By Rumble of The Builder Academy    tbamud.com 9091
* This trigger blocks all mobs except those that are following.
* If actor is a mob, !is_pc actually means not a PC.
if !%actor.is_pc%
  * If the mob does not have a master.
  if !%actor.master%
    * Return 0 prevents the mob from entering.
    return 0
  end
end
~
#27
Actor Variables Example~
0 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A listing and demonstration of all actor variables.
* Random triggers do not have actors so we must define one.
set actor %random.char%
* Make sure the actor can be seen and is a player.
if %actor.canbeseen% && %actor.is_pc%
  %echo% CHARISMA:        %actor.cha%
  %echo% CONSTITUTION:    %actor.con%
  %echo% DEXTERITY:       %actor.dex%
  %echo% INTELLIGENCE:    %actor.int%
  %echo% STRENGTH/STRADD: %actor.str%/%actor.stradd%
  %echo% WISDOM:          %actor.wis%
  %echo% ALIAS:           %actor.alias%
  %echo% ALIGN:           %actor.align%
  %echo% AFFECT:          %actor.affect%
  %echo% CANBESEEN:       %actor.canbeseen%
  %echo% CLASS:           %actor.class%
  %echo% DRUNK:           %actor.drunk%
  %echo% EXPERIENCE:      %actor.exp%
  %echo% FIGHTING:        %actor.fighting%
  %echo% FOLLOWER:        %actor.follower%
  %echo% GOLD:            %actor.gold%
  %echo% HAS_ITEM:        %actor.has_item(1332)%
  %echo% HESHE:           %actor.heshe%
  %echo% HIMHER:          %actor.himher%
  %echo% HISHER:          %actor.hisher%
  %echo% HITPOINTS:       %actor.hitp%
  %echo% HUNGER:          %actor.hunger%
  %echo% ID:              %actor.id%
  %echo% INV:             %actor.inventory%
  %echo% IS_KILLER:       %actor.is_killer%
  %echo% IS_PC:           %actor.is_pc%
  %echo% IS_THIEF:        %actor.is_thief%
  %echo% LEVEL:           %actor.level%
  %echo% MANA:            %actor.mana%
  %echo% MASTER:          %actor.master%
  %echo% MAX HP:          %actor.maxhitp%
  %echo% MAX MANA:        %actor.maxmana%
  %echo% MAX MOVE:        %actor.maxmove%
  %echo% MOVE:            %actor.move%
  %echo% NAME:            %actor.name%
  %echo% NEXT_IN_ROOM:    %actor.next_in_room%
  %echo% POSITION:        %actor.pos%
  %echo% PRACTICES:       %actor.prac%
  %echo% QUESTPOINTS:     %actor.questpoints%
  %echo% ROOM:            %actor.room%
  %echo% SAVING_PARA:     %actor.saving_para%
  %echo% SAVING_ROD:      %actor.saving_rod%
  %echo% SAVING_PETRI:    %actor.saving_petri%
  %echo% SAVING_BREATH:   %actor.saving_breath%
  %echo% SAVING_SPELL:    %actor.saving_spell%
  %echo% SEX:             %actor.sex%
  %echo% SKILL BACKSTAB:  %actor.skill(backstab)%
  %echo% THIRST:          %actor.thirst%
  %echo% TITLE:           %actor.title%
  %echo% VAREXISTS:       %actor.varexists(tba_greeting)%
  %echo% VNUM:            %actor.vnum%
  %echo% WEIGHT:          %actor.weight%
  * Objects TSTAT 28, Rooms TSTAT 29, Text TSTAT 30, Special TSTAT 31.
  *
  * equipment positions: 0-18 or light, rfinger, lfinger, neck1, neck2,
  * body, head, legs, feet, hands, arms, shield, about, waist, rwrist,
  * lwrist, wield, hold, inv
  * This example will check wield 16
  if %actor.eq(wield)%
    eval wield %actor.eq(wield)%
    %echo% WIELDING ID: %wield.id%, NAME: %wield.name%, SHORTDESC: %wield.shortdesc%, TIMER:: %wield.timer%, TYPE: %wield.type%, VAL0: %wield.val0%, VAL1: %wield.val1%, VAL2: %wield.val2%, VAL3: %wield.val3%, VNUM: %wield.vnum%,
    %echo% CARRIED_BY: %wield.carried_by%, NEXT_IN_LIST: %wield.next_in_list%, WORN_BY: %wield.worn_by%, WEIGHT: %wield.weight%, COST: %wield.cost%, COST_PER_DAY: %wield.cost_per_day%
  end
end
~
#28
Object Variables Example~
1 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A listing and demonstration of all object variables.
%echo% CARRIED_BY:   %self.carried_by%
%echo% CONTENTS:     %self.contents%
%echo% COST:         %self.cost%
%echo% COST_PER_DAY: %self.cost_per_day%
%echo% COUNT:        %self.count%
%echo% HAS_IN:       %self.has_in%
%echo% ID:           %self.id%
%echo% IS_INROOM:    %self.is_inroom%
%echo% NAME:         %self.name%
%echo% NEXT_IN_LIST: %self.next_in_list%
%echo% ROOM:         %self.room%
%echo% SHORTDESC:    %self.shortdesc%
%echo% TIMER:        %self.timer%
%echo% TYPE:         %self.type%
%echo% VAL0:         %self.val0%
%echo% VAL1:         %self.val1%
%echo% VAL2:         %self.val2%
%echo% VAL3:         %self.val3%
%echo% VNUM:         %self.vnum%
%echo% WEARFLAG:     %self.wearflag%
%echo% WEIGHT:       %self.weight%
%echo% WORN_BY:      %self.worn_by%
%purge% %self%
~
#29
Room Variables Example~
2 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A listing and demonstration of all room variables.
%echo% CONTENTS:   %self.contents%
%echo% NORTH:      %self.north% %self.north(vnum)%
%echo% SOUTH:      %self.south% %self.south(key)%
%echo% SOUTH:      %self.south% %self.south(vnum)%
%echo% EAST:       %self.east% %self.east(bits)%
%echo% WEST:       %self.west% %self.west(room)%
%echo% UP:         %self.up%
%echo% DOWN:       %self.down%
%echo% ID:         %self.id%
%echo% NAME:       %self.name%
%echo% PEOPLE:     %self.people%
%echo% RANDOM.DIR: %self.random.dir%
%echo% SECTOR:     %self.sector%
%echo% VNUM:       %self.vnum%
%echo% WEATHER:    %self.weather%
%echo% CONTENTS 1:   %self.contents(1)%
~
#30
Text Variables Example~
2 d 100
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* A listing and demonstration of all text variables.
%echo% SPEECH:     %speech%
%echo% STRLEN:     %speech.strlen%
%echo% TRIM:       %speech.trim%
%echo% CAR:        %speech.car%
%echo% CDR:        %speech.cdr%
%echo% MUDCOMMAND: %speech.mudcommand%
%echo% CONTAINS:   %speech.contains(test)%
%echo% CHARAT 3:   %speech.charat(3)%
~
#31
Special Variables Example~
2 d 100
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* A listing and demonstration of all special variables.
%echo% SELF ID:    %self.id%
%echo% HOUR:       %time.hour%
%echo% DAY:        %time.day%
%echo% MONTH:      %time.month%
%echo% YEAR:       %time.year%
%echo% PEOPLE:     %people.3%
%echo% RANDOM NUM: %random.99%
%echo% RANDOM PC:  %random.char%
%echo% SPEECH:     %speech%
%echo% FINDOBJ:    There are %findobj.3(33)% objects of vnum 33 in room 3.
%echo%             There are %findobj.3(guide)% objects of name guide in room 3.
%echo% FINDMOB:    There are %findmob.3(33)% mobs of vnum 33 in room 3.
~
#32
Mob Checks Player Inventory~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* This trigger has been replaced by the has_item variable. But this is still a
* good example. if %actor.has_item(1300)% does the same thing.
if %actor.eq(wield)%
  eval wep %actor.eq(wield)%
  if %wep.vnum%==1300
    set has_it 1
  end
end
* Checks the actors inventory if not wielding it.
if !%has_it%
  eval i %actor.inventory%
  while %i%
    set next %i.next_in_list%
    if %i.vnum%==1300
      set has_it 1
      break
    end
    * checks inside containers
    if %i.type% == CONTAINER
      while %i.contents%
        if %i.contents.vnum%==1300
          set has_it 1
          break
        end
      done
    end
    set i %next%
  done
end
*
if %has_it%
  say %actor.name% has that special item.
else
  say %actor.name% doesnt have that crappy item.
end
~
#33
Mob Quote Using Arrays~
0 d 100
quote~
* By Jamie Nelson from the forum http://groups.yahoo.com/group/dg_scripts/
eval w1max %random.21%
eval w2max %random.21%
eval w3max %random.21%
eval w4max %random.21%
eval w5max %random.11%
eval w6max %random.21%l
set  w1[1] rapid
set  w1[2] chilling
set  w1[3] insipid
set  w1[4] nauseating
set  w1[5] astronomical
set  w1[6] austere
set  w1[7] inevitable
set  w1[8] inescapable
set  w1[9] reckless
set  w1[10] haphazard
set  w1[11] accelerating
set  w1[12] profound
set  w1[13] awesome
set  w1[14] terrifying
set  w1[15] ubiquitous
set  w1[16] ignominious
set  w1[17] unprecedented
set  w1[18] unparalleled
set  w1[19] insidious
set  w1[20] broad
set  w1[21] phenomena
set  w2[1] decline
set  w2[2] prospects
set  w2[3] acceleration
set  w2[4] threat
set  w2[5] expansion
set  w2[6] oneness
set  w2[7] outgrowth
set  w2[8] madness
set  w2[9] evacuation
set  w2[10] diminishment
set  w2[11] consumption
set  w2[12] decay
set  w2[13] putrefaction
set  w2[14] vapidity
set  w2[15] downsizing
set  w2[16] degeneration
set  w2[17] litigation
set  w2[18] declivity
set  w2[19] hastening
set  w2[20] paradigm shifting
set  w2[21] growth
set  w3[1] urban tax dollars
set  w3[2] new technologies
set  w3[3] gender identification disorders
set  w3[4] censorship
set  w3[5] interpersonal communications
set  w3[6] modern life
set  w3[7] rampant paradigm shifts
set  w3[8] consumer spending
set  w3[9] rain forests
set  w3[10] human literacy
set  w3[11] natural resources
set  w3[12] traditional values
set  w3[13] media junk food
set  w3[14] family values
set  w3[15] corporate mentality
set  w3[16] the American justice system
set  w3[17] technological change
set  w3[18] the ozone layer
set  w3[19] human resources
set  w3[20] current epistemologies
set  w3[21] the Internet
set  w4[1] escalate
set  w4[2] aggrandize
set  w4[3] overhaul
set  w4[4] deteriorate
set  w4[5] revolutionize
set  w4[6] uglify
set  w4[7] put an end to
set  w4[8] enslave
set  w4[9] bankrupt
set  w4[10] truncate
set  w4[11] nullify
set  w4[12] sabotage
set  w4[13] destabilize
set  w4[14] incapacitate
set  w4[15] hasten
set  w4[16] dehumanize
set  w4[17] evaporate
set  w4[18] indenture
set  w4[19] intensify
set  w4[20] undermine
set  w4[21] forever dissipate
set  w5[1] tomorrow's
set  w5[2] the entrenchment of our
set  w5[3] worldwide
set  w5[4] our children's
set  w5[5] modern
set  w5[6] all of our
set  w5[7] our future
set  w5[8] our
set  w5[9] the demise of our
set  w5[10] our grandchildren's
set  w5[11] all hope for
set  w5[12] today's
set  w6[1] re-ruralization
set  w6[2] human condition
set  w6[3] family values
set  w6[4] self-esteem
set  w6[5] medical insights
set  w6[6] human psyche
set  w6[7] human depth
set  w6[8] egalitarianism
set  w6[9] World Wide Web
set  w6[10] future values
set  w6[11] hopes and dreams
set  w6[12] business models
set  w6[13] political climate
set  w6[14] education
set  w6[15] cultural heritage
set  w6[16] lifestyles
set  w6[17] fiduciary responsibility
set  w6[18] genetic diversity
set  w6[19] intestinal fortitude
set  w6[20] computer literacy
set  w6[21] business models
eval w1 %%w1[%w1max%]%%
eval w2 %%w2[%w2max%]%%
eval w3 %%w3[%w3max%]%%
eval w4 %%w4[%w4max%]%%
eval w5 %%w5[%w5max%]%%
eval w6 %%w6[%w6max%]%%
set msg The %w1% %w2% of %w3% will %w4% %w5% %w6%
say %msg%
~
#34
IS_PC Example~
2 c 100
target~
* By Rumble of The Builder Academy    tbamud.com 9091
* Player = 1, mob = 0, object = -1.
%echo% %actor.name% is targetting %arg% IS_PC: %arg.is_pc%
if %arg.is_pc% == 1
  %echo% It is a player.
elseif %arg.is_pc% == 0
  %echo% It is a mob.
else
  %echo% It is an object.
end
~
#35
Mob Room Specific Speeches~
0 b 5
~
* By Rumble
* So we don't get problems if more than one is loaded.
set room %self.room%
switch %room.vnum%
  case 1300
    say this is where I began my journey.
  break
  case 1301
    say Ah, yes, the beginning.
  break
  case 1302
    say TBA, The Builder Academy Implementation explained!
  break
  case 1303
    say Building blocks for beginners.
  break
  case 1304
    say Writing good descriptions. Very important.
  break
  case 1305
    say Learning redit.
  break
  case 1306
    say Oedit by osmosis.
  break
  case 1307
    say Medit by the numbers.
  break
  case 1308
    say Zedit confuses everyone.
  break
  case 1309
    say Sedit. We all love capitalism.
  break
  case 1310
    say Trigedit is tricky.
  break
  case 1311
    say Planning for those who lack direction.
  break
  case 1312
    say Advanced building, what an interesting topic.
  break
  case 1421
    say Storytelling, now that is a good idea.
  break
  case 1313
    say I'm finished. Finally. That Rumble is long winded.
  break
  default
    say so much reading, so little time.
  break
done
~
#36
Time Example~
0 b 1
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A replica of the "time" command.
* Convert hour from 24 hour to 12 hour clock with am/pm
if %time.hour% > 12
  eval hour %time.hour% - 12
  set ampm pm
else
  set hour %time.hour%
  set ampm am
end
*
* No 0 hour. Change it to 12.
if %time.hour% == 0
  set hour 12
end
*
* Figure out what day (1-35).
switch %time.day%
  case 1
  case 7
  case 14
  case 21
  case 28
  case 35
    set day the Day of the Moon
    break
  case 2
  case 8
  case 15
  case 22
  case 29
  set day the Day of the Bull
    break
  case 3
  case 9
  case 16
  case 23
  case 30
  set day the Day of the Deception
    break
  case 4
  case 10
  case 17
  case 24
  case 31
  set day the Day of Thunder
    break
  case 5
  case 11
  case 18
  case 25
  case 32
  set day the Day of Freedom
    break
  case 6
  case 12
  case 19
  case 26
  case 33
  set day the Day of the Great Gods
    break
  case 7
  case 13
  case 20
  case 27
  case 34
  set day the Day of the Sun
    break
  default
    set day I don't know what day it is
    break
done
*
* What suffix should we use for the number of the day.
switch %time.day%
  case 1
  case 21
  case 31
    set suf st
    break
  case 2
  case 22
  case 32
    set suf nd
    break
  case 3
  case 23
  case 33
    set suf rd
    break
  default
    set suf th
    break
done
*
* What month are we in (1-17).
    set m1 Month of Winter
    set m2 Month of the Winter Wolf
    set m3 Month of the Frost Giant
    set m4 Month of the Old Forces
    set m5 Month of the Grand Struggle
    set m6 Month of the Spring
    set m7 Month of Nature
    set m8 Month of Futility
    set m9 Month of the Dragon
    set m10 Month of the Sun
    set m11 Month of the Heat
    set m12 Month of the Battle
    set m13 Month of the Dark Shades
    set m14 Month of the Shadows
    set m15 Month of the Long Shadows
    set m16 Month of the Ancient Darkness
    set m17 Month of the Great Evil
eval months %%m%time.month%%%
*
* My test to make sure my variables are printing out what I expect them to.
* %echo% Hour: %time.hour% Day: %time.day% Month: %time.month% Year: %time.year%
*
* Finally the output.
%echo% It is %hour% o'clock %ampm%, on %day%.
%echo% The %time.day%%suf% Day of the %months%, Year %time.year%.
~
#37
Obj Command quarter flip example~
1 c 2
flip~
* By Rumble of The Builder Academy    tbamud.com 9091
* Numeric Arg: 2 means in character's carried inventory
if coin /= %arg% || quarter /= %arg%
  %echoaround% %actor%  %actor.name% flips a coin high up into the air.
  %send% %actor% You flip the coin up into the air.
  switch %random.2%
    case 1
      %echo% The coin falls to the ground, bounces, rolls, and ends up showing heads!
    break
    case 2
      %echo% The coin falls to the ground, bounces, rolls, and ends up showing tails!
    break
    default
      %echo% The coin falls on its edge and balances perfectly.
    break
  done
else
  %send% %actor% flip What?
end
~
#38
Mob Receive and Assemble Example~
0 j 100
~
* By Aeon of The Builder Academy    tbamud.com 9091
* Check mob's inventory (this does not include equipped items).
if %self.inventory(16701)% && %self.inventory(16702)% && %self.inventory(16703)% && %self.inventory(16704)% && %self.inventory(16705)%
  * Removing items from the mob.
  %purge% %self.inventory(16701)%
  %purge% %self.inventory(16702)%
  %purge% %self.inventory(16703)%
  %purge% %self.inventory(16704)%
  %purge% %self.inventory(16705)%
  wait 2 sec
  * For roleplay sake
  say Thank you %actor.name%! Now here is the cake.
  * Give the reward
  %load% obj 16706 %actor%
  %send% %actor% %self.name% gives you a cake.
  %echoaround% %actor% %self.name% gives %actor.name% a cake.
end
~
#39
Mob Following Assist Master~
0 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Make following mob assist.
if %self.master%
  set master %self.master%
  if %master.fighting%
    say I will save you Master %master.name%
    wait 1 sec
    assist %master.name%
  end
end
~
#40
Random Equipment Scatter and Teleport~
2 g 100
~
* By Random of The Builder Academy    tbamud.com 9091
* Scatter the player and all of their equipment randomly across the zone.
wait 1 sec
%send% %actor% You feel you must not have been worthy when a powerful force hurls you back through the gates.
wait 2 sec
eval stunned %actor.hitp% -1
%damage% %actor% %stunned%
eval num %random.99% + 20300
%teleport% %actor% %num%
while %actor.inventory%
  set item %actor.inventory%
  if %item.type% == CONTAINER
    while %item.contents%
      set stolen %item.contents.vnum%
      %echo% purging %item.contents.shortdesc% in container.
      %purge% %item.contents%
      eval num %random.99% + 2300
      %at% %num% %load% obj %stolen%
    done
  end
  set item_to_purge %actor.inventory%
  set stolen %item.vnum%
  %purge% %item_to_purge%
  eval num %random.99% + 2300
  %at% %num% %load% obj %stolen%
done
set i 0
while %i% < 18
  set item %actor.eq(%i%)%
  if %item%
    set stolen %item.vnum%
    set item_to_purge %%actor.eq(%i%)%%
    %send% %actor% You drop %item.shortdesc%
    %purge% %item_to_purge%
    eval num %random.99% + 20300
    %at% %num% %load% obj %stolen%
  end
  eval i %i% + 1
done
%force% %actor% look
~
#41
Memory and Mhunt Example~
0 o 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires if player is in mobs memory via mremember and the mob sees the actor.
%echo% Sleeping for 10 secs, give %actor.name% a head start.
sleep
%echo% Targetting %actor.name%
wait 10 s
wake
stand
%random.dir%
%random.dir%
%random.dir%
%echo% Hunting...%actor.name%
mhunt %actor%
~
#42
Container with Personal Key~
1 c 100
*~
* By Jamie Nelson from the forum http://groups.yahoo.com/group/dg_scripts/
* Container script for use with no key
if !%actor.is_pc%
  return 0
  halt
end
*
switch %cmd%
  case codeset
    if %actor.name% != rumble
      %send% %actor% I am sorry %actor.name%, only Rumble can set the code.
      halt
    end
    if !%arg%
      %send% %actor% You must supply a code.
      halt
    else
      if %arg% <= 999
        %send% %actor% You must supply a code that is a number. (more then 1)
        %send% %actor% And for security reasons, greater then 4 digits.
        halt
      else
        nop %self.val3(%arg%)%
        %send% %actor% You set the code on %self.shortdesc% to %arg%.
        set fingerprint %arg%
        remote fingerprint %actor.id%
      end
    end
  break
  case recognise
    %send% %actor% A laser scans your fingerprint.
    if %arg% != %self.val3%
      %send% %actor% Access Denied.
      halt
    else
      %send% %actor% Access Granted. Fingerprint Memorized.
      set fingerprint %self.val3%
      remote fingerprint %actor.id%
    end
  break
  case fingerprint
    if %arg% != open
      if %arg% != close
        %send% %actor% You must type either:
        %send% %actor% fingerprint open
        %send% %actor% or
        %send% %actor% fingerprint close
        halt
      else
        set oc 2
      end
    else
      set oc 1
    end
    %send% %actor% A laser scans your fingerprints.
    if !%actor.varexists(fingerprint)%
      %send% %actor% Access Denied.
      halt
    else
      %send% %actor% Access Granted.
      if %oc% == 2
        nop %self.val1(15)%
        %send% %actor% Closed and locked.
      elseif %oc% == 1
        nop %self.val1(0)%
        %send% %actor% Unlocked and open.
      else
        %send% Broken.
      end
    end
  break
  default
    return 0
  break
done
~
#43
Mob Wait Until Example~
0 ab 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Shopkeeper only works 6-9 then goes upstairs to sleep. The door is locked
* without a key so the player has to follow the mob and be quick about it.
* Closing time
wait until 21:00
emote looks sleepy.
wait 3 s
yawn
wait 10 s
say I'm closing up, have a nice day.
wait 10 s
emote works her way up the stairs and unlocks the door.
* Remove the locked and pickproof flags.
%door% 225 up flags ab
wait 2 s
open door
wait 2 s
up
wait 5 s
* Close the door and lock/pickproof it.
emote closes and locks the door behind %self.hisher%.
%door% 367 down flags abcd
wait 10 s
emote gets ready for bed.
wait 20 s
sleep
wait until 06:00
wake
wait 1 s
stand
say It's morning already!
wait 3 s
yawn
wait 10 s
emote goes about her business getting ready for the day.
wait 20 s
emote unlocks the door.
%door% 367 down flags ab
wait 3 s
open door
wait 1 s
d
wait 3 s
%door% 225 up flags abcd
~
#44
Mob Death Purges Equipment~
0 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Purge all inventory first.
say You damn whipper snappers. You may have beat me this time, but my equipment goes only to those who deserve it.
emote donates everything.
while %self.inventory%
  %purge% %self.inventory%
done
* While we have an equipment slot, purge that too.
set i 0
while %i% < 18
  eval item %%self.eq(%i%)%%
  if %item%
    %purge% %item%
  end
  eval i %i% + 1
done
~
#45
Rumble's Shotgun~
1 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* If the object is being wielded.
if %self.worn_by%
  * This is a random trigger so actor has to be defined.
  set actor %self.worn_by%
  * If the person wielding the object is fighting.
  if %actor.fighting%
    * Count the shots.
    eval shotgun_rounds %shotgun_rounds% + 1
    * Remember the count for the next time this trig fires.
    global shotgun_rounds
    * This double barrel shotgun, only has 2 rounds.
    if %shotgun_rounds% > 2
      * Detaching trig since gun is out of ammo.
      detach 45 %self.id%
      halt
    end
    * We also have to define the victim.
    set victim %actor.fighting%
    * Send the messages and do the damage.
    %echoaround% %actor% %actor.name% points %self.shortdesc% at %victim.name% and pulls the trigger.
    %send% %actor% You point %self.shortdesc% at %victim.name% and pull the trigger.
    %damage% %victim% 10
  end
end
~
#46
Parrot Array Example~
0 d 100
*~
* By Meyekul of The Builder Academy    tbamud.com 9091
* Squak for the nice people, Polly
wait 1 s
emote squawks loudly.
wait 1 s
* 75 percent chance of learning phrase.
eval polly %random.4%
if %polly% > 1
  * Ignore if already known
  switch %speech%
    case %phrase(1)%
      case %phrase(2)%
      case %phrase(3)%
      case %phrase(4)%
      case %phrase(5)%
      case %phrase(6)%
      case %phrase(7)%
      case %phrase(8)%
      case %phrase(9)%
      case %phrase(10)%
      emote looks at you curiously.
    break
    default
      eval say %phrase(%%random.%number%%%)%
      emote squawks, %say%
    break
  done
  * Learn new phrases
  eval number (%number% + 1)
  set phrase(%number%) %speech%
  global number
  global phrase(%number%)
  * Reset array after 10 phrases
  if %number% == 10
    unset number
  end
end
~
#47
Mob Greet Steal~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Steal the first item in the players inventory.
set item %actor.inventory%
if %item%
  if %item.vnum% != 1
    %echo% purging %item.shortdesc% with vnum %item.vnum% in %actor.name%'s inventory.
    %load% obj %item.vnum%
    %purge% %item%
  else
    %echo% can't purge %item.shortdesc% with vnum %item.vnum% in %actor.name%'s inventory because it may be a unique item.
  end
else
  %echo% I can't find %item.shortdesc% with vnum %item.vnum% in %actor.name%'s inventory.
  %echo% I can't find an item in %actor.name%'s inventory.
end
~
#48
Object Command Assemble~
1 c 7
join~
* By Rumble of The Builder Academy    tbamud.com 9091
* Assemble an orb onto a staff to make a new item. Trig attached to obj 189
set currentroom %self.room%
* Make sure they are in room 133 with the 2 objects.
if %currentroom.vnum(133)%
  if %actor.inventory(189)% && %actor.inventory(191)%
    * Purge 191, but leave 189 since we are going to %transform% it.
    %purge% %actor.inventory(191)%
    * lets make it a 50/50 chance of working.
    switch %random.2%
      case 1
        %transform% 12
        %send% %actor% As you join the orb to the staff it clicks into place.
        %echoaround% %actor% %actor.name% places an orb onto %actor.hisher% staff.
      break
      default
        %transform% 192
        %send% %actor% As you try to join the orb to the staff it turns in your hands and snaps in half.
        %echoaround% %actor% %actor.name% tries to place an orb onto %actor.hisher% staff until the staff twists in %actor.hisher% hands and snaps in half.
      break
    done
    detach 48 %self.id%
  else
    %send% %actor% You do not have the required items to do this.
  end
else
  %send% %actor% You can not do that here.
end
~
#49
Eval and Set Example~
2 d 100
test~
* By Rumble of The Builder Academy    tbamud.com 9091
* This is a speech trig @RHELP TRIGEDIT ROOM SPEECH@n, say 'test' to activate.
* There is much confusion about the difference between set and eval. So this is
* the simplest way I can think of to explain it (assume %actor.level% = 34):
*
* Set stores the variable and will not process it until called.
* In the example below %set_variable% will contain '34 + 1'
set set_variable %actor.level% + 1
%echo% Set Variable: %set_variable%
*
* Eval immediately evaluates the variable.
* In the example below %eval_variable% will contain '35'
eval eval_variable %actor.level% + 1
%echo% Eval Variable: %eval_variable%
*
%echo% Level: %actor.level%
~
#50
Room Global Random Example~
2 ab 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires whether a player is in the room or not.
wait until 05:00
%echo% The butcher's assistant steps out of the shop to the North and turns the sign over from "closed" to "open."
%load% obj 69
if %findobj.291(70)%
  %purge% signclosed
end
wait until 21:00
%echo% The butcher's assistant steps out of the shop to the North and turns the sign over from "open" to "closed."
%load% obj 70
if %findobj.291(69)%
  %purge% signopened
end
~
#51
Room Random heal Example~
2 b 20
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires only when a player is in the room.
* Actor is not defined with trigger type random, you must define it.
set actor %random.char%
%damage% %actor% -10
%send% %actor% A healing breeze flows through the room.
%echoaround% %actor% %actor.name% looks refreshed.
~
#52
Room Command Example~
2 c 100
l~
* By Rumble of The Builder Academy    tbamud.com 9091
set bridge 'bridge
if %cmd.mudcommand% == look && %bridge.contains('%arg%)%
  %send% %actor% As you look at the bridge a small form staggers out from underneath it.
  %echoaround% %actor% As %actor.name% peers under the bridge a small form emerges.
  %load% mob 207
  wait 1 sec
  %echo% A filthy dog crawls out from underneath the bridge. It is covered in MUD.
else
  * If it doesn't match let the command continue. Without a return 0 a player
  * will not be able to "look" at anything else.
  return 0
end
~
#53
Room Speech Example~
2 d 1
*~
* By Rumble of The Builder Academy    tbamud.com 9091
*
* .car returns the first word in a string.
* .cdr returns the remaining string.
*
%echo% The first word is: %speech.car%
%echo% The rest of the string is: %speech.cdr%
*
* To go through a long string of text looking at each word you can
* use a while loop. You could also check for matching text.
*
* set the first word
set word %speech.car%
* set the rest of the speech string
set rest %speech.cdr%
* while there is a first word keep going
while %word%
  %echo% the first word is: %word%
  %echo% the remaining text is: %rest%
  set word %rest.car%
  set rest %rest.cdr%
done
~
#54
Room Global Zone Reset Example~
2 f 100
~
%door% 23667 east purge 0
%echo% As if by magic all the tapestries rise to their previous positions.
~
#55
Room Enter Example~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  if %actor.sex% == male
    return 0
    %send% %actor% An invisible barrier blocks you. A voice rings out "No men allowed."
    %echoaround% %actor% %actor.name% walks into an invisible barrier and a voice booms "No men allowed."
  elseif %actor.sex% == female
    %echo% Ladies are always welcome!
  else * must be neutral.
    return 0
    %send% %actor% An invisible barrier blocks you. A voice rings out "No... things allowed."
    %echoaround% %actor% %actor.name% walks into an invisible barrier and a voice booms "No... things allowed."
  end
end
~
#56
Room Drop Example~
2 h 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Prevent players from littering.
if %object.type% == TRASH
  %send% %actor% Magical forces prevent you from dropping %object.shortdesc%.
  %send% %actor% No Littering! Drop your trash somewhere else.
  return 0
end
~
#57
Room Cast Example~
2 p 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Prevent spell casting in this room.
* %echo% %actor.name% tried to cast spellnumber: %spell%: %spellname% on %vict.name% %obj.name%.
%send% %actor% Your magic fizzles out and dies.
%echoaround% %actor% %actor.name%'s magic fizzles out and dies.
return 0
~
#58
Room Leave Example~
2 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %direction% == north
  %send% %actor% You try to go %direction% but Hannibal steps in front of you blocking the way.
  return 0
elseif %direction% == south && %actor.is_pc%
  %echo% Hannibal says, 'Thanks for stopping by.'
end
~
#59
Room Door Example~
2 r 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* The first time this door is opened dump a bucket of water on the actor.
if %cmd% == open
  %echoaround% %actor% As %actor.name% tries to %cmd% the door to the %direction% a bucket of water dumps on %actor.hisher% head.
  %send% %actor% Splash!!
  %send% %actor% A bucket of water drops on top of your head as you open the door.
  %damage% %actor% 1
  %echo% The door slams shut again.
  * Detach this trigger so it only happens once per reboot/copyover.
  detach 59 %self.id%
  * Don't allow the door to be opened.
  return 0
end
~
#60
Mob Global Random Example~
0 ab 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Global Random trigs fire whether a player is in the room or not.
switch %random.6%
  case 1
    emote whistles an annoying tune.
  break
  case 2
    emote stares at you with interest.
  break
  case 3
    emote ponders your existence.
  break
  case 4
    emote considers your potential.
  break
  case 5
    emote hums an unrecognizable tune.
  break
  default
    emote starts to sing a piercing melody about a lost love.
  break
done
~
#61
Mob Random Example~
0 b 2
~
* By Rumble of The Builder Academy    tbamud.com 9091
* With random triggers ACTOR is NOT defined. So set it.
set actor %random.char%
wait 1 sec
say Hey!  You don't belong here!
emote mumbles, 'Now what was that spell...'
wait 1 sec
* Senile old guard casts random spells on intruders.
* Don't cast if incapacitated
if %self.hitp% > 0
  switch %random.17%
    case 1
      dg_cast 'cure light' %actor%
    break
    case 2
      dg_cast 'magic missile' %actor%
    break
    case 3
      dg_cast 'detect invisibility'
    break
    case 4
      dg_cast 'detect magic'
    break
    case 5
      dg_cast 'bless' %actor%
    break
    case 6
      dg_cast 'heal' %actor%
    break
    case 7
      dg_cast 'infravision'
    break
    case 8
      dg_cast 'invisibility' %actor%
    break
    case 9
      dg_cast 'armor' %actor%
    break
    case 10
      dg_cast 'strength' %actor%
    break
    case 11
      dg_cast 'sleep' %actor%
    break
    case 12
      dg_cast 'blindness' %actor%
    break
    case 13
      dg_cast 'detect poison' %actor%
    break
    case 14
      dg_cast 'curse' %actor%
    break
    case 15
      dg_cast 'poison' %actor%
    break
    case 16
      if %actor.align% > 0
        dg_cast 'dispel good' %actor%
      else
        dg_cast 'dispel evil' %actor%
      end
    break
    default
      * Senile magi "almost" kills himself.
      say That wasn't right...
      %echo% A failed spell backfires on the mage!
      %damage% %self% %self.hitp%
    break
  done
end
~
#62
Mob Command Example~
0 c 100
l~
* By Rumble of The Builder Academy    tbamud.com 9091
* does not work for level 32 and above.
* Make sure the command is look, check for any abbrev of orb
if %cmd.mudcommand% == look && orb /= %arg%
  %send% %actor% As you look at the orb a feeling of peace and serenity comes over you.
  %echoround% %actor% %actor.name% stares at the orb.
else
  * If it doesn't match let the command continue.
  return 0
end
~
#63
Mob Speech and Expressions Example~
0 d 1
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* Arg list * means it will fire whenever someone talks.
* Check if it is a player character (not a mob).
if %actor.is_pc%
  wait 1 sec
  * Check the actors sex.
  if %actor.sex% == male
    say Good day sir, what would you like?
  elseif %actor.sex% == female
    wait 1 sec
    say Good day maam, what can I get you?
  else
    say What do you want?
  end
  * Check if the player level is less than 10 (9 and lower).
  if %actor.level% < 10
    say you are a player below level 10.
  else
    say you are not below level 10.
  end
  * Check class not equal to warrior.
  if %actor.class% != warrior
    say you are not a warrior.
  else
    say you are a warrior.
  end
  * Check alignment between 350 and -350 (neutral).
  if %actor.align% <= 350 && %actor.align% >= -350
    say you are neutral.
  else
    say you are not neutral.
  end
  * Check strength greater than or equal to 16 and less than or equal to 18. (16-18)
  * If this was done without the equalities (=) it would check str 17 only.
  if %actor.str% >= 16 && %actor.str% <= 18
    say your strength is 16-18.
  else
    say your strength is not 16-18.
  end
  * Check the speech to see if it is a substring of the word concatenated.
  * concaten caten cat and any abbreviations.
  if concatenated /= %speech%
    say your speech was a substring of concatenated.
  else
    say your speech was not a substring of concatenated.
  end
  * Create a random number 1-10 and assign it to rnumber.
  set rnumber %random.10%
  say your random number is: %rnumber%
  * increment it by 1.
  eval rnumber %rnumber% + 1
  say your incremented random number is: %rnumber%
else
  say you are not a player.
end
~
#64
Mob Action Example~
0 e 0
kisses you.~
* By Rumble of The Builder Academy    tbamud.com 9091
wait 1 sec
slap %actor.name%
wait 1 sec
say I'm not that kind of girl.
pout
set inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% shouts, '%actor.name% kisses like a fish.'
~
#65
Mob Death Example~
0 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
%send% %actor% %self.name% curses at you before drawing %self.hisher% final breath.
%echoround% %actor% %self.name% curses %actor.name% before drawing %self.hisher% final breath.
* If you don't want a corpse just send it to the void.
%echo% %self.name%'s body fades out of existence.
%teleport% %self% 0
~
#66
Mob Greet Example~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* To make a trigger fire only on players use:
if %actor.is_pc%
  say Hello, and welcome, %actor.name%
end
* Check what direction they came from.
if %direction%
  say Hello, %actor.name%, how are things to the %direction%?
else
  * If the character popped in (word of recall, etc) this will be hit
  say Where did YOU come from, %actor.name%?
end
if %direction% == down
  say Ah, hello %actor.name% I have been waiting for you. Go learn to build.
  %teleport% %actor% 1300
  %force% %actor% look
end
if %actor.name% == Rumble
  say Make yourself comfortable %actor.name%.
else
  say get lost %actor.name%.
end
~
#67
Mob Greet-All Example~
0 h 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  say Hello, and welcome, %actor.name%
  if %direction% == north
    say how are things to the north?
  else
    say how are things to the %direction%?
  end
  wait 1 sec
  %send% %actor% %self.name% bows deeply.
  %echoaround% %actor% %self.name% takes off %self.hisher% hat and bows deeply.
end
~
#68
Mob Entry Example~
0 i 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* First find the room the mob is in and put the value in %inroom%
set inroom %self.room%
* then check on the rooms vnum
if %inroom.vnum% == 33
  say I, %self.name%, declare this room Rumble's.
end
set person %inroom.people%
wait 1 sec
* While there are still people in the room.
while %person%
  %echo% I am targetting %person.name%.
  * Target the next person in the room.
  set person %person.next_in_room%
done
~
#69
Mob Receive Example~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Recruit really wants some beer.
if %object.vnum% == 101 || %object.vnum% == 147
  say thanks, I really needed this.
  drink beer
  %purge% %object%
  * Give some basic directions to newbies.
  if %actor.level% < 5
    say I owe you one, take this in exchange.
    %load% obj 159
    give directions %actor.name%
  end
else
  say I don't want that!
  return 0
end
~
#70
Mob fight example - dg_cast missile~
0 k 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Only cast the spell every 30 seconds.
if %already_cast%
  wait 30 s
  unset already_cast
else
  dg_cast 'magic missile' %actor.name%
  set already_cast 1
  * By globalling the variable it can be accessed by other triggers or when
  * this trigger fires a second time.
  global already_cast
end
~
#71
Mob Hitprcnt Example~
0 l 50
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires at 50 percent hitpoints.
%zoneecho% %self.room.vnum% %self.name% shouts 'HELP! I'm under ATTACK! HELP!'
* Remove the trigger so it won't fire again if the mob heals.
detach 71 %self.id%
~
#72
Mob Bribe Example~
0 m 1
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Charge 100 gold to get out.
if %amount% >= 100
  if %amount% > 100
    eval change %amount% - 100
    give %change% coin %actor.name%
  end
  emote opens a concealed door.
  wait 1 sec
  say Thank you, step inside.
  wait 2 sec
  %echoaround% %actor% %self.name% pushes %actor.name% through a concealed door.
  %send% %actor% %self.name% helps you through a concealed door.
  %teleport% %actor% 130
else
  if %amount% > 1
    set s s
  end
  say Only %amount% coin%s%? I don't think so.
  give %amount% coin %actor.name%
end
~
#73
Mob Load Example~
0 n 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* When the mob is created give it a random weapon.
switch %random.5%
  case 1
    %load% obj 3010
    wield dagger
  break
  case 2
    %load% obj 3011
    wield sword
  break
  case 3
    %load% obj 3012
    wield club
  break
  case 4
    %load% obj 3013
    wield mace
  break
  case 5
    %load% obj 3014
    wield sword
  break
  default
    * this should be here, even if it's never reached
  break
done
~
#74
Mob Memory Example Part 1~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* mremember must be used before you can use the trigger type memory.
mremember %actor.name%
say I'll remember you now, %actor.name%
~
#75
Mob Memory Example Part 2~
0 o 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires if player is in mobs memory via mremember and the mob sees the actor.
wait 4 s
poke %actor.name%
say i've seen you before, %actor.name%.
mforget %actor.name%
~
#76
Mob Cast Example~
0 p 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %spellname% == magic missile
  %echo% %self.name% is protected by a shield spell negating %actor.name%s %spellname%.
  * Return 0 prevents the spell from working.
  return 0
else
  %echo% %self.name%s shield spell doesn't protect %self.himher% from %actor.name%'s %spellname%.
  * Return 1 allows the trigger to continue and the spell to work. It is not needed
  * since this is the normal return state of a trigger.
  return 1
end
~
#77
Mob Leave Example~
0 q 100
~
if %actor.level% > 10
  say You may not leave here, %actor.name%.
  %send% %actor% %self.name% prevents you from leaving the room.
  %echoaround% %actor% As %actor.name% tries to leave the room, %self.name% stops %actor.himher%.
  return 0
end
~
#78
Mob Door Example~
0 r 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
say %actor.name% do not try to %cmd% the door to the %direction% again. Or else!
return 0
~
#79
Obj Global Random Example~
1 ab 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
wait until 06:00
* Shop is open, fill the container.
%echo% A small trickle of red liquid begins to flow from the shop to the North.
oset 0 4
oset 1 4
wait until 22:00
* Shop is closed, empty it.
%echo% The shop to the north starts to close up for the night.
oset 0 0
oset 1 0
~
#80
Obj Random Example~
1 b 1
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %self.worn_by%
  set actor %self.worn_by%
  %send% %actor% War's Blood thirsts for battle.
end
~
#81
Obj Command Example~
1 c 7
o~
* By Rumble of The Builder Academy    tbamud.com 9091
* Numeric Arg: 7 means obj can be worn, carried, or in room.
* Make sure the command is open, check for any abbrev of closet
if %cmd.mudcommand% == open && closet /= %arg%
  %send% %actor% As you open the closet something runs out.
  %echoaround% %actor% %actor.name% opens a closet and something comes out.
  %load% mob 207
  detach 81 %self.id%
else
  * If it doesn't match let the command continue.
  return 0
end
~
#82
Obj Timer Example~
1 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* This object will disappear once its timer value reaches 0.
%echo% The ice cream melts away.
%purge% %self%
~
#83
Obj Get Example~
1 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.align% > 350
  %send% %actor% As you reach for %self.shortdesc% it fades out of existence.
  %echoaround% %actor% As %actor.name% reaches for %self.shortdesc% it fades out of existence.
  return 0
else
  %send% %actor% As you pick up %self.shortdesc% it seems to vibrate with a life of its own.
end
~
#84
Obj Drop Example~
1 h 100
~
if %actor.level% < 31
  %send% %actor% You fail to drop %self.shortdesc%.
  return 0
else
  %send% %actor% %self.shortdesc% glows a bright yellow, then explodes.
  %purge% %self%
end
~
#85
Obj Give Example~
1 i 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.level% < 20
  %send% %actor% You can't give %victim.himher% that.
  %send% %victim% %actor.name% tries to give you %self.shortdesc%, but fails.
  return 0
else
  %send% %actor% You feel diminished.
end
~
#86
Obj Wear Example~
1 j 100
~
if %actor.is_pc%
  * By Rumble of The Builder Academy    tbamud.com 9091
  if %actor.str% < 17
    %send% %actor% %self.shortdesc% is too heavy for you to use.
    %echoaround% %actor% %actor.name% tries to use %self.shortdesc% but can't seem to hold it up.
    return 0
  end
end
~
#87
Obj Remove Example~
1 l 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.int% < 12
  %send% %actor% You can't figure out how to remove %self.shortdesc%.
  %echoaround% %actor% %actor.name% tries to remove %self.shortdesc% but can't.
  return 0
end
~
#88
Obj Load Example~
1 n 50
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Squeak when loading.
%echo% %self.shortdesc% lets out a faint squeak as if gasping for breath.
~
#89
Shackles O89~
1 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
%send% %actor% You try walk to the %direction% but simply reach the end of your chain and fall flat on your face.
%echoaround% %actor% %actor.name% walks to the end of %actor.hisher% chain and falls over gracefully.
nop %actor.pos(sitting)%
return 0
~
#90
Obj Cast Example~
1 p 100
~
%echo% %actor.name% cast spell number: %spell%: %spellname% on %self.shortdesc%
~
#91
Special Characters Example~
1 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Special Characters Example - how to automatically substitute possessive pronouns.
%echo% VNUM:                    %self.vnum%
set thing %self.name.car%
%echo% FIRST KEYWORD:           %thing%
%echo% NAME'S, SOMEONE'S, YOUR: |%thing%
%echo% NAME, SOMEONE, YOU:      %thing%
%echo% IT, YOU, HE/SHE:         &%thing%
%echo% IT, YOU, HIM/HER:        *%thing%
%echo% ITS, YOUR' HIS/HER:      ^%thing%
~
#92
Room Command Combination Lock~
2 c 100
push~
* By Rumble of The Builder Academy    tbamud.com 9091
*
* To allow multiple scripts based on this script we set a context:
  context %self.id%
* clear old var, if set.
  unset reset_buttons
* check argument
  if %arg% == red
    %send% %actor% You push the red button.
    set pushed_red 1
    global pushed_red
  elseif %arg% == yellow
    %send% %actor% You push the yellow button.
    if %pushed_red%
      set pushed_yellow 1
      global pushed_yellow
    else
      set reset_buttons 1
    end
  elseif %arg% == green
    %send% %actor% You push the green button.
    if %pushed_red% && %pushed_yellow%
      set pushed_green 1
      global pushed_green
    else
      set reset_buttons 1
    end
  elseif %arg% == blue
    %send% %actor% You push the blue button.
    if %pushed_red% && %pushed_yellow% && %pushed_green%
      set pushed_blue 1
      global pushed_blue
    else
      set reset_buttons 1
    end
  elseif %arg% == purple
    %send% %actor% You push the purple button.
    if %pushed_red% && %pushed_yellow% && %pushed_green% && %pushed_blue%
      %send% %actor% You hear a loud click as you push the final button.
      set reset_buttons 1
    else
      set reset_buttons 1
      end
  elseif %arg% == white
    %send% %actor% You push the white reset button.
    set reset_buttons 1
  else
    %send% %actor% Push what ?!
    halt
  end
* reset the buttons if necessary
  if %reset_buttons%
    unset reset_buttons
    unset pushed_red
    unset pushed_yellow
    unset pushed_green
    unset pushed_blue
    %send% %actor% The buttons all suddenly pop back out.
  end
* send the state of the buttons
  %send% %actor% red:%pushed_red% yellow:%pushed_yellow% green:%pushed_green% blue:%pushed_blue% purple:%pushed_purple%
~
#93
if/elseif/else Example~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    %echo% male
  elseif %actor.sex% == female
    %echo% elseif female
  else
    %echo% else neutral
  end
else
  %echo% mob
end
~
#94
Kind Soul 13 - Give Newbie Equipment~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* If a player is < level 3 and naked it fully equips them. If < 3 and missing
* some equipment it will equip one spot.
if %actor.is_pc% && %actor.level% < 3
  wait 2 sec
  if !%actor.eq(*)%
    say get some clothes on! Here, I will help.
    %load% obj 50 %actor% light
    %load% obj 51 %actor% rfinger
    %load% obj 52 %actor% lfinger
    %load% obj 53 %actor% neck1
    %load% obj 54 %actor% neck2
    %load% obj 55 %actor% body
    %load% obj 56 %actor% head
    %load% obj 57 %actor% legs
    %load% obj 58 %actor% feet
    %load% obj 59 %actor% hands
    %load% obj 60 %actor% arms
    %load% obj 61 %actor% shield
    %load% obj 62 %actor% about
    %load% obj 63 %actor% waist
    %load% obj 64 %actor% rwrist
    %load% obj 65 %actor% lwrist
    %load% obj 66 %actor% wield
    %load% obj 67 %actor% hold
    halt
  end
  if !%actor.eq(light)%
    say you really shouldn't be wandering these parts without a light source %actor.name%.
    shake
    %load% obj 50
    give generic %actor.name%
    halt
  end
  if !%actor.eq(rfinger)% || !%actor.eq(lfinger)%
    say did you lose one of your rings?
    sigh
    %load% obj 51
    give generic %actor.name%
    halt
  end
  if !%actor.eq(neck1)% || !%actor.eq(neck2)%
    say you lose everything don't you?
    roll
    %load% obj 53
    give generic %actor.name%
    halt
  end
  if !%actor.eq(body)%
    say you won't get far without some body armor %actor.name%.
    %load% obj 55
    give generic %actor.name%
    halt
  end
  if !%actor.eq(head)%
    say protect that noggin of yours, %actor.name%.
    %load% obj 56
    give generic %actor.name%
    halt
  end
  if !%actor.eq(legs)%
    say why do you always lose your pants %actor.name%?
    %load% obj 57
    give generic %actor.name%
    halt
  end
  if !%actor.eq(feet)%
    say you can't go around barefoot %actor.name%.
    %load% obj 58
    give generic %actor.name%
    halt
  end
  if !%actor.eq(hands)%
    say need some gloves %actor.name%?
    %load% obj 59
    give generic %actor.name%
    halt
  end
  if !%actor.eq(arms)%
    say you must be freezing %actor.name%.
    %load% obj 60
    give generic %actor.name%
    halt
  end
  if !%actor.eq(shield)%
    say you need one of these to protect yourself %actor.name%.
    %load% obj 61
    give generic %actor.name%
    halt
  end
  if !%actor.eq(about)%
    say you are going to catch a cold %actor.name%.
    %load% obj 62
    give generic %actor.name%
    halt
  end
  if !%actor.eq(waist)%
    say better use this to hold your pants up %actor.name%.
    %load% obj 63
    give generic %actor.name%
    halt
  end
  if !%actor.eq(rwrist)% || !%actor.eq(lwrist)%
    say misplace something?
    smile
    %load% obj 65
    give generic %actor.name%
    halt
  end
  if !%actor.eq(wield)%
    say without a weapon you will be Fido food %actor.name%.
    %load% obj 66
    give generic %actor.name%
    halt
  end
end
~
#95
Puff - Random Advice~
0 ab 12
~
* By Rumble of The Builder Academy    tbamud.com 9091
set max %random.197%
set  text[1]   Meu Deus!  Aqui está cheio de estrelas!
set  text[2]   Como todos estes peixes vieram parar aqui?
set  text[3]   Eu sou um dragão muito feminino.
set  text[4]   Aqui está muito tranqüilo."
set  text[5]   Hoje parece um dia perfeito para compras!
set  text[6]   Eu sou uma das coisas ruins que acontecem às pessoas boas.
set  text[7]   Tragam os seus mortos, tragam os seus mortos!
set  text[8]   Sou uma garota tão ocupada...
set  text[9]   Você já imaginou um mundo sem situações hipoteticas?
set  text[10]  Adoro as cores do arco-íris!
set  text[11]  Se nada gruda no Teflon, comoneles grudam o Teflon na panela?
set  text[12]  Melhor ser legal ou eu vou usar uma fireball em você!
set  text[13]  Você acha que eu vou ficar careca?
set  text[14]  Este é o seu cerebro, este é o MUD, este é o seu cerebro no MUD. Alguma pergunta?
set  text[15]  As vozes em minha cabeça querem que eu domine o mundo...
set  text[16]  Rapido! Reverta a polaridade do fluxonde neutrons!
set  text[17]  Shh...  Estou testando o beta. Preciso de completo silencio!
set  text[18]  Eu sou a verdadeira implementadora, você sabia?
set  text[19]  Se o amor é cego, por quê lingerie é tão popular?
set  text[20]  Vou escrever um poema... Sim! Falarei de flores. De flores perfumadas!
set  text[21]  Vou viajar de cometa!
set  text[22]  Prefiro comida sem conservantes!
set  text[23]   Se você não pode vencê-los, arranje para que eles sejam espancados.
set  text[24]   É preciso um homem grande para chorar, mas um homem maior para rir dele.
set  text[25]   Amigos vêm e vão, mas inimigos se acumulam.
set  text[26]   Uma mentira é rápida, mas a verdade tem resistência.
set  text[27]   Não culpe a espada pela mão que a empunha.
set  text[28]   A intenção não é enxergar através dos outros, mas sim ajudar uns aos outros a superar as dificuldades.
set  text[29]   Você pode construir um trono de baionetas, mas não poderá se sentar nele por muito tempo.
set  text[30]   A verdadeira amizade surge quando o silêncio entre duas pessoas é reconfortante.
set  text[31]   Eu consigo imaginar um mundo sem ódio ou raiva, e consigo imaginar nós atacando esse mundo, porque eles nunca esperariam isso.
set  text[32]   A vida é como uma equipe de trenó puxado por cães. Se você não for o cão líder, a paisagem nunca muda.
set  text[33]   O único homem completamente em paz é aquele sem umbigo.
set  text[34]   Os perdedores falam sobre o quanto se esforçaram, enquanto o vencedor vai para casa com a rainha do baile.
set  text[35]   Dê um peixe a um homem e ele comerá por um dia. Ensine-o a pescar, e ele ficará em um barco bebendo cerveja o dia todo.
set  text[36]   Mantenha seus amigos por perto, mas seus inimigos ainda mais perto.
set  text[37]   Não, meus poderes só podem ser usados para o bem.
set  text[38]   Quem foi a primeira pessoa a olhar para uma vaca e dizer, "acho que vou apertar essas coisas penduradas aqui e beber o que sair"?
set  text[39]   Por que as torradeiras sempre têm uma configuração que queima a torrada até ficar horrivelmente crocante, de modo que nenhum ser humano decente comeria?
set  text[40]   Por que há uma luz na geladeira e não no freezer?
set  text[41]   Por que é que algumas pessoas parecem inteligentes até que você as ouça falar?
set  text[42]   Um carro funerário carregando um cadáver pode circular na faixa compartilhada?
set  text[43]   Por que as pessoas apontam para o pulso quando perguntam as horas, mas não apontam para a virilha quando perguntam onde fica o banheiro?
set  text[44]   Por que os Trix são apenas para crianças?
set  text[45]   Por que uma pessoa que lida com seu dinheiro é chamada de "corretor"?
set  text[46]   De quem foi a ideia cruel de a palavra "lisp" ter um "s" nela?
set  text[47]   Se o óleo de milho é feito de milho e o óleo vegetal de vegetais, então de que é feito o óleo para bebês?
set  text[48]   Se a eletricidade vem dos elétrons, a moralidade vem dos idiotas?
set  text[49]   Disney World é a única armadilha para pessoas operada por um rato?
set  text[50]   "I am" é supostamente a frase mais curta na língua inglesa. Será que "I do" é a frase mais longa?
set  text[51]   Pessoas analfabetas sentem o efeito completo da sopa de letrinhas?
set  text[52]   Você já notou que, quando você sopra no rosto de um cachorro, ele fica bravo, mas quando o leva para um passeio de carro, ele coloca a cabeça para fora da janela?
set  text[53]   Minha mente funciona como um relâmpago: num brilhante instante, ela se vai.
set  text[54]   100.000 espermatozoides e você foi o mais rápido?
set  text[55]   Boca fechada não leva o pé.
set  text[56]   Algum dia, todos nós olharemos para isso, riremos nervosamente e mudaremos de assunto.
set  text[57]   Um diplomata é alguém que pode te mandar para o inferno de um jeito que você até espera ansiosamente pela viagem.
set  text[58]   Todas as generalizações são falsas, inclusive esta.
set  text[59]   Nascemos nus, molhados e famintos. Depois, as coisas pioram.
set  text[60]   Qual foi a melhor coisa ANTES do pão fatiado?
set  text[61]   Estressados e sem ninguém para sufocar.
set  text[62]   Antes de criticar alguém, você deveria andar uma milha com os sapatos dessa pessoa. Assim, quando os criticar, estará a uma milha de distância e com os sapatos dela.
set  text[63]   É melhor entender um pouco do que interpretar mal muito.
set  text[64]   As contas viajam pelo correio duas vezes mais rápido do que os cheques.
set  text[65]   NÃO comece comigo. Você NÃO vai ganhar.
set  text[66]   Não seja insubstituível; se você não pode ser substituído, não pode ser promovido.
set  text[67]   Não me irrite! Estou ficando sem lugares para esconder os corpos.
set  text[68]   Não leve a vida tão a sério; você não sairá dela vivo.
set  text[69]   Fita adesiva é como a Força: tem um lado luminoso e um lado sombrio, e mantém o universo unido.
set  text[70]   Águias podem voar alto, mas doninhas não são sugadas por motores a jato.
set  text[71]   Já parou para pensar e esqueceu de recomeçar?
set  text[72]   Esqueça a paz mundial. Use o seu pisca.
set  text[73]   Dê-me ambiguidade ou me dê outra coisa.
set  text[74]   Por que pessoas de mente fechada sempre abrem a boca?
set  text[75]   Quem ri por último pensa mais devagar.
set  text[76]   Eu não disse que era sua culpa, Relsqui. Disse que iria te culpar.
set  text[77]   Eu não sofro de insanidade. Eu desfruto cada minuto dela.
set  text[78]   Sinto como se estivesse estacionado diagonalmente em um universo paralelo.
set  text[79]   Acabei de me perder em pensamentos. Era território desconhecido.
set  text[80]   Preciso muito de alguém. Você é realmente irresistível?
set  text[81]   Eu derramei removedor de manchas no meu cachorro. Agora ele sumiu.
set  text[82]   Eu costumava ser indeciso. Agora, não tenho certeza.
set  text[83]   Eu tinha o controle da minha vida, mas então ele se quebrou.
set  text[84]   Se a ignorância é felicidade, você deve ser orgásmico.
set  text[85]   Algumas pessoas só estão vivas porque é ilegal matá-las.
set  text[86]   É muito mais impressionante quando os outros descobrem suas qualidades sem a sua ajuda.
set  text[87]   Pode ser que seu único propósito na vida seja servir de aviso para os outros.
set  text[88]   Nunca misture um pedido de desculpas com uma desculpa.
set  text[89]   Ok, quem bloqueou o pagamento do meu cheque de realidade?
set  text[90]   Claro que não pareço ocupado... Eu fiz tudo certo de primeira.
set  text[91]   Mecânica quântica: é disso que os sonhos são feitos.
set  text[92]   Guarde seu fôlego. Você vai precisar dele para explodir seu encontro!
set  text[93]   Smith & Wesson: A interface original de apontar e clicar.
set  text[94]   Alguns dias você é o inseto, outros dias você é o para-brisa.
set  text[95]   Uns bebem na fonte do conhecimento. Outros apenas gargarejam.
set  text[96]   O pássaro madrugador pode apanhar o verme, mas o segundo rato consegue o queijo.
set  text[97]   O único substituto para bons modos são reflexos rápidos.
set  text[98]   O problema com o conjunto genético é que não há salva-vidas.
set  text[99]   Lembre-se do meu nome – você vai gritá-lo mais tarde.
set  text[100]  A severidade da coceira é inversamente proporcional à capacidade de alcançá-la.
set  text[101]  Muito engraçado, Scotty, agora desça com minhas roupas.
set  text[102]  Por que a palavra "abreviação" é tão longa?
set  text[103]  Por que "fonético" não é escrito da forma que se pronuncia?
set  text[104]  Você está com ciúmes porque as vozes estão falando comigo e não com você!
set  text[105]  O proctologista ligou; eles encontraram sua cabeça.
set  text[106]  Todo mundo tem memória fotográfica; alguns simplesmente não têm filme.
set  text[107]  Tente não deixar sua mente divagar. Ela é pequena demais para ficar sozinha.
set  text[108]  Você só precisa de duas ferramentas: WD-40 e fita adesiva. Se algo não se move e deveria, use WD-40. Se se move e não deveria, use a fita.
set  text[109]  Se você acordou respirando, parabéns! Você tem mais uma chance!
set  text[110]  Eu não acredito em milagres. Eu conto com eles.
set  text[111]  Quando estou para baixo, gosto de assobiar. Isso faz o cachorro do vizinho, que late o tempo todo, correr até o fim de sua corrente e se engasgar.
set  text[112]  Por que os pilotos kamikaze usavam capacetes?
set  text[113]  Eu não estou tenso, apenas terrivelmente, terrivelmente alerta.
set  text[114]  Como configuro uma impressora a laser para atordoar?
set  text[115]  Tudo o que preciso saber sobre a vida, aprendi matando pessoas inteligentes e comendo seus cérebros.
set  text[116]  Eu achava que queria uma carreira; na verdade, eu só queria contracheques.
set  text[117]  Já é hora da sua medicação ou da minha?
set  text[118]  Muitos esquisitos, poucos circos.
set  text[119]  Quantas vezes tenho que dar descarga antes que você vá embora?
set  text[120]  Nenhuma palavra na língua inglesa rima com month, orange, silver e purple.
set  text[121]  Se advogados são cassados e clérigos despojados, não significa que eletricistas fiquem encantados, músicos se destaquem, cowboys enlouqueçam, modelos sejam depostos, cirurgiões de árvores percam a compostura e as lavanderias fiquem deprimidas?
set  text[122]  Sarcasmo: apenas mais um serviço que oferecemos aqui.
set  text[123]  Este é um mundo mau e cruel. Eu quero minha fralda e medicação agora mesmo!
set  text[124]  Afaste-se! Você está pisando na minha aura.
set  text[125]  Mais pessoas são mortas anualmente por burros do que morrem em acidentes aéreos.
set  text[126]  Um "jiffy" é uma unidade de tempo real de 1/100 de segundo.
set  text[127]  Seu trem de pensamentos tem um vagão traseiro?
set  text[128]  O dinheiro não é feito de papel, é feito de algodão.
set  text[129]  Eu levantei da cama por isso?
set  text[130]  Você, você e você: entrem em pânico. Os demais, venham comigo.
set  text[131]  Estresse é quando você acorda gritando e percebe que ainda não adormeceu.
set  text[132]  Eu não sou o seu tipo. Eu não sou inflável.
set  text[133]  Se é estúpido mas funciona, então não é estúpido.
set  text[134]  Se ao menos você usasse seus poderes para o bem em vez do mal...
set  text[135]  Quanto mais você sua em paz, menos sangra na guerra.
set  text[136]  Os traçadores funcionam nos dois sentidos.
set  text[137]  Quem se importa se uma bomba de 500 libras guiada a laser tem precisão de 1 metro?
set  text[138]  Já se perguntou sobre aquelas pessoas que gastam 2,00 dólares em pequenas garrafinhas de água Evian? Tente soletrar "Evian" de trás para frente.
set  text[139]  Não é como criar uma área para fumantes em um restaurante ser igual a criar uma área para urinar em uma piscina?
set  text[140]  Por que os croutons vêm em embalagens herméticas? Eles não são apenas pão velho, afinal?
set  text[141]  Por que a chuva cai e a neve desce?
set  text[142]  Se é verdade que estamos aqui para ajudar os outros, então para que exatamente os outros estão aqui?
set  text[143]  A luz no fim do túnel foi apagada por causa dos cortes no orçamento.
set  text[144]  %random.4% dias sem uma violação dos direitos humanos!
set  text[145]  Pelo menos você não está sendo sondado retalmente por alienígenas.
set  text[146]  A força mais poderosa do universo é a fofoca.
set  text[147]  Você não deve confundir sua carreira com sua vida.
set  text[148]  Não importa o que aconteça, alguém sempre encontrará uma maneira de levar isso muito a sério.
set  text[149]  Quando os problemas surgem e as coisas parecem ruins, sempre há alguém que percebe uma solução e está disposto a tomar as rédeas. Muito frequentemente, esse alguém é louco.
set  text[150]  Há uma linha muito tênue entre "hobby" e "doença mental."
set  text[151]  Aceite que alguns dias você é o pombo, e outros dias você é a estátua.
set  text[152]  Nunca, sob nenhuma circunstância, tome um comprimido para dormir e um laxante na mesma noite.
set  text[153]  Você nunca deve dizer nada a uma mulher que, mesmo que remotamente, sugira que você acha que ela está grávida, a menos que veja um bebê realmente surgindo dela naquele momento.
set  text[154]  Uma pessoa que é gentil com você, mas rude com o garçom, não é uma pessoa legal.
set  text[155]  Quando tudo está a seu favor, você está na faixa errada.
set  text[156]  Eu vivo no meu próprio mundinho, mas tudo bem, eles me conhecem aqui.
set  text[157]  Mostre-me um homem com os dois pés bem no chão, e eu te mostrarei um homem que não consegue tirar as calças.
set  text[158]  Eu não aprovo piadas políticas... Já vi muitos políticos sendo eleitos.
set  text[159]  Eu adoro ser casado. É maravilhoso encontrar aquela pessoa especial que você quer irritar pelo resto da vida.
set  text[160]  Eu sou um ninguém; ninguém é perfeito, portanto eu sou perfeito.
set  text[161]  Todos os dias eu supero meu próprio recorde de quantos dias consecutivos eu fiquei vivo.
set  text[162]  Se as cenouras são tão boas para os olhos, por que vejo tantos coelhos mortos na estrada?
set  text[163]  Bem-vindo ao Shit Creek – Desculpe, estamos sem remos!
set  text[164]  Por que escolhemos apenas duas pessoas para concorrer à presidência e 50 para o Miss America?
set  text[165]  Já notou que as pessoas que gastam dinheiro com cerveja, cigarros e bilhetes de loteria estão sempre reclamando de estar quebradas e de não se sentirem bem?
set  text[166]  Da próxima vez que sentir vontade de reclamar, lembre-se: seu triturador de lixo provavelmente come melhor do que 30% das pessoas deste mundo.
set  text[167]  Bonecos de neve caem do céu sem estarem montados.
set  text[168]  Toda vez que entro em um bar de solteiros, posso ouvir as sábias palavras da minha mãe: "Não pegue isso, você não sabe de onde veio."
set  text[169]  Sem juízo... De volta em cinco minutos.
set  text[170]  Eu quero morrer pacificamente em meu sono, como meu avô... Não gritando e berrando, como os passageiros em seu carro.
set  text[171]  Homem que corre na frente do carro se cansa.
set  text[172]  Homem que corre atrás do carro fica exausto.
set  text[173]  Homem que coça as costas não deve roer as unhas.
set  text[174]  Homem que solta pum na igreja senta no próprio banco.
set  text[175]  Alguns dias, simplesmente não estou a fim de matar dragões.
set  text[176]  Obrigado por não ser tão animado.
set  text[177]  Não incomode o louco.
set  text[178]  Em qual parque de trailers você cresceu?
set  text[179]  Quando a única ferramenta que você possui é um martelo, todo problema começa a parecer um prego.
set  text[180]  E qual é a sua opinião choramingona?
set  text[181]  A palavra de uma sílaba mais longa na língua inglesa é "screeched."
set  text[182]  Em uma nota canadense de dois dólares, a bandeira que tremula sobre o Parlamento é americana.
set  text[183]  Eu não lutei até o topo da cadeia alimentar para ser vegetariana.
set  text[184]  O que sou eu? Papel pega-moscas para esquisitos?
set  text[185]  Você gostaria de batatas fritas com isso?
set  text[186]  Por que trancam os banheiros dos postos de gasolina? Será que têm medo que alguém os limpe?
set  text[187]  Eu posso não ser a garota mais bonita aqui, mas sou a única que está conversando com você.
set  text[188]  Onde os guardas florestais vão para se afastar de tudo?
set  text[189]  Quem são essas crianças e por que estão me chamando de mamãe?
set  text[190]  Você não é o giz de cera mais brilhante da caixa, não é?
set  text[191]  Não me incomode. Estou vivendo feliz para sempre.
set  text[192]  Eu comecei com nada e ainda tenho a maior parte dele.
set  text[193]  Você! Saia do meu planeta!
set  text[194]  A terapia é cara, estourar plástico bolha é barato! Você escolhe.
set  text[195]  Será que os alienígenas esqueceram de remover sua sonda anal?
set  text[196]  É tão ruim quanto você imagina e eles estão atrás de você.
set  text[197]  Não é assustador que os médicos chamem o que fazem de "prática"?
eval speech %%text[%max%]%%
say %speech%
~
#96
Obj Command 81 - Paintball Shoot Blue~
1 c 2
shoot~
* By Rumble of The Builder Academy    tbamud.com 9091
if %arg.room% != %actor.room% || %arg.id% == %actor.id%
  %send% %actor% Shoot: Invalid Target!
  halt
end
set inroom %actor.room%
if %arg.inventory(80)%
  %echoaround% %actor% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
  %send% %actor% You blast %arg.name%.
  %send% %arg% You lose!
  %purge% %arg.inventory(80)%
  %zoneecho% %inroom.vnum% %actor.name% shoots %arg.name%. A score for the Blue Team.
elseif %arg.inventory(81)%
  %send% %actor% They are on your team!
elseif
  %send% %actor% %arg.name% is not playing.
end
~
#97
Obj Command 80 - Paintball Shoot Red~
1 c 2
shoot~
* By Rumble of The Builder Academy    tbamud.com 9091
if %arg.room% != %actor.room% || %arg.id% == %actor.id%
  %send% %actor% Shoot: Invalid Target!
  halt
end
set inroom %actor.room%
if %arg.inventory(81)%
  %echoaround% %actor% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
  %send% %actor% You blast %arg.name%.
  %send% %arg% You lose!
  %purge% %arg.inventory(81)%
  %zoneecho% %inroom.vnum% %actor.name% shoots %arg.name%. A score for the Red Team.
elseif %arg.inventory(80)%
  %send% %actor% They are on your team!
elseif
  %send% %actor% %arg.name% is not playing.
end
~
#98
Mob Act - 98 Teleporter Give~
0 e 0
steps out of space and time.~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  if !%actor.eq(*)%
    %load% obj 50 %actor% light
    %load% obj 51 %actor% rfinger
    %load% obj 52 %actor% lfinger
    %load% obj 53 %actor% neck1
    %load% obj 54 %actor% neck2
    %load% obj 55 %actor% body
    %load% obj 56 %actor% head
    %load% obj 57 %actor% legs
    %load% obj 58 %actor% feet
    %load% obj 59 %actor% hands
    %load% obj 60 %actor% arms
    %load% obj 61 %actor% shield
    %load% obj 62 %actor% about
    %load% obj 63 %actor% waist
    %load% obj 64 %actor% rwrist
    %load% obj 65 %actor% lwrist
    %load% obj 66 %actor% wield
    %load% obj 67 %actor% hold
  end
  wait 2 sec
  if !%actor.inventory(82)%
    wait 1 s
    say You are not prepared to travel these realms to their fullest.
    wait 1 s
    say Maybe I can help you.
    %load% obj 82
    give teleporter %actor.name%
    * If the players inventory is full drop it for them.
    if !%actor.inventory(82)%
      drop teleporter
    end
    wait 2 s
    say With this you may teleport to areas that may not be accessible in any other way.
    wait 2 s
    say HELP AREAS
  end
end
~
#99
Obj Command 82 - Teleporter~
1 c 3
teleport~
* By Rumble and Jamie Nelson of The Builder Academy    tbamud.com 9091
%send% %actor% You attempt to manipulate space and time.
%echoaround% %actor% %actor.name% attempts to manipulate space and time.
wait 1 sec
set sanctus 100
set jade 400
set newbie 500
set sea 600
set camelot 775
set nuclear 1800
set spider 1999
set arena 2000
set tower 2200
set memlin 2798
set mudschool 2800
set midgaard 3001
set capital 3702
set haven 3998
set chasm 4200
set arctic 4396
set Orc 4401
set monastery 4512
set ant 4600
set zodiac 5701
set grave 7401
set zamba 7500
set gidean 7801
set glumgold 8301
set duke 8660
set oasis 9000
set domiae 9603
set northern 10004
set south 10101
set dbz 10301
set orchan 10401
set elcardo 10604
set iuel 10701
set omega 11501
set torres 11701
set dollhouse 11899
set hannah 12500
set maze 13001
set wyvern 14000
set caves 16999
set cardinal 17501
set circus 18700
set western 20001
set sapphire 20101
set kitchen 22001
set terringham 23200
set dragon 23300
set school 23400
set mines 23500
set aldin 23601
set crystal 23875
set pass 23901
set maura 24000
set enterprise 24100
set new 24200
set valley 24300
set prison 24457
set nether 24500
set yard 24700
set elven 24801
set jedi 24901
set dragonspyre 25000
set ape 25100
set vampyre 25200
set windmill 25300
set village 25400
set shipwreck 25516
set keep 25645
set jareth 25705
set light 25800
set mansion 25907
set grasslands 26000
set igor's 26100
set forest 26201
set farmlands 26300
set banshide 26400
set beach 26500
set ankou 26600
set vice 26728
set desert 26900
set wasteland 27001
set sundhaven 27119
set station 27300
set smurfville 27400
set sparta 27501
set shire 27700
set oceania 27800
set notre 27900
set motherboard 28000
set khanjar 28100
set kerjim 28200
set haunted 28300
set ghenna 28400
set hell 28601
set goblin 28700
set galaxy 28801
set werith's 28900
set lizard 29000
set black 29100
set kerofk 29202
set trade 29400
set jungle 29500
set froboz 29600
set desire 29801
set cathedral 29900
set ancalador 30000
set campus 30100
set bull 30401
set chessboard 30537
set tree 30600
set castle 30700
set baron 30800
set westlawn 30900
set graye 31003
set teeth 31100
set leper 31200
set altar 31400
set mcgintey 31500
set wharf 31700
set dock 31801
set yllnthad 31900
set bay 32200
set pale 32300
set army 32400
set revelry 32500
set perimeter 32600
set asylum 34501
set ultima 55685
set tarot 21101
if !%arg%
  *they didnt type a location
  set fail 1
else
  *take the first word they type after the teleport command
  *compare it to a variable above
  eval loc %%%arg.car%%%
  if !%loc%
    *they typed an invalid location
    set fail 1
  end
end
if %fail%
  %send% %actor% You fail.
  %echoaround% %actor% %actor.name% fails.
  halt
end
%echoaround% %actor% %actor.name% seems successful as %actor.heshe% steps into another realm.
%teleport% %actor% %loc%
%force% %actor% look
%echoaround% %actor% %actor.name% steps out of space and time.
~
$~
