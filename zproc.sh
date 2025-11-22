#!/bin/bash

# --- Configuração ---
# Caminho para o seu utilitário
UTILITY="./bin/textextract"

# Diretório base onde as pastas wld, mob e obj estão
BASE_DIR="lib/world"

# Arquivo de saída final
OUTPUT_FILE="training_text.txt"

# --- Verificações Iniciais ---

# 1. Verificar se o utilitário existe e é executável
if [ ! -x "$UTILITY" ]; then
    echo "Erro: O utilitário não foi encontrado ou não é executável em $UTILITY"
    exit 1
fi

# 2. Verificar se os diretórios de entrada existem
WLD_DIR="$BASE_DIR/wld"
MOB_DIR="$BASE_DIR/mob"
OBJ_DIR="$BASE_DIR/obj"

if [ ! -d "$WLD_DIR" ] || [ ! -d "$MOB_DIR" ] || [ ! -d "$OBJ_DIR" ]; then
    echo "Erro: Um ou mais diretórios ($WLD_DIR, $MOB_DIR, $OBJ_DIR) não foram encontrados."
    exit 1
fi

# --- Processamento Principal ---

# Limpa (ou cria) o arquivo de saída para esta execução.
# O utilitário irá "anexar" (append) a este arquivo limpo.
echo "Iniciando extração... Criando/Limpando $OUTPUT_FILE"
> "$OUTPUT_FILE"

# O loop principal itera sobre os arquivos .wld
# Usamos 'find' para melhor robustez e 'sort -n' para garantir a ordem numérica
find "$WLD_DIR" -maxdepth 1 -name "*.wld" | sort -n | while read wld_file; do

    # Extrai apenas o nome do arquivo (ex: "30.wld")
    filename=$(basename "$wld_file")
    
    # Extrai apenas o número (ex: "30")
    number="${filename%.wld}"

    echo "Processando ID: $number"

    # --- 1. Processar arquivo WLD ---
    # (Sabemos que este existe porque o loop 'find' o encontrou)
    echo "  -> Processando $wld_file"
    "$UTILITY" wld "$wld_file" "$OUTPUT_FILE"

    # --- 2. Processar arquivo MOB ---
    mob_file="$MOB_DIR/${number}.mob"
    if [ -f "$mob_file" ]; then
        echo "  -> Processando $mob_file"
        "$UTILITY" mob "$mob_file" "$OUTPUT_FILE"
    else
        echo "  -> Ignorando: $mob_file (não encontrado)"
    fi

    # --- 3. Processar arquivo OBJ ---
    obj_file="$OBJ_DIR/${number}.obj"
    if [ -f "$obj_file" ]; then
        echo "  -> Processando $obj_file"
        "$UTILITY" obj "$obj_file" "$OUTPUT_FILE"
    else
        echo "  -> Ignorando: $obj_file (não encontrado)"
    fi

done

echo "---"
echo "Processamento concluído!"
echo "Todos os dados foram anexados em $OUTPUT_FILE"

