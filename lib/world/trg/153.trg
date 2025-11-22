#15300
Portal de entrada - Ciclo Sol e Lua~
2 g 100
~
if %time.hour% >= 18 || %time.hour% < 6
  * Configurar porta norte
    %door% 15300 north purge
    %door% 15300 north flags a
    %door% 15300 north room 15330
    %door% 15300 north name portal lunar crepusculo
    %door% 15300 north description Uma névoa prateada vibra sobre a passagem ao Norte, levando ao Reino Lunar.
  * Mensagem visual de clima noturno
    %echo% O torii vibra com 	wluz prateada	n, revelando que está sob o domínio de Tsukuyomi.
    wait 1 sec
  * LIMPEZA KOMAINU
  * LOOP DE LIMPEZA 
  eval komainu_loop 0
  eval komainu_total_purged 0
  while %komainu_loop% < 20
    eval komainu_count %findmob.15300(15300)%
    if %komainu_count% > 0
      eval komainu_total_purged %komainu_total_purged% + 1
      wait 1 c
      %at% 15300 %purge% komainu
      wait 1 c
      eval komainu_loop %komainu_loop% + 1
      wait 1 c
    else
      eval komainu_loop 20
end
done
    %echo% 	C	WUm brilho lunar impacta o centro do templo!	n
    wait 1 sec
    eval kitsune_count %findmob.15300(15301)%
    eval needed 0
    if %kitsune_count% == 0
    eval needed 2
    elseif %kitsune_count% == 1
    eval needed 1
    else
    eval needed 0
    end
    eval loaded 0
    if %needed% > 0
    %echo% 	W[Tsukuyomi invoca suas mensageiras para o seu templo]	n
    while %loaded% < %needed%
    %echo% 	wUma Kitsune atravessa o portal lunar, banhada por luz prateada.	n
    %load% mob 15301
    eval loaded %loaded% + 1
    wait 0.5 sec
    done
    else
    end
else
%door% 15300 north purge
%door% 15300 north flags a
%door% 15300 north room 15310
%door% 15300 north name portal solar aurora
%door% 15300 north description Pétalas douradas dançam até a passagem ao Norte, iluminando o Reino Solar.
%echo% O torii resplandece com 	Yluz dourada	n, revelando que está sob o domínio de Amaterasu.
wait 1 sec
  * LOOP DE LIMPEZA 
  eval kitsune_loop 0
  eval kitsune_total_purged 0
  
  while %kitsune_loop% < 20
    eval kitsune_count %findmob.15300(15301)%
    if %kitsune_count% > 0
      eval kitsune_total_purged %kitsune_total_purged% + 1
      wait 1 c
      %at% 15300 %purge% kitsune
      wait 1c
      eval kitsune_loop %kitsune_loop% + 1
      wait 1 c
    else
      eval kitsune_loop 20
end
done
    wait 1 sec
    %echo% 	C	YUm brilho matinal explode no centro do templo!	n
    wait 1 sec
    eval komainu_count %findmob.15300(15300)%
    eval needed 0
    if %komainu_count% == 0
    eval needed 2
    elseif %komainu_count% == 1
    eval needed 1
    else
    eval needed 0
    end
    eval loaded 0
    if %needed% > 0
    %echo% 	Y[Amaterasu invocou seus guardiões para o seu templo]	n
    while %loaded% < %needed%
    %echo% 	WUm Komainu atravessa o portal solar, envolto por luz dourada.	n
    %load% mob 15300
    eval loaded %loaded% + 1
    wait 0.5 sec
    done
    else
    end
end
~
#15301
Komainu drop~
0 f 100
~
%echo% 	c[DEBUG] Checando vnum do self: %self.vnum%	n
if %self.vnum% == 15300
  %echo% 	c[DEBUG] Self é 15300 - carregando obj 15300	n
  %load% obj 15300
elseif %self.vnum% == 15301
  %echo% 	c[DEBUG] Self é 15301 - carregando obj 15301	n
  %load% obj 15301
elseif %self.vnum% == 15303
  %echo% 	c[DEBUG] Self é 15303 - carregando obj 15306	n
  %load% obj 15306
else
  %echo% 	c[DEBUG] Vnum não corresponde a nenhuma condição	n
