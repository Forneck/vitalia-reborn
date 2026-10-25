# RFC-0000 — Foundational Constraints
## Vitalia Reborn MUD Engine — Causal Ledger

**Status:** Proposta para revisão técnica  
**Tipo:** Arquitetura / Restrições Fundamentais  
**Precede:** RFC-0001 (Ledger Implementation), RFC-0002 (Retention Policy)  
**Relacionados:** RFC-0003 (Shadow Timeline), RFC-1002 (MALP/MPLP), Causal Ledger Audit v4.0  
**Data:** 2026-05-24  
**Versão:** 1.0  

---

## Propósito

Este RFC estabelece as restrições fundamentais que governam o Causal Ledger antes de qualquer decisão de implementação. Ele existe para impedir dissipação arquitetural — a tendência de sistemas complexos expandirem indefinidamente em abstração sem produzir código executável.

**Princípio metodológico central:** novos insights devem reduzir incerteza dentro da estrutura existente, não criar novas estruturas paralelas. Absorver, não empilhar.

Cada bloco estabelece restrições sobre o bloco seguinte. As perguntas devem ser respondidas nesta ordem.

---

## Bloco 0 — Instrumentabilidade (Pré-requisito)

Estas perguntas devem ser respondidas pela equipe de código antes de qualquer outra decisão, pois determinam se o restante do RFC é aplicável ao engine atual ou a uma versão futura hipotética.

**0.1** O engine atual pode emitir eventos do Ledger sem refatoração profunda, ou o Ledger pressupõe uma versão futura do engine?

**0.2** Onde existem pontos de interceptação naturais no código atual?  
Candidatos esperados: `fight.c:die()`, `limits.c:gain_exp()`, `quest.c:generic_complete_quest()`, `shop.c`, `objsave.c:Crash_rentsave()`. Confirmar ou corrigir esta lista.

**0.3** Quais subsistemas têm efeitos colaterais implícitos não capturáveis por hooks externos?  
Identificar especificamente: flags mutadas em locais dispersos, scripts com side effects, estados parcialmente persistidos, ordem de execução contextual.

**0.4** Quais rotinas têm ordering de execução estável por tick, e quais são dependentes de contexto de execução?

**0.5** Qual é o custo estimado de adicionar logging passivo (sem alterar lógica) nos pontos de emissão candidatos?

> **Critério de desbloqueio:** O Bloco 1 só pode ser respondido após a equipe de código produzir um relatório de instrumentabilidade respondendo 0.1–0.5 com referências a arquivos e funções específicas do codebase atual.

---

## Bloco 1 — Autoridade e Escopo

Estas perguntas definem o que o Ledger é antes de qualquer decisão técnica.

**1.1 O Ledger é online-critical ou forensic-only?**

*Online-critical:* sistemas do engine dependem dele em tempo real para tomar decisões.  
*Forensic-only:* consultado apenas para auditoria, narrativa e replay posterior.

**Posição recomendada:** Forensic-first. Transformar Circle/tbaMUD legado em sistema ledger-native é equivalente a reescrever o engine. O Ledger deve começar como mirror append-only, não como sistema autoritativo de runtime.

Esta decisão afeta: latência aceitável, disponibilidade requerida, tolerância a falha, custo de migração.

**1.2 Quem é o consumidor primário do Ledger?**

Ordenar por prioridade:
- [ ] Engine (decisões de gameplay em tempo real)
- [ ] MALP/SEC (memória e emoção de NPCs)
- [ ] Sistemas narrativos (crônicas, títulos, história emergente)
- [ ] Administradores (auditoria e debugging)

A ordem define quais queries precisam ser rápidas e quais podem ser lentas.

**1.3 O Ledger é fonte de verdade ou espelho dela?**

*Fonte de verdade (ledger-native):* o engine deriva estado do Ledger.  
*Espelho (mirror):* o engine mantém estado autoritativo; o Ledger registra o que aconteceu.

**Posição recomendada:** Espelho append-only no estado atual. A bifurcação para ledger-native é uma decisão de engine futuro, não do Ledger atual.

**1.4 Qual é a política de bootstrapping?**

O mundo tem história anterior à ativação do Ledger. Três posições possíveis:

*Ledger Day Zero:* história auditável começa na ativação. Simples, limpo, perde historicidade acumulada.  
*Retroactive reconstruction:* tentar reconstruir eventos passados a partir do estado atual. Parcialmente possível para alguns subsistemas (level history, quest flags), impossível para outros (combat events, corpse history).  
*Explicit Amnesia:* declarar formalmente que o mundo tem história não-auditável antes de uma data (`audit_epoch_begin`), e o Ledger é autoritativo apenas a partir dela.

