RFC-1002 — Memória Emocional de Longo Prazo (MALP / MPLP)

Autor: Equipe de Arquitetura VitaliaMUD Reborn
Data: 2026-03-03
Status: Proposta para revisão técnica e aprovação de arquitetura


---

1. Resumo executivo

Esta RFC define um modelo técnico e cognitivo para Memória Ativa de Longo Prazo (MALP) e Memória Passiva de Longo Prazo (MPLP) para o sistema emocional do VitaliaMUD Reborn. O desenho combina evidências da psicologia cognitiva, neurociência e psiquiatria — em particular mecanismos de consolidação e reconsolidação, papel da amígdala/hipocampo, influência do sono e distinção entre memórias explícitas e implícitas — com a arquitetura já existente do projeto (SEC, OCEAN, appraisal, buffer episódico com decay).   

Objetivos principais:

Formalizar formatos de dados, APIs e regras de consolidação/reconsolidação.

Garantir coerência com o modelo SEC + OCEAN já implementado. 

Fornecer parâmetros padrão testáveis e um plano de validação.



---

2. Motivação / Justificativa

Atualmente o motor já possuão/WTA, appraisal contextual, Big Five parcial (N, C, O, A, E) e um buffer episódico com decay e intensidade. Esses componentes permitem que a camada de consolidação (como uma forma de “ aprendizagem social estável”) seja implementada de forma coerente sem refatorações profundas. 

A MALP/MPLP objetiva:

1. Converter eventos episódicos de alta salience em traços sociais estáveis (ex.: “jogador X é confiável”). mplícitas que modifiquem respostas automáticas (ex.: aproximação/evitação, aumento de arousal em presença de cue).


2. Preservar propriedades psicológicas: salience dependente de arousal, reconsolidação ao reativar, e maior persistência para eventos traumáticos. 




---

3. Premissas cognitivas (invariantes)

1. Arousal × Valence: A consolidação depende fortemente do arousal emocional (amygdala modulation). 


2. Dual-store: Episódico (slots com intensidade e decay) e semântico/traço (consolidação gradual) coexistem; procedimentos derivam de sistemas de memória humanos. 


3. Reconsolidação: Reativação torna memória maleável por uma janela temporária; novas experiências podem atualizar valência/intensidade. 


4. Separação de responsabilidades: OCEAN continua como camada de traço de personalidade (modula ganho/decay), não é substituído por MALP/MPLP. 




---

4. Definições operacionais

MALP (Memória Ativa de Longo Prazo): Representações episódicas/semânticas acessíveis deliberadamente (recuperáveis via get_malp(...)), usadas em diálogos/planejamento social.

MPLP (Memória Passiva de Longo Prazo): Traços implícitos que afetam comportamento automático (modificadores de decisão, thresholds, approach/avoid). Não são narrados direta (S):** Escalar [0..1] calculado a partir de arousal, repetição, grupo social, e característica de evento (major/minor).

Cue Match Score: Similaridade entre contexto atual e tags de memória (local, olfato, agente, frase).

Intensity / Decay: Valor float associado a cada slot; decai por função exponencial/power-law conforme o changelog já define. 



---

5. Arquitetura proposta (visão alto-nível)

1. Camada episódica (existente) — continua como buffer rápido (EMOTION_MEMORY_SIZE, já em uso). Esses slots permanecem com intensity e age. 


2. Consolidador (novo serviço assíncrono/funcional por tick) — avalia slots episódicos e, quando salience >= θ_cons, transfere/integra traços para MALP e/ou MPLP*aços (MALP/MPLP) — storage persistente (pfile/worldfile/db) com índices por agent_id, context_tag, sensory_tag.


