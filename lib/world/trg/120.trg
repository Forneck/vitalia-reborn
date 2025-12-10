#12000
Armadilha Mortal Leões - 12017~
2 g 100
~
* Armadilha de quase morte que atordoa o ator
wait 1 sec
eval stunned %actor.hitp% - 1
wdamage %actor.name% %stunned%
wait 5 sec
wsend %actor.name% Os leões ficam entediados quando você para de se debater e te deixam para morrer.
~
#12001
Mago - 12009, 20, 25, 30-32~
0 k 10
~
switch %actor.level%
  case 1
  case 2
  case 3
  break
  case 4
    dg_cast 'magic missile' %actor%
  break
  case 5
    dg_cast 'chill touch' %actor%
  break
  case 6
    dg_cast 'burning hands' %actor%
  break
  case 7
  case 8
    dg_cast 'shocking grasp' %actor%
  break
  case 9
  case 10
  case 11
    dg_cast 'lightning bolt' %actor%
  break
  case 12
    dg_cast 'color spray' %actor%
  break
  case 13
    dg_cast 'energy drain' %actor%
  break
  case 14
    dg_cast 'curse' %actor%
  break
  case 15
    dg_cast 'poison' %actor%
  break
  case 16
    if %actor.align% > 0
      dg_cast 'dispel good' %actor%
    else
      dg_cast 'dispel evil' %actor%
    end
  break
  case 17
  case 18
    dg_cast 'call lightning' %actor%
  break
  case 19
  case 20
  case 21
  case 22
    dg_cast 'harm' %actor%
  break
  default
    dg_cast 'fireball' %actor%
  break
done
~
#12002
Guarda da Cidade - 12018, 21~
0 b 50
~
if !%self.fighting%
  set actor %random.char%
  if %actor%
    if %actor.is_killer%
      emote grita 'HEI!!! Você é um daqueles MATADORES DE JOGADORES!!!!!!'
      kill %actor.name%
    elseif %actor.is_thief%
      emote grita 'HEI!!! Você é um daqueles LADRÕES DE JOGADORES!!!!!!'
      kill %actor.name%
    elseif %actor.cha% < 6
      %send% %actor% %self.name% cospe em seu rosto.
      %echoaround% %actor% %self.name% cospe no rosto de %actor.name%.
    end
    if %actor.fighting%
      eval victim %actor.fighting%
      if %actor.align% < %victim.align% && %victim.align% >= 0
        emote grita 'PROTEJAM OS INOCENTES! BANZAI! CARGA! ARARARAGGGHH!'
        kill %actor.name%
      end
    end
  end
end
~
#12015
Número da Zona da Sala~
2 bg 100
~
set room %room.vnum%
eval number %room.strlen%
switch %number%
  case 3
    set zone %room.charat(1)%
  break
  case 4
    set 1st %room.charat(1)%
    set 2nd %room.charat(2)%
    set zone %1st%%2nd%
  break
  case 5
    set 1st %room.charat(1)%
    set 2nd %room.charat(2)%
    set 3rd %room.charat(3)%
    set zone %1st%%2nd%%3rd%
  break
done
%echo% Sala #%room.vnum% faz parte da zona: %zone%
~
$~
