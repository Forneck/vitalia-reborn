#11400
Magik Zone 114 - Unlock and Open East Avarohana~
2 d 100
*avarohana*~
* Magik trigger - Unlocks then opens east exit when player says "avarohana"
* Original: OX magik "*avarohana*" unlock east; OX magik "*avarohana*" open east
* Note: merged into single trigger because speech_wtrigger only fires one trigger per say
* The east exit (escuridao) has EX_GHOSTPROOF; flags ac = EX_ISDOOR + EX_GHOSTPROOF (open state)
if %speech.contains(avarohana)%
  if %self.east(room)%
    * Step 1: unlock (EX_ISDOOR + EX_GHOSTPROOF + EX_CLOSED, removes EX_LOCKED)
    %door% %self.vnum% east flags aco
    %echo% Você escuta um ruído vindo da porta.
    eval otherroom %self.east(vnum)%
    %door% %otherroom% west flags aco
    wait 1s
    * Step 2: open (EX_ISDOOR + EX_GHOSTPROOF, removes EX_CLOSED)
    %door% %self.vnum% east flags ac
    %echo% A porta se abre.
    %at% %otherroom% %echo% A porta se abre.
    %door% %otherroom% west flags ac
  end
end
~
#11401
Magik Zone 114 - Open East Avarohana (merged into 11400)~
2 d 100
*UNUSED*~
* Merged into trigger 11400 — speech_wtrigger only fires one trigger per say event
~
$~
