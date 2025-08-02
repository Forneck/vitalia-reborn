#2900
Gatekeeper Welcome - 2997~
0 g 33
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote says in a bored tone, 'Welcome to Haven, stranger.  Enjoy your stay.'
~
#2901
Mad Prisoner - 2978~
0 c 100
listen~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Diamo... Diamo, I can't reach the pile Diamo!  Help me, please!
%force% %actor% look dungeon
~
#2902
Scream - 2975~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote screams, 'Get out of this room RIGHT NOW!'
~
#2903
Shop A - 2973~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote says eagerly, 'How may I help you?  Hm... You look familiar.  Perhaps I have heard of you.  Would you be %actor.name%?'
~
#2904
Shop B - 2973~
0 d 100
yes~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Are you really now?
peer %actor.name%
say You could be... You look about like your friend described.
say Well in that case... they wanted me to give this to you.
give flagon %actor.name%
say He bought it from Gilles, at the bar.  Drink well.
wink %actor.name%
~
#2905
Tia A - 2956~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say A little bit more of...  Oh!  A customer!  How may I provide thee with service?
~
#2906
Mithroq A - 2955~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote growls, 'What is it you want, stranger?'
~
#2908
Gilles A - 2971~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Would you like a drink?  They are good for the thirst.
~
#2909
Milo A - 2957~
0 g 25
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say 200789... 200790... 200791...
emote gets a dreamy look in his eyes as he stares at his pile of glittering gold.
emote snaps to attention, finally noticing that there is a person in the room.
say Well, if it isn't %actor.name%.  Your reputation preceeds you.  Welcome to my humble bank.
bow %actor.name%
~
#2910
Yelling Woman - Not Attached~
0 gn 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
load obj 2952
give yelling platter
load obj 2951
give yelling pitcher
load 2912
give yelling spoon
~
#2911
Near Death Trap on the Rocks - 2975~
2 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
* Near Death Trap stuns actor
wait 6 sec
set stunned %actor.hitp%
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% You lay among the jagged rocks.
~
$~
