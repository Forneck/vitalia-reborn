echo "Removendo ~ com espaço"
find ./* -type f -exec sed -i 's/~ /~/g' {} \;
echo "feito!"
echo "Removendo D N"
find ./* -type f -exec sed -i 's/D N/D 0/g' {} \;
echo "Removendo DN"
find ./* -type f -exec sed -i 's/DN/D 0/g' {} \;
echo "feito!"
