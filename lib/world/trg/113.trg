#11300
Mage Guildguard - 3024~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == east
  * Stop them if they are not the appropriate class.
  if %actor.class% != Mago
    return 0
    %send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#11301
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
#11302
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
#11303
Warrior Guildguard - 3027~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop them if they are not the appropriate class.
  if %actor.class% != Guerreiro
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#11304
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
#11305
Druid Guildguard~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == north
  * Stop them if they are not the appropriate class.
  if %actor.class% != Druida
    return 0
%send% %actor% O guarda humilha você, e bloqueia o seu caminho.
    %echoaround% %actor% O guarda humilha %actor.name%, e bloqueia o caminho d%actor.eleela%.
  end
end
~
#11306
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
#11307
Profundezas~
2 h 100
~
%echo% %object.shortdesc% some nas profundezas das águas.
%purge% %object%
~
$~