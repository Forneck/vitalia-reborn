#9800
Angels Mob Behavior~
0 b 10
~
* Old SpecProc: angels - Angel mob behavior
set actor %random.char%
if %actor.is_pc%
  if %random.10% == 1
    %echo% %self.name% bate suas asas brilhantes.
  elseif %random.10% == 2
    %echo% %self.name% emite uma luz suave e reconfortante.
  elseif %random.10% == 3
    say Que a paz esteja convosco, mortais.
  end
end
~
#9801
Scroll Activate 9808~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_9808 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho antigo se desfaz em suas mãos.
  %echoaround% %actor% %actor.name% ativa um pergaminho antigo.
  * Object should be extracted after use
end
~
#9802
Evil Boss Aggressive~
0 e 0
entra entrou entrar enter entered~
* Old SpecProc: evil_boss - Aggressive behavior for boss mobs
if %actor.is_pc%
  wait 2
  emote rosna ameaçadoramente para %actor.name%.
  say Você ousa entrar em meu domínio?
  wait 3
  kill %actor.name%
end
~
$~