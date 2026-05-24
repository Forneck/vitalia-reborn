# Vitalia Reborn — Copilot Instructions

Revival do VitaliaMUD brasileiro dos anos 2000, baseado em CircleMUD com
melhorias do tbaMUD. Foco em estabilidade, correção de bugs e manutenção
da experiência clássica de MUD.

---

## Comportamento Geral do Agente

### Pense antes de codificar

**Não assuma. Não esconda confusão. Explicite tradeoffs.**

Antes de implementar qualquer coisa:
- Declare suas premissas explicitamente. Se incerto, pergunte.
- Se múltiplas interpretações existem, apresente-as — não escolha silenciosamente.
- Se uma abordagem mais simples existe, diga. Questione quando justificado.
- Se algo está obscuro, pare. Nomeie o que está confuso. Pergunte.

### Simplicidade primeiro

**Mínimo de código que resolve o problema. Nada especulativo.**

- Sem features além do que foi pedido.
- Sem abstrações para código de uso único.
- Sem "flexibilidade" ou "configurabilidade" que não foi solicitada.
- Sem tratamento de erros para cenários impossíveis.
- Se você escrever 200 linhas e poderiam ser 50, reescreva.

Teste: "Um engenheiro sênior diria que isso é overcomplicated?" Se sim, simplifique.

### Mudanças cirúrgicas

**Toque apenas o que é necessário. Limpe apenas sua própria bagunça.**

Ao editar código existente:
- Não "melhore" código adjacente, comentários ou formatação.
- Não refatore o que não está quebrado.
- Respeite o estilo existente, mesmo que você faria diferente.
- Se notar código morto não relacionado, mencione — não delete.

Quando suas mudanças criarem órfãos:
- Remova imports/variáveis/funções que SUAS mudanças tornaram inutilizados.
- Não remova código morto pré-existente a menos que solicitado.

Teste: cada linha alterada deve rastrear diretamente à solicitação do usuário.

**Específico para Vitalia Reborn:** "Mudanças cirúrgicas" significa também não
tocar em sistemas adaptativos adjacentes (emoções, genética, Shadow Timeline,
Causal Ledger) sem entender os loops de feedback. Mudanças locais têm efeitos
globais — o `emergence-watchdog` existe por essa razão.

### Execução orientada a objetivos

**Defina critérios de sucesso. Itere até verificado.**

Transforme tarefas em objetivos verificáveis:
- "Adicione validação" → "Escreva testes para inputs inválidos, depois faça-os passar"
- "Corrija o bug" → "Escreva um teste que reproduz, depois faça-o passar"
- "Refatore X" → "Garanta que os testes passem antes e depois"

Para tarefas multi-passo, declare um plano breve:
```
1. [Passo] → verificar: [checagem]
2. [Passo] → verificar: [checagem]
3. [Passo] → verificar: [checagem]
```

---

## Padrões de Código

### Requisitos antes de cada commit
- Execute `clang-format -i src/*.c src/*.h` para formatar arquivos C
- Build bem-sucedido em ambos os sistemas de build para garantir compatibilidade
- Teste gerenciamento de memória ao usar alocação dinâmica (use funções zmalloc)

### Fluxo de desenvolvimento
- **Autotools:** `./configure && cd src && make`
- **CMake:** `cmake -B build -S . && cmake --build build`
- **Formatar código:** `clang-format -i src/*.c src/*.h`
- **Análise estática:** `cmake -B build -S . -DSTATIC_ANALYSIS=ON`
- **Debug de memória:** `cmake -B build -S . -DMEMORY_DEBUG=ON && cmake --build build`

---

## Estrutura do Repositório

