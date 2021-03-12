
#include "stdio.h"
#include "./include/fann.h"

int main(){
	struct fann *ann;
	struct fann_train_data *train_data;
	const float desired_error = (const float)0.0;
	unsigned int max_neurons = 350;
	unsigned int neurons_between_reports = 1;
	unsigned int bit_fail_train;
	float mse_train;
	unsigned int i = 0;
	fann_type *output;
	fann_type steepness;
	int multi = 0;
	enum fann_activationfunc_enum activation;
	enum fann_train_enum training_algorithm = FANN_TRAIN_RPROP;
	
	printf("Reading data.\n");
	 
	train_data = fann_read_train_from_file("./bot-dataset.train");
	
	printf("Creating network.\n");
	
	ann = fann_create_shortcut(2, fann_num_input_train_data(train_data), fann_num_output_train_data(train_data));
		
	fann_set_training_algorithm(ann, training_algorithm);
	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(ann, FANN_LINEAR);
	fann_set_train_error_function(ann, FANN_ERRORFUNC_LINEAR);
	
	if(!multi)
	{
		/*steepness = 0.5;*/
		steepness = 1;
		fann_set_cascade_activation_steepnesses(ann, &steepness, 1);
		/*activation = FANN_SIN_SYMMETRIC;*/
		activation = FANN_SIGMOID_SYMMETRIC;
		
		fann_set_cascade_activation_functions(ann, &activation, 1);		
		fann_set_cascade_num_candidate_groups(ann, 8);
	}	
		
	if(training_algorithm == FANN_TRAIN_QUICKPROP)
	{
		fann_set_learning_rate(ann, 0.35f);
		fann_randomize_weights(ann, -2.0f, 2.0f);
	}
	
	fann_set_bit_fail_limit(ann, (fann_type)0.9);
	fann_set_train_stop_function(ann, FANN_STOPFUNC_BIT);
	fann_print_parameters(ann);
		
	fann_save(ann, "cascade_train2.net");
	
	printf("Training network.\n");

	fann_cascadetrain_on_data(ann, train_data, max_neurons, neurons_between_reports, desired_error);
	
	fann_print_connections(ann);
	
	mse_train = fann_test_data(ann, train_data);
	bit_fail_train = fann_get_bit_fail(ann);
	
	printf("\nTrain error: %f, Train bit-fail: %d \n\n", 
		   mse_train, bit_fail_train);
	
	for(i = 0; i < train_data->num_data; i++)
	{
		output = fann_run(ann, train_data->input[i]);
		if((train_data->output[i][0] >= 0 && output[0] <= 0) ||
		   (train_data->output[i][0] <= 0 && output[0] >= 0))
		{
			printf("ERROR: %f does not match %f\n", train_data->output[i][0], output[0]);
		}
	}
	
	printf("Saving network.\n");
	
	fann_save(ann, "cascade_train.net");
	
	printf("Cleaning up.\n");
	fann_destroy_train(train_data);
	fann_destroy(ann);
	
	return 0;
	
}