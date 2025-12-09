#3500
Descartar - 3578~
2 h 100
~
%echo% %object.shortdesc% desaparece em uma nuvem de fumaça!
%send% %actor% Você foi recompensado por seu desempenho excepcional.
%echoaround% %actor% %actor.name% foi recompensado por ser um bom cidadão.
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
$~
