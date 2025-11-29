#6900
Blocker Movement Check~
0 q 100
~
* Old SpecProc: blocker - Blocks movement for non-immortals
* Check the direction the player must go to enter restricted area
if %actor.level% >= 101
  * Let immortals pass
  return 0
end
%send% %actor% %self.name% encara você e bloqueia sua passagem.
%echoaround% %actor% %self.name% bloqueia a passagem de %actor.name%.
return 0
~
#6901
Airflow Room Effect~
2 g 100
~
* Old SpecProc: airflow - Converts room between FLYING and AIR_FLOW sectors
* This creates an alternating air current effect in the room
if %self.sector% == 9
  %echo% Uma intensa coluna de ar surge, impelindo tudo para o alto.
  wsector 11
else
  %echo% A coluna de ar desaparece.
  wsector 9
end
~
$~
