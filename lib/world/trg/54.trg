#5400
Mage Guildguard - 3024~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == north
  * Stop them if they are not the appropriate class.
  if %actor.class% != Mago
    return 0
    %send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5401
Cleric Guildguard - 3025~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == south
  * Stop them if they are not the appropriate class.
  if %actor.class% != Clerigo
    return 0
  %send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5402
Thief Guildguard - 3026~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == south
  * Stop them if they are not the appropriate class.
  if %actor.class% != Ladrao
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5403
Warrior Guildguard - 3027~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == south
  * Stop them if they are not the appropriate class.
  if %actor.class% != Guerreiro
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5404
Bard Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == south
  * Stop them if they are not the appropriate class.
  if %actor.class% != Bardo
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5405
Druid Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == down
  * Stop them if they are not the appropriate class.
  if %actor.class% != Druida
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#5406
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
#5407
Magic User - 5421-5428, 5440, 5455~
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
#5408
Cityguard - 5434, 61-63, 82~
0 b 50
~
if !%self.fighting%
  set actor %random.char%
  if %actor%
    if %actor.is_killer%
      emote screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'
      kill %actor.name%
    elseif %actor.is_thief%
      emote screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'
      kill %actor.name%
    elseif %actor.cha% < 6
      %send% %actor% %self.name% spits in your face.
      %echoaround% %actor% %self.name% spits in %actor.name%'s face.
    end
    if %actor.fighting%
      eval victim %actor.fighting%
      if %actor.align% < %victim.align% && %victim.align% >= 0
        emote screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'
        kill %actor.name%
      end
    end
  end
end
~
#5409
Healer - 5481~
0 b 10
~
* This is required because a random trig does not have an actor.
set actor %random.char%
* only continue if an actor is defined.
if %actor%
  * if they have lost more than half their hitpoints heal em
  if %actor.hitp% < %actor.maxhitp% / 2
    wait 1 sec
    say Voce se machucou, me deixe ajudar.
    wait 2 sec
    %echoaround% %actor% %self.name% coloca as maos nas feridas de %actor.name% e se concentra.
    %send% %actor% %self.name% coloca as maos nas tuas feridas e se concentra.
    dg_cast 'heal' %actor%
  end
end
~
#5410
Thief - 5435~
0 b 10
~
set actor %random.char%
if %actor%
  if %actor.is_pc% && %actor.gold%
    %send% %actor% You discover that %self.name% has %self.hisher% hands in your wallet.
    %echoaround% %actor% %self.name% tries to steal gold from %actor.name%.
    eval coins %actor.gold% * %random.10% / 100
    nop %actor.gold(-%coins%)%
    nop %self.gold(%coins%)%
  end
 end
 ~
#5511
Portal para Midgaard~
1 c 7
en~
* By Cansian
if %cmd.mudcommand% == enter && %arg% == estatua
  %send% %actor% Voce passa atraves da estatua.
  %echoaround% %actor% %actor.name% bravamente entra na estatua.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% acabou de sair da parede.
else
  %send% %actor% %cmd% Oque?!
end
~
$~