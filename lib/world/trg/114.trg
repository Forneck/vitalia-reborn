#11400
Magik Zone 114 - Unlock East Avarohana~
2 d 100
*avarohana*~
* Magik trigger - Unlocks east exit when player says "avarohana"
* Original: OX magik "*avarohana*" unlock east
if %speech.contains(avarohana)%
  * Check if door exists
  if %self.east(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% east flags ab
    %echo% Você escuta um ruido vindo da porta.
    * Unlock the other side if it exists
    eval otherroom %self.east(vnum)%
    %door% %otherroom% west flags ab
  end
end
~
#11401
Magik Zone 114 - Open East Avarohana~
2 d 100
*avarohana*~
* Magik trigger - Opens east exit when player says "avarohana"
* Original: OX magik "*avarohana*" open east
if %speech.contains(avarohana)%
  * Check if door exists
  if %self.east(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% east flags a
    %echo% A porta se abre.
    * Open the other side if it exists
    eval otherroom %self.east(vnum)%
    %at% %otherroom% %echo% A porta se abre.
    %door% %otherroom% west flags a
  end
end
~
$~
