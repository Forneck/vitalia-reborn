echo "Removendo ~ com espaço"
find ./* -type f -exec sed -i 's/~ /~/g' {} \;
echo "feito!" 