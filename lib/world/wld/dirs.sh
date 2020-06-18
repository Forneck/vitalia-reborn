find ./* -type f -exec sed -i 's/DN/D 0/g' {} \;
find ./* -type f -exec sed -i 's/DE/D 1/g' {} \;
find ./* -type f -exec sed -i 's/DS/D 2/g' {} \;
find ./* -type f -exec sed -i 's/DW/D 3/g' {} \;
find ./* -type f -exec sed -i 's/DU/D 4/g' {} \;
find ./* -type f -exec sed -i 's/DD/D 5/g' {} \;
find ./* -type f -exec sed -i 's/DNW/D 6/g' {} \;
find ./* -type f -exec sed -i 's/DNE/D 7/g' {} \;
find ./* -type f -exec sed -i 's/DSE/D 8/g' {} \;
find ./* -type f -exec sed -i 's/DSW/D 9/g' {} \;
echo "feito!"