end
~
#15302
y~
1 q 10
~
%load% obj 15301
~
#15303
Benção da Lanterna~
2 c 4
tou~
if %cmd.mudcommand% /= touch && lanterna /= %arg% || espiritual /= %arg% || ancestral /= %arg% || lanterna /= %arg%
    * Momento inicial - o toque
    %send% %actor% Seus dedos roçam a superfície musgosa da lanterna de pedra...
    %echoaround% %actor% %actor.name% toca gentilmente a antiga lanterna coberta de musgo.
    wait 10 c
    * A lanterna responde
    %echo% 	GO musgo verde na lanterna começa a brilhar suavemente! 	n
    wait 10 c
    * Aquecimento mágico
    %send% %actor% 	YA pedra aquece sob seu toque, vibrando com energia ancestral.	n
    %echoaround% %actor% A lanterna pulsa com uma luz âmbar crescente.
    wait 10 c
    * As chamas reagem
    %echo% 	YAs chamas dentro da lanterna dançam e se intensificam! 	n
    wait 10 c
    * Liberação da magia
    %echo% 	MUM BRILHO PÚRPURA IRRADIA DA LANTERNA!!	n
    wait 10 c    
    * Carregamento do ofuda
    %echo% 	m[Invocando o Ofuda Animado...]	n
    %load% mob 15302
    wait 10 c
    * Manifestação do ofuda
    wait 10 c
    wait 10 c
    wait 10 c
    wait 1 sec
    %send% %actor% 	YDezenas de vaga-lumes surgem, circulando em sua volta! 	n
    %echoaround% %actor% 	YDezenas de vaga-lumes surgem, circulando ao redor de %actor.name%! 	n
    wait 1 sec
    %send% %actor% 	WUma constelação viva de luzes douradas te envolve!	n
    %send% %actor% 	YCada vaga-lume toca sua pele, deixando um rastro de calor mágico.	n
    %echoaround% %actor% %actor.name% é envolvido por uma nuvem luminosa!
    wait 3 sec
    %echo% 	mO Ofuda Animado se inclina respeitosamente.	n
    * Remover o ofuda (clean up)
    %purge% ofuda
    %echo% 	GA pedra esfria, o musgo volta ao seu verde tranquilo.	n
    %echo% 	yAs chamas da lanterna retomam seu brilho constante e suave.	n
    wait 1 sec
    %send% %actor% 	cVocê sente uma conexão profunda com as forças arcanas...	n
    %echo% 	G A magia da lanterna se completa. 	n
else
    %echo% 	c[DEBUG 15308] Alvo não é a lanterna - ignorando	n
Return 0
end
~
#15304
Ofuda Lanca Magia na Sala Correta~
0 n 100
~
%echo% 	c[DEBUG 15304] === TRIGGER ATIVADO ===	n
%echo% 	c[DEBUG 15304] Ofuda carregado	n
* Verificar em qual sala o ofuda foi carregado
if %self.room(15301)%
  %echo% 	c[DEBUG 15304] CONDIÇÃO ATENDIDA - Sala 15301 detectada	n
    * Manifestação do ofuda
    %echo% 	MUm redemoinho de tinta e papel manifesta-se diante da lanterna!	n
    wait 10 c
    %echo% 	MUm Ofuda Animado emerge da névoa brilhante! 	n
    %echo% 	mSeus olhos brilham com poder arcano enquanto os símbolos se aquecem.	n
    wait 10 c
    * Encontrar o alvo (primeiro personagem na sala)
    set target %random.char%
    %echo% 	c[DEBUG 15304] Alvo encontrado: %target.name%	n
      if %target%
    %echo% 	c[DEBUG 15304] LANÇANDO MAGIA	n
    %send% %target% 	MO Ofuda Animado olha fixamente para você	n
    %echoaround% %target% 	MO Ofuda Animado se volta para %target.name%! 	n
    wait 10 c
    %echo% 	mO papel brilha intensamente, símbolos mágicos voam pela sala!	n
    wait 1 sec
    %send% %target.name% 	YDezenas de vaga-lumes surgem, circulando em sua volta! 	n
    %echoaround% %target.name% 	YDezenas de vaga-lumes surgem, circulando ao redor de %target.name%! 	n
    wait 1 sec
    %send% %target% 	WUma constelação viva de luzes douradas te envolve!	n
    %send% %target% 	YCada vaga-lume toca sua pele, deixando um rastro de calor mágico.	n
    %echoaround% %target% %target.name% é envolvido por uma nuvem luminosa!
    wait 1 sec
    dg_cast 'dance of fireflies' %target%
    %echo% 	c[DEBUG 15304] dg_cast executado	n
    wait 2 sec
    %echo% 	mO Ofuda Animado se inclina após canalizar a magia.	n
    %echo% 	c[DEBUG 15304] Magia concluída	n
  else
    %echo% 	c[DEBUG 15304] ERRO: Nenhum alvo detectado	n
    %echo% 	m[Nenhum alvo disponível]	n
  end
  
