#14000
Tree Mob Behavior~
0 b 15
~
* Old SpecProc: tree - Tree mob behavior
if %random.5% == 1
  %echo% %self.name% sussurra no vento.
elseif %random.5% == 2
  %echo% As folhas de %self.name% balançam suavemente.
end
~
#14001
Magic Missile Caster~
0 k 15
~
* Old SpecProc: magic_mmissile - Casts magic missile in combat
dg_cast 'magic missile' %actor%
~
#14002
Color Spray Caster~
0 k 15
~
* Old SpecProc: magic_cspray - Casts color spray in combat
dg_cast 'color spray' %actor%
~
#14003
Chill Touch Caster~
0 k 15
~
* Old SpecProc: magic_ctouch - Casts chill touch in combat
dg_cast 'chill touch' %actor%
~
$~