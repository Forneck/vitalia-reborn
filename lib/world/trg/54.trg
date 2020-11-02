#5502
Portal para Midgaard~
1 c 7
en~
* By Cansian
if %cmd.mudcommand% == enter && %arg% == estatua
  %send% %actor% Voce passa atraves da estatua.
  %echoaround% %actor% %actor.name% bravamente entra na estatua.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% acabou de sair da parede.
else
  %send% %actor% %cmd% Oque?!
end
~
$~
