#5500
Dump - 5547~
2 h 100
~
%echo% %object.shortdesc% vanishes in a puff of smoke!
%send% %actor% You are awarded for outstanding performance.
%echoaround% %actor% %actor.name% has been awarded for being a good citizen.
eval value %object.cost% / 10
%purge% %object%
if %value% > 50
  set value 50
elseif %value% < 1
  set value 1
end
if %actor.level% < 3
  nop %actor.exp(%value%)%
else
  nop %actor.gold(%value%)%
end
~
#5501
portal to Midgaard~
1 c 7
en~
* By Cansian
if %cmd.mudcommand% == enter && %arg% == estatua
  %send% %actor% Você passa através da estatua.
  %echoaround% %actor% %actor.name% bravamente entra na estátua.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% acabou de sair da parede.
else
  %send% %actor% %cmd% oquê?!
end
~
$~