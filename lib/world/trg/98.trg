#9800
Angels Mob Behavior~
0 b 10
~
* Old SpecProc: angels - Angel mob behavior
set actor %random.char%
if %actor.is_pc%
  if %random.10% == 1
    %echo% %self.name% bate suas asas brilhantes.
  elseif %random.10% == 2
    %echo% %self.name% emite uma luz suave e reconfortante.
  elseif %random.10% == 3
    say Que a paz esteja convosco, mortais.
  end
end
~
#9801
Scroll Activate 9808~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_9808 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho antigo se desfaz em suas mãos.
  %echoaround% %actor% %actor.name% ativa um pergaminho antigo.
  * Object should be extracted after use
end
~
#9802
Evil Boss Aggressive~
0 e 0
entra entrou entrar enter entered~
* Old SpecProc: evil_boss - Aggressive behavior for boss mobs
if %actor.is_pc%
  wait 2
  emote rosna ameaçadoramente para %actor.name%.
  say Você ousa entrar em meu domínio?
  wait 3
  kill %actor.name%
end
~
#9803
Angel 9806 Greet~
0 g 100
~
* Old SpecProc: angels - Angel mob 9806 (Anjo da Luz)
* Greets players who enter the room (not NPCs or invisible immortals)
if %actor.is_pc% && %self.vnum% == 9806
  if %self.canbeseen% && !%actor.invis%
    tell %actor.name% Você passou pelo primeiro teste, a cada passagem saiba guardar os ensinamentos, pois com eles você derrotará o Mal Supremo. Agora siga e trace seu destino.
  end
end
~
#9804
Angel 9807 Greet~
0 g 100
~
* Old SpecProc: angels - Angel mob 9807 (Anjo do Amor)
* Greets players who enter the room (not NPCs or invisible immortals)
if %actor.is_pc% && %self.vnum% == 9807
  if %self.canbeseen% && !%actor.invis%
    tell %actor.name% Sabia foi sua escolha, assim como sábio você deve ser. Para poder derrotar o inimigo maior, deve adquirir aliados também poderosos. Lembre-se com a magia do amor seus inimigos serão os mais fiéis soldados.
  end
end
~
#9805
Angel 9808 Greet~
0 g 100
~
* Old SpecProc: angels - Angel mob 9808 (Anjo da Honestidade)
* Greets players who enter the room (not NPCs or invisible immortals)
if %actor.is_pc% && %self.vnum% == 9808
  if %self.canbeseen% && !%actor.invis%
    tell %actor.name% Vitalia está em festa, pois o grande aventureiro se mostra digno. Quando estiver diante dos maléficos poderes use o encantamento do amor, para que o mal se transforme em amigo e juntos vão em busca de libertar a legião do bem, pois lá receberá o poder de por os maus contra os maus, e só assim o Mal Supremo será derrotado.
  end
end
~
#9806
Angel 9809 Greet and Give Scroll~
0 g 100
~
* Old SpecProc: angels - Angel mob 9809 (Anjo da Vitória)
* Greets players who enter the room and gives scroll 9808 (not NPCs or invisible immortals)
if %actor.is_pc% && %self.vnum% == 9809
  if %self.canbeseen% && !%actor.invis%
    tell %actor.name% Abençoado seja você aventureiro de Vitalia, que com honestidade e amor chegou até aqui, receba esse poderoso pergaminho escritos pelos Deuses de Vitalia. Use-o com fé e seus mais fortes inimigos lhe serão os servos mais fiéis. Agora dê um passo atrás para que seu destino seja traçado.
    * Check if mob has scroll 9808 in inventory
    if %self.inventory(9808)%
      * Give the scroll to the player
      give pergaminho %actor.name%
    end
  end
end
~
#9807
Evil Boss Fight - Charmed Allies~
0 k 100
~
* Old SpecProc: evil_boss - When boss fights, charmed mobs assist player
* Trigger fires when boss enters combat
if %self.vnum% == 9804
  * Set the room to check
  set room_people %self.room.people%
  * Loop through all characters in the room
  while %room_people%
    set next_person %room_people.next_in_room%
    * Check if it's a charmed mob (9801, 9802, or 9803)
    if %room_people.vnum% == 9801 || %room_people.vnum% == 9802 || %room_people.vnum% == 9803
      * Check if the mob is charmed and has a master
      if %room_people.master%
        * Check if mob is not already fighting
        if !%room_people.fighting%
          * Make the charmed mob attack the boss
          %at% %room_people.room.vnum% %echo% %room_people.name% diz, 'Lutarei ao lado de %room_people.master.sex(meu mestre,minha mestra,meu mestre)%!'
          %force% %room_people.name% kill %self.name%
        end
      end
    end
    set room_people %next_person%
  done