else
  %echo% 	c[DEBUG 15304] Sala incorreta - não é 15301	n
  %echo% 	c[DEBUG 15304] Sala atual: %room_vnum% (esperado: 15301)	n
  end
%echo% 	c[DEBUG 15304] === TRIGGER FINALIZADO ===	n
~
#15305
Bencao da agua~
1 s 100
~
      %send% %actor% 	YUm brilho dourado te envolve enquanto a água sagrada flui pelo seu corpo!	n
      %echoaround% %actor% %actor.name% brilha com uma luz dourada!
      wait 1 sec
      * Lançar bless
      dg_cast 'bless person' %actor.name%
      wait 1 sec
      %send% %actor% 	YVocê se sente abenoado pelas forças sagradas!	n
      %echoaround% %actor% %actor.name% parece estar abençoado.
end
~
#15306
Tanuki Astral Travesso - Troca Item~
0 g 100
~
* Trigger MOB tipo Greet - ativa quando jogador entra na sala
* Se jogador tem bilhete: não faz nada
* Se jogador NÃO tem bilhete: coloca no inventário
* Verificar se é player
if %actor.is_pc%
  
  * Procurar bilhete no inventário usando hasobj
  if %actor.inventory(15304)%
  else
    
    * Ação casual
    eval action %random.3%
    if %action% == 1
      %echoaround% %actor% O Tanuki dá um esbarrão brincalhão em %actor.name%.
      %send% %actor% O Tanuki esbarra em você desajeitadamente.
    elseif %action% == 2
      %echoaround% %actor% O Tanuki abraça %actor.name% rapidamente.
      %send% %actor% O Tanuki te dá um abraço rápido e caloroso.
    else
      %echoaround% %actor% O Tanuki dá tapinhas nas costas de %actor.name%.
      %send% %actor% Você sente tapinhas amigáveis nas suas costas.
    end
    
    wait 1 sec
    
    * Carregar bilhete
    %load% obj 15304 %actor% inv
  end
else
end
~
#15307
Mobs reagem ao bilhete~
1 b 100
~
* Anexar ao objeto bilhete (VNUM 15304)
eval wearer %self.carried_by%
      * Sala onde está o bilhete
      eval current_room %self.room%
      * Loop para encontrar um mob válido
      eval attacker %current_room.people%
      eval found 0
      while %attacker% && !%found%
        if %attacker.is_pc% == 0
          if %attacker% != %wearer%
            eval found 1
          else
            eval attacker %attacker.next_in_room%
          end
        else
          eval attacker %attacker.next_in_room%
        end
      done
      if %found%
        * Mob reage ao ver o bilhete
        %echoaround% %wearer% %attacker.name% vê o bilhete em %wearer.name% e sorri maliciosamente!
        wait 1 sec
        %force% %attacker% poke %wearer.name%
        wait 1 sec
        %force% %attacker% laugh
        wait 1 sec
      else
  end
