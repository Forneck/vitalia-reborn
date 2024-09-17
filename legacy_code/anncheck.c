#include "stdio.h"

#include "./include/fannconfig.h"
#include "./include/floatfann.h"
#include "math.h"

int main(){
struct fann *avent, *map, *move;
   avent = fann_create_from_file("../lib/etc/aventureiro.fann");
   move = fann_create_from_file("../lib/etc/move.fann");
   map = fann_create_from_file("../lib/etc/map.fann");

 fann_print_connections(avent);
 fann_print_connections(move);
  fann_print_connections(map);

fann_destroy(avent);
fann_destroy(move);
fann_destroy(map);
   
}