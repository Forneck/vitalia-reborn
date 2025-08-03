#1800
Saudacao e Memoria~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* mremember must be used before you can use the trigger type memory.
mremember %actor.name%
say Por aqui passam os espíritos de todos os aventureiros que morreram durante suas batalhas. Não adianta tentar voltar. A entrada é muito bem protegida, para evitar que os mortos voltem para assombrar os vivos, ou vice-versa.
wait 1 second
say Quando você morre, seu corpo é carregado até a Cabana da Ressurreição, e lá ele permanece para se reconstituir, com a ajuda da poderosa magia dos sacerdotes de Midgaard. Enquanto isso, seu espírito é trazido para cá, para que possa aguardar ewait 1 second
say Sete dias depois de morrer, seu corpo já deverá estar totalmente reconstituído, e você será ressuscitado pelos Deuses (isso equivale a 3 horas e meia na vida real.)
wait 1 second
say Também pode acontecer de outro aventureiro encontrar seu corpo e lhe ressuscitar usando magias muito poderosas, como 	rraise dead	n ou 	ressurect	n. Dessa forma, você não precisará esperar tanto tempo.
wait 1 second
say Todos os bens que você carregava continuarão no seu corpo. Mas não se preocupe, pois os Deuses protegem os corpos dos aventureiros mortos para que eles não sejam roubados.
waiy 1 second
say Por outro lado, caso seu corpo tenha sido desintegrado na luta, não será possível reconstituí-lo, e você terá que esperar pacientemente até que um clérigo muito poderoso ou um Deus ressuscite você em um novo corpo.
wait 1 second
say Agora você pode entrar para o Reino da Morte, enquanto espera pacientemente pela sua ressurreição. 
open portao
wait 1 second
say Tenha uma boa morte!!
wait 10 s
close portao
~
#1801
Saudacao Memoria~
0 o 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Fires if player is in mobs memory via mremember and the mob sees the actor.
wait 4 s
if  %actor.sex% == male
  say %actor.name% Você denovo por aqui?! Seja bem-vindo, e sinta-se em casa!
elseif %actor.sex% == female
  say %actor.name% Você denovo por aqui?! Seja bem-vinda, e sinta-se em casa!
else
  say Bom te ver, %actor.name%! Sinta-se em casa!
end
open north
~
$~
