#9500
Função de Combate do Clérigo~
0 k 35
~
* Escrito por Fizban para imitar a Função de Clérigo do ROM
set current_hp %actor.hitp%
set rand %random.7%
* O dano 5000 é na verdade não-dano, não dano.
switch %rand%
  case 1
    case 2
    case 3
    case 4
    set dmg 5000
  break
  case 5
    eval dmg (%random.2% * %random.8%) + (%self.level% / 2)
    set spellname causa sérias feridas
  break
  case 6
    eval dmg (%random.3% * %random.8%) + (%self.level% - 6)
    set spellname causa feridas críticas
  break
  case 7
    eval dmg %random.6% + %self.level%
    set spellname coluna de fogo
  break
done
eval new_current_hp %current_hp% - %dmg%
eval dmgpc (%dmg% * 100) / %current_hp%
if %dmgpc% == 0
  set vp erra
elseif %dmgpc% <= 4
  set vp arranha
elseif %dmgpc% <= 8
  set vp raspa
elseif %dmgpc% <= 12
  set vp acerta
elseif %dmgpc% <= 16
  set vp machuca
elseif %dmgpc% <= 20
  set vp fere
elseif %dmgpc% <= 24
  set vp espanca
elseif %dmgpc% <= 28
  set vp dizima
elseif %dmgpc% <= 32
  set vp devasta
elseif %dmgpc% <= 36
  set vp mutila
elseif %dmgpc% <= 40
  set vp MUTILA
elseif %dmgpc% <= 44
  set vp ESTRIPAR
elseif %dmgpc% <= 48
  set vp EVISCERA
elseif %dmgpc% <= 52
  set vp MASSACRA
elseif %dmgpc% <= 100
  set vp DEMOLE
else
  set vp ANIQUILA
end
if %dmg% > 4000
  return 1
else
  %send% %actor% O %spellname% de %self.name% %vp% você!
  %echoaround% %actor% O %spellname% de %self.name% %vp% %actor.name%!
end
switch %rand%
  case 1
    dg_cast 'poison' %actor%
  break
  case 2
    dg_cast 'curse' %actor%
  break
  case 3
    dg_cast 'blindness' %actor%
  break
  case 4
    dg_cast 'earthquake'
  break
  case 5
    case 6
    case 7
    %damage% %actor% %dmg%
  break
done
~
#9501
Função de Combate do Mago~
0 k 35
~
* Escrito por Fizban para imitar a Função de Clérigo do ROM
* ajustado para imitar as Funções de Mago do TBA
set current_hp %actor.hitp%
set rand %random.5%
switch %rand%
  case 1
    eval dmg (%random.1% * %random.8%) + 1
    set spellname toque gélido
  break
  case 2
    eval dmg (%random.3% * %random.8%) + 3
    set spellname mãos flamejantes
  break
  case 3
    eval dmg (%random.7% * %random.8%) + 7
    set spellname raio elétrico
  break
  case 4
    eval dmg (%random.9% * %random.8%) + 9
    set spellname borrifada colorida
  break
  case 5
    eval dmg (%random.11% * %random.8%) + 11
    set spellname bola de fogo
  break
done
eval new_current_hp %current_hp% - %dmg%
eval dmgpc (%dmg% * 100) / %current_hp%
if %dmgpc% == 0
  set vp erra
elseif %dmgpc% <= 4
  set vp arranha
elseif %dmgpc% <= 8
  set vp raspa
elseif %dmgpc% <= 12
  set vp acerta
elseif %dmgpc% <= 16
  set vp machuca
elseif %dmgpc% <= 20
  set vp fere
elseif %dmgpc% <= 24
  set vp espanca
elseif %dmgpc% <= 28
  set vp dizima
elseif %dmgpc% <= 32
  set vp devasta
elseif %dmgpc% <= 36
  set vp mutila
elseif %dmgpc% <= 40
  set vp MUTILA
elseif %dmgpc% <= 44
  set vp ESTRIPAR
elseif %dmgpc% <= 48
  set vp EVISCERA
elseif %dmgpc% <= 52
  set vp MASSACRA
elseif %dmgpc% <= 100
  set vp DEMOLE
else
  set vp ANIQUILA
end
if %dmg% > 4000
  return 1
else
  %send% %actor% O %spellname% de %self.name% %vp% você!
  %echoaround% %actor% O %spellname% de %self.name% %vp% %actor.name%!
