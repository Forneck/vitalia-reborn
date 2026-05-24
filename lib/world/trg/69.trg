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
Magik Zone 69 - Unlock and Open Down BUFONIDAE~
2 d 100
*bufonidae*~
* Magik trigger - Unlocks then opens down exit when player says "BUFONIDAE"
* Original: OX magik "*BUFONIDAE*" unlock down; OX magik "*BUFONIDAE*" open down
* Note: merged into single trigger because speech_wtrigger only fires one trigger per say
if %speech.contains(BUFONIDAE)% || %speech.contains(bufonidae)%
  if %self.down(room)%
    * Step 1: unlock (EX_ISDOOR + EX_CLOSED, removes EX_LOCKED)
    %door% %self.vnum% down flags ao
    %echo% Você escuta um ruído vindo do alçapão.
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ao
    wait 1s
    * Step 2: open (EX_ISDOOR only, removes EX_CLOSED)
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6903
Magik Zone 69 - Open Down BUFONIDAE (merged into 6902)~
2 d 100
*UNUSED*~
* Merged into trigger 6902 — speech_wtrigger only fires one trigger per say event
~
#6904
Magik Zone 69 - Unlock and Open Down OPISTHOCOELA~
2 d 100
*opisthocoela*~
* Magik trigger - Unlocks then opens down exit when player says "OPISTHOCOELA"
* Original: OX magik "*OPISTHOCOELA*" unlock down; OX magik "*OPISTHOCOELA*" open down
if %speech.contains(OPISTHOCOELA)% || %speech.contains(opisthocoela)%
  if %self.down(room)%
    %door% %self.vnum% down flags ao
    %echo% Você escuta um ruído vindo do alçapão.
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ao
    wait 1s
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6905
Magik Zone 69 - Open Down OPISTHOCOELA (merged into 6904)~
2 d 100
*UNUSED*~
* Merged into trigger 6904
~
#6906
Magik Zone 69 - Unlock and Open Down PIPIDAE~
2 d 100
*pipidae*~
* Magik trigger - Unlocks then opens down exit when player says "PIPIDAE"
* Original: OX magik "*PIPIDAE*" unlock down; OX magik "*PIPIDAE*" open down
if %speech.contains(PIPIDAE)% || %speech.contains(pipidae)%
  if %self.down(room)%
    %door% %self.vnum% down flags ao
    %echo% Você escuta um ruído vindo do alçapão.
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ao
    wait 1s
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6907
Magik Zone 69 - Open Down PIPIDAE (merged into 6906)~
2 d 100
*UNUSED*~
* Merged into trigger 6906
~
#6908
Magik Zone 69 - Unlock and Open Down BREVICIPITIDAE~
2 d 100
*brevicipitidae*~
* Magik trigger - Unlocks then opens down exit when player says "BREVICIPITIDAE"
* Original: OX magik "*BREVICIPITIDAE*" unlock down; OX magik "*BREVICIPITIDAE*" open down
if %speech.contains(BREVICIPITIDAE)% || %speech.contains(brevicipitidae)%
  if %self.down(room)%
    %door% %self.vnum% down flags ao
    %echo% Você escuta um ruído vindo do alçapão.
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ao
    wait 1s
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6909
Magik Zone 69 - Open Down BREVICIPITIDAE (merged into 6908)~
2 d 100
*UNUSED*~
* Merged into trigger 6908
~
#6910
Magik Zone 69 - Unlock and Open Down SALIENTIA~
2 d 100
*salientia*~
* Magik trigger - Unlocks then opens down exit when player says "SALIENTIA"
* Original: OX magik "*SALIENTIA*" unlock down; OX magik "*SALIENTIA*" open down
if %speech.contains(SALIENTIA)% || %speech.contains(salientia)%
  if %self.down(room)%
    %door% %self.vnum% down flags ao
    %echo% Você escuta um ruído vindo do alçapão.
    eval otherroom %self.down(vnum)%
    %door% %otherroom% up flags ao
    wait 1s
    %door% %self.vnum% down flags a
    %echo% O alçapão se abre.
    %at% %otherroom% %echo% O alçapão se abre.
    %door% %otherroom% up flags a
  end
end
~
#6911
Magik Zone 69 - Open Down SALIENTIA (merged into 6910)~
2 d 100
*UNUSED*~
* Merged into trigger 6910
~
$~
