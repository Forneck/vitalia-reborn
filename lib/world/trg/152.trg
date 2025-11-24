#15200
lamento_da_alma_ambiente~
0 b 100
~
if %self.fighting%
  halt
end
set i %random.13%
switch %i%
  case 0
    say 	cOnde... onde estou...? 	n
    halt
  case 1
    emote 	Wchora silenciosamente, finas lágrimas de ectoplasma escorrendo por um rosto que já não existe.	n
    halt
  case 2
    say 	cTanto frio...	n
    halt
  case 3
    emote 	Wtreme violentamente, sua forma espectral quase se desfazendo antes de se solidificar novamente.	n
    halt
  case 4
    say 	cEu tinha um nome... eu acho...	n
    halt
  case 5
    emote 	Westende uma mão translúcida na sua direção, como se buscasse calor, apenas para recuar com um gemido silencioso.	n
    halt
  case 6
    say 	cA floresta... ela não me deixa sair...	n
    halt
  case 7
    emote 	Wolha através de você, seus buracos vazios focados numa memória distante e dolorosa.		n
    halt
  case 8
    say 	cA dor... nunca para...	n
    halt
  case 9
    emote 	Wflutua em círculos lentos, como se procurasse algo que nunca mais encontrará.	n
    halt
  case 10
    say 	cPor favor... não me esqueça...	n
    halt
  case 11
    emote 	Wparece desvanecer-se por um instante, tornando-se quase invisível antes de lentamente reaparecer.	n
    halt
  case 12
    emote 	Wtoca o tronco de uma árvore próxima, sua mão fantasmagórica atravessando a casca como fumo.		n
    halt
~
#15201
saudacao_da_alma~
0 g 100
~
* Trigger de Saudação (g) para a Alma Penada.
if %self.fighting%
  halt
end
emote 	Yolha para %actor.name%, um frio sobrenatural preenchendo o ar.	n
~
#15202
lamento_da_alma_morte~
0 f 100
~
* Trigger de Morte (f) para a Alma Penada.
emote 	Wsolta um último suspiro que soa menos a dor e mais a alívio.	n
say 	cFinalmente... paz...	n
emote 	Wdissolve-se numa poeira de luz pálida.	n
~
#15203
sussurros_da_floresta~
2 ab 100
~
* Trigger dos sussuros. Dispara em toda zona
set i %random.20%
switch %i%
  case 0
    %echo% 	gAs árvores gemem em uníssono, um lamento longo e arrastado que ecoa pela floresta...	n
    halt
  case 1
    %echo%  	cUm sussurro frio percorre a zona, dizendo numa língua esquecida: '	wpercam-se...	c'	n
    halt
  case 2
    %echo%  	YUm grito agudo e distante ecoa por entre as árvores, terminando num soluço súbito.	n
    halt
  case 3
    %echo%  	GO chão treme ligeiramente, e um rosnar baixo parece subir das próprias raízes sob os seus pés.	n
    halt
  case 4
    %echo%  	cPor um breve instante, todos os sons da floresta cessam, criando um silêncio pesado e antinatural.		n
    halt
~
#15204
kodama_ambiente~
0 b 100
~
* Trigger aleatório para o Kodama Desnaturado
if %self.fighting%
  halt
end
set i %random.12%
switch %i%
  case 0
    emote 	ginclina lentamente a sua cabeça de madeira, produzindo um som oco de estalar de galhos.	n
    halt
  case 1
    say 	gA floresta... precisa... de mais... dor...	n
    halt
  case 2
    emote 	golha para o seu próprio braço retorcido, e um som que se assemelha a um soluço de dor ecoa da sua forma de madeira.		n
    halt
  case 3
    say 	gNinguém... vai... parar... o florescer...	n
    halt
~
#15205
kodama_combate~
0 k 100
~
* Trigger de Combate (k) para o Kodama Desnaturado
switch %random.4%
  case 0
    say 	R'Junte-se a nós no apodrecimento!'	n
    halt
  case 1
    emote 	Yruge, e a seiva negra do seu braço-arma ferve e sibila.	n
    halt
  case 2
    say 	R'A dor... é purificadora!'	n
    halt
