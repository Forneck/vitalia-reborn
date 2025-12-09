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
#6902
Magik Zone 69 - Unlock Down BUFONIDAE~
2 d 100
*bufonidae*~
* Magik trigger - Unlocks down exit when player says "BUFONIDAE"
* Original: OX magik "*BUFONIDAE*" unlock down
if %speech.contains(BUFONIDAE)% || %speech.contains(bufonidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% down flags ab
    %echo% Você escuta um ruido vindo do alçapão.
    * Unlock the other side if it exists
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ab
  end
end
~
#6903
Magik Zone 69 - Open Down BUFONIDAE~
2 d 100
*bufonidae*~
* Magik trigger - Opens down exit when player says "BUFONIDAE"
* Original: OX magik "*BUFONIDAE*" open down
if %speech.contains(BUFONIDAE)% || %speech.contains(bufonidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    * Open the other side if it exists
    eval otherroom %self.down(vnum)%
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6904
Magik Zone 69 - Unlock Down OPISTHOCOELA~
2 d 100
*opisthocoela*~
* Magik trigger - Unlocks down exit when player says "OPISTHOCOELA"
* Original: OX magik "*OPISTHOCOELA*" unlock down
if %speech.contains(OPISTHOCOELA)% || %speech.contains(opisthocoela)%
  * Check if door exists
  if %self.down(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% down flags ab
    %echo% Você escuta um ruido vindo do alçapão.
    * Unlock the other side if it exists
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ab
  end
end
~
#6905
Magik Zone 69 - Open Down OPISTHOCOELA~
2 d 100
*opisthocoela*~
* Magik trigger - Opens down exit when player says "OPISTHOCOELA"
* Original: OX magik "*OPISTHOCOELA*" open down
if %speech.contains(OPISTHOCOELA)% || %speech.contains(opisthocoela)%
  * Check if door exists
  if %self.down(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    * Open the other side if it exists
    eval otherroom %self.down(vnum)%
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6906
Magik Zone 69 - Unlock Down PIPIDAE~
2 d 100
*pipidae*~
* Magik trigger - Unlocks down exit when player says "PIPIDAE"
* Original: OX magik "*PIPIDAE*" unlock down
if %speech.contains(PIPIDAE)% || %speech.contains(pipidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% down flags ab
    %echo% Você escuta um ruido vindo do alçapão.
    * Unlock the other side if it exists
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ab
  end
end
~
#6907
Magik Zone 69 - Open Down PIPIDAE~
2 d 100
*pipidae*~
* Magik trigger - Opens down exit when player says "PIPIDAE"
* Original: OX magik "*PIPIDAE*" open down
if %speech.contains(PIPIDAE)% || %speech.contains(pipidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    * Open the other side if it exists
    eval otherroom %self.down(vnum)%
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6908
Magik Zone 69 - Unlock Down BREVICIPITIDAE~
2 d 100
*brevicipitidae*~
* Magik trigger - Unlocks down exit when player says "BREVICIPITIDAE"
* Original: OX magik "*BREVICIPITIDAE*" unlock down
if %speech.contains(BREVICIPITIDAE)% || %speech.contains(brevicipitidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% down flags ab
    %echo% Você escuta um ruido vindo do alçapão.
    * Unlock the other side if it exists
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ab
  end
end
~
#6909
Magik Zone 69 - Open Down BREVICIPITIDAE~
2 d 100
*brevicipitidae*~
* Magik trigger - Opens down exit when player says "BREVICIPITIDAE"
* Original: OX magik "*BREVICIPITIDAE*" open down
if %speech.contains(BREVICIPITIDAE)% || %speech.contains(brevicipitidae)%
  * Check if door exists
  if %self.down(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    * Open the other side if it exists
    eval otherroom %self.down(vnum)%
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6910
Magik Zone 69 - Unlock Down SALIENTIA~
2 d 100
*salientia*~
* Magik trigger - Unlocks down exit when player says "SALIENTIA"
* Original: OX magik "*SALIENTIA*" unlock down
if %speech.contains(SALIENTIA)% || %speech.contains(salientia)%
  * Check if door exists
  if %self.down(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% down flags ab
    %echo% Você escuta um ruido vindo do alçapão.
    * Unlock the other side if it exists
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ab
  end
end
~
#6911
Magik Zone 69 - Open Down SALIENTIA~
2 d 100
*salientia*~
* Magik trigger - Opens down exit when player says "SALIENTIA"
* Original: OX magik "*SALIENTIA*" open down
if %speech.contains(SALIENTIA)% || %speech.contains(salientia)%
  * Check if door exists
  if %self.down(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    * Open the other side if it exists
    eval otherroom %self.down(vnum)%
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
$~
