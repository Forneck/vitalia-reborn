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
#3802
Card Garoto Behavior~
0 b 10
~
* Old SpecProc: card_garoto - Card game NPC behavior
if %random.10% == 1
  say Venham, venham! Testem sua sorte nas cartas!
elseif %random.10% == 2
  emote embaralha um baralho de cartas com destreza.
end
~
#3803
Card Golem Behavior~
0 e 0
entra entrou entrar enter entered~
* Old SpecProc: card_golem - Animated card golem
if %actor.is_pc%
  emote suas cartas flutuam no ar formando padrões mágicos.
  say As cartas revelam seu destino...
end
~
$~
