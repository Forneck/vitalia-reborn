#3118
Scroll Activate 3118~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_3118 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho emite uma luz dourada antes de desaparecer.
  %echoaround% %actor% %actor.name% ativa um pergaminho dourado.
  * Object should be extracted after use
end
~
$~