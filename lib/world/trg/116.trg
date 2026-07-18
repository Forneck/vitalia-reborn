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
$~