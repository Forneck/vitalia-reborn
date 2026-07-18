#!/data/data/com.termux/files/usr/bin/bash
# Sincroniza branch local com o remoto, preservando alterações locais
# Autor: Forneck (automatizado por ChatGPT)

echo "📦 [1/6] Criando backup dos arquivos locais..."
BACKUP_DIR="$HOME/backup_vitalia_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Lista de arquivos que costumam causar conflitos
FILES_TO_BACKUP=("confdefs.h" "config.log")

for f in "${FILES_TO_BACKUP[@]}"; do
  if [ -f "$f" ]; then
    cp "$f" "$BACKUP_DIR/"
    echo "  → Backup de $f salvo em $BACKUP_DIR/"
  fi
done

echo "💾 [2/6] Salvando alterações locais (git stash)..."
git stash push -u -m "Auto-stash antes do sync" >/dev/null

echo "🧹 [3/6] Limpando arquivos não rastreados..."
git clean -f -d >/dev/null

echo "🔄 [4/6] Puxando alterações do repositório remoto..."
if ! git pull --rebase; then
  echo "❌ Erro no git pull — verifique conflitos manualmente."
  exit 1
fi

echo "💡 [5/6] Restaurando alterações locais (git stash pop)..."
git stash pop >/dev/null 2>&1 || echo "  Nenhum stash para restaurar."

echo "🚀 [6/6] Sincronização concluída com sucesso!"
echo "Se quiser enviar as alterações, use: git add . && git commit -m 'atualizações' && git push"

