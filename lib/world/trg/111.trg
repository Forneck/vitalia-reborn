#11100
Urso com Fome~
0 b 25
~
if %actor.fighting% || %actor.pos(sleeping)%
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
if  %object.type% ==  FOOD 
  wait 2 seconds
  emote come %object.name%.
  oremove %object.name%
  purge %object.name%
end
~
#11102
Urso Acorda~
0 h 100
*~
if %self.fighting%
  halt
end
emote se mexe ligeiramente, soltando um resmungo sonolento.
wait 2
emote abre os olhos devagar, observando %actor.name%.
wait 2
emote se espreguia e se levanta, sacudindo o pelo.
nop %self.pos(standing)%
follow %actor.name%
~
$~