~
#15206
kappa_combate~
0 k 100
~
* Trigger de Combate (k) para o Kappa do Lodo
set i %random.10%
switch %i%
  case 0
    emote 	gagarra-se às suas pernas, tentando desequilibrá-lo e arrastá-lo para a água!	n
    halt
  case 1
    say 	c'Meu prato! Não derrame a água do meu prato!'		n
    halt
  case 2
    emote 	gmergulha na água escura, desaparecendo por um instante antes de ressurgir atrás de você!	n
    halt
~
#15207
eremita_transformacao~
0 k 100
~
* Trigger de Combate (k) que dispara uma vez para transformar o Eremita.
emote 	Yao ser atingido, o corpo do eremita frágil dissolve-se numa poça de lodo borbulhante!	n
say 	R'GRRRRAAAAAAAARRRGGGHHH!'	n
%echo% 	YDo chão, uma monstruosidade de lama e ossos ergue-se, virando os seus olhos brilhantes para %actor.name%!	n
%load% mob 15210
~
#15208
furia_do_pantano_nascimento~
0 n 100
~
* Anúncio global para toda a zona, o prenúncio da desgraça.
%zoneecho% %self.vnum% 	RDas profundezas do pântano, um rugido silencioso ecoa, não nos ouvidos, mas na alma de cada ser vivo. A Fúria despertou.	n
%load% obj 15222 %self% neck1
%load% obj 15239 %self% waist
wait 2s
emote 	gabre a sua bocarra de lodo e galhos e solta um rugido que não é som, mas pura força sísmica. O chão sob os seus pés racha e ondula violentamente!	n
dg_cast 'earthquake'
wait 2s
set vict %random.char%
if %vict% && !isnpc(%vict%)
  %force% %self% kill %vict.name%
end
~
#15209
Veneno aranha~
0 k 10
~
%send% %actor% %self.name% te mordeu!
%echoaround% %actor% %self.name% morde %actor.name%.
dg_cast 'poison' %actor%
~
#15210
sussurro_da_camelia~
2 c 4
tou~
if %cmd.mudcommand% /= touch && flor /= %arg% || camelia /= %arg%
%send% %actor% Quando a sua mão mortal ousa tocar as pétalas da camélia, um 	wfulgor prateado	n, puro como uma lágrima de estrela, emana da flor. A muralha de espinhos, negra como a traição de um rei, 	gretorce-se e geme	n, não de dor, mas de uma vontade antiga a ser quebrada. Galhos grossos como braços de guerreiros recuam, revelando um caminho de 	mluz serena	n para o leste onde antes só havia desespero.
%echoaround% %actor% Ao toque de %actor.name%, a camélia solitária irrompe em 	wluz prateada	n. Com um protesto rangente, como mil escudos a serem partidos, a 	gmuralha de espinhos	n abre-se, revelando um caminho secreto para o leste.
%door% 15214 east flags a
%door% 15214 east room 15248
%door% 15214 east name passagem muralha espinhos
%door% 15214 east description O Grande Portão de Shizuka brilha com uma luz suave para além dos espinhos.
wait 10s
%door% 15214 east purge
%echo% O caminho de luz desvanece-se. Com o som de 	grochas a ranger e uma fome antiga	n, a muralha de espinhos volta a selar a passagem, não deixando para trás nem mesmo a memória da sua abertura.
elseif %cmd.mudcommand% /= touch && camelia /= %arg% || flor /= %arg%
%send% %actor% Quando a sua mão mortal ousa tocar as pétalas da camélia, um 	wfulgor prateado	n, puro como uma lágrima de estrela, emana da flor. A muralha de espinhos, negra como a traição de um rei, 	gretorce-se e geme	n, não de dor, mas de uma vontade antiga a ser quebrada. Galhos grossos como braços de guerreiros recuam, revelando um caminho de 	mluz serena	n para o leste onde antes só havia desespero.
%echoaround% %actor% Ao toque de %actor.name%, a camélia solitária irrompe em 	wluz prateada	n. Com um protesto rangente, como mil escudos a serem partidos, a 	gmuralha de espinhos	n abre-se, revelando um caminho secreto para o leste.
%door% 15214 east flags a
%door% 15214 east room 15248
%door% 15214 east name passagem muralha espinhos
%door% 15214 east description O Grande Portão de Shizuka brilha com uma luz suave para além dos espinhos.
wait 10s
%door% 15214 east purge
%echo% O caminho de luz desvanece-se. Com o som de 	grochas a ranger e uma fome antiga	n, a muralha de espinhos volta a selar a passagem, não deixando para trás nem mesmo a memória da sua abertura.
else
Return 0
end
~
#15211
orbe_eterno_drop~
1 h 100
~
%send% %actor% O orbe de musgo reage com 	rfúria 	nquando você tenta se separar dele. Filamentos de uma substância viscosa o seguram, filamentos que pulsam com uma vontade própria e aterradora. A orbe irrompe com uma 	wluz anêmica e pálida	n, e seus sussurros tornam-se gritos agônicos numa língua morta. Você sente a presença gélida da floresta concentrando-se no objeto, seus fios negros ondulando como serpentes enfurecidas, recusando qualquer separação. 	cNão... nunca... somente você... 	n.
%echoaround% %actor% %actor.name% tenta desesperadamente se separar do orbe, mas ele retorna sempre para suas mãos. %actor.name% está condenado a carregá-lo. 
return 0
~
#15212
presente_indesejado~
2 bg 100
~
if %actor.is_pc% && %actor.class% == Ranger && !%actor.has_item(15259)%
  %send% %actor% 	cDo canto do olho, você vê algo escuro a cair de um galho acima de si. Antes que possa reagir, a coisa aterra suavemente no bolso da sua mochila com um som úmido e discreto.	n
  %load% obj 15259 %actor%
