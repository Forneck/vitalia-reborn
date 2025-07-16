#200
Entrada do Castelo - Atmosfera - R2XX~
2 b 10
~
* This is for room 2XX - Castle entrance atmospheric effects
%echo% Cristais de gelo fazem um som melodioso com o vento gelado.
%echo% Flocos de neve caem silenciosamente do céu escuro.
%echo% O ar fica mais frio e sua respiração se torna visível.
%echo% Um vento gelado sopra através das torres do castelo.
%echo% Você pode ouvir o eco distante de passos em corredores de gelo.
if %random.10% == 1
  %load% mob 201
end
~
#201
Guarda Gelo - Desafio - M201~
0 g 100
~
* This is for mob 201 - Ice guard challenge trigger
if %direction% == south
  %send% %actor% %self.name% bloqueia sua passagem com uma lança de gelo.
  %echoaround% %actor% %self.name% impede a passagem de %actor.name%.
  %force% %actor% look
  say Alto! Quem ousa entrar no domínio da Rainha Gelada?
  wait 2 sec
  if %actor.level% < 20
    say Você não parece forte o suficiente para enfrentar os perigos aqui dentro.
    say Volte quando for mais experiente, %actor.name%.
    %send% %actor% %self.name% o empurra gentilmente para trás.
    %teleport% %actor% 211
  else
    say Muito bem, %actor.name%. Você parece capaz. Pode passar, mas cuidado!
    %echo% %self.name% se afasta, permitindo a passagem.
  end
end
~
#202
Rainha Gelada - Boss Fight - M2XX~
0 b 10
~
* This is for mob 2XX - Ice Queen boss random actions
switch %random.4%
  case 1
    %echo% A Rainha Gelada levanta sua mão e cristais de gelo começam a se formar no ar!
    %areaattack% 50 Uma rajada de cristais afiados atinge todos ao redor!
    break
  case 2
    %echo% A temperatura despenca drasticamente!
    %echo% Um frio intenso drena suas forças!
    break
  case 3
    %echo% A Rainha Gelada convoca servos de gelo para ajudá-la!
    %load% mob 201
    %load% mob 201
    break
  case 4
    %echo% Os olhos da Rainha brilham com poder sobrenatural!
    say Mortais tolos! Vocês não podem derrotar o inverno eterno!
    break
  default
    %echo% O trono de gelo pulsa com energia mágica.
    break
done
~
#203
Prisão - Evento Escape - R2XX~
2 b 5
~
* This is for room 2XX - Prison random events
%echo% Um prisioneiro geme baixinho em uma das celas próximas.
%echo% Você pode ouvir correntes balançando no vento gelado.
%echo% Passos ecoam nos corredores distantes da prisão.
if %random.10% == 1
  %echo% CLANG! Alguma coisa pesada cai no chão de uma cela próxima!
  %echo% Uma voz fraca grita: "Por favor! Alguém me ajude!"
  %load% mob 205
end
~
#204
Tesouro Real - Armadilha - O2XX~
1 g 100
~
* This is for obj 2XX - Treasure chest trap trigger
%send% %actor% Ao tocar o baú, uma armadilha de gelo é ativada!
%echoaround% %actor% %actor.name% ativa uma armadilha no baú!
%echo% Espinhos de gelo brotam do chão ao redor do baú!
%damage% %actor% 75
if %actor.hitp% > 0
  %send% %actor% Você consegue pegar o baú, mas não sem se machucar!
  %echoaround% %actor% %actor.name% consegue pegar o baú apesar dos ferimentos.
else
  %send% %actor% Os espinhos de gelo o ferem gravemente!
  %echoaround% %actor% %actor.name% é gravemente ferido pelos espinhos de gelo!
end
~
#205
Elemental Gelo - Invocação - M2XX~
0 f 100
~
* This is for mob 2XX - Ice elemental death trigger
%echo% Quando o elemental morre, sua essência se dispersa pelo ar!
%echo% Cristais de gelo se espalham pelo chão onde ele estava.
%load% obj 207
%echo% Uma gema de gelo eterno cristaliza a partir da essência dispersa!
if %random.3% == 1
  %echo% A morte do elemental desperta outros seres gelados!
  %load% mob 208
  %echo% Outro elemental do gelo se materializa, enfurecido!
end
~
#206
Biblioteca Gélida - Conhecimento - R2XX~
2 c 100
e~
* This is for room 2XX - Icy library book command trigger
if %cmd.mudcommand% == examine && %livros.contains('%arg%)%
  %send% %actor% Você examina os tomos antigos preservados pelo frio eterno.
  %echoaround% %actor% %actor.name% estuda os livros gelados da biblioteca.
  wait 2 sec
  %send% %actor% As páginas contêm segredos sobre magia de gelo e inverno.
  if %actor.class% == Mago
    %send% %actor% Como mago, você compreende melhor estes conhecimentos arcanos!
    %send% %actor% Sua compreensão sobre magias de gelo aumenta temporariamente.
    %affect% %actor% 'bless' 300
  end
  %load% obj 206
  %send% %actor% Você encontra um pergaminho útil entre os livros!
else
  return 0
end
~
#207
Torres - Sistema Vigilância - R2XX~
2 b 15
~
* This is for room 2XX - Tower surveillance random events
%echo% De uma das torres, você pode ver guardas patrulhando as muralhas.
%echo% Tochas mágicas queimam com chamas azuis nas ameias do castelo.
%echo% O vento uiva através das aberturas da torre.
if %random.5% == 1
  %echo% Um guarda na torre aponta em sua direção e grita um alerta!
  %echo% "Intrusos no pátio!" ecoa por todo o castelo.
  %load% mob 200
  %echo% Um guarda de gelo responde ao chamado e vem investigar!
end
~
#208
Coroa Real - Maldição - O2XX~
1 j 100
~
* This is for obj 2XX - Royal crown wear trigger
%send% %actor% Ao colocar a coroa, você sente um frio sobrenatural!
%echoaround% %actor% %actor.name% coloca a coroa e uma aura azul o envolve!
%echo% A temperatura ao redor diminui drasticamente!
if %actor.alignment% < 0
  %send% %actor% A coroa aceita sua natureza sombria e concede poder!
  %affect% %actor% 'strength' 600
  %affect% %actor% 'armor' 600
else
  %send% %actor% A coroa rejeita sua natureza e causa dor intensa!
  %damage% %actor% 50
  %send% %actor% Você sente como se estivesse congelando por dentro!
end
~
#209
Passagem Secreta - Descoberta - R2XX~
2 c 100
examine pesquisa procura~
* This is for room 2XX - Secret passage discovery trigger
if %cmd% == pesquisa || %cmd% == procura || %cmd% == examine
  if %arg% == parede || %arg% == muralha || %arg% == pedra
    %send% %actor% Você examina cuidadosamente as paredes de gelo...
    %echoaround% %actor% %actor.name% examina as paredes com atenção.
    wait 3 sec
    if %random.3% == 1
      %send% %actor% Você nota uma marca estranha em uma das pedras!
      %send% %actor% Pressionando-a, uma passagem secreta se abre!
      %echoaround% %actor% %actor.name% descobre uma passagem secreta!
      %door% 258 south flags a
      %echo% Uma passagem secreta se abre na parede sul!
    else
      %send% %actor% Você não encontra nada de interessante nas paredes.
    end
  end
end
~
$~
