#1290
Sala de Experiencias 1~
2 d 100
abra~
%echo% This trigger commandlist is not complete!
~
#1200
Magik Zone 12 - Open North~
2 d 100
*abra*~
* Magik trigger - Opens north door when player says "abra"
* Original: OX magik "*abra!*" open north
if %speech.contains(abra)%
  * Check if door exists and is closed
  if %self.north(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% north flags a
    %echo% A porta se abre.
    * Open the other side if it exists
    eval otherroom %self.north(vnum)%
    %at% %otherroom% %echo% A porta se abre.
    %door% %otherroom% south flags a
  end
end
~
#1201
Magik Zone 12 - Close North~
2 d 100
*fecha*~
* Magik trigger - Closes north door when player says "fecha"
* Original: OX magik "*fecha!*" close north
if %speech.contains(fecha)%
  * Check if door exists
  if %self.north(room)%
    * Set closed flag
    %door% %self.vnum% north flags ab
    %echo% A porta se fecha.
    * Close the other side if it exists
    eval otherroom %self.north(vnum)%
    %at% %otherroom% %echo% A porta se fecha.
    %door% %otherroom% south flags ab
  end
end
~
#1202
Magik Zone 12 - Lock North~
2 d 100
*tranca*~
* Magik trigger - Locks north door when player says "tranca"
* Original: OX magik "*tranca!*" lock north
if %speech.contains(tranca)%
  * Check if door exists
  if %self.north(room)%
    * Set closed and locked flags
    %door% %self.vnum% north flags abc
    %echo% Você escuta um ruido vindo da porta.
    * Lock the other side if it exists
    eval otherroom %self.north(vnum)%
    %door% %otherroom% south flags abc
  end
end
~
#1203
Magik Zone 12 - Unlock North~
2 d 100
*destranca*~
* Magik trigger - Unlocks north door when player says "destranca"
* Original: OX magik "*destranca!*" unlock north
if %speech.contains(destranca)%
  * Check if door exists
  if %self.north(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% north flags ab
    %echo% Você escuta um ruido vindo da porta.
    * Unlock the other side if it exists
    eval otherroom %self.north(vnum)%
    %door% %otherroom% south flags ab
  end
end
~
$~
