#3800
garoto~
0 b 30
~
if %self.fighting%  
  halt
end
if %random.4%
  halt
end
if %random.1%
  say Ol, tem cards pra trocar comigo?
else
  emote confere sua coleo, pra ver que cartas ainda esto faltando.
end
~
#3801
Golem pede Bilhete~
0 q 100
~
if !%actor.is_pc%
  halt
end
if %direction% != north
  halt
end
set has_ticket 0
set obj_list %actor.inventory%
while %obj_list%
  if %obj_list.vnum% == 3803
    set has_ticket 1
  break
end
if !%has_ticket%
  emote barra a sua passagem.
  emote barra a passagem de %actor.name%.
  say Para entrar aqui, você terá que comprar o bilhete.
  return 0
else
  say Pode passar %actor.name%
end
~
$~
