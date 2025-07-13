* Zone 2 Triggers - O Castelo da Rainha Gelada

#200
Name: 'Entrada do Castelo - Atmosfera'~
Argument List: 100~
Trigger Intended for: Rooms
Trigger Types: Random (auto)
Commands:
%echo% %Cristais de gelo fazem um som melodioso com o vento gelado.
%echo% %Flocos de neve caem silenciosamente do céu escuro.
%echo% %O ar fica mais frio e sua respiração se torna visível.
%echo% %Um vento gelado sopra através das torres do castelo.
%echo% %Você pode ouvir o eco distante de passos em corredores de gelo.
%load% mob 201 ; Carrega um soldado gelado ocasionalmente
~

#201  
Name: 'Guarda Gelo - Desafio'~
Argument List: entra~
Trigger Intended for: Mobiles
Trigger Types: Entry
Commands:
if %direction% == north
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
Name: 'Rainha Gelada - Boss Fight'~
Argument List: 10~
Trigger Intended for: Mobiles  
Trigger Types: Random (auto)
Commands:
switch %random.4%
  case 1
    %echo% A Rainha Gelada levanta sua mão e cristais de gelo começam a se formar no ar!
    %areaattack% 50 %Uma rajada de cristais afiados atinge todos ao redor!
  case 2
    %echo% A temperatura despenca drasticamente!
    %echo% %Um frio intenso drena suas forças!
    %damage% all 25
  case 3
    %echo% A Rainha Gelada convoca servos de gelo para ajudá-la!
    %load% mob 201
    %load% mob 201
  case 4
    %echo% Os olhos da Rainha brilham com poder sobrenatural!
    say Mortais tolos! Vocês não podem derrotar o inverno eterno!
  default
    %echo% O trono de gelo pulsa com energia mágica.
done
~

#203
Name: 'Prisão - Evento Escape'~
Argument List: 5~
Trigger Intended for: Rooms
Trigger Types: Random (auto)
Commands:
%echo% %Um prisioneiro geme baixinho em uma das celas próximas.
%echo% %Você pode ouvir correntes balançando no vento gelado.
%echo% %Passos ecoam nos corredores distantes da prisão.
if %random.10% == 1
  %echo% %CLANG! Alguma coisa pesada cai no chão de uma cela próxima!
  %echo% %Uma voz fraca grita: "Por favor! Alguém me ajude!"
  %load% mob 205 ; Carrega um prisioneiro
end
~

#204
Name: 'Tesouro Real - Armadilha'~
Argument List: baú tesouro~
Trigger Intended for: Objects
Trigger Types: Get
Commands:
%send% %actor% Ao tocar o baú, uma armadilha de gelo é ativada!
%echoaround% %actor% %actor.name% ativa uma armadilha no baú!
%echo% %Espinhos de gelo brotam do chão ao redor do baú!
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
Name: 'Elemental Gelo - Invocação'~
Argument List: morre~  
Trigger Intended for: Mobiles
Trigger Types: Death
Commands:
%echo% %Quando o elemental morre, sua essência se dispersa pelo ar!
%echo% %Cristais de gelo se espalham pelo chão onde ele estava.
%load% obj 207 ; Carrega uma gema de gelo eterno
%echo% %Uma gema de gelo eterno cristaliza a partir da essência dispersa!
if %random.3% == 1
  %echo% %A morte do elemental desperta outros seres gelados!
  %load% mob 208 ; Carrega outro elemental
  %echo% %Outro elemental do gelo se materializa, enfurecido!
end
~

#206  
Name: 'Biblioteca Gélida - Conhecimento'~
Argument List: livro pergaminho~
Trigger Intended for: Rooms
Trigger Types: Command
Commands:
%send% %actor% Você examina os tomos antigos preservados pelo frio eterno.
%echoaround% %actor% %actor.name% estuda os livros gelados da biblioteca.
wait 2 sec
%send% %actor% As páginas contêm segredos sobre magia de gelo e inverno.
if %actor.class% == Mago
  %send% %actor% Como mago, você compreende melhor estes conhecimentos arcanos!
  %send% %actor% Sua compreensão sobre magias de gelo aumenta temporariamente.
  %affect% %actor% 'bless' 300
end
%load% obj 206 ; Carrega um pergaminho de congelamento
%send% %actor% Você encontra um pergaminho útil entre os livros!
~

#207
Name: 'Torres - Sistema Vigilância'~
Argument List: 15~
Trigger Intended for: Rooms  
Trigger Types: Random (auto)
Commands:
%echo% %De uma das torres, você pode ver guardas patrulhando as muralhas.
%echo% %Tochas mágicas queimam com chamas azuis nas ameias do castelo.
%echo% %O vento uiva através das aberturas da torre.
if %random.5% == 1
  %echo% %Um guarda na torre aponta em sua direção e grita um alerta!
  %echo% %"Intrusos no pátio!" ecoa por todo o castelo.
  %load% mob 200 ; Carrega um guarda de gelo
  %echo% %Um guarda de gelo responde ao chamado e vem investigar!
end
~

#208
Name: 'Coroa Real - Maldição'~
Argument List: coroa~
Trigger Intended for: Objects
Trigger Types: Wear  
Commands:
%send% %actor% Ao colocar a coroa, você sente um frio sobrenatural!
%echoaround% %actor% %actor.name% coloca a coroa e uma aura azul o envolve!
%echo% %A temperatura ao redor diminui drasticamente!
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
Name: 'Passagem Secreta - Descoberta'~
Argument List: pesquisa procura examina~
Trigger Intended for: Rooms
Trigger Types: Command  
Commands:
if %cmd% == pesquisa || %cmd% == procura || %cmd% == examina
  if %arg% == parede || %arg% == muralha || %arg% == pedra
    %send% %actor% Você examina cuidadosamente as paredes de gelo...
    %echoaround% %actor% %actor.name% examina as paredes com atenção.
    wait 3 sec
    if %random.3% == 1
      %send% %actor% Você nota uma marca estranha em uma das pedras!
      %send% %actor% Pressionando-a, uma passagem secreta se abre!
      %echoaround% %actor% %actor.name% descobre uma passagem secreta!
      %door% 258 south flags a ; Abre passagem para sala secreta
      %echo% %Uma passagem secreta se abre na parede sul!
    else
      %send% %actor% Você não encontra nada de interessante nas paredes.
    end
  end
end
~

$~