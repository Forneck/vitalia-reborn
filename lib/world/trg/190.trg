#19000
Elfo Jardineiro~
0 b 50
*~
if %self.pos% == "sleeping"  
  halt
end
set action %random.3%
if %action% == 0
  say Eu adoro cuidar dessas plantas.
  emote rega algumas plantas.
elseif %action% == 1
  say Essa roseira precisa de um pouco mais de ateno.
  emote poda cuidadosamente uma roseira.
elseif %action% == 2
  say Como esto crescendo lindas!
  emote conversa suavemente com as flores do jardim.
end
~
#19001
Elfo Jardineiro Saudacao~
0 g 100
~
say Bem-vindo ao jardim! Espero que aproveite a beleza das flores.
emote sorri gentilmente para %actor.name%.
~
#19002
Elfo Apressado~
0 b 30
~
if %self.fighting% || %self.pos% == "sleeping"
  halt
end
/* Se a varivel de controle no existir, inicia em 0 */
if !%self.counter%
  set %self.counter% 0
end
set counter %self.counter%
if %counter% == 0
  emote termina de comer uma fatia de torta.
elseif %counter% == 1 || %counter% == 2 || %counter% == 3
  perform_move west 1
elseif %counter% == 4
  perform_move south 1
elseif %counter% == 5
  perform_move up 1
elseif %counter% == 6
  emote tira o p de suas roupas.
elseif %counter% == 7
  emote bate na porta.
elseif %counter% == 8
  do_command open porta
elseif %counter% == 9
  perform_move up 1
elseif %counter% == 10
  do_command close porta
elseif %counter% == 11
  emote se aproxima do prefeito.
elseif %counter% == 12
  emote sussurra para o prefeito, 'Prefeito? Senhor? O Senhor est acordado?'.
elseif %counter% == 13
  emote coloca alguns papis na mesa do Prefeito.
elseif %counter% == 14
  emote despede-se de todos.
elseif %counter% == 15
  do_command open porta
elseif %counter% == 16
  perform_move down 1
elseif %counter% == 17
  do_command close porta
elseif %counter% == 18
  perform_move down 1
elseif %counter% == 19
  perform_move north 1
elseif %counter% == 20 || %counter% == 21 || %counter% == 22
  perform_move east 1
elseif %counter% == 23
  emote pede um copo de limonada e uma fatia de torta ao barman.
else
  set counter 0
end
/* Incrementa e armazena o contador */
set counter %counter% + 1
if %counter% > 23
  set counter 0
end
set %self.counter% %counter%
~
#19003
Apressado Greet~
0 g 100
~
if !%actor.is_pc%
  halt
end
if !%self.canbeseen(%actor%)
  halt
end
greet %actor.name%
~
$~
