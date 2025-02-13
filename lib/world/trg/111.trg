#11100
Urso com Fome~
0 b 30
~
if %actor.fighting% || !%actor.awake%
  halt
end
if %random.4%
  halt
end
if %random.1%
  emote ouve o estmago de 	 si roncando... Alimente-me!
else
  if %self.master% && %self.master.room% == %self.room%
    emote sente o seu cheiro... acho que 	 est com fome!
  end
end
~
#11101
Urso recebe comida~
0 j 100
*~
if  %obj.type% == "FOOD"
  emote come %obj.name%.
  oremove %obj.name%
  purge %obj.name%
end
~
#11102
Urso Acorda~
0 i 100
~
if %self.fighting%
  halt
end
if %self.awake% || %self.aff_flags.contains("sleep")% || %self.master% || %actor.master%
  halt
end
emote acorda.
%self.position% = standing
follow %actor%
~
$~
