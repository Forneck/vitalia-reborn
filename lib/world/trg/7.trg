#700
Leprechaun Drunk Behavior~
0 b 15
~
* Leprechaun drunk behavior - random actions
if %random.10% == 1
  emote cambaleante tropeça e quase cai.
  say *hic* Quem... quem pegou meu ouro? *hic*
elseif %random.10% == 2
  emote tenta dar um passo mas perde o equilíbrio.
  say Malditos ladrões! Enterraram no quintal... *hic*
elseif %random.10% == 3
  emote segura a cabeça com as mãos.
  say Meus sapatos... jogaram na marginal! *hic*
elseif %random.10% == 4
  say Meu chapéu... puseram num animal! *hic* Que vergonha!
elseif %random.10% == 5
  say Minha moeda de prata... esconderam pela mata! *hic*
end
~
#701
Saci Greet and Taunt~
0 g 100
~
* Saci greets players and taunts about Leprechaun
if %actor.is_pc% && %self.vnum% == 783
  wait 1 sec
  emote pula em uma perna só, rindo maliciosamente.
  say Oi %actor.name%! Viram aquele Leprechaun bebum por ai?
  wait 2 sec
  emote da uma baforada no cachimbo.
  say Eu nasci em Itu, sou cachimbeiro como ele, mas eu nao bebo tanto! Hahaha!
  wait 2 sec
  say Dizem que enganei ele... mas quem bebe demais se engana sozinho!
end
~
#702
Curupira Confuse Tracks~
0 g 100
~
* Curupira confuses those who enter
if %actor.is_pc% && %self.vnum% == 784
  wait 1 sec
  emote olha para você com seus olhos verdes penetrantes.
  say Cuidado com suas pegadas, forasteiro.
  wait 2 sec
  emote mostra seus pés virados para trás.
  say Vê? Minhas pegadas confundem os tolos. O Leprechaun me acusou de roubar
  say seus sapatos, mas ele estava tão bebado que nem viu quem foi!
  wait 2 sec
  emote ri com desdém.
  say Mentiroso! Ele ficou pra trás porque bebe demais!
end
~
#703
Mula sem Cabeca Fire Breath~
0 b 20
~
* Mula sem Cabeça breathes fire randomly
if %random.10% == 1
  emote solta fogo pelo pescoço decepado!
  %echo% O ar fica sufocante com o calor intenso!
elseif %random.10% == 2
  emote relincha de forma assustadora, mesmo sem cabeça!
  %echo% Chamas dançam onde deveria estar sua cabeça!
elseif %random.10% == 3
  emote galopa em círculos, deixando marcas queimadas no chão.
  %echo% Um chapéu verde está preso em sua crina flamejante!
end
~
#704
Mula sem Cabeca Greet Actress~
0 g 100
~
* Mula greets and talks about her past as actress
if %actor.is_pc% && %self.vnum% == 785
  wait 1 sec
  emote para de galopar e se aproxima.
  %send% %actor% A Mula sem Cabeça emite sons estranhos, como se tentasse falar!
  wait 2 sec
  %echo% Uma voz fantasmagórica ecoa: "Eu era uma atriz... antes da maldicao..."
  wait 2 sec
  %echo% A voz continua: "Puseram um chapeu no meu lombo... um chapeu verde irlandes!"
  wait 1 sec
  %echo% A voz termina: "O Leprechaun me acusa, mas ele bebe demais para lembrar!"
end
~
#705
Boitata Thief Behavior~
0 b 15
~
* Boitatá thief behavior
if %random.10% == 1
  emote seus olhos flamejantes brilham com cobiça.
  say Tesourosss... eu guardo todosss...
elseif %random.10% == 2
  emote serpenteia pelo chão, deixando um rastro luminoso.
  say A moeda de prata do Leprechaun... está bem guardada na minha toca...
elseif %random.10% == 3
  emote levanta a cabeça, farejando o ar.
  say Peguei a grana e fiquei na miuda... ninguem me encontra...
elseif %random.10% == 4
  say Moro numa toca por aqui... cheia de tesourosss roubadosss...
end
~
#706
Boitata Greet and Hypnotize~
0 g 100
~
* Boitatá greets and tries to hypnotize
if %actor.is_pc% && %self.vnum% == 786
  wait 1 sec
  emote seus olhos flamejantes fixam em você hipnoticamente.
  %send% %actor% Você sente uma estranha compulsão ao olhar nos olhos do Boitatá!
  wait 2 sec
  say Olhe para meusss olhosss... relaxe...
  wait 2 sec
  emote ri sibilantemente.
  say O Leprechaun me acusa de ladrao... ele está certo! Hissss!
  wait 1 sec
  say Mas ele bebe demaisss para fazer algo a respeito!
end
~
#707
Leprechaun Receive Gold Quest~
0 j 100
~
* Leprechaun receives his gold back
if %object.vnum% == 782
  say Meu ouro! *hic* Voce encontrou meu ouro! *hic*
  wait 1 sec
  emote abraca a pepita de ouro com lagrimas nos olhos.
  say Obrigado %actor.name%! *hic* Tome, uma recompensa!
  wait 1 sec
  * Give a reward (we'll spawn something simple)
  say Agora preciso achar meus sapatos... *hic*
end
~
#708
Saci Receive Pipe~
0 j 100
~
* Saci receives his pipe (if taken)
if %object.vnum% == 783
  emote pula de alegria em sua unica perna!
  say Meu cachimbo! Valeu, %actor.name%!
  wait 1 sec
  emote da uma longa baforada satisfeita.
  say Agora sim! Saci sem cachimbo nao e Saci!
  wait 1 sec
  say Se precisar de ajuda, pode contar comigo!
end
~
#709
Room Folklore Gathering~
2 g 100
~
* Room trigger when player enters an area with folklore creatures
if %actor.is_pc%
  wait 2 sec
  %echo% Voce sente uma estranha energia folclorica no ar...
  wait 2 sec
  %echo% Ecos de uma cancao antiga chegam aos seus ouvidos:
  wait 1 sec
  %echo% "Leprechaun, Leprechaun, quem foi que te fez mal?"
end
~
#710
Leprechaun Complain About Saci~
0 c 100
falar say dizer~
* Leprechaun complains when someone talks near him
if %actor.is_pc%
  wait 1 sec
  emote interrompe cambaleante.
  say *hic* Voce viu o Saci por ai? *hic*
  wait 1 sec
  say Aquele maldito nascido em Itu me enganou! *hic*
  wait 1 sec
  say Pegaram meu ouro e enterraram no quintal! *hic*
end
~
$~
