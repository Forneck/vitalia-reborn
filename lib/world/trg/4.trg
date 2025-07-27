#400
Secretaria~
0 b 100
*~
if %self.fighting%  
  halt
end
set i %random.40%
switch %i%
  case 0
    say Você tem hora marcada?
    halt
    case 1
    say O salão de reunies é só para líderes e seus sucessores, lamento.
    halt
    case 2
    say Este prédio é lindo, no acha?
    halt
    case 3
    say Tenho que regar as plantas.
    halt
    case 4
    say Com licença, tenho que escrever uns papiros e marcar umas reuniões.
    halt
    case 5
    say Visite toda a nossa Torre, heim?
    halt
    case 6
    say Já escolheu um de nossos Clãs?
    halt
    case 7
    say Não se esqueça de ler sempre o Policy de Vitália e o Clã Policy.
    halt
    case 8
    emote escreve em um papiro.
    halt
    case 9
    emote rega as plantas.
    halt
    case 10
    emote olha pela janela.
    halt
    case 11
    emote sorri.
    halt
    case 12
    emote anota recados.
    halt
~
#401
Saudacao~
0 gi 100
*~
if %time.hour% < 6 || %time.hour% >= 18
  say Boa noite.
else
  if %time.hour% < 12
    say Bom dia.
  else
    say Boa tarde.
  end
end
~
$~
