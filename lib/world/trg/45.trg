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
$~