#23839
Vomitar suas entranhas - 23842~
2 c 100
exa~
if %cmd.mudcommand% == examine && corpse /= %arg%
  %send% %actor% Ao se aproximar da pilha de cadáveres em decomposição, o cheiro de morte e cadáveres podres o atinge.
  %echoaround% %actor% %actor.name% começa a se aproximar dos cadáveres e é parado pelo cheiro horrível.
  %force% %actor% puke
else
  %send% %actor% Você não pode %cmd% isso!
end
~
#23840
Puxar tocha - 23840~
2 c 100
pu~
if pull /= %cmd% && torch /= %arg%
  %send% %actor% A tocha gira facilmente em sua mão revelando um alçapão.
  %echoaround% %actor% Quando %actor.name% puxa a tocha, um alçapão se abre levando para cima.
  %door% 23840 up room 23841
  %door% 23840 up name alçapão trap door
  wait 60s
  %echo% O alçapão se fecha sozinho como se por mágica.
  %door% 23840 up purge
else
  %send% %actor% OK!
  %echoaround% %actor% Você começa a questionar a sanidade de %actor.name% enquanto eles tropeçam pela sala.
end
~
#23841
Monstro cumprimenta - 23841~
0 hi 100
~
if %actor.is_pc%
  wait 2s
  emote olha para cima e sorri para %actor.name%
  wait 6s
  say Bah!! sua cabeça é muito pequena para adicionar à minha coleção!!
  wait 2s
  emote volta sua atenção para as cabeças que revestem a parede.
end
~
#23842
Ainda muito pequeno - 23841~
0 f 100
~
say Sua cabeça ainda é muito pequena!!
emote ri com alegria insana.
~
#23843
Portal - 23843~
1 c 7
en~
if %cmd.mudcommand% == enter && box /= %arg%
  %send% %actor% Você entra em uma caixa misteriosa.
  %echoaround% %actor% %actor.name% entra em uma caixa misteriosa.
  %teleport% %actor% 23844
  %force% %actor% look
  %echoaround% %actor% %actor.name% sai de uma caixa misteriosa.
else
  %send% %actor% %cmd.mudcommand% o quê?!
end
~
#23853
Enigma 1 - 23860~
0 g 100
~
if %actor.is_pc%
  wait 1s
  say olá estranho. Eu sei por que você está aqui, você quer entrar no portão, não é.
end
~
#23854
Comando yes - 23860~
0 d 1
y~
if yes /= %speech%
  say Bem, então você tem que responder alguns enigmas primeiro. Espero que você seja esperto e não esteja desperdiçando meu tempo.
  wait 3s
  say hhhhmmmmmm qual devo usar primeiro.
  wait 1s
  say ahhh entendi.
  wait 1s
  say ok lá vai, até onde você pode correr para dentro da floresta?
end
~
#23855
Portal da fonte - 23855~
1 c 7
en~
if %cmd.mudcommand% == enter && fountain /= %arg%
  %send% %actor% Você mergulha em uma grande fonte.
  %echoaround% %actor% %actor.name% mergulha em uma grande fonte.
  %teleport% %actor% 23856
  %force% %actor% look
  %echoaround% %actor% %actor.name% emerge de uma grande fonte.
else
  %send% %actor% entrar onde?!
end
~
#23856
Enigma 2 - 23860~
0 d 0
halfway~
  emote aplaude %actor.name%
say bem feito.
wait 2 sec
say hhhhhmmmmm o que devo usar a seguir?
wait 1 s
say ahh entendi.
wait 2 s
say ok lá vai, em linguagem mágica qual palavra significa voar?
~
#23857
Enigma 3 - 23860~
0 d 100
yrl~
say muito bom
wait 2 s
emote pondera sobre qual pergunta usar a seguir.
wai 3 s
say ahh entendi
wait 1 s
say ok o que tem 4 pernas para cima, 4 pernas para baixo e é macio no meio?
~
#23858
Enigma 4 - 23860~
0 d 100
bed~
say bem feito!!
wait 2 s
say Tenho mais uma para você e então você pode passar.
wait 1 s
say ok à noite eles vêm sem serem buscados, e de dia são perdidos sem serem roubados, o que é?
~
#23859
Enigma 5 - 23860~
0 d 100
stars~
say muito bom, mas mudei de ideia, você acertou muito rápido, precisa responder mais alguns para passar.
wait 2 s
say ok lá vai
wait 1 s
say Eu nunca fui mas sempre serei, ninguém nunca me viu, ninguém nunca verá, mas sou a confiança de todos que vivem e respiram nesta bola terrestre, o que sou eu?
~
#23860
Enigma 6 - 23860~
0 d 100
tomorrow~
say bem feito!!
wait 1 s
emote aplaude %actor.name% por um trabalho bem feito.
wait 1 s
say você pode agora entrar no portão ao norte, você possui o conhecimento necessário.
wait 1 s
%load% obj 23860
drop all
wait 2 s
say use-a bem, meu filho.
~
#23861
Entrada portal verde - 23863~
1 c 100
en~
if %cmd.mudcommand% == enter && green /= %arg%
  %send% %actor% Você entra em um grande portal verde.
  %echoaround% %actor% %actor.name% entra em um grande portal verde.
  %teleport% %actor% 23864
  %force% %actor% look
  %echoaround% %actor% entra em um grande portal verde.
else
  %send% %actor% entrar onde?!
end
~
#23862
Entrada portal azul - 23861~
1 c 100
en~
if %cmd.mudcommand% == enter && blue /= %arg%
  %send% %actor% Você entra em um grande portal azul.
  %echoaround% %actor% %actor.name% entra em um grande portal azul.
  %teleport% %actor% 23862
  %force% %actor% look
  %echoaround% %actor% atravessa um grande portal azul.
else
  %send% %actor% entrar onde?!
end
~
#23863
Entrada portal vermelho - 23862~
1 c 100
en~
if %cmd.mudcommand% == enter && red /= %arg%
  %send% %actor% Você entra em um grande portal vermelho.
  %echoaround% %actor% %actor.name% entra em um grande portal vermelho.
  %teleport% %actor% 23863
  %force% %actor% look
  %echoaround% %actor% atravessa um grande portal vermelho.
else
  %send% %actor% entrar onde?!
end
~
$~