**Posição recomendada:** Explicit Amnesia. É a única opção epistemologicamente honesta. Retroactive reconstruction produziria pseudo-história com falsas garantias de completude. A distinção Pre-Ledger Era / Ledger Era deve ser explícita no modelo de dados.

> **Invariante 1.A:** O Ledger não pode esquecer causalidade ainda ativa. Um evento cujas consequências permanecem observáveis no mundo atual não pode ser deletado sem snapshot âncora verificável.

> **Invariante 1.B:** O sistema não pode afirmar auditabilidade que não possui. Se um evento foi deletado, ele não é "era pré-ledger" — é violação de integridade. A distinção entre "nunca tivemos esse dado" e "tivemos e deletamos" deve ser preservada e nunca confundida.

---

## Bloco 2 — Semântica de Replay

**2.1 Replay precisa ser bit-perfect ou semantically equivalent?**

*Bit-perfect:* reproduzir exatamente o mesmo estado byte a byte. Exige RNG seed logging, ordering determinístico, elimina praticamente toda otimização. Provavelmente inviável no engine atual.  
*Semantically equivalent:* os efeitos observáveis são os mesmos; o caminho interno pode variar.

**Posição recomendada:** Semantically equivalent como meta, com verificação por equivalence class (ver 2.2).

**2.2 O que constitui divergência de replay inaceitável?**

Precisa haver definição formal de equivalência antes de qualquer teste de determinismo. Exemplos:

| Divergência | Aceitável? |
|---|---|
| Posição final diferente em 1 tile | Provavelmente sim |
| XP final diferente em ≤ 50 pontos | A definir |
| Morte vs. sobrevivência | Não |
| Item presente vs. ausente no inventário | Não |
| Reputação NPC diferente em ≤ 5% | Provavelmente sim |

A equipe de código deve completar esta tabela para os subsistemas prioritários.

**2.3 Quais subsistemas precisam de garantia de determinismo forte?**

Categorização inicial proposta — confirmar ou corrigir:

| Subsistema | Nível requerido | Justificativa |
|---|---|---|
| Economia (Value Transfer) | Forte | Conservação de valor |
| Morte / Combat outcomes | Forte | Irreversibilidade |
| Quest completion | Forte | Permissões permanentes |
| Level advancement | Forte | Monotônico por definição |
| Reputação / Emoção NPC | Eventual | Convergência aceitável |
| Zone resets | Eventual | Determinístico por schedule |

**2.4 Replay Confidence Grades**

Todo replay produz um resultado classificado. A classificação determina o uso permitido do resultado:

| Grade | Definição | Uso permitido |
|---|---|---|
| `VERIFIED` | Replay bit-equivalent ao original | Admissível em auditoria formal |
| `SEMANTICALLY_EQUIVALENT` | Efeitos observáveis idênticos por equivalence class | Admissível em queries narrativas |
| `DEGRADED` | Divergência menor em estado não-autoritativo | Admissível com flag; requer revisão |
| `NON_REPLAYABLE` | Divergência em estado autoritativo | Invalida branch para fins de audit; requer compensatory event |

**2.5 O que acontece operacionalmente quando replay diverge?**

Definir protocolo para cada grade antes de qualquer implementação. `NON_REPLAYABLE` em subsistema de determinismo forte deve ter protocolo de escalação definido — não pode ser silencioso.

> **Critério de desbloqueio:** O Bloco 3 requer que a equipe de código execute medição real de event throughput por subsistema durante pelo menos 5 dias de operação normal. Estimativas calculadas não são aceitas como baseline.

---

## Bloco 3 — Granularidade e Volume

**3.1 Qual o volume real de eventos por tick por subsistema?**

Esta pergunta exige instrumentação, não cálculo. A equipe de código deve adicionar contadores nos pontos de emissão candidatos e medir em operação real. Métricas requeridas:

- Eventos/tick por subsistema (combat, economy, quest, lifecycle)
- Distribuição de tamanho por tipo de evento
- Hotspots de cardinalidade (zonas, subsistemas, horários)
- Frequência de uso de RNG por subsistema

> A estimativa de 250 MB/ano da v4.0 deve ser validada ou corrigida por esta medição antes de qualquer decisão de retenção.

**3.2 Itens: fungíveis ou históricos por padrão?**

Esta é a decisão com maior impacto em volume. A fronteira entre "comum" e "único" é dinâmica — um item comum pode tornar-se historicamente significativo após um evento raro.

