#3000
Mage Guildguard - 3024~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == south
  * Stop them if they are not the appropriate class.
  if %actor.class% != Mago
    return 0
    %send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3001
Cleric Guildguard - 3025~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == north
  * Stop them if they are not the appropriate class.
  if %actor.class% != Clerigo
    return 0
  %send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3002
Thief Guildguard - 3026~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == east
  * Stop them if they are not the appropriate class.
  if %actor.class% != Ladrao
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3003
Warrior Guildguard - 3027~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == east
  * Stop them if they are not the appropriate class.
  if %actor.class% != Guerreiro
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3004
Bard Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == east
  * Stop them if they are not the appropriate class.
  if %actor.class% != Bardo
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3005
Druid Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop them if they are not the appropriate class.
  if %actor.class% != Druida
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3006
Ranger Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop them if they are not the appropriate class.
  if %actor.class% != Ranger
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#3007
Ladrão Stock~
0 b 10
~
set actor %random.char%
if %actor%
  if %actor.is_pc% && %actor.gold%
    %send% %actor% Você descobre que %self.name% está com as suas mãos em seu bolso.
    %echoaround% %actor% %self.name% tebta roubar dinheiro de %actor.name%.
    eval coins %actor.gold% * %random.10% / 100
    nop %actor.gold(-%coins%)%
    nop %self.gold(%coins%)%
  end
end
~
#3008
Cobra Stock~
0 k 10
~
%send% %actor% %self.name% te mordeu!
%echoaround% %actor% %self.name% morde %actor.name%.
dg_cast 'poison' %actor%
~
#3009
Magic User Tipo 1~
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
#3010
Near Death Trap~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Near Death Trap stuns actor
set stunned %actor.hitp%
%damage% %actor% %stunned%
%send% %actor% You are on the brink of life and death.
%send% %actor% The Gods must favor you this day.
~
#3011
Newbie Tour Guide Loader~
0 e 0
entrou no jogo.~
* By Rumble of The Builder Academy    tbamud.com 9091
* Num Arg 0 means the argument has to match exactly. So trig will only fire off:
* "has entered game." and not "has" or "entered" etc. (that would be num arg 1).
* Figure out what vnum the mob is in so we can use zoneecho.
eval inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% grita, 'Ola!'
~
#3012
Kind Soul Gives Newbie Equipment~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* If a player is < level 10 and naked it fully equips them. If < 10 and missing
* some equipment it will equip one spot.
if %actor.is_pc% && %actor.level% < 10
  wait 2 sec
  if !%actor.eq(*)%
    say Ei, pegue algumas roupas! Aqui, deixa eu te ajudar.
    %load% obj 3037 %actor% light
    %load% obj 3040 %actor% body
    %load% obj 3076 %actor% head
    %load% obj 3081 %actor% legs
    %load% obj 3071 %actor% hands
    %load% obj 3086 %actor% arms
    %load% obj 3042 %actor% shield
    %load% obj 3021 %actor% wield
    %load% obj 3055 %actor% hold
    halt
  end
  if !%actor.eq(light)%
    say Voce nao devia andar aqui sem uma luz, %actor.name%.
    shake
    %load% obj 3037
    give vela %actor.name%
    halt
  end
  if !%actor.eq(body)%
    say Voce nao vai longe sem armadura %actor.name%.
    %load% obj 3040
    give peitoral %actor.name%
    halt
  end
  if !%actor.eq(head)%
    say Proteja essas tuas orelhas, %actor.name%.
    %load% obj 3076
    give gorro %actor.name%
    halt
  end
  if !%actor.eq(legs)%
    say Por que voce sempre perde as suas calcas, %actor.name%?
    %load% obj 3081
    give calca %actor.name%
    halt
  end
  if !%actor.eq(hands)%
    say Precisa de luvas, %actor.name%?
    %load% obj 3071
    give luvas %actor.name%
    halt
  end
  if !%actor.eq(arms)%
    say Voce deve estar com frio, %actor.name%.
    %load% obj 3086
    give braca %actor.name%
    halt
  end
  if !%actor.eq(shield)%
    say Voce precisa de um deste para se proteger, %actor.name%.
    %load% obj 3042
    give escudo %actor.name%
    halt
  end
  if !%actor.eq(wield)%
    say Sem uma arma vai virar comida de Fido, %actor.name%.
    %load% obj 3021
    give espad %actor.name%
    halt
  end
end
~
#3013
Cityguard Stock~
0 b 50
~
if !%self.fighting%
  set actor %random.char%
  if %actor%
    if %actor.is_killer%
      emote grita 'EI!!!  Você é um FORA-DA-LEI!!!'
      kill %actor.name%
    elseif %actor.is_thief%
      emote grita 'EI!!!  Você é um FORA-DA-LEI!!!'
      kill %actor.name%
    elseif %actor.cha% < 6
      %send% %actor% %self.name% cospe no seu rosto.
      %echoaround% %actor% %self.name% cospe no rosto de %actor.name%.
    end
    if %actor.fighting%
      eval victim %actor.fighting%
      if %actor.align% < %victim.align% && %victim.align% >= 0
        emote  grita 'PROTEJAM OS INOCENTES!  BANZAI!  ARARARAGGGHH!'
        kill %actor.name%
      end
    end
  end
end
~
#3014
Stock Fido~
0 b 100
~
set inroom %self.room%
set item %inroom.contents%
while %item%
  * Target the next item in room. In case it is devoured.
  set next_item %item.next_in_list%
  * Check for a corpse. Corpse on TBA is vnum 65535. Stock is -1.
  if %item.vnum(65535)%
    emote brutalmente devora $p.
    %purge% %item%
    halt
  end
  set item %next_item%
  * Loop back
done
~
#3015
Thief Master Idle Steal~
0 b 5
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Idea taken from cheesymud.com
* Thief guildmaster steals from players who idle in his guild. Then pawns the
* item in the shop downstairs so player has to buy their equipment back :-P
* Random trigs have no actors so pick one and make sure it is a player.
set actor %random.char%
if %actor.is_pc%
  * Pick the first item in the actors inventory.
  eval item %actor.inventory%
  * I'll be nice and not steal containers or mail.
  if %item.type% == CONTAINER || %item.vnum% <= 1
    halt
  end
  * If they have an item let the master thief steal it.
  eval item_to_steal %%actor.inventory(%item.vnum%)%%
  if %item_to_steal%
    * Give some hints that the guildmaster is not to be trusted.
    %echo% %self.name% examina %item.shortdesc%.
    wait 2 sec
    * Purge the actors object and load it to the master thief.
    eval stolen %item_to_steal.vnum%
    eval name %item_to_steal.name%
    %load% obj %stolen%
    %purge% %item_to_steal%
    wait 2 sec
    * Lets go sell it to Morgan using its first keyword.
    north
    sell %name.car%
    wait 2 sec
    wink garcom
    wait 2 sec
    south
  else
    emote resmunga infeliz.
  end
else
    emote resmunga infeliz.
end
~
#3018
portal to NT~
1 c 7
en~
* By Rumble of The Builder Academy    tbamud.com 9091
if %cmd.mudcommand% == enter && %arg% == parede
  %send% %actor% Você passa através da parede.
  %echoaround% %actor% %actor.name% bravamente entra na parede.
  %teleport% %actor% 5505
  %force% %actor% look
  %echoaround% %actor% %actor.name% acabou de sair da estátua.
else
  %send% %actor% %cmd% oquê?!
end
~
$~