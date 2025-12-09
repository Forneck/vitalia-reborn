#4500
Ulmo north~
2 c 100
s~
if %cmd.mudcommand% == say && ulmo /= %arg%
  unlock portao north
else
end
~
#4501
Turgon north~
2 c 100
s~
if %cmd.mudcommand% == say && turgon /= %arg%
  open portao north
else
end
~
#4502
fecha north~
2 c 100
s~
if %cmd.mudcommand% == say && fecha /= %arg%
  close portao north
else
end
~
#4503
tranca north~
2 c 100
s~
if %cmd.mudcommand% == say && tranca /= %arg%
  lock portao north
else
end
~
#4504
Ulmo south~
2 d 1
*~
* evaluate the first word.
eval word %speech.car%
* evaluate the rest of the speech string.
eval rest %speech.cdr%
* keep looping until there are no more words.
while %word%
  if %word% == ulmo
  unlock portao south
  end
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#4505
Turgon south~
2 d 1
*~
* evaluate the first word.
eval word %speech.car%
* evaluate the rest of the speech string.
eval rest %speech.cdr%
* keep looping until there are no more words.
while %word%
  if %word% == turgon
  open portao south
  end
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#4506
fecha south~
2 d 1
*~
* evaluate the first word.
eval word %speech.car%
* evaluate the rest of the speech string.
eval rest %speech.cdr%
* keep looping until there are no more words.
while %word%
  if %word% == fechw
  close portao south
  end
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#4507
tranca south~
2 d 1
*~
* evaluate the first word.
eval word %speech.car%
* evaluate the rest of the speech string.
eval rest %speech.cdr%
* keep looping until there are no more words.
while %word%
  if %word% == tranca
  lock portao south
  end
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#4508
Flow Room Effect~
2 g 100
~
* Old SpecProc: flow - Room flow effect
* This creates a flowing current that moves players
if %self.people%
  wflow A correnteza o arrasta! south
end
~
#4510
Magik Zone 45 - Unlock North Ulmo (New)~
2 d 100
*ulmo*~
* Magik trigger - Unlocks north door when player says "Ulmo"
* Original: OX magik "*Ulmo*" unlock north
* This is an improved version using .contains() for better matching
if %speech.contains(Ulmo)% || %speech.contains(ulmo)%
  * Check if door exists
  if %self.north(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% north flags ab
    %echo% Você escuta um ruído vindo da porta.
    * Unlock the other side if it exists
    eval otherroom %self.north(vnum)%
    %door% %otherroom% south flags ab
  end
end
~
#4511
Magik Zone 45 - Open North Turgon (New)~
2 d 100
*turgon*~
* Magik trigger - Opens north door when player says "Turgon"
* Original: OX magik "*Turgon*" open north
* This is an improved version using .contains() for better matching
if %speech.contains(Turgon)% || %speech.contains(turgon)%
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
#4512
Magik Zone 45 - Close North Fecha (New)~
2 d 100
*fecha*~
* Magik trigger - Closes north door when player says "fecha"
* Original: OX magik "*fecha*" close north
* This is an improved version using .contains() for better matching
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
#4513
Magik Zone 45 - Lock North Tranca (New)~
2 d 100
*tranca*~
* Magik trigger - Locks north door when player says "tranca"
* Original: OX magik "*tranca*" lock north
* This is an improved version using .contains() for better matching
if %speech.contains(tranca)%
  * Check if door exists
  if %self.north(room)%
    * Set closed and locked flags
    %door% %self.vnum% north flags abc
    %echo% Você escuta um ruído vindo da porta.
    * Lock the other side if it exists
    eval otherroom %self.north(vnum)%
    %door% %otherroom% south flags abc
  end
end
~
#4514
Magik Zone 45 - Unlock South Ulmo (New)~
2 d 100
*ulmo*~
* Magik trigger - Unlocks south door when player says "Ulmo"
* Original: OX magik "*Ulmo*" unlock south
* This is an improved version using .contains() for better matching
if %speech.contains(Ulmo)% || %speech.contains(ulmo)%
  * Check if door exists
  if %self.south(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% south flags ab
    %echo% Você escuta um ruído vindo da porta.
    * Unlock the other side if it exists
    eval otherroom %self.south(vnum)%
    %door% %otherroom% north flags ab
  end
end
~
#4515
Magik Zone 45 - Open South Turgon (New)~
2 d 100
*turgon*~
* Magik trigger - Opens south door when player says "Turgon"
* Original: OX magik "*Turgon*" open south
* This is an improved version using .contains() for better matching
if %speech.contains(Turgon)% || %speech.contains(turgon)%
  * Check if door exists and is closed
  if %self.south(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% south flags a
    %echo% A porta se abre.
    * Open the other side if it exists
    eval otherroom %self.south(vnum)%
    %at% %otherroom% %echo% A porta se abre.
    %door% %otherroom% north flags a
  end
end
~
#4516
Magik Zone 45 - Close South Fecha (New)~
2 d 100
*fecha*~
* Magik trigger - Closes south door when player says "fecha"
* Original: OX magik "*fecha*" close south
* This is an improved version using .contains() for better matching
if %speech.contains(fecha)%
  * Check if door exists
  if %self.south(room)%
    * Set closed flag
    %door% %self.vnum% south flags ab
    %echo% A porta se fecha.
    * Close the other side if it exists
    eval otherroom %self.south(vnum)%
    %at% %otherroom% %echo% A porta se fecha.
    %door% %otherroom% north flags ab
  end
end
~
#4517
Magik Zone 45 - Lock South Tranca (New)~
2 d 100
*tranca*~
* Magik trigger - Locks south door when player says "tranca"
* Original: OX magik "*tranca*" lock south
* This is an improved version using .contains() for better matching
if %speech.contains(tranca)%
  * Check if door exists
  if %self.south(room)%
    * Set closed and locked flags
    %door% %self.vnum% south flags abc
    %echo% Você escuta um ruído vindo da porta.
    * Lock the other side if it exists
    eval otherroom %self.south(vnum)%
    %door% %otherroom% north flags abc
  end
end
~
$~