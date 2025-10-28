#15300
Portal de entrada - Ciclo Sol e Lua~
2 g 100
~
%echo% @c[DEBUG 15300] === TRIGGER ATIVADO ===@n
%echo% @c[DEBUG 15300] Hora atual: %time.hour%@n

if %time.hour% >= 18 || %time.hour% < 6
%echo% @c[DEBUG 15300] CICLO: NOITE (Portal Lunar)@n

  * Configurar porta norte
    %door% 15300 north purge
    %door% 15300 north flags a
    %door% 15300 north room 15330
    %door% 15300 north name portal lunar crepusculo
    %door% 15300 north description Uma névoa prateada vibra sobre a passagem ao Norte, levando ao Reino Lunar.
    %echo% @c[DEBUG 15300] Porta configurada@n

  * Mensagem visual de clima noturno
    %echo% O torii vibra com @wluz prateada@n, revelando que está sob o domínio de Tsukuyomi.
    wait 1 sec

  * LIMPEZA KOMAINU

  * LOOP DE LIMPEZA 
  %echo% @c[DEBUG 15300] Iniciando loop de limpeza de Komainu@n
  
  eval komainu_loop 0
  eval komainu_total_purged 0
  
  while %komainu_loop% < 20
    %echo% 	c[DEBUG 15303] === ITERAÇÃO %komainu_loop% ===	n
    
    eval komainu_count %findmob.15300(15300)%
    %echo% 	c[DEBUG 15303] Komainu detectados nesta iteração: %komainu_count%	n
    
    if %komainu_count% > 0
      eval komainu_total_purged %komainu_total_purged% + 1
      %echo% 	c[DEBUG 15303] Total purificado até agora: %komainu_total_purged%	n
      
      %echo% 	c[DEBUG 15303] Executando: %at% 15300 %purge% komainu	n
      wait 1 c
      %at% 15300 %purge% komainu
      %echo% 	c[DEBUG 15303] Purge executado	n
      wait 1 c
      eval komainu_loop %komainu_loop% + 1
      %echo% 	c[DEBUG 15303] Loop incrementado para: %komainu_loop%	n
      wait 1 c
    else
      %echo% 	Y✓ Substituição Completa! ✓	n
      %echo% @WUm brilho celestial prateado consome os Komainu solares!@n
      %echo% 	c[DEBUG 15303] Nenhum komainu encontrado - saindo do loop	n
      %echo% 	c[DEBUG 15303] Total final de komainu eliminados: %komainu_total_purged%	n
      eval komainu_loop 20
      %echo% 	c[DEBUG 15303] Loop forçado para 20 (saída)	n
end
done

    %echo% @c[DEBUG 15300] INVOCANDO KITSUNE @n
    wait 1 sec
    %echo% @C@WUm brilho lunar impacta o centro do templo!@n
    wait 1 sec
    %echo% @c[DEBUG 15300] Verificando Kitsune...@n
    eval kitsune_count %findmob.15300(15301)%
    %echo% @c[DEBUG 15300] Kitsune antes do load: %kitsune_count%@n
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
    %echo% @Y[Deus da noite invoca suas mensageiras para o seu templo]@n
    while %loaded% < %needed%
    %echo% @wUma Kitsune atravessa o portal lunar, banhada por luz prateada.@n
    %load% mob 15301
    eval loaded %loaded% + 1
    wait 0.5 sec
    done
    %echo% @c[DEBUG 15300] Kitsune carregadas: %needed%@n
    else
    %echo% @c[DEBUG 15300] Kitsune já corretas na sala - nenhum load necessário@n
    end

%echo% @c[DEBUG 15300] === Ciclo Noturno Encerrado ===@n

