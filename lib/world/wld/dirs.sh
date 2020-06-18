find ./* -type f -exec sed -i 's/D N/D 0/g' {} \;
find ./* -type f -exec sed -i 's/D E/D 1/g' {} \;
find ./* -type f -exec sed -i 's/D S/D 2/g' {} \;
find ./* -type f -exec sed -i 's/D W/D 3/g' {} \;
find ./* -type f -exec sed -i 's/D U/D 4/g' {} \;
find ./* -type f -exec sed -i 's/D D/D 5/g' {} \;
find ./* -type f -exec sed -i 's/D NW/D 6/g' {} \;
find ./* -type f -exec sed -i 's/D NE/D 7/g' {} \;
find ./* -type f -exec sed -i 's/D SE/D 8/g' {} \;
find ./* -type f -exec sed -i 's/D SW/D 9/g' {} \;
echo "feito!"