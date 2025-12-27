#26700
Mestre do Harém - 26706~
0 b 5
~
emote olha para seus subordinados e rosna, 'voltem ao trabalho, vadias!'
~
#26701
Escravas se Curvam - 26701-26704~
0 e 0
Um gordo mestre do harém olha para seus subordinados e rosna, 'voltem ao trabalho, vadias!'~
wait 1 sec
bow mestre
~
#26702
Mestre Precisa de Ajuda - 26706~
0 k 10
~
emote grita alto, 'ME AJUDEM, MINHAS ESCRAVAS!'
~
#26703
Escravas Ajudam Mestre - 26701-26704~
0 e 0
Um gordo mestre do harém grita alto, 'ME AJUDEM, MINHAS ESCRAVAS!'~
wait %random.6% sec
assist mestre
~
#26705
Magia do Medo - 26710~
0 k 5
~
emote pronuncia as palavras, 'pabrow'.
%force% %actor% flee
~
#26706
Açougueiro - 26713~
0 b 10
~
emote começa a afiar sua faca.
~
#26707
Bebê de Piche Segue - 26708~
0 b 10
~
set actor %random.char%
if %actor.is_pc%
  mfollow %actor%
  say Ho ho, hee hee, %actor.name% você é tão engraçadooo!
  %send% %actor% %self.name% olha para você com a expressão mais fofa.
  %echoaround% %actor% %self.name% olha para %actor.name% com a expressão mais fofa.
end
~
#26708
Grande Inquisidor - 26719~
0 k 100
5~
switch %random.2%
  case 1
    emote pronuncia as palavras, 'ordalaba'.
    dg_cast 'energy drain' %actor%
  break
  case 2
    emote sorri malignamente e estala as mãos.
    dg_cast 'animate dead' corpse
    order followers assist
  break
  default
  break
done
~
#26709
Alto Sacerdote do Terror - 26714~
0 k 5
~
switch %random.2%
  case 1
    emote pronuncia as palavras, 'ordalaba'.
    dg_cast 'charm' %actor%
  break
  case 2
    emote agita $s mãos em um movimento giratório.
    dg_cast 'earthquake'
  break
  default
  break
done
~
#26710
Armadilha Quase Mortal - 26744~
2 g 100
~
* Armadilha quase mortal atordoa o ator
wait 3 sec
set stunned %actor.hitp%
%damage% %actor% %stunned%
wait 3 sec
%send% %actor% Os Deuses permitem que sua insignificante existência continue.
~
$~
