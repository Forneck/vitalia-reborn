#400
Secretaria~
0 b 30
~
if %self.fighting%  
  halt
end
set i %random.40%
switch %i%
  case 0
    say Você tem hora marcada?
    case 1
    say O salão de reunies é só para líderes e seus sucessores, lamento.
    case 2
    say Este prdio é lindo, no acha?
    case 3
    say Tenho que regar as plantas.
    case 4
    say Com licença, tenho que escrever uns papiros e marcar umas reunies.
    case 5
    say Visite toda a nossa Torre, heim?
    case 6
    say Já escolheu um de nossos Clãs?
    case 7
    say Não se esqueça de ler sempre o Policy de Vitália e o Clã Policy.
    case 8
    emote escreve em um papiro.
    case 9
    emote rega as plantas.
    case 10
    emote olha pela janela.
    case 11
    emote sorri.
    case 12
    emote anota recados.
~
#401
Saudacao~
0 i 100
~
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
