#5000
Airflow Room Effect~
2 g 100
~
* Old SpecProc: airflow - Converts room between FLYING and AIR_FLOW sectors
* This creates an alternating air current effect in the room
if %self.sector% == 8
  %echo% Uma intensa coluna de ar surge, impelindo tudo para o alto.
  * Change sector would need MUD code support
else
  %echo% A coluna de ar desaparece.
  * Change sector would need MUD code support
end
~
#5001
Blocker Movement Check~
0 q 100
~
* Old SpecProc: blocker - Blocks movement for non-immortals
* Check the direction the player must go to enter restricted area
if %actor.level% >= 31
  * Let immortals pass
  return 0
end
%send% %actor% %self.name% encara você e bloqueia sua passagem.
%echoaround% %actor% %self.name% bloqueia a passagem de %actor.name%.
return 0
~
#5002
Waters Room Effect~
2 g 100
~
* Old SpecProc: waters - Room water effect (need to examine original more)
%echo% As águas borbulham suavemente ao redor.
~
#5003
Flow Room Effect~
2 g 100
~
* Old SpecProc: flow - Room flow effect
%echo% A correnteza flui forte nesta direção.
~
#5004
Show Head Room Effect~
2 c 1
olha olhar look~
* Old SpecProc: show_head_room - Shows details of previous room in zone
* This would need significant MUD code to implement properly in triggers
* For now, just provide a message
%send% %actor% Você pode ver a área principal desta região ao longe.
~
#5005
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
#5006
Tree Mob Behavior~
0 b 15
~
* Old SpecProc: tree - Tree mob behavior
if %random.5% == 1
  %echo% %self.name% sussurra no vento.
elseif %random.5% == 2
  %echo% As folhas de %self.name% balançam suavemente.
end
~
#5007
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
#5008
Horacio Special Behavior~
0 g 10
~
* Old SpecProc: horacio - Special NPC behavior
set actor %random.char%
if %actor.is_pc%
  if %random.10% == 1
    say Ah, mais um visitante em minha humilde morada.
  elseif %random.10% == 2
    emote ajusta seus óculos e olha atentamente para %actor.name%.
  end
end
~
#5009
Honireves Special Behavior~
0 b 20
~
* Old SpecProc: honireves - Special NPC behavior  
if %random.10% == 1
  say O tempo está passando... sempre passando.
elseif %random.10% == 2
  emote contempla algo distante.
end
~
#5010
Elven Mayor Behavior~
0 g 5
~
* Old SpecProc: elven_mayor - Elven mayor behavior
set actor %random.char%
if %actor.is_pc%
  if %actor.race% == Elfo
    say Bem-vindo, irmão élfico.
  else
    say Seja bem-vindo às terras élficas, forasteiro.
  end
end
~
#5011
Magic Missile Caster~
0 k 15
~
* Old SpecProc: magic_mmissile - Casts magic missile in combat
dg_cast 'magic missile' %actor%
~
#5012
Color Spray Caster~
0 k 15
~
* Old SpecProc: magic_cspray - Casts color spray in combat
dg_cast 'color spray' %actor%
~
#5013
Chill Touch Caster~
0 k 15
~
* Old SpecProc: magic_ctouch - Casts chill touch in combat
dg_cast 'chill touch' %actor%
~
#5014
Lightning Bolt Caster~
0 k 15
~
* Old SpecProc: magic_lbolt - Casts lightning bolt in combat
dg_cast 'lightning bolt' %actor%
~
#5015
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
#5016
Card Golem Behavior~
0 e 0
entra entrou entrar enter entered~
* Old SpecProc: card_golem - Animated card golem
if %actor.is_pc%
  emote suas cartas flutuam no ar formando padrões mágicos.
  say As cartas revelam seu destino...
end
~
#5017
Scroll Activate 24400~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_24400 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho brilha intensamente e se dissolve.
  %echoaround% %actor% %actor.name% ativa um pergaminho mágico.
  * Object should be extracted after use
end
~
#5018
Scroll Activate 3118~
1 c 2
usar use ativar activate~
* Old SpecProc: scroll_3118 - Special scroll behavior
if %cmd.mudcommand% == use || %cmd.mudcommand% == ativar
  %send% %actor% O pergaminho emite uma luz dourada antes de desaparecer.
  %echoaround% %actor% %actor.name% ativa um pergaminho dourado.
  * Object should be extracted after use
end
~
#5019
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
$