~
#15308
Morte do monge~
0 f 100
~
%purge% selo
~
#15309
Conjunto Caminho Sagrado - Ativando~
1 j 100
~
%echo% 	c[DEBUG 15308] === TRIGGER ATIVADO ===	n
eval wearer %actor%
if %wearer.is_pc%
  %echo% 	c[DEBUG 15308] Verificando conjunto: %wearer.name%	n
  
  eval item_kimono %wearer.eq(body)%
  eval has_kimono 0
  if %item_kimono% && %item_kimono.vnum% == 15309
    eval has_kimono 1
    %echo% 	c[DEBUG 15308] Kimono (15309): ENCONTRADO	n
  else
    %echo% 	c[DEBUG 15308] Kimono (15309): NAO ENCONTRADO	n
  end
  
  eval item_hakama %wearer.eq(legs)%
  eval has_hakama 0
  if %item_hakama% && %item_hakama.vnum% == 15310
    eval has_hakama 1
    %echo% 	c[DEBUG 15308] Hakama (15310): ENCONTRADO	n
  else
    %echo% 	c[DEBUG 15308] Hakama (15310): NAO ENCONTRADO	n
  end
  
  eval item_luvas %wearer.eq(hands)%
  eval has_luvas 0
  if %item_luvas% && %item_luvas.vnum% == 15311
    eval has_luvas 1
    %echo% 	c[DEBUG 15308] Luvas (15311): ENCONTRADO	n
  else
    %echo% 	c[DEBUG 15308] Luvas (15311): NAO ENCONTRADO	n
  end
  
  eval item_botas %wearer.eq(feet)%
  eval has_botas 0
  if %item_botas% && %item_botas.vnum% == 15312
    eval has_botas 1
    %echo% 	c[DEBUG 15308] Botas (15312): ENCONTRADO	n
  else
    %echo% 	c[DEBUG 15308] Botas (15312): NAO ENCONTRADO	n
  end
  
  %echo% 	c[DEBUG 15308] Status: Kimono=%has_kimono% Hakama=%has_hakama% Luvas=%has_luvas% Botas=%has_botas%	n
  
  if %has_kimono% && %has_hakama% && %has_luvas% && %has_botas%
    %echo% 	c[DEBUG 15308] CONJUNTO COMPLETO DETECTADO	n
    
    * Capturar valores ORIGINAIS
    eval str_atual %wearer.str%
    eval con_atual %wearer.con%
    eval dex_atual %wearer.dex%
    eval cha_atual %wearer.cha%
    eval int_atual %wearer.int%
    eval wis_atual %wearer.wis%
    eval maxhp_atual %wearer.maxhitp%
    
    %echo% 	c[DEBUG 15308] VALORES ORIGINAIS:	n
    %echo% 	c[DEBUG 15308]   STR: %str_atual%	n
    %echo% 	c[DEBUG 15308]   CON: %con_atual%	n
    %echo% 	c[DEBUG 15308]   DEX: %dex_atual%	n
    %echo% 	c[DEBUG 15308]   CHA: %cha_atual%	n
    %echo% 	c[DEBUG 15308]   INT: %int_atual%	n
    %echo% 	c[DEBUG 15308]   WIS: %wis_atual%	n
    %echo% 	c[DEBUG 15308]   MAXHP: %maxhp_atual%	n
    
    * Aplicar novos valores
    %echo% 	c[DEBUG 15308] APLICANDO NOVOS VALORES...	n
    %wearer.str(3)
    %wearer.con(3)
    %wearer.dex(3)
    %wearer.cha(3)
    %wearer.int(3)
    %wearer.wis(3)
    %wearer.maxhitp(250)
    %echo% 	c[DEBUG 15308] VALORES APLICADOS COM SUCESSO	n
    
    * Verificar se foi aplicado (debug confirma)
    eval str_check %wearer.str%
    eval con_check %wearer.con%
    eval dex_check %wearer.dex%
    eval cha_check %wearer.cha%
    eval int_check %wearer.int%
    eval wis_check %wearer.wis%
    eval maxhp_check %wearer.maxhitp%
    
    %echo% 	c[DEBUG 15308] VERIFICACAO FINAL (POS-APLICACAO):	n
    %echo% 	c[DEBUG 15308]   STR: %str_check% (esperado: %str_atual% +3)	n
    %echo% 	c[DEBUG 15308]   CON: %con_check% (esperado: %con_atual% +3)	n
    %echo% 	c[DEBUG 15308]   DEX: %dex_check% (esperado: %dex_atual% +3)	n
    %echo% 	c[DEBUG 15308]   CHA: %cha_check% (esperado: %cha_atual% +3)	n
    %echo% 	c[DEBUG 15308]   INT: %int_check% (esperado: %int_atual% +3)	n
    %echo% 	c[DEBUG 15308]   WIS: %wis_check% (esperado: %wis_atual% +3)	n
    %echo% 	c[DEBUG 15308]   MAXHP: %maxhp_check% (esperado: %maxhp_atual% +250)	n
    
    * Aplicar spells
    %echo% 	c[DEBUG 15308] APLICANDO SPELLS...	n
    dg_cast 'sanctuary' %wearer%
    %echo% 	c[DEBUG 15308] Sanctuary aplicado	n
    dg_cast 'bless person' %wearer%
    %echo% 	c[DEBUG 15308] Bless aplicado	n
    dg_cast 'improved armor' %wearer%
    %echo% 	c[DEBUG 15308] Armor aplicado	n
    
    %send% %wearer% 	Y...CONJUNTO DO CAMINHO SAGRADO ATIVADO...	n
    %echoaround% %wearer% 	Y%wearer.name% brilha com uma aura dourada sagrada!	n
    %echo% 	c[DEBUG 15308] TRIGGER EXECUTADO COM SUCESSO	n
  else
    %echo% 	c[DEBUG 15308] CONJUNTO INCOMPLETO - NENHUM BONUS APLICADO	n
  end
