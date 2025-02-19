#3800
garoto~
0 g 100
~
if !%self.canbeseen(%actor%)%
  halt
end
if %actor.is_pc%
  if %random.5% == 0
    say Ol, tem cards pra trocar comigo?
  else
    emote confere sua coleção, pra ver que cartas ainda estão faltando.
  end
end
~
#3801
Golem pede Bilhete~
0 q 100
~
if !%self.canbeseen(%actor%)%
  halt
end
if !%actor.is_pc%
  halt
end
if %direction% != north
  return 0
end
if !%actor.has_item(3803)% && !%actor.eq().vnum(3803)%
  emote barra a sua passagem.
  say Para entrar aqui, você ter que comprar o bilhete.
  return 0
  halt
end
emote toma um bilhete do Museu da sua mão e lhe concede passagem.
if %actor.has_item(3803)%
  %purge% %actor.inventory(3803)%
end
~
$~