Critério formal proposto para item histórico:
- Flag explícita no prototype (`ITEM_UNIQUE`)
- Participação em evento Layer 1 (morte de personagem, quest completion, value transfer acima de threshold)
- Designação administrativa explícita

**Posição recomendada:** Itens comuns são fungíveis e não entram no Ledger individualmente. Itens únicos recebem UUID persistente e entram no Ledger. A decisão de quando um item "vira histórico" deve ter critério formal — sem isso, a fronteira vira decisão arbitrária por caso.

> **Invariante 3.A:** Storage pressure é sintoma de modelagem incorreta na emissão, não de retenção insuficiente. Políticas de deleção não podem ser usadas para compensar granularidade excessiva.

**3.3 Política de retenção por categoria causal**

Retenção deve ser semântica, não temporal. Cada categoria tem um horizonte causal distinto:

| Categoria | Retenção | Política pós-retenção |
|---|---|---|
| Character lifecycle | Permanente enquanto personagem existe | Archive após deleção de personagem |
| Quest completion | Permanente | — |
| Value transfer acima de threshold | Permanente | — |
| Value transfer rotineiro | 1 ano | Snapshot âncora + cold storage |
| Combat com morte | Permanente | — |
| Combat sem morte | 90 dias | Snapshot âncora |
| Zone resets | 30 dias | Descartável após snapshot |

**3.4 Definição formal de snapshot âncora**

Um snapshot âncora não é estado salvo — é prova condensada de historicidade. Para ser considerado âncora válida, deve conter:

- Hash verificável do estado comprimido
- Lista dos event IDs substituídos
- Timestamp e branch_context
- Critério de causal liveness verificado no momento da compressão

> **Invariante 3.B:** Deleção sem snapshot âncora é proibida para qualquer evento com consequências causais ativas verificáveis. Critério de causal liveness: existe alguma entidade no mundo atual cuja propriedade deriva deste evento?

> **Invariante 3.C:** Cold storage é permitido. Compressão com snapshot âncora é permitida. Esquecimento causal não é.

---

## Bloco 4 — Modelo de Branch e Verdade

**4.1 Semântica de verdade**

O Ledger não é uma história única do universo. É um espaço indexado de causalidades locais.

Formalmente: `Truth = (event, branch_context)`

Um evento em branch colapsado existiu como fato local àquele branch. "Colapsado" não significa "nunca aconteceu" — significa "não pertence à linha canônica ativa". Branches colapsados são marcados `branch_collapsed=true` e preservados.

**4.2 Queries são branch-aware por padrão ou opt-in?**

*Branch-aware por padrão:* toda query requer `branch_context` explícito. API mais correta, mais custosa para consumidores.  
*Opt-in com default canônico:* default retorna timeline canônica ativa; branching requer parâmetro explícito.

**Posição recomendada:** Opt-in com default canônico, mas com type discipline rigorosa em contextos temporalmente sensíveis. Defaults errados aqui produzem bugs silenciosos semanticamente catastróficos.

**4.3 Memórias de NPC sobrevivem a branch collapse?**

**Posição recomendada:** Memórias pré-branch são preservadas como-estão. Branch collapse não as invalida. Eventos pós-collapse no branch colapsado são marcados como não-canônicos na cognição do NPC. Esta decisão deve estar fixada antes de qualquer implementação conjunta de MALP e branching.

**4.4 Limite operacional de branches ativos**

Branches ativos simultâneos devem ter limite explícito. Proposta inicial:

- Máximo de branches ativos simultâneos: a definir pela equipe (sugestão: 3–5)
- Branches sem atividade por N dias são auto-colapsados (N a definir)
- Branches não podem ser criados se o limite está atingido sem colapsar um existente

Sem este limite, timelines são conceito narrativo elegante. Com ele, são recurso operacional gerenciável.

---

## Bloco 5 — Governança e Erro

**5.1 Compensatory events: autorização e audit trail**

Compensatory events existem para corrigir erros sem mutar o passado. Todo compensatory event deve:

- Entrar no Ledger como fato imutável
- Registrar: quem autorizou, quando, justificativa, event ID corrigido
- Ser emitido apenas por entidades autorizadas explicitamente

**Posição recomendada:** Autorização unilateral com audit trail obrigatório é suficiente para um MUD. Quorum distribuído é overkill operacional. O compensatory event em si é imutável — a governança é garantida pela rastreabilidade, não pelo consenso.

**5.2 Janela temporal para compensatory events**

Erros descobertos em janelas diferentes têm tratamento diferente. A equipe deve definir:

- Janela imediata (ex: < 1 hora): compensatory event direto
- Janela estendida (ex: 1 hora – 7 dias): compensatory event com revisão obrigatória
- Janela longa (ex: > 7 dias): compensatory event com documentação de impacto causal

**5.3 O que é irrecuperável?**

Deve existir lista explícita de situações onde o Ledger considera o estado corrompido além de compensação. Protocolo para esses casos deve estar definido antes do deploy. Situação irrecuperável não pode ser tratada como caso excepcional durante operação.

---

## Estrutura de Camadas (Referência)

O Ledger opera em camadas com dependências explícitas. Camadas superiores só podem ser construídas após as inferiores estarem estabilizadas:

```
Layer 0A — Primitive Canonical Events
  Primitivos comprovadamente fechados e replayáveis
  (HP delta, object spawn/destroy, variable mutation, location transition)

Layer 0B — Transitional Semantic Anchors
  Eventos semânticos mantidos até fechamento completo do primitive set
  Cada evento em 0B requer: Required Primitive Coverage, Replay Determinism
  Threshold, Allowed Lifetime, Replacement Projection
  (Quest Completion permanece aqui até primitive closure verificado)

Layer 1 — Semantic Events
  (CharacterDeath, QuestArcCompleted, Betrayal)
  Requer: Layer 0A estável

Layer 2 — Narrative Interpretation
  (Títulos, crônicas, história emergente, cognição NPC histórica)
  Requer: Layer 1 estável + Narrative Index com política de
  consistência eventual explícita
```

> **Invariante de dependência:** Nenhuma camada pode assumir capacidades de camadas inferiores não ainda implementadas e verificadas. Narrative Index não é "próxima etapa" — é downstream de primitive closure, deterministic ordering e RNG auditability.

---

## Critérios de Desbloqueio — Resumo

| Bloco | Desbloqueado por |
|---|---|
| Bloco 1 | Relatório de instrumentabilidade (Bloco 0) com referências a código real |
| Bloco 2 | Decisões do Bloco 1 formalizadas |
| Bloco 3 | Medição real de volume (≥ 5 dias de operação) |
| Bloco 4 | Decisões do Bloco 2 sobre semântica de replay |
| Bloco 5 | Decisões dos Blocos 1–4 consolidadas |
| Implementação | RFC-0000 completamente respondido + aprovação de arquitetura |

---

## Questões Abertas para a Equipe de Código

As seguintes questões não podem ser respondidas por análise arquitetural — requerem inspeção do codebase:

1. Existe algum efeito de quest completion que não mapeie para nenhum primitive candidato atual? (Determina se Quest Completion permanece em Layer 0B ou pode ser removido)

2. Quais rotinas em `fight.c`, `limits.c`, `quest.c`, `shop.c`, `objsave.c` têm side effects não-locais que escapariam de hooks de emissão?

3. O campo `obj_script_id()` pode ser extendido para UUID persistente por instância sem quebrar compatibilidade de save format? (Determina viabilidade de item histórico único)

4. Qual é o custo real de adicionar `RNG seed logging` nas chamadas a `number()` nos subsistemas de determinismo forte?

5. Existe algum subsistema onde ordering por tick não é estável e não pode ser tornado estável sem refatoração profunda?

---

## Resumo de Invariantes

| ID | Invariante |
|---|---|
| 1.A | O Ledger não pode esquecer causalidade ainda ativa |
| 1.B | O sistema não pode afirmar auditabilidade que não possui |
| 3.A | Storage pressure é sintoma de emissão incorreta, não de retenção insuficiente |
| 3.B | Deleção sem snapshot âncora é proibida para eventos com causal liveness ativo |
| 3.C | Cold storage e compressão são permitidos; esquecimento causal não é |
| 4.A | Truth = (event, branch_context); branches colapsados existiram como fatos locais |
| 5.A | Todo compensatory event entra no Ledger como fato imutável com audit trail |

---

## Próximos Passos

1. **Equipe de código:** Produzir relatório de instrumentabilidade respondendo Bloco 0 com referências a arquivos e funções específicas
2. **Equipe de código:** Executar instrumentação de medição de volume
3. **Arquitetura:** Revisar este RFC com os resultados dos dois itens acima
4. **Decisão formal:** Responder cada pergunta dos Blocos 1–5 com posição aprovada pela equipe
5. **Próximo documento:** RFC-0001 (Ledger Implementation) começa apenas após RFC-0000 completamente respondido

---

**Autores:** Vitalia Reborn Architecture Team  
**Data:** 2026-05-24  
**Status:** Proposta — aguardando revisão técnica e aprovação  
**Versão:** 1.0
