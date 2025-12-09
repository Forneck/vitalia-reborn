#11600
Flow Room Effect~
2 g 100
~
* Old SpecProc: flow - Room flow effect
* This creates a flowing current that moves players
if %self.people%
  wflow A correnteza o arrasta! south
end
~
#11601
Lightning Bolt Caster~
0 k 15
~
* Old SpecProc: magic_lbolt - Casts lightning bolt in combat
dg_cast 'lightning bolt' %actor%
~
#11602
Kanakajja Greet Message~
0 g 100
~
* Old SpecProc: kanakajja - Greets players entering the room
* Only fires in room 11642 (the golden river room) and if mob is awake
if %self.room.vnum% != 11642 || %actor.is_npc%
  halt
end
if %self.pos% < 8
  halt
end
%send% %actor% %self.name% disse, 'Não é permitido, para o bem de todos, que se vá além daqui nesse rio.'
~
#11603
Kanakajja Blocks South Movement~
0 q 100
~
* Old SpecProc: kanakajja - Blocks movement to the south
* Only fires in room 11642, when mob is awake, and for non-NPCs
if %self.room.vnum% != 11642 || %actor.is_npc%
  return 0
  halt
end
if %self.pos% < 8
  return 0
  halt
end
* Check if player is trying to go south
if %direction% == south
  %echo% %self.name% disse, 'Ir além é possível. Mas não posso permitir. Perigos demais.'
  return 1
  halt
end
* Allow movement in other directions
return 0
~
#11604
Kanakajja Combat Messages~
0 k 100
~
* Old SpecProc: kanakajja - Message when entering combat
if %self.room.vnum% != 11642
  halt
end
%echo% %self.name% disse, 'Imprudente, sofrerás agora de qualquer maneira.'
~
#11605
Kanakajja Kill Message~
0 t 100
~
* Old SpecProc: kanakajja - Message when killing someone
if %self.room.vnum% != 11642
  halt
end
%echo% %self.name% disse, 'Regozije-se desta morte. Poupei-lhe de uma pior.'
~
#11606
Kanakajja Death Message~
0 f 100
~
* Old SpecProc: kanakajja - Message when dying
if %self.room.vnum% != 11642
  halt
end
%echo% %self.name% disse, 'Prepare-se agora para seu merecido castigo. Eu avisei.'
~
$~