else
  if !%actor.is_pc%
  end
  if %actor.class% != Ranger
  end
  if %actor.has_item(15259)%
  end
end
halt
~
#15213
orbe_eterno_give~
1 i 100
~
set victim_name %actor.name%
if %victim%
  %send% %actor% 	yVocê estende a mão para entregar o orbe a %victim.name%, mas no instante em que os dedos tocam o musgo, ele se dissolve em pó. Um novo orbe, frio e úmido, materializa-se na sua palma vazia. Você não pode partilhar o seu fardo.	n
  %send% %victim% %actor.name% tenta dar-lhe um orbe de musgo, mas ele se desfaz em pó antes que você possa pegá-lo.
return 0
~
#15214
orbe_eterno_junk~
1 c 2
j~
if %cmd.mudcommand% /= junk && orbe /= %arg%
%send% %actor% Num acesso de fúria, você tenta esmagar o orbe, mas ele cede sob a sua força como 	rcarne podre	n. Uma dor aguda floresce no seu peito, e você sente algo a contorcer-se atrás dos seus olhos. O orbe reforma-se na sua mão, e os sussurros agora falam com a sua própria voz: 	y'Nós somos um. Tentar ferir-me é ferir a ti mesmo.'	n
%echoaround% %actor% %actor.name% grita de dor ao tentar destruir o orbe, que pulsa com uma 	rluz doentia	n e permanece intacto.
elseif %cmd.mudcommand% /= junk && musgo /= %arg%
%send% %actor% Num acesso de fúria, você tenta esmagar o orbe, mas ele cede sob a sua força como 	rcarne podre	n. Uma dor aguda floresce no seu peito, e você sente algo a contorcer-se atrás dos seus olhos. O orbe reforma-se na sua mão, e os sussurros agora falam com a sua própria voz: 	y'Nós somos um. Tentar ferir-me é ferir a ti mesmo.'	n
%echoaround% %actor% %actor.name% grita de dor ao tentar destruir o orbe, que pulsa com uma 	rluz doentia	n e permanece intacto.
elseif %cmd.mudcommand% /= junk && all /= %arg%
%send% %actor% Num acesso de fúria, você tenta esmagar o orbe, mas ele cede sob a sua força como 	rcarne podre	n. Uma dor aguda floresce no seu peito, e você sente algo a contorcer-se atrás dos seus olhos. O orbe reforma-se na sua mão, e os sussurros agora falam com a sua própria voz: 	y'Nós somos um. Tentar ferir-me é ferir a ti mesmo.'	n
%echoaround% %actor% %actor.name% grita de dor ao tentar destruir o orbe, que pulsa com uma 	rluz doentia	n e permanece intacto.
else
return 0
end
~
#15215
orbe_eterno_put~
1 c 2
p~
if %cmd.mudcommand% /= put && orbe /= %arg%
%send% %actor% Você tenta selar o orbe num recipiente, mas no momento em que ele é envolvido pela escuridão, um 	wterror claustrofóbico	n inunda a sua alma. Finos 	gtentáculos de musgo	n emergem do objeto, agarrando-se à sua mão, puxando-o de volta para a luz do seu inventário. Ele não suporta ser aprisionado. Ele precisa de você.	n
%echoaround% %actor% %actor.name% tenta guardar o orbe, mas o objeto parece lutar e saltar de volta para as suas mãos por vontade própria.
elseif %cmd.mudcommand% /= put && musgo /= %arg%
%send% %actor% Você tenta selar o orbe num recipiente, mas no momento em que ele é envolvido pela escuridão, um 	wterror claustrofóbico	n inunda a sua alma. Finos 	gtentáculos de musgo	n emergem do objeto, agarrando-se à sua mão, puxando-o de volta para a luz do seu inventário. Ele não suporta ser aprisionado. Ele precisa de você.	n
%echoaround% %actor% %actor.name% tenta guardar o orbe, mas o objeto parece lutar e saltar de volta para as suas mãos por vontade própria.
elseif %cmd.mudcommand% /= put && all /= %arg%
%send% %actor% Você tenta selar o orbe num recipiente, mas no momento em que ele é envolvido pela escuridão, um 	wterror claustrofóbico	n inunda a sua alma. Finos 	gtentáculos de musgo	n emergem do objeto, agarrando-se à sua mão, puxando-o de volta para a luz do seu inventário. Ele não suporta ser aprisionado. Ele precisa de você.	n
%echoaround% %actor% %actor.name% tenta guardar o orbe, mas o objeto parece lutar e saltar de volta para as suas mãos por vontade própria.
else
return 0
end
~
#15216
orbe_eterno_donate~
1 c 2
don~
if %cmd.mudcommand% /= donate && orbe /= %arg%
%send% %actor% Com uma prece silenciosa, você oferece o orbe ao abismo das doações. Ele cai... mas o silêncio que se segue é quebrado por um 	cvento gelado	n que sobe do fosso. O orbe flutua de volta, parando diante do seu rosto. Os sussurros são claros pela primeira vez: 	y'Tolos não podem doar o que já foi reivindicado. E nós reivindicamos-te.'	n
%echoaround% %actor% %actor.name% atira o orbe para doação, mas um 	cvento misterioso	n o devolve diretamente para a sua mão.
elseif %cmd.mudcommand% /= donate && musgo /= %arg%
%send% %actor% Com uma prece silenciosa, você oferece o orbe ao abismo das doações. Ele cai... mas o silêncio que se segue é quebrado por um 	cvento gelado	n que sobe do fosso. O orbe flutua de volta, parando diante do seu rosto. Os sussurros são claros pela primeira vez: 	y'Tolos não podem doar o que já foi reivindicado. E nós reivindicamos-te.'	n
%echoaround% %actor% %actor.name% atira o orbe para doação, mas um 	cvento misterioso	n o devolve diretamente para a sua mão.
elseif %cmd.mudcommand% /= donate && all /= %arg%
%send% %actor% Com uma prece silenciosa, você oferece o orbe ao abismo das doações. Ele cai... mas o silêncio que se segue é quebrado por um 	cvento gelado	n que sobe do fosso. O orbe flutua de volta, parando diante do seu rosto. Os sussurros são claros pela primeira vez: 	y'Tolos não podem doar o que já foi reivindicado. E nós reivindicamos-te.'	n
%echoaround% %actor% %actor.name% atira o orbe para doação, mas um 	cvento misterioso	n o devolve diretamente para a sua mão.
else
return 0
end
~
#15217
purificacao_da_lamina~
1 l 0
~
%ECHO% [DEBUG 15217] eval vnum %self.vnum()%
if %self.vnum()% != 15208
return 1
elseif %self.room.vnum% == 15248
  wait 1s
  %echoaround% %actor% 	wUm zumbido baixo e harmonioso emana do Grande Portão de Shizuka, a sua luz prateada focando-se com uma intensidade súbita na lâmina negra empunhada por %actor.name%.	n
  %send% %actor% 	yA sua 	rKatana Muramasa	y grita na sua mão, não um som que seus ouvidos possam captar, mas um uivo de agonia psíquica que reverbera na sua alma. A lâmina vibra violentamente, expelindo uma 	Cfumaça negra e fétida	y, como se a própria maldição estivesse a sangrar para fora do aço amaldiçoado.	n
  wait 3s
  %echoaround% %actor% 	wO zumbido do portão transforma-se num cântico, e uma cascata de 	cluz prateada e líquida	w desce sobre a lâmina de %actor.name%, silenciando o seu grito com um chiado de vapor sagrado.	n
  %send% %actor% 	wA luz fria e pura inunda você, não para queimar, mas para limpar. Você sente o peso de mil almas torturadas a ser erguido dos seus ombros, e o sussurro sedento de sangue na sua mente finalmente se cala, substituído por um silêncio pacífico e profundo.	n
  
  * Carrega nova Masamune no inventário do jogador
  %transform% 15260
  wait 1s
  %send% %actor% 	cA provação terminou. A maldição foi desfeita. Onde antes havia um demonio de aço, agora repousa em seu inventário a 	wMasamune Sagrada	c, a sua lâmina a brilhar com uma promessa de justiça.	n
  
  return 0