end
~
#9808
Evil Boss Death - Free Charmed Mobs~
0 f 100
~
* Old SpecProc: evil_boss - When boss dies, charmed mobs are freed and removed
* Trigger fires when mob dies (after being killed)
if %self.vnum% == 9804
  * Create a list of global mobs to check
  set global_list %world.people%
  * Loop through all mobs in the world
  while %global_list%
    set next_mob %global_list.global_next%
    * Check if it's one of the three special mobs
    if %global_list.vnum% == 9801 || %global_list.vnum% == 9802 || %global_list.vnum% == 9803
      * Check if the mob is charmed
      if %global_list.master%
        * Free the mob and remove it
        %at% %global_list.room.vnum% %echo% %global_list.name% diz, 'Minha missão aqui terminou.'
        wait 1 sec
        %at% %global_list.room.vnum% %echo% %global_list.name% desaparece misteriosamente.
        %purge% %global_list%
      end
    end
    set global_list %next_mob%
  done
end
~
#9809
Room 9823 Entry Message~
2 g 100
~
* Old SpecProc: room_9823 - Displays message when player enters with scroll
* Room trigger for room 9823 (Calabouço dos Anjos)
if %actor.is_pc% && %self.vnum% == 9823
  * Check if scroll 9808 is in the room
  set room_obj %self.contents%
  set found_scroll 0
  while %room_obj%
    if %room_obj.vnum% == 9808
      set found_scroll 1
    end
    set room_obj %room_obj.next_in_list%
  done
  * If scroll is in room, show message
  if %found_scroll%
    %send% %actor% Alguém lhe diz, 'Louvados sejam os Deuses de Vitalia por nos libertar. Em nome da honra e da bondade lhe damos esse pergaminho que há de lhe ajudar a derrotar o mal que impera nesse lugar.
  end
end
~
#9810
Scroll 9808 Use Command~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_9808 - Using the scroll to charm special mobs
* Object trigger for scroll 9808 (pergaminho dourado)
if %self.vnum% == 9808
  if %cmd.mudcommand% == usar || %cmd.mudcommand% == use || %cmd.mudcommand% == ativar || %cmd.mudcommand% == activate
    * Check if a target was specified
    if !%arg%
      %send% %actor% Como? Usar os poderes deste pergaminho em quem?
      halt
    end
    * Find the target in the room by vnums 9801-9803
    set target %actor.room.people%
    while %target%
      set next_target %target.next_in_room%
      if %target.vnum% == 9801 || %target.vnum% == 9802 || %target.vnum% == 9803
        * Check if target name matches what player typed
        eval target_name %target.name%
        if %target_name.contains(%arg%)%
          set victim %target%
          break
        end
      end
      set target %next_target%
    done
    * If target not found (victim variable not set)
    if !%victim%
      %send% %actor% Não há ninguém aqui com esse nome.
      return 1
    end
    * Check if target is the actor or a player
    if %victim% == %actor% || %victim.is_pc%
      %send% %actor% Acho que não seria uma boa idéia...
      return 1
    end
    * Perform the scroll activation
    %send% %actor% Você enrola o pergaminho e aponta-o para %victim.name%.
    %echoaround% %actor% %actor.name% enrola o pergaminho dourado, e aponta-o para %victim.name%.
    * Check if target is one of the three special mobs
    if %victim.vnum% == 9801 || %victim.vnum% == 9802 || %victim.vnum% == 9803
      * Wake up the mob if sleeping  
      if %victim.pos% < 8
        %force% %victim.name% stand
        %at% %victim.room.vnum% %echo% %victim.name% se levanta.
      end
      * Make the mob follow the player and charm it
      %force% %victim.name% follow %actor.name%
      dg_affect %victim% charm on 999
      * Make mob non-aggressive
      %at% %victim.room.vnum% %echo% %victim.name% diz a %actor.name%, 'Reconheço você como %actor.sex(meu senhor,minha senhora,meu senhor)%, e irei ajudar-lhe em sua batalha.'
      * Destroy the scroll
      %send% %actor% O pergaminho se desfaz.
      %purge% %self%
    else
      %send% %actor% O pergaminho não tem efeito.
    end
    return 1
  else
    return 0
  end
end
~
$~