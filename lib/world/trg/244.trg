#24400
Scroll Activate 24400~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_24400 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho brilha intensamente e se dissolve.
  %echoaround% %actor% %actor.name% ativa um pergaminho mágico.
  * Object should be extracted after use
end
~
$~