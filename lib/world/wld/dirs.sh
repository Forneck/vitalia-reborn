echo "Removendo ~ com espaço"
find ./* -type f -exec sed -i 's/~ /~/g' {} \;
echo "feito!"
echo "Removendo D N"
find ./* -type f -exec sed -i 's/D N/D 0/g' {} \;
echo "Removendo DN"
find ./* -type f -exec sed -i 's/DN/D 0/g' {} \;
echo "feito!"
echo "Removendo D E"
find ./* -type f -exec sed -i 's/D E/D 1/g' {} \;
echo "Removendo DE"
find ./* -type f -exec sed -i 's/DE/D 1/g' {} \;
echo "feito!"
echo "Removendo D S"
find ./* -type f -exec sed -i 's/D S/D 2/g' {} \;
echo "Removendo DS"
find ./* -type f -exec sed -i 's/DS/D 2/g' {} \;
echo "feito!"
echo "Removendo D W"
find ./* -type f -exec sed -i 's/D W/D 3/g' {} \;
echo "Removendo DW"
find ./* -type f -exec sed -i 's/DW/D 3/g' {} \;
echo "feito!"
echo "Removendo D U"
find ./* -type f -exec sed -i 's/D U/D 4/g' {} \;
echo "Removendo DU"
find ./* -type f -exec sed -i 's/DU/D 4/g' {} \;
echo "feito!"
echo "Removendo D D"
find ./* -type f -exec sed -i 's/D D/D 5/g' {} \;
echo "Removendo DD"
find ./* -type f -exec sed -i 's/DD/D 5/g' {} \;
echo "feito!"
echo "Removendo D NW"
find ./* -type f -exec sed -i 's/D NW/D 6/g' {} \;
echo "Removendo DNW"
find ./* -type f -exec sed -i 's/DNW/D 6/g' {} \;
echo "feito!"
echo "Removendo D NE"
find ./* -type f -exec sed -i 's/D NE/D 7/g' {} \;
echo "Removendo DNE"
find ./* -type f -exec sed -i 's/DNE/D 7/g' {} \;
echo "feito!"
echo "Removendo D SE"
find ./* -type f -exec sed -i 's/D SE/D 8/g' {} \;
echo "Removendo DSE"
find ./* -type f -exec sed -i 's/DSE/D 8/g' {} \;
echo "feito!"
echo "Removendo D SW"
find ./* -type f -exec sed -i 's/D SW/D 9/g' {} \;
echo "Removendo DSW"
find ./* -type f -exec sed -i 's/DSW/D 9/g' {} \;
echo "feito!"
echo "Alterando Cores"
find ./* -type f -exec sed -i 's/&/@/g' {} \;
echo "feito!"