else
  %echo% 	c[DEBUG 15308] ATOR NAO EH PLAYER	n
end
%echo% 	c[DEBUG 15308] === TRIGGER FINALIZADO ===	n
~
#15310
Conjunto Caminho Sagrado - Remove Bonificacao~
1 a 0
~
%echo% 	c[DEBUG 15308] === TRIGGER DE RETIRADA ATIVADO ===	n
eval wearer %actor%
if %wearer.is_pc%
  %echo% 	c[DEBUG 15308] Removendo bônus de: %wearer.name%	n
  
  * Capturar valores ATUAIS
  eval str_atual %wearer.str%
  eval con_atual %wearer.con%
  eval dex_atual %wearer.dex%
  eval cha_atual %wearer.cha%
  eval int_atual %wearer.int%
  eval wis_atual %wearer.wis%
  eval maxhp_atual %wearer.maxhitp%
  
  %echo% 	c[DEBUG 15308] VALORES ATUAIS (COM BONUS):	n
  %echo% 	c[DEBUG 15308]   STR: %str_atual%	n
  %echo% 	c[DEBUG 15308]   CON: %con_atual%	n
  %echo% 	c[DEBUG 15308]   DEX: %dex_atual%	n
  %echo% 	c[DEBUG 15308]   CHA: %cha_atual%	n
  %echo% 	c[DEBUG 15308]   INT: %int_atual%	n
  %echo% 	c[DEBUG 15308]   WIS: %wis_atual%	n
  %echo% 	c[DEBUG 15308]   MAXHP: %maxhp_atual%	n
  
  * Calcular SUBTRACAO (-3 em todos os atributos, -250 HP)
  eval str_novo %str_atual% - 3
  eval con_novo %con_atual% - 3
  eval dex_novo %dex_atual% - 3
  eval cha_novo %cha_atual% - 3
  eval int_novo %int_atual% - 3
  eval wis_novo %wis_atual% - 3
  eval maxhp_novo %maxhp_atual% - 250
  
  %echo% 	c[DEBUG 15308] CALCULO (SUBTRACAO):	n
  %echo% 	c[DEBUG 15308]   STR: %str_atual% - 3 = %str_novo%	n
  %echo% 	c[DEBUG 15308]   CON: %con_atual% - 3 = %con_novo%	n
  %echo% 	c[DEBUG 15308]   DEX: %dex_atual% - 3 = %dex_novo%	n
  %echo% 	c[DEBUG 15308]   CHA: %cha_atual% - 3 = %cha_novo%	n
  %echo% 	c[DEBUG 15308]   INT: %int_atual% - 3 = %int_novo%	n
  %echo% 	c[DEBUG 15308]   WIS: %wis_atual% - 3 = %wis_novo%	n
  %echo% 	c[DEBUG 15308]   MAXHP: %maxhp_atual% - 250 = %maxhp_novo%	n
  
  * Aplicar novos valores (removendo bonus)
  %echo% 	c[DEBUG 15308] REMOVENDO VALORES...	n
  %wearer.str(%str_novo%)
  %wearer.con(%con_novo%)
  %wearer.dex(%dex_novo%)
  %wearer.cha(%cha_novo%)
  %wearer.int(%int_novo%)
  %wearer.wis(%wis_novo%)
  %wearer.maxhitp(%maxhp_novo%)
  %echo% 	c[DEBUG 15308] VALORES REMOVIDOS COM SUCESSO	n
  
  * Verificar se foi aplicado (debug confirma)
  eval str_check %wearer.str%
  eval con_check %wearer.con%
  eval dex_check %wearer.dex%
  eval cha_check %wearer.cha%
  eval int_check %wearer.int%
  eval wis_check %wearer.wis%
  eval maxhp_check %wearer.maxhitp%
  
  %echo% 	c[DEBUG 15308] VERIFICACAO FINAL (POS-REMOCAO):	n
  %echo% 	c[DEBUG 15308]   STR: %str_check% (esperado: %str_novo%)	n
  %echo% 	c[DEBUG 15308]   CON: %con_check% (esperado: %con_novo%)	n
  %echo% 	c[DEBUG 15308]   DEX: %dex_check% (esperado: %dex_novo%)	n
  %echo% 	c[DEBUG 15308]   CHA: %cha_check% (esperado: %cha_novo%)	n
  %echo% 	c[DEBUG 15308]   INT: %int_check% (esperado: %int_novo%)	n
  %echo% 	c[DEBUG 15308]   WIS: %wis_check% (esperado: %wis_novo%)	n
  %echo% 	c[DEBUG 15308]   MAXHP: %maxhp_check% (esperado: %maxhp_novo%)	n
  
  %send% %wearer% 	RBônus do Caminho Sagrado removidos.	n
  %echoaround% %wearer% A aura dourada ao redor de %wearer.name% desaparece.	n
  
  %echo% 	c[DEBUG 15308] TRIGGER DE RETIRADA EXECUTADO COM SUCESSO	n