end
~
#9502
Ladrão de Estoque~
0 b 10
~
set actor %random.char%
if %actor%
  if %actor.is_pc% && %actor.gold%
    %send% %actor% Você descobre que %self.name% tem %self.hisher% mãos em sua carteira.
    %echoaround% %actor% %self.name% tenta roubar ouro de %actor.name%.
    eval coins %actor.gold% * %random.10% / 100
    nop %actor.gold(-%coins%)%
    nop %self.gold(%coins%)%
  end
end
~
#9503
Função de Sopro de Fogo~
0 k 100
~
set current_hp %actor.hitp%
eval low (%self.hitp% / 9)
eval high %self.hitp% / 5
eval range %high% - %low%
eval dmg %%random.%range%%% + %low%
eval dmgpc (%dmg% * 100) / %current_hp%
set spellname sopro de fogo
if %dmgpc% == 0
  set vp erra
elseif %dmgpc% <= 4
  set vp arranha
elseif %dmgpc% <= 8
  set vp raspa
elseif %dmgpc% <= 12
  set vp acerta
elseif %dmgpc% <= 16
  set vp machuca
elseif %dmgpc% <= 20
  set vp fere
elseif %dmgpc% <= 24
  set vp espanca
elseif %dmgpc% <= 28
  set vp dizima
elseif %dmgpc% <= 32
  set vp devasta
elseif %dmgpc% <= 36
  set vp mutila
elseif %dmgpc% <= 40
  set vp MUTILA
elseif %dmgpc% <= 44
  set vp ESTRIPAR
elseif %dmgpc% <= 48
  set vp EVISCERA
elseif %dmgpc% <= 52
  set vp MASSACRA
elseif %dmgpc% <= 100
  set vp DEMOLE
else
  set vp ANIQUILA
end
%echoaround% %actor% %self.name% sopra um cone de fogo.
%send% %actor% %self.name% sopra um cone de fogo quente sobre você!
%send% %actor% O %spellname% de %self.name% %vp% você!
%echoaround% %actor% O %spellname% de %self.name% %vp% %actor.name%!
%damage% %actor% %dmg%
~
#9504
Função de Sopro Ácido~
0 k 100
~
set current_hp %actor.hitp%
eval low (%self.hitp% / 11)
eval high %self.hitp% / 6
eval range %high% - %low%
eval dice_dam %self.level% * 16
eval hp_dam %%random.%range%%% + %low%
eval dmg (%hp_dam% + %dice_dam%) / 10
eval dmgpc (%dmg% * 100) / %current_hp%
set spellname sopro ácido
if %dmgpc% == 0
  set vp erra
elseif %dmgpc% <= 4
  set vp arranha
elseif %dmgpc% <= 8
  set vp raspa
elseif %dmgpc% <= 12
  set vp acerta
elseif %dmgpc% <= 16
  set vp machuca
elseif %dmgpc% <= 20
  set vp fere
elseif %dmgpc% <= 24
  set vp espanca
elseif %dmgpc% <= 28
  set vp dizima
elseif %dmgpc% <= 32
  set vp devasta
elseif %dmgpc% <= 36
  set vp mutila
elseif %dmgpc% <= 40
  set vp MUTILA
elseif %dmgpc% <= 44
  set vp ESTRIPAR
elseif %dmgpc% <= 48
  set vp EVISCERA
elseif %dmgpc% <= 52
  set vp MASSACRA
elseif %dmgpc% <= 100
  set vp DEMOLE
else
  set vp ANIQUILA
end
%echoaround% %actor% %self.name% cospe ácido em %actor.name%.
%send% %actor% %self.name% cospe um jato de ácido corrosivo em você.
%send% %actor% O %spellname% de %self.name% %vp% você!
%echoaround% %actor% O %spellname% de %self.name% %vp% %actor.name%!
%damage% %actor% %dmg%
~
#9520
Armadilha Mortal Supernova~
2 g 100
~
* Adaptado de um gatilho por Rumble do The Builder Academy
* Armadilha de quase morte que atordoa o ator
wait 1 sec
%send% %actor% Deve haver um preço a pagar para testemunhar esta visão deslumbrante!
wait 2 sec
set stunned %actor.hitp% - 2
%send% %actor% Isso realmente DOI! E de fato...
%damage% %actor% %stunned%
%echo% @n
%force% %actor% look
~
#9530
Entrar na Bola de Cristal~
1 c 100
l~
if %cmd.mudcommand% == look && (%arg% /= cristal || %arg% /= bola)
  %send% %actor% Você se sente atraído pela bola de cristal. Sua mão se estende...
  %echoaround% %actor% %actor.name% é sugado para dentro da bola de cristal!
  %teleport% %actor% 9502
  wait 2 sec
  %at% 9502 %echoaround% %actor% %actor.name% aparece do nada!
  %force% %actor% look
else
  return 0
end
~
$~