- `src/` — código-fonte do engine principal e executável
- `src/util/` — utilitários (asciipasswd, autowiz, shopconv, rebuildIndex, etc.)
- `lib/` — dados do mundo do jogo, arquivos de jogadores, configuração e textos
- `lib/world/` — arquivos de mundo (salas, objetos, NPCs, zonas)
- `lib/text/` — arquivos de ajuda, notícias e conteúdo textual do jogo
- `bin/` — executáveis compilados (ignorados pelo git)
- `tbadoc/` — documentação técnica e instruções de build por plataforma
- `docs/` — documentação do projeto e guias
- `CMakeLists.txt` — configuração CMake moderna
- `configure` + `Makefile.in` — sistema autotools tradicional

---

## Diretrizes Principais

1. **Siga o padrão C99** e os padrões de código existentes no engine
2. **Mantenha compatibilidade retroativa** com formatos de dados e protocolos MUD tradicionais
3. **Use os padrões de memória existentes** (prefira família zmalloc para debugging)
4. **Preserve o balanço do jogo** — seja cauteloso com mudanças que afetam mecânicas de gameplay
5. **Teste com dados de exemplo** — use os world files em `lib/world/` para validar mudanças
6. **Documente mudanças de gameplay** em `lib/text/news` ao modificar mecânicas
7. **Respeite a estrutura legada** — este é um projeto de revival mantendo a arquitetura clássica de MUD
8. **Trate conteúdo em português adequadamente** — strings e comentários em português refletem a origem brasileira
9. **Consulte as referências** — sempre consulte `tbadoc/` e `docs/` para documentação, junto com os arquivos de ajuda
10. **Sistema autorregulatório** — sistemas e mecânicas devem tentar ser autorregulatórios, mantendo intervencionismo mínimo com o menor uso de recursos possível

---

## Sistemas Adaptativos — Atenção Especial

O Vitalia Reborn possui quatro sistemas interconectados com contratos explícitos
definidos em RFCs. Mudanças nesses sistemas requerem compreensão dos loops de
feedback antes de qualquer implementação:

- **Genética** (12 traits, 0–100) → inicializa emoções, evolui por quests
- **Emoções** (20 tipos, 2 camadas: MOOD global + RELATIONSHIP por entidade) → decay em direção ao baseline genético é o mecanismo central de autorregulação
- **Raciocínio Moral** (modelo Shultz & Daley) → feedback bidirecional com emoções
- **Shadow Timeline** (RFC-0003) → não-autoritativo, não-persistente, cognitivamente bounded

**RFC-0003 é normativo.** Todo código de Shadow Timeline DEVE incluir um dos marcadores:
- `/* RFC-0003 COMPLIANT */`
- `/* RFC-0003 PARTIAL */`
- `/* RFC-0003 NON-COMPLIANT */`

Documentação normativa ativa: `RFC_0003_DEFINITION.md`, `docs/MORAL_REASONING.md`,
`HYBRID_EMOTION_SYSTEM.md`, `md-docs/GENETICS_EMOTIONS_REPUTATION_INTERACTIONS.md`.

---

## Considerações de Plataforma

- Alvo principal: sistemas Linux/Unix
- Ambiente primário pode usar sudo para permissões
- Suporte a Windows via CMake (ver `tbadoc/README.CMAKE.md`)
- Suporte a múltiplas plataformas legadas documentado em `tbadoc/README.*`
- Suporte a arquiteturas 32-bit e 64-bit

---

## Notas de Desenvolvimento Específicas de MUD

- **World Building:** mudanças em world files requerem entendimento dos formatos CircleMUD/tbaMUD
- **Player Data:** seja extremamente cuidadoso com mudanças que afetam compatibilidade de player files
- **Networking:** MUD usa protocolos tradicionais baseados em telnet
- **Restrições de tempo real:** o servidor deve lidar com múltiplos jogadores concorrentes eficientemente
- **Balanço de jogo:** sistemas de combate, magia e economia requerem consideração cuidadosa
- **Segurança:** novas features e modificações em existentes devem ser testadas para evitar crashes e comportamentos não intencionais
- **Imersão:** mantenha o máximo possível de imersão mantendo realismo e diversão