else
  %echo% 	c[DEBUG 15308] ATOR NAO EH PLAYER	n
end
%echo% 	c[DEBUG 15308] === TRIGGER DE RETIRADA FINALIZADO ===	n
~
#15311
Dano especial~
1 b 100
~
* Pega quem está carregando a arma
eval wearer %self.worn_by%
if !%wearer%
else
eval posicao %wearer.pos()%
if %posicao% !=Lutando
  else
    * Pega quem está lutando contra
    eval target %wearer.fighting%
    if !%target%
    else
      * GATILHO PRINCIPAL: 50% de chance de ativar
      eval chance %random.100%
      if %chance% < 50
        * Calculo 6d5
        eval dado1 %random.5% + 1
        eval dado2 %random.5% + 1
        eval dado3 %random.5% + 1
        eval dado4 %random.5% + 1
        eval dado5 %random.5% + 1
        eval dado6 %random.5% + 1
        eval dado %dado1% + %dado2% + %dado3% + %dado4% + %dado5% + %dado6%	n
        * Calculo nível/2
        eval lvlwearer %wearer.level%
        eval lvlbonus %lvlwearer% / 2
        * Calculo inteligência*3
        eval intwearer %wearer.int%
        eval intbonus %intwearer% + %intwearer% + %intwearer%
        * Dano total: 6d5 + (nível/2) + (int*3)
        eval extra %dado% + %lvlbonus% + %intbonus%
        * Aplica dano proporcional
        %damage% %target% %extra%
          %echoaround% %wearer% 	YUm raio dourado percorre a lâmina de %wearer.name%, atingindo %target.name% com força!	n
          %send% %wearer% 	YSua espada brilha com energia elemental, causando dano elemental!	n
          %send% %target% 	RVocê é atingido por uma descarga elétrica que percorre a arma de %wearer.name%!	n
      else
      end
    end
end
end
~
#15312
Trigger do Tomo~
1 c 1
c~
%echo% [DEBUG] TRIGGER ATIVADO
if %cmd.mudcommand% /= cast && 'fireball' /= %arg% || 'disintegrate' /= %arg% || 'magic missile' /= %arg% || 'chill touch' /= %arg% || 'color spray' /= %arg% || 'vampiric touch' /= %arg% || 'lightning bolt' /= %arg% || 'shocking grasp' /= %arg% || 'burning touch' /= %arg%
else [DEBUG] %cmd.mudcommand% e %arg% utilizados

%echo% [DEBUG] Magia arcana detectada %cmd.mudcommand% e %arg%
* Pega quem está carregando a arma
eval posicao %actor.pos()%
%echo% [DEBUG] %posicao%
if %posicao% !=Lutando
return 0
  else
    * Pega quem está lutando contra
    eval target %actor.fighting%
%echo% alvo = %target%
    if !%target%
return 0
    else
%echo% [DEBUG] Lançando magia
%force% %actor% %cmd.mudcommand% %arg%
%actor.mana(35)%
%force% %actor% %cmd.mudcommand% %arg%
%actor.mana(35)%
%force% %actor% %cmd.mudcommand% %arg%
%actor.mana(35)%
%force% %actor% %cmd.mudcommand% %arg%
%actor.mana(35)%
return 0
end
end
else
return 0
end
~
$~