else
  return 1
else
return 1
end
~
#15218
guardiao_do_portao_shizuka~
2 q 100
~
if %direction% == north
  if %actor.is_pc% && %actor.eq(wield)%
    %send% %actor% 	wQuando você tenta avançar para além do portão, uma barreira de luz prateada ondula no ar à sua frente. A arma em sua mão parece pesar uma tonelada, uma profanação que o impede de prosseguir. A voz sem som ecoa em sua mente: 	y'Somente os puros de coração e de mãos podem entrar. Remova seu aço.'	n
    %echoaround% %actor% Uma barreira de luz prateada bloqueia a passagem de %actor.name%, que recua do caminho ao norte.
    return 0
end
else
* Permite o movimento em todos os outros casos (outras direções, desarmado, etc.).
return 1
end
~
#15219
trilha_confusa_labirinto~
2 q 90
~
if %actor.is_pc%
  %send% %actor% 	yVocê tenta seguir a trilha, mas as 	gárvores retorcidas	y parecem zombar da sua noção de direção. Os seus pés movem-se, mas o cenário de pesadelo não muda. 	rApós alguns passos, você se encontra exatamente onde começou.	n
  %echoaround% %actor% 	g%actor.name%	y caminha em círculos, parecendo completamente perdido, e retorna ao centro da sala com uma expressão de frustração.	n
  %force% %actor% look
  return 0