3. Reconsolidação hook — ao retrieve de um MALP, abre janela TTL (em ticksção suave da entrada (valence/integrity).


4. Query layer — funções de alta eficiência: get_malp_by_agent(agent_id, cue_context), get_mplp_modifiers(context).



> Observação: o consolidator pode executar durante ticks regulares (não bloqueante), mas a RFC exige regras determinísticas (abaixo) — não execução ad-hoc sem limites.




---

6. Formato de dados (exemplo JSON para cada registro MALP/MPLP)

MALP entry

{
  "malp_id":"uuid",
  "owner":"mob_1234",
  "source_slots":["slot_uuid","slot_uuid"],
  "timestamp_created":"2026-03-03T18:00:00Z",
  "valence": -0.72,               // -1..+1
  "arousal": 0.92,               // 0..1
  "salience": 0.89,              // 0..1 (calc)
  "context_tags":["tavern","rain"],
  "sensory_tags":["smell_burnt","song_guitar"],
  "social_agents":["player_55"],
  "narrative":"Player_55 roubou meu amuleto",
  "rehearsal": 2,
  "last_retrieved":"2026-03-04T01:10:00Z",
  "decay_model":"powerlaw",
  "intensity":0.86,
  "reconsolidation_locked_until":null,
  "meta":{
     "major_event": true,
     "evidence_count":3
  }
}

MPLP entry (trait / implicit)

{
  "mplp_id":"uuid",
  "owner":"mob_1234",
  "anchor_cues":["smell_burnt","player_class_thief"],
  "trait_type":"avoidance",
  "magnitude":0.40,             // multiplier on approach-avoidance decisions
  "valence": -0.6,
  "last_updated":"2026-03-04T01:10:00Z",
  "persistence": "high",        // low/medium/high (configurable via decay params)
  "linked_malp":["malp_uuid"]
}


---

7. Regras e algoritmos (implementáveis)

7.1 Cálculo de Salience (S)

S = normalize( w_a * arousal + w_r * log(1 + rehearsal) + w_s * social_weight ) − decay_component

Sugestão inicial: w_a=0.5, w_r=0.25, w_s=0.25.

decay_component = age_factor(t) usando power-law (mais humano que exponencial). Evidência: sistemas de consolidação dependentes de replay e de reativação offline (sono). 


7.2 Critério de Consolidação (episódico → MALP/MPLP)

Se S >= θ_cons então:

Cria/atualiza MALP entry com integração incremental (peso baseado em S).

Para eventos com major_event==true ou arousal ≥ 0.85, set persistence = high.
Sugestão inicial θ_cons = 0.65.


7.3 Geração de MPLP (traços implícitos)

Regra Hebb-simplificada:

Se o mesmo cue co-ocorre N vezes (configurável) com valência consistente, incrementar mplp.magnitude por delta proporcional a média de S.

Alta Neuroticism (N) reduz decay da MPLP negativa (ruminação). OCEAN já modela esse comportamento e deve modular persistence. 


7.4 Recuperação (P_ret)

P_ret = sigmoid( k * (S + cue_match_score + trait_bias) )

cue_match_score = soma ponderada de matches entre context_tags, sensory_tags, social_agents.

trait_bias = contribuição de MPLP (por exemplo avoidance aumenta P_ret de memórias negativas quando o cue aparece).


7.5 Reconsolidação

Ao retrieve com P_ret >= θ_react abre-se window_recon_ms (configurável; ex.: 30000–600000 ms em tempo de jogo).

Dentro da janela,em atualizam o MALP por formula: new_valence = (1-λ_upd)*old_valence + λ_upd*delta_valence com λ_upd dependente de salience e rehearsal.

Recomenda-se limitar magnitude de mudança por reativação (proteção contra inversões abruptas). Evidence: reconsolidation literature mostra janela limitada e condições boundary para alteração. 


7.6 Decay / Forgetting

MALP: power-law decay on intensity. Parâmetros ajustáveis por persistence (major events: τ maior). Implementação compatível com decay_emotion_memories já no log. 

MPLP: decai mais lentamente, especialmente se ligado a traumas (não zerar rápido). Alta C reduz plasticidade de traços positivos; alta N aumenta persistência de traços negativos.



---

8. Integração com subsistemas existentes

SEC pipeline: Todas as mudanças emocionais derivadas de MALP/MPLP aplicam-se via adjust_emotion() seguindo pipeline (EI scaling → N gain → rate limit). Não alterar pipeline; usar canais previstos para modular gains. 

Appraisal: Ao processar input social, appraise_s23uta weight → afeta delta emocional e possivelmente gatilho de encode_memory()`. 

Shadow Timeline / Decision Engine: Ao pontuar ações, incluir mplp.modifiers no scoring (avoidance reduces approach action scores, etc.).

OLC / CEDIT: Expor thresholds (θ_cons, λ_upd, decay params) em cedit para balanceamento dinâmico; o changelog já mostra prática de expor parâmetros OCEAN via CEDIT — seguir padrão. 25propostas de funções)

encode_emotional_slot(owner, event_struct) — cria slot episódico (já existente).

consolidator_tick(owner) — avalia slots e console regras.

get_malp_by_agent(owner, agent_id, cues) → lista ordenada por P_ret.

get_mplp_modifiers(owner, context) → dict {avoidance: +0.3, arousal_bias: +0.1}

retrieve_and_reconsolidate(malp_id, new_event) → aplica reconsolidação segura.


(Fornecer stubs C / pseudocode com assinatura compatível ao estilo do código base se aprMecanismo de testes e validação

10.1 Testes unitários

criar fixtures de eventos com variação em arousal, repetição e social_weight; verificar que S cresce conforme esperado.

simular reativação + intervenção (ex.: desculpa do jogador) e checar que valence atualiza incrementalmente e que reconsolidation_locked_until expira.


10.2 Testes de integração

cenário: NPC testemunha roubo (major_event) → registra MALP (alto S) → jogador faz repetidas ações reparadoras → medir redução de valence em X sessões.

comparar comportamento de mobs com N alto vs N baixo (ruminação negativa vs perdão).


10.3 Métricas observáveis

taxa de conversão episodic→malp por evento tipo.

false-positive de trait generation (quantas MPLP criadas com magnitude > threshold).

impacto em diálogo: % de consultas que apresentam memória de um jogador.



---

11. Limites, riscos e salvaguardas

Saturação de memória: limite de MALP por mob (ex: 200 entries); poda por menor salience. 

Alteração narrativa abrupta: limitar magnitude de reconsolidação por janela e requerer múltiplas reativações para reversão completa. (Evita NPCs perdoando ou odiando instantaneamente.) 

Viés/overfitting social: impedir que um único agente cause traço global sem evidência (exigir evidence_count ou multiple agent witnesses para generalização).

Ética e abuso: evitar que jogadores manipulem MALP/MPLP para “armazenar” conteúdos sensíveis; logs de dev e ferramentas de limpeza (admin) para removicas.



---

12. Plano de implementação (primeira release)

Fase A — API e armazenamento (2 sprints)

Implementar schema MALP/MPLP persistente + indices (agent/context).

Expor cedit para parâmetros principais (θ_cons, decay τ, recon_window, thresholds).


Fase B — Consolidator & Retrieval (2 sprints)

Implementar consolidator_tick() e get_* APIs; integrar com appraise_social_context().


Fase C — Reconsolidação & Rollout limitado (1–2 sprints)

Implementar reconsolidation hook com safe-guards; habilitar em zonas sandbox.

Testes de integração e AB-tests com grupos de mobs (low-N vs high-N).


Fase D — Tunagem e observability (1 sprint)

Expor métricas, dashboards dev (salience distribution, MALP counts).

Ajustes de parâmetros com base em testes.



---

13. Migração / compatibilidade

Migrar conteúdo do buffer episódico atual (slots com intensity) para MALP candidate pool; converter intensity → inicial salience. 

Legacy: manter compatibilidade com stat mob e com EMOTION_MEMORY_SIZE (exibir MALP counts).



---

14. Considerações clínicas e científicas (resumo bibliográfico)

Importante — as seguintes bases sustentam decisões de projeto:

James L. McGaugh — amígdala modula consolidação emocional; arousal facilita armazenamento duradouro. 

Jan Born — papel do sono (replay, SWS) na transferência hippocampo→neocórtex (sustentação da consochael M. Bradley — amígdala ativa durante encoding/retrieval de estímulos emocionais, justifica peso do arousal. 

L. Astill Wright — estado atual das terapias de reconsolidação em PTSD; confirma relevância de manipular janelas de reativação. 

L. Schwabe — reconsolidação como mecanismo de alteração de memórias humanas; boundary conditions. 


(Referências listadas aqui são exemplares; ver seção de referências completa abaixo.)


---

15. Segurança, privacidade e ética

Memórias não devem conter conteúdo sensível de jogadores (proteger PII). Filtrar/mascarar strings de entrada que contenham IPs/endereços/nomes reais.

Fornecer utilitário admin forget_malp(owner, malp_id) e anonymize_malp(owner) para remoção/edição manual.

Documentar no repositório e no changelog quando memórias forem podadas/reescritas (audit logs).



---

16. Métricas de sucesso (KPIs)

Conversão episodic→MALP em ambientes de teste: alvo inicial 5–10% para eventos de médio S; 50–70% para major events.

Redução de comportamento incoerente (contradictory acts/tick) medida depois da integração: manter <1% de ocorrências por semana.

Taxa de falsos positivos de MPLP (criação de trait por evento isolado): <2%.



---

17. Referências (selecionadas)











Implementações e estado atual do projeto: changelog VitaliaMUD Reborn (memória episódica, decay, OCEAN integration). 



---

18. Ação solicitada / próximos passos

1. Aprovação conceitual da RFC-1002 por arquitetos do projeto.


2. Escolher parâmetros iniciais (θ_cons, decay τ_major/τ_std, recon_window) ou autorizar valores sugeridos do documento.


3. Planejar sprint A (API + armazenamento) com owner técnico (sugerido: equipe ai-mood).




---

Apêndice A — Valores iniciais sugeridos (ratchet para tuning)

θ_cons = 0.65

w_a=0.5, w_r=0.25, w_s=0.25

rehearsal_threshold_for_trait = 3

recon_window_ticks = 60 (equivalente a ~1min de jogo-time)

malp_limit_per_mob = 200

mplp_decay_half_life_standard ≈ 24 horas, major_event_half_life ≈ 72 horas (ajustáveis) — coerente com half-life macro adotada no changelog.
