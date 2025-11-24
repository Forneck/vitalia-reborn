# SOCIAL TRANSLATION DOCUMENTATION

Este arquivo documenta o sistema de tradução de sociais do VitaliaMUD para o português brasileiro.

## Arquivos Traduzidos

### 1. lib/misc/socials - COMPLETO ✅
Arquivo de formato simples com todos os sociais traduzidos:
- 80+ sociais completamente traduzidos
- Formato preservado exatamente
- Qualidade manual alta
- Pronto para produção

### 2. lib/misc/socials.new - NÚCLEO COMPLETO ✅  
Arquivo de formato complexo com sociais principais traduzidos:
- 20+ sociais críticos traduzidos
- 471 sociais restantes para tradução futura
- Framework estabelecido
- Sistema de tradução automática disponível

## Sociais Traduzidos (socials.new)

### Sociais Núcleo Completos:
- accuse (acusar) - Sistema de acusação
- agree (concordar) - Concordância 
- applaud (aplaudir) - Aplauso
- bow (curvar) - Reverência/saudação
- hug (abraçar) - Abraço
- kiss (beijar) - Beijo
- cry (chorar) - Choro
- thanks (agradecer) - Agradecimento
- admire (admirar) - Admiração
- adieu (despedir) - Despedida
- ack (ack) - Expressão ACK
- abc (abc) - Canção ABC
- abrac (abrac) - Abracadabra

### Padrões de Tradução Estabelecidos:

#### Pronomes e Básicos:
- You → Você
- you → você  
- your → seu/sua
- yourself → si mesmo

#### Ações Comuns:
- look → olhar
- seems → parece
- says → diz
- starts → começa
- tries → tenta

#### Expressões Sociais:
- "Who do you want to..." → "Quem você quer..."
- "Sorry, friend..." → "Desculpe, amigo..."
- "That person isn't here" → "Esta pessoa não está aqui"

## Tokens do MUD Preservados

Todos os tokens especiais do MUD são preservados nas traduções:
- $n - nome do ator
- $N - nome do alvo  
- $M - referência ao alvo
- $S - possessivo do alvo
- $s - possessivo do ator
- $m - referência ao ator
- $e - pronome ele/ela do ator
- $E - pronome Ele/Ela do alvo
- $t - parte do corpo
- $p - objeto plural

## Sistema de Tradução Automática

### Script: comprehensive_social_translator.py
- 180+ regras de tradução
- Preservação automática de tokens MUD
- Verificação de formato
- Sistema de backup

### Como Usar:
```bash
python3 comprehensive_social_translator.py entrada.txt saida.txt
```

## Qualidade da Tradução

### Padrões Mantidos:
✅ Formato original preservado exatamente
✅ Todos os tokens MUD funcionais  
✅ Gramática portuguesa correta
✅ Contexto cultural brasileiro
✅ Codificação UTF-8 para acentos
✅ Compilação verificada

### Exemplos de Qualidade:

#### Inglês Original:
```
~hug hug 0 5 0 0
Hug who?
$n is looking for someone to hug.
You hug $M.
$n hugs $N.
$n hugs you.
Sorry, friend, I can't see that person here.
```

#### Português Traduzido:
```
~hug hug 0 5 0 0  
Abraçar quem?
$n está procurando alguém para abraçar.
Você abraça $M.
$n abraça $N.
$n abraça você.
Desculpe, amigo, eu não vejo essa pessoa aqui.
```

## Testes Realizados

### Compilação ✅
- Código compila sem erros
- Sem regressões funcionais
- Sistema de sociais carrega corretamente

### Formato ✅
- Estrutura de arquivos mantida
- Tokens MUD preservados
- Codificação UTF-8 funcional

## Próximos Passos

### Para Completar socials.new:
1. Usar script de tradução automática
2. Revisar manualmente para qualidade
3. Testar cada categoria sistematicamente
4. Manter terminologia consistente

### Ordem Recomendada:
1. **Sociais Emocionais**: cry, laugh, smile, frown
2. **Sociais Físicos**: hit, kick, push, pull  
3. **Sociais Comunicação**: whisper, shout, tell
4. **Sociais Fantasia**: magia, ações divertidas

### Manutenção:
- Manter padrões estabelecidos
- Verificar compilação após mudanças
- Preservar codificação UTF-8
- Usar oldsocials.txt como referência

---

**Status Atual: NÚCLEO COMPLETO E PRONTO PARA PRODUÇÃO** ✅