else
%echo% @c[DEBUG 15300] CICLO: DIA (Portal Solar)@n
%door% 15300 north purge
%door% 15300 north flags a
%door% 15300 north room 15310
%door% 15300 north name portal solar aurora
%door% 15300 north description Pétalas douradas dançam até a passagem ao Norte, iluminando o Reino Solar.
%echo% @c[DEBUG 15300] Porta configurada@n
%echo% O torii resplandece com @Yluz dourada@n, revelando que está sob o domínio de Amaterasu.
wait 1 sec

  * LOOP DE LIMPEZA 
  %echo% @c[DEBUG 15300] Iniciando loop de limpeza de Kitsune@n
  
  eval kitsune_loop 0
  eval kitsune_total_purged 0
  
  while %kitsune_loop% < 20
    %echo% 	c[DEBUG 15303] === ITERAÇÃO %kitsune_loop% ===	n
    
    eval kitsune_count %findmob.15300(15301)%
    %echo% 	c[DEBUG 15303] Kitsune detectados nesta iteração: %kitsune_count%	n
    
    if %kitsune_count% > 0
      eval kitsune_total_purged %kitsune_total_purged% + 1
      %echo% 	c[DEBUG 15303] Total purificado até agora: %kitsune_total_purged%	n
      
      %echo% 	c[DEBUG 15303] Executando: %at% 15300 %purge% kitsune	n
      wait 1 c
      %at% 15300 %purge% kitsune
      %echo% 	c[DEBUG 15303] Purge executado	n
      wait 1c
      eval kitsune_loop %kitsune_loop% + 1
      %echo% 	c[DEBUG 15303] Loop incrementado para: %kitsune_loop%	n
      wait 1 c
    else
      %echo% 	Y✓ Substituição Completa! ✓	n
      %echo% @YUm brilho celestial dourado consome os Kitsune lunares!@n
      %echo% 	c[DEBUG 15303] Nenhum kitsune encontrado - saindo do loop	n
      %echo% 	c[DEBUG 15303] Total final de kitsune eliminados: %kitsune_total_purged%	n
      eval kitsune_loop 20
      %echo% 	c[DEBUG 15303] Loop forçado para 20 (saída)	n
end
done

    %echo% @C[DEBUG 15300]INVOCANDO KOMAINU
    wait 1 sec
    %echo% @C@YUm brilho matinal explode no centro do templo!@n
    wait 1 sec
    %echo% @c[DEBUG 15300] Verificando Komainu...@n
    eval komainu_count %findmob.15300(15300)%
    %echo% @c[DEBUG 15300] Komainu antes do load: %komainu_count%@n
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
    %echo% @Y[Deus da manhã invocou seus guardiões para o seu templo]@n
    while %loaded% < %needed%
    %echo% @WUm Komainu atravessa o portal solar, envolto por luz dourada.@n
    %load% mob 15300
    eval loaded %loaded% + 1
    wait 0.5 sec
    done
    %echo% @c[DEBUG 15300] Komainu carregados: %needed%@n
    else
    %echo% @c[DEBUG 15300] Komainu já corretos na sala - nenhum load necessário@n
    end

%echo% @c[DEBUG 15300] === Ciclo Diurno Encerrado ===@n

end

%echo% @c[DEBUG 15300] === TRIGGER FINALIZADO ===@n
~
#15301
Komainu drop~
0 f 10
~
%load% obj 15300
~
#15302
Kitsune drop~
0 f 10
~
%load% obj 15301
~
#15303
Raijin Limpa a Praga de Coelhos~
2 b 50
~
%echo% 	c[DEBUG 15303] === TRIGGER ATIVADO ===	n
%echo% 	c[DEBUG 15303] Procurando coelhos (VNUM 15302 e 15303) em sala 15300	n
    eval coelhos_count %findmob.15300(15302)% 
    eval coelhos_count %coelhos_count% + %findmob.15300(15303)%
