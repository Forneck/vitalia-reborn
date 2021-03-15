
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"				/* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"
#include "./include/fann.h"
#include "ann.h"

void ann_move_train(struct char_data *ch, int dir, room_rnum going_to){
     struct fann *ann;
     struct fann *map;
     fann_type mapin[2];
     fann_type mapout[1];
     
	fann_type input[29];
	fann_type output[2];
	struct obj_data *object;
   int grupo;
	int count_obj = 0;
   struct fann_train_data *map_train, *move_train;
   ann = fann_create_from_file("etc/move.fann");
   map = fann_create_from_file("etc/map.fann");
   
    /* verifica grupo e inventario */
	if (GROUP(ch) != NULL)
		grupo = 1;
	else
		grupo = 0;

	for (object= ch->carrying; object; object = object->next_content)
	{
			count_obj++;
	}

	   /*pega dados pro aventureiro */
	input[0] = (float)GET_HIT(ch) / 5000;
	input[1] = (float) GET_MAX_HIT(ch) / 5000;

	input[2] = (float)GET_MANA(ch) / 5000;
	input[3] = (float) GET_MAX_MANA(ch) / 5000;
	input[4] = (float)GET_MOVE(ch) / 5000;
	input[5] = (float) GET_MAX_MOVE(ch) / 5000;
	input[6] = (float) GET_EXP(ch) / 500000000;
	input[7] = (float)  GET_ROOM_VNUM(IN_ROOM(ch)) / 100000;  
	input[8] = 1/(1+exp(-GET_CLASS(ch)));
	input[9] = 1/(1+exp(-GET_POS(ch)));
	input[10] = (float) GET_ALIGNMENT(ch) / 1000;
	input[11] = (float) compute_armor_class(ch) / 200;
	input[12] = (float) GET_STR(ch) / 25;
	input[13] = (float) GET_ADD(ch) / 25;
	input[14] = (float) GET_INT(ch) / 25;
	input[15] = (float) GET_WIS(ch) / 25;
	input[16] = (float) GET_CON(ch) / 25;
	input[17] = (float) GET_DEX(ch) / 25;
	input[18] = (float) GET_GOLD(ch) / 100000000;
	input[19] = (float) GET_BANK_GOLD(ch) / 100000000;
	input[20] = 1/(1+exp(-GET_COND(ch, HUNGER)));
	input[21] = 1/(1+exp(-GET_COND(ch, THIRST)));
	input[22] = (float) GET_PRACTICES(ch) / 100;
	input[23] = grupo;
	//input 24 eh o clan em vez de mudhora
	input[24] = 1/(1+exp(-0));
	input[25] = (float) GET_BREATH(ch) / 15;
	input[26] = (float) GET_HITROLL(ch) / 100;
	input[27] = (float) GET_DAMROLL(ch) / 100;
	input[28] = (float) count_obj / 100;
   
   output[0] = (float) ( (float) dir/ NUM_OF_DIRS);
   output[1] = (float)  GET_ROOM_VNUM(going_to) / 100000;
     
      move_train =  fann_create_train_array(1,29,input,2,output);
		fann_train_on_data(ann,move_train,250,500,0);
		fann_save(ann, "etc/move.fann");
	   
		mapin[0] = (float)  GET_ROOM_VNUM(IN_ROOM(ch)) / 100000;
    mapin[1] =(float) ( (float) dir/ NUM_OF_DIRS);
   mapout[0] = (float)  GET_ROOM_VNUM(going_to) / 100000;
   
    map_train =  fann_create_train_array(1,2,mapin,1,mapout);
  
	fann_train_on_data(map,map_train,250,500,0);
		fann_save(map, "etc/map.fann");
		
	   fann_destroy(ann);
      fann_destroy(map);
        fann_destroy_train(map_train);
        fann_destroy_train(move_train);
}

void avalia_fitness(){
   struct descriptor_data *d;
   struct char_data *ch;
   int fit[2];
   struct fann *ann;
   
   for (d = descriptor_list; d; d = d->next)
	{
	  if ((ch = d->character) != NULL)
	  if (IN_ROOM(ch) != NOWHERE) {
   if (IS_PLAYING(d) && PLR_FLAGGED(ch, PLR_AUTO)) {
   fit[0] = GET_FIT(ch);
   GET_FIT(ch) = 0;
   }
	else if (IS_PLAYING(d) && PLR_FLAGGED(ch, PLR_RIVAL)) {
   fit[1] = GET_FIT(ch);
      GET_FIT(ch) = 0;
   }
	}
	}
  if (fit[0] > fit[1]) {
     	ann = fann_create_from_file("etc/aventureiro2.fann");
fann_randomize_weights(ann,-0.77,0.77);
   fann_save(ann,"etc/aventureiro2.fann");
     	fann_destroy(ann);
  }
  else if (fit[0] < fit[1]) {
     	ann = fann_create_from_file("etc/aventureiro.fann");
fann_randomize_weights(ann,-0.77,0.77);
   fann_save(ann,"etc/aventureiro.fann");
     	fann_destroy(ann);
  }

}