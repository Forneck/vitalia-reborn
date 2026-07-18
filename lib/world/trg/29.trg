#2900
Gatekeeper Welcome - 2997~
0 g 33
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote fala em um tom entediado, 'Bem-vindo(a) a Porto, forasteiro.  Aoroveite a estadia.'
~
#2901
Mad Prisoner - 2978~
0 c 100
listen~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Diamo... Diamo, Eu não alcanço a pilha Diamo!  Ajuda, por favor!
%force% %actor% look masmorra
~
#2902
Scream - 2975~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote grita, 'Saia daqui AGORA!'
~
#2903
Shop A - 2973~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Como eu posso ajudar?  Hm... Você parece familiar.  Acho que já ouvi falar de você.  Por acaso você é %actor.name%?
~
#2904
Shop B - 2973~
0 d 100
sim~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Sério?
peer %actor.name%
say Talvez... Poderia ser... Parece como o meu amigo descreveu.
say De qualquer forma... Pediram para te dar isto.
give jarro  %actor.name%
say Ela comprou de Gilles, no bar. Aproveite.
wink %actor.name%
~
#2905
Tia A - 2956~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Um pouco mais de...  Oh! Um cliente!  Como posso eu ajudar vossa senhoria hoje?
~
#2906
Mithroq A - 2955~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
emote resmunga, 'O que você quer, forasteiro?'
~
#2908
Gilles A - 2971~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say Você gostaria de uma bebida? São boas para matar a sede.
~
#2909
Milo A - 2957~
0 g 25
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
say 200789... 200790... 200791...
emote tem um olhar sonhador enquanto olha para a pilha de moedas de ouro.
emote acorda subitamente, finalmente notando que tem alguem aqui.
say Bem, se não é  %actor.name%. A sua reputação já chegou até aqui.  Bem-vindo(a) ao meu humilde banco.
bow %actor.name%
~
#2910
Yelling Woman - Not Attached~
0 gn 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
load obj 2952
give gritando bandeja
load obj 2951
give gritando jarro
load 2912
give gritando colher
~
#2911
Near Death Trap on the Rocks - 2975~
2 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 29 on The Builder Academy, so you
* should be looking for 29xx, where xx is 00-99.
* Near Death Trap stuns actor
wait 6 sec
set stunned %actor.hitp%
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% Você escorrega perto de pedras pontudas.
~
$~
