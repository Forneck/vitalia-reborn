#3800
Card Garoto Random Speech~
0 b 10
~
* Old SpecProc: card_garoto - Random speech about cards
* Trigger fires on random pulse (10% chance)
if %self.fighting% || !%self.pos% >= 8
  halt
end
* 20% chance to say something (number 0-4, only 0 triggers)
if %random.5% == 0
  * 50% chance for each message
  if %random.2% == 0
    say Olá, tem cards pra trocar comigo?
  else
    emote confere sua coleção, pra ver que cartas ainda estão faltando.
  end
end
~
#3801
Card Golem Blocks Without Ticket~
0 q 100
~
* Old SpecProc: card_golem - Blocks passage north without ticket 3803
* Trigger fires when player tries to leave the room
* Check if mob can see the actor and actor is awake
if !%self.canbeseen% || !%actor.is_pc%
  return 0
  halt
end
* Only block movement to the north
if %direction% != north
  return 0
  halt
end
* Check if player has ticket 3803 in inventory or equipped (WEAR_HOLD)
set has_ticket 0
* Check inventory
set obj %actor.inventory%
while %obj%
  if %obj.vnum% == 3803
    set has_ticket 1
    set ticket %obj%
  end
  set obj %obj.next_in_list%
done
* Check if held (WEAR_HOLD is position 17)
if !%has_ticket%
  set held %actor.eq(hold)%
  if %held%
    if %held.vnum% == 3803
      set has_ticket 1
      set ticket %held%
    end
  end
end
* If no ticket, block passage
if !%has_ticket%
  %send% %actor% %self.name% barra a sua passagem.
  %echoaround% %actor% %self.name% barra a passagem de %actor.name%.
  %at% %self.room.vnum% %echo% %self.name% diz, 'Para entrar aqui, você terá que comprar o bilhete.'
  return 1
  halt
end
* Has ticket - take it and allow passage
%send% %actor% %self.name% toma %ticket.shortdesc% da sua mão e lhe concede passagem.
%echoaround% %actor% %self.name% toma %ticket.shortdesc% da mão de %actor.name% e, concedendo-lhe passagem.
%purge% %ticket%
return 0
~
$~
