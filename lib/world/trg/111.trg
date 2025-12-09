#11100
Urso Random Hunger Messages~
0 b 10
~
* Old SpecProc: mob_11103 - Random pulse behavior
* Displays hunger messages randomly
if %self.fighting% || !%self.pos% >= 8
  halt
end
* 20% chance (number 0-4, only fires on 0)
if %random.5% != 0
  halt
end
* 50% chance for each message
if %random.2% == 0
  %echo% Você ouve o estômago de %self.name% roncando... Alimente-%self.hisher%(o,a)%!
else
  * Check if has master and master is in same room
  if %self.master%
    eval master_room %self.master.room%
    eval self_room %self.room%
    if %master_room% == %self_room%
      %send% %self.master% %self.name% sente o seu cheiro... acho que %self.heshe% está com fome!
      %echoaround% %self.master% %self.name% sente o cheiro de %self.master.name%... acho que %self.heshe% está com fome!
    end
  end
end
~
#11101
Urso Recebe e Come Comida~
0 j 100
~
* Old SpecProc: mob_11103 - Receives and eats food
* When given food, eats it immediately
if %self.pos% >= 8 && %object.type% == FOOD
  wait 1 sec
  emote come %object.shortdesc%.
  %purge% %object%
else
  * Return 0 to reject non-food items
  return 0
end
~
#11102
Urso Acorda e Segue Jogador~
0 g 100
~
* Old SpecProc: mob_11103 - Greet trigger, wakes up and follows player
* Only triggers when sleeping and player enters
if %self.fighting%
  halt
end
* Check if already awake, affected by sleep, has master, or actor has master
if %self.pos% >= 8 || %self.affect(sleep)% || %self.master% || %actor.master%
  halt
end
* Check if actor is immortal or mob can't see them
if %actor.level% >= 101 || !%self.canbeseen%
  halt
end
* Wake up and follow
emote acorda.
nop %self.pos(standing)%
wait 1 sec
* Add actor as master/follower
follow %actor.name%
~
$~
