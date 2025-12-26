#27800
Bola de Fogo da Serpente Marinha - 27801~
0 k 5
~
if %actor.is_pc%
  emote pronuncia as palavras, 'hisssssss'.
  dg_cast 'fireball' %actor%
end
~
#27802
Leviatã - 27803~
0 k 5
~
if %actor.is_pc%
  switch %random.3%
    case 1
      emote pronuncia as palavras, 'transvecta aqua'.
      %echo% uma onda gigante esmaga %actor.name%.
      %damage% %actor% 50
    break
    case 2
      emote olha para você com a mais profunda tristeza.
    break
    case 3
      emote pronuncia as palavras, 'transvecta talon'.
      dg_cast 'cure critic' %self%
    break
    default
    break
  done
end
~
$~