%echo% 	c[DEBUG 15303] Coelhos encontrados: %coelhos_count%	n
if %coelhos_count% > 0
  %echo% 	c[DEBUG 15303] DETECÇÃO POSITIVA - ATIVANDO RAIJIN	n
  
  * RAIJIN ATIVA COM FÚRIA DIVINA
  %echo% 		Y RAIJIN, SENHOR DOS TROVÕES ACORDA COM FÚRIA!	n
  
  wait 10 c
  %echo% 	c[DEBUG 15303] Wait 1 completado	n
  
  %echo% 	Y O torii vibra violentamente! Raios lascam do céu! 	n
  wait 10 c
  
  %echo% 	R	bRAIJIN: MISERÁVEL PRAGA! COELHOS!?!? NO MEU TEMPLO SAGRADO!?	n	n
  wait 1 s
  %echo% 	c[DEBUG 15303] Diálogo 1 completo	n
  
  %echo% 	R	bRAIJIN: VOCÊS OUSAM CONTAMINAR ESTE SANTUÁRIO CELESTIAL?!	n	n
  wait 1 s
  
  %echo% 	Y Os trovões explodem com raiva indescritível! 	n
  wait 1 s
  %echo% 	c[DEBUG 15303] Diálogo 2 completo	n
  
  %echo% 	R	bRAIJIN: EU VOU PURIFICÁ-LOS COM AS CHAMAS DA VINDITA DIVINA!	n	n
  wait 1 s
  
  * LOOP DE PURIFICAÇÃO
  %echo% 	Y✦✦✦ COMEÇANDO A PURIFICAÇÃO CELESTIAL ✦✦✦	n
  %echo% 	c[DEBUG 15303] Iniciando loop de purificação	n
  
  eval coelhos_loop 0
  eval total_purged 0
  
  while %coelhos_loop% < 20
    %echo% 	c[DEBUG 15303] === ITERAÇÃO %coelhos_loop% ===	n
    
    eval coelhos_count %findmob.15300(15302)% 
    eval coelhos_count %coelhos_count% + %findmob.15300(15303)%
    eval coelhos_count %coelhos_count% + %findmob.15300(15304)%
    %echo% 	c[DEBUG 15303] Coelhos detectados nesta iteração: %coelhos_count%	n
    
    if %coelhos_count% > 0
      eval total_purged %total_purged% + 1
      %echo% 	c[DEBUG 15303] Total purificado até agora: %total_purged%	n
      
      %echo% 	R	b RAIJIN INVOCA UM RAIO DIVINO! 	n
      %echo% 	c[DEBUG 15303] Executando: LIGHTNING BLAST n
      wait 0.5 sec
          dg_cast 'lightning blast' %actor.name(coelh)%
      %echo% 	c[DEBUG 15303] Lightning Blast executado	n
      
      %echo% 	Y≡≡≡≡≡≡≡≡≡≡ 	CUm coelho é atingido por raios celestiais! 	Y≡≡≡≡≡≡≡≡≡≡	n
      eval coelhos_loop %coelhos_loop% + 1
      %echo% 	c[DEBUG 15303] Loop incrementado para: %coelho_loop%	n
      wait 0.5 sec
    else
      %echo% 	Y✓ Purificação Completa! ✓	n
      %echo% 	c[DEBUG 15303] Nenhum coelho encontrado - saindo do loop	n
      %echo% 	c[DEBUG 15303] Total final de coelhos eliminados: %total_purged%	n
      eval coelhos_loop 20
      %echo% 	c[DEBUG 15303] Loop forçado para 20 (saída)	n
    end
  done
  
  %echo% 	c[DEBUG 15303] === LOOP FINALIZADO ===	n
  
  wait 1 sec
  
  %echo% 	R	bRAIJIN: HAHAHAHA! A PRAGA FOI VARRIDA!	n	n
  wait 1 sec
  
  %echo% 	Y O torii brilha com luz divina restaurada 	n
  wait 1 sec
  
  %echo% 	R	bRAIJIN: QUE ISSO SIRVA DE LIÇÃO A TODOS OS COELHOS MISERÁVEIS!	n	n
  %echo% 	R	bRAIJIN: NENHUMA CRIATURA INSIGNIFICANTE PROFANARÁ MEUS DOMÍNIOS!	n	n
  
  wait 1 sec
  
  %echo% 	Y O torii retorna ao seu estado sagrado, protegido pela ira de Raijin 	n
  %echo% 	Y A PURIFICAÇÃO DIVINA FOI CONCLUÍDA COM ÊXITO	n
  
  %echo% 	c[DEBUG 15303] === TRIGGER FINALIZADO COM SUCESSO ===	n
  
else
  * Nenhum coelho detectado
  %echo% 	c[DEBUG 15303] Nenhum coelho detectado - trigger inativo	n
  
end
~
$~