else
return 1
end
~
#15220
sussurro_da_camelia_retorno~
2 c 4
tou~
if %cmd.mudcommand% /= touch && flor /= %arg% || camelia /= %arg%
%send% %actor% Quando a sua mão mortal ousa tocar as pétalas da camélia, um 	wfulgor prateado	n, puro como uma lágrima de estrela, emana da flor. A muralha de espinhos, negra como a traição de um rei, 	gretorce-se e geme	n, não de dor, mas de uma vontade antiga a ser quebrada. Galhos grossos como braços de guerreiros recuam, revelando o caminho de volta para os horrores da floresta.
%echoaround% %actor% Ao toque de %actor.name%, a camélia solitária irrompe em 	wluz prateada	n. Com um protesto rangente, como mil escudos a serem partidos, a 	gmuralha de espinhos	n abre-se, revelando um caminho secreto para o oeste.
%door% 15248 west flags a
%door% 15248 west room 15214
%door% 15248 west name passagem muralha espinhos
%door% 15248 west description A floresta sussura ao oeste.
wait 10s
%door% 15248 west purge
%echo% O caminho desvanece-se. Com o som de 	grochas a ranger e uma fome antiga	n, a muralha de espinhos volta a selar a passagem, não deixando para trás nem mesmo a memória da sua abertura.
elseif %cmd.mudcommand% /= touch && camelia /= %arg% || flor /= %arg%
%send% %actor% Quando a sua mão mortal ousa tocar as pétalas da camélia, um 	wfulgor prateado	n, puro como uma lágrima de estrela, emana da flor. A muralha de espinhos, negra como a traição de um rei, 	gretorce-se e geme	n, não de dor, mas de uma vontade antiga a ser quebrada. Galhos grossos como braços de guerreiros recuam, revelando o caminho de volta para os horrores da floresta.
%echoaround% %actor% Ao toque de %actor.name%, a camélia solitária irrompe em 	wluz prateada	n. Com um protesto rangente, como mil escudos a serem partidos, a 	gmuralha de espinhos	n abre-se, revelando um caminho secreto para o oeste.
%door% 15248 west flags a
%door% 15248 west room 15214
%door% 15248 west name passagem muralha espinhos
%door% 15248 west description A floresta sussura ao oeste.
wait 10s
%door% 15248 west purge
%echo% O caminho desvanece-se. Com o som de 	grochas a ranger e uma fome antiga	n, a muralha de espinhos volta a selar a passagem, não deixando para trás nem mesmo a memória da sua abertura.
else
Return 0
end
~
#15221
Lojista Pocoes - Aleatorios~
0 n 100
~
* Criar array com todos os 85 vnums
eval item1 205
eval item2 302
eval item3 304
eval item4 305
eval item5 902
eval item6 1606
eval item7 1623
eval item8 1703
eval item9 1704
eval item10 1722
eval item11 2314
eval item12 2315
eval item13 2316
eval item14 2557
eval item15 2558
eval item16 2559
eval item17 2560
eval item18 2561
eval item19 2932
eval item20 2933
eval item21 2934
eval item22 2935
eval item23 2936
eval item24 2937
eval item25 3049
eval item26 3051
eval item27 3306
eval item28 4050
eval item29 4223
eval item30 4225
eval item31 4343
eval item32 4829
eval item33 5019
eval item34 5210
eval item35 5211
eval item36 5305
eval item37 5306
eval item38 5321
eval item39 5470
eval item40 5471
eval item41 5472
eval item42 5473
eval item43 5474
eval item44 5501
eval item45 5927
eval item46 6109
eval item47 6110
eval item48 6305
eval item49 6405
eval item50 6413
eval item51 6414
eval item52 7204
eval item53 8150
eval item54 8151
eval item55 9704
eval item56 9909
eval item57 11328
eval item58 11345
eval item59 11511
eval item60 12030
eval item61 13438
eval item62 13520
eval item63 13521
eval item64 13522
eval item65 13523
eval item66 13524
eval item67 13525
eval item68 13526
eval item69 14033
eval item70 15012
eval item71 15286
eval item72 17025
eval item73 17031
eval item74 17307
eval item75 17621
eval item76 19009
eval item77 19011
eval item78 19704
eval item79 19705
eval item80 19713
eval item81 19717
eval item82 28010
eval item83 28011
eval item84 29016
eval item85 29017
* Carregar 10 itens aleatórios
eval random1 %random.85%
eval vnum1 %%item%random1%%%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
eval random2 %random.85%
eval vnum2 %%item%random2%%%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
eval random3 %random.85%
eval vnum3 %%item%random3%%%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
eval random4 %random.85%
eval vnum4 %%item%random4%%%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
eval random5 %random.85%
eval vnum5 %%item%random5%%%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
eval random6 %random.85%
eval vnum6 %%item%random6%%%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
eval random7 %random.85%
eval vnum7 %%item%random7%%%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
eval random8 %random.85%
eval vnum8 %%item%random8%%%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
eval random9 %random.85%
eval vnum9 %%item%random9%%%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
eval random10 %random.85%
eval vnum10 %%item%random10%%%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
~
#15222
Lojista Pergaminhos - Aleatorios~
0 n 100
~
* Criar array com todos os 52 pergaminhos
eval perg1 206
eval perg2 720
eval perg3 721
eval perg4 722
eval perg5 1204
eval perg6 2206
eval perg7 2317
eval perg8 2320
eval perg9 2322
eval perg10 2550
eval perg11 2551
eval perg12 2553
eval perg13 2554
eval perg14 3050
eval perg15 3052
eval perg16 3056
eval perg17 3057
eval perg18 3118
eval perg19 4102
eval perg20 4901
eval perg21 5022
eval perg22 5215
eval perg23 5216
eval perg24 5315
eval perg25 5478
eval perg26 5906
eval perg27 8147
eval perg28 8148
eval perg29 8149
eval perg30 8152
eval perg31 10221
eval perg32 10733
eval perg33 10734
eval perg34 10735
eval perg35 10736
eval perg36 11331
eval perg37 11332
eval perg38 11333
eval perg39 11334
eval perg40 12024
eval perg41 12025
eval perg42 12026
eval perg43 14021
eval perg44 14027
eval perg45 14041
eval perg46 15292
eval perg47 15297
eval perg48 16009
eval perg49 17023
eval perg50 17327
eval perg51 24400
eval perg52 29008
* ===============================================
* Carregar 10 seleções aleatórias
* Cada seleção terá 05 unidades do mesmo item
* ===============================================
* --- Seleção 1 ---
eval random1 %random.52%
eval vnum1 %%perg%random1%%%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
%load% obj %vnum1%
* --- Seleção 2 ---
eval random2 %random.52%
eval vnum2 %%perg%random2%%%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
%load% obj %vnum2%
* --- Seleção 3 ---
eval random3 %random.52%
eval vnum3 %%perg%random3%%%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
%load% obj %vnum3%
* --- Seleção 4 ---
eval random4 %random.52%
eval vnum4 %%perg%random4%%%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
%load% obj %vnum4%
* --- Seleção 5 ---
eval random5 %random.52%
eval vnum5 %%perg%random5%%%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
%load% obj %vnum5%
* --- Seleção 6 ---
eval random6 %random.52%
eval vnum6 %%perg%random6%%%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
%load% obj %vnum6%
* --- Seleção 7 ---
eval random7 %random.52%
eval vnum7 %%perg%random7%%%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
%load% obj %vnum7%
* --- Seleção 8 ---
eval random8 %random.52%
eval vnum8 %%perg%random8%%%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
%load% obj %vnum8%
* --- Seleção 9 ---
eval random9 %random.52%
eval vnum9 %%perg%random9%%%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
%load% obj %vnum9%
* --- Seleção 10 ---
%echo% DEBUG: ------ Iniciando Seleção 10 ------
eval random10 %random.52%
eval vnum10 %%perg%random10%%%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
%load% obj %vnum10%
~
#15223
Lojista Entalhador de Sonhos - Aleatorios~
0 n 100
~
* Criar array com todos os 100 vnums
eval item1 1000
eval item2 1001
eval item3 1002
eval item4 1003
eval item5 1004
eval item6 1005
eval item7 1006
eval item8 1007
eval item9 1008
eval item10 1009
eval item11 1010
eval item12 1011
eval item13 1012
eval item14 1013
eval item15 1014
eval item16 1015
eval item17 1016
eval item18 1017
eval item19 1018
eval item20 1019
eval item21 1020
eval item22 1021
eval item23 1022
eval item24 1023
eval item25 1024
eval item26 1025
eval item27 1026
eval item28 1027
eval item29 1028
eval item30 1030
eval item31 1031
eval item32 1032
eval item33 1033
eval item34 1034
eval item35 1035
eval item36 1036
eval item37 1037
eval item38 1038
eval item39 1039
* Carregar 5 itens aleatórios
eval random1 %random.39%
eval vnum1 %%item%random1%%%
%load% obj %vnum1%
eval random2 %random.39%
eval vnum2 %%item%random2%%%
%load% obj %vnum2%
eval random3 %random.39%
eval vnum3 %%item%random3%%%
%load% obj %vnum3%
eval random4 %random.39%
eval vnum4 %%item%random4%%%
%load% obj %vnum4%
eval random5 %random.39%
eval vnum5 %%item%random5%%%
%load% obj %vnum5%
~
#15224
Lojista Ferreiro - Aleatórios~
0 n 100
~
* Criar array com todos os 100 vnums
eval item1 12011
eval item2 2735
eval item3 12011
eval item4 2735
eval item5 11309
eval item6 12610
eval item7 12618
eval item8 7190
eval item9 9981
eval item10 11309
eval item11 12610
eval item12 12618
eval item13 13419
eval item14 14543
eval item15 19003
eval item16 19023
eval item17 19040
eval item18 7190
eval item19 9981
eval item20 11318
eval item21 12601
eval item22 2574
eval item23 8407
eval item24 11318
eval item25 12601
eval item26 14418
eval item27 14423
eval item28 20115
eval item29 2574
eval item30 2909
eval item31 808
eval item32 8407
eval item33 10004
eval item34 10007
eval item35 10211
eval item36 10835
eval item37 10849
eval item38 10854
eval item39 11110
eval item40 11301
eval item41 11314
eval item42 11320
eval item43 11369
eval item44 11519
eval item45 12008
eval item46 12611
eval item47 12626
eval item48 12629
eval item49 12639
eval item50 12652
eval item51 12663
eval item52 12672
eval item53 12685
eval item54 12753
eval item55 1725
eval item56 1726
eval item57 2742
eval item58 2766
eval item59 2767
eval item60 2767
eval item61 3043
eval item62 3044
eval item63 3045
eval item64 3071
eval item65 3076
eval item66 3081
eval item67 3086
eval item68 3318
eval item69 3414
eval item70 3503
eval item71 4511
eval item72 4909
eval item73 5423
eval item74 5424
eval item75 5425
eval item76 5426
eval item77 5427
eval item78 5428
eval item79 5429
eval item80 5430
eval item81 5431
eval item82 5432
eval item83 5433
eval item84 5916
eval item85 5918
eval item86 5921
eval item87 6002
eval item88 6119
eval item89 6511
eval item90 7514
eval item91 7517
eval item92 7528
eval item93 7529
eval item94 7547
eval item95 7914
eval item96 7916
eval item97 7917
eval item98 7920
eval item99 8158
eval item100 8159
* Debug: Array criado
* Carregar 5 itens aleatórios
eval random1 %random.100%
eval vnum1 %%item%random1%%%
%load% obj %vnum1%
eval random2 %random.100%
eval vnum2 %%item%random2%%%
%load% obj %vnum2%
eval random3 %random.100%
eval vnum3 %%item%random3%%%
%load% obj %vnum3%
eval random4 %random.100%
eval vnum4 %%item%random4%%%
%load% obj %vnum4%
eval random5 %random.100%
eval vnum5 %%item%random5%%%
%load% obj %vnum5%
~
#15228
Orbe Sacrifice~
1 c 2
sac~
if %cmd.mudcommand% /= sac && orbe /= %arg%
%send% %actor% Voce sacrifica um orbe de musgo sussurrante para os deuses.
%send% %actor% Voce recebeu 1 pontos de experiencia.
%echoaround% %actor% %actor.name% tenta sacrificar o orbe, o objeto parece aceitar o desafio.
wait 5s
%send% %actor% Os deuses ficaram 	genojados 	ne devolvem o orbe para voce.
%echoaround% %actor% %actor.name% Os deuses devolvem o orbe para %actor.name%.
elseif %cmd.mudcommand% /= sac && musgo /= %arg%
%send% %actor% Voce sacrifica um orbe de musgo sussurrante para os deuses.
%send% %actor% Voce recebeu 1 pontos de experiencia.
%echoaround% %actor% %actor.name% tenta sacrificar o orbe, o objeto parece aceitar o desafio.
wait 5s
%send% %actor% Os deuses ficaram 	genojados 	ne devolvem o orbe para voce.
%echoaround% %actor% %actor.name% Os deuses devolvem o orbe para %actor.name%.
elseif %cmd.mudcommand% /= sac && all /= %arg%
%send% %actor% Voce sacrifica um orbe de musgo sussurrante para os deuses.
%send% %actor% Voce recebeu 1 pontos de experiencia.
%echoaround% %actor% %actor.name% tenta sacrificar o orbe, o objeto parece aceitar o desafio.
wait 5s
%send% %actor% Os deuses ficaram 	genojados 	ne devolvem o orbe para voce.
%echoaround% %actor% %actor.name% Os deuses devolvem o orbe para %actor.name%.
else
return 0
end
~
$~
