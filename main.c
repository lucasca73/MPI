#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>


#define VEC_LENGTH 10000000//100000000 //100M, 1B crashes by memory
#define ROOT 0 // 0 is the 'main' process
#define WORLD MPI_COMM_WORLD

//Comunication Tags
#define TASK_SETUP_TAG 40
#define TASK_ARG_COUNT_TAG 41
#define TASK_INDEX_TAG 42
#define SHOULD_CONTINUE 43
#define END_TASK_TAG 44



float* initialize_array();
void show_smaller_and_bigger(float* array);
void show_all(float* array);
void send_integer(int* value_pointer, int count, int destination, int tag);
void send_float(float* value_pointer, int count, int destination, int tag);


int main(int argc, char** argv){

  int error;
  int rank, size;
  MPI_Status status;

  //Initializing
  error = MPI_Init(&argc, &argv);

  //Setting up rank and size
  error = MPI_Comm_rank(WORLD, &rank);
  error = MPI_Comm_size(WORLD, &size);

  //printf("rank_id: %d - processes: %d \n", rank, size);

  //printf("Running mpi...\n");

  int keep = 1;

  if(rank == ROOT){
    printf("\n\t -- MPI LAB --\n\n");

    printf("Setting up array...\n");

    float* array = initialize_array();

    //printf("I am ROOT!\n");

    printf("Calculating...\n");

    // Dividing tasks for process amount
    int minor_chunk = 10;
    int number_of_tasks = floor(VEC_LENGTH/minor_chunk);

    int slave_rank = 1;
    int iteration = 0;

    //Distribute computing
    while(1){

      //printf("Sending to: %d... ", slave_rank);

      //Sending message to slave
      send_integer(&keep, 1,
        slave_rank, SHOULD_CONTINUE);

      send_integer(&minor_chunk, 1,
        slave_rank, TASK_ARG_COUNT_TAG);

      int index = (iteration * minor_chunk);

      send_integer(&index, 1,
        slave_rank, TASK_INDEX_TAG);

      send_float(&array[index], minor_chunk,
        slave_rank, TASK_SETUP_TAG);

      //Iterate slave
      slave_rank = (slave_rank % (size - 1) ) + 1;

      iteration += 1;

      if (iteration >= number_of_tasks){
        break;
      }

    }

    printf(".\n");

    //Reset variables for iteration
    slave_rank = 1;
    iteration = 0;

    //Listen to all messages with calculated values
    while(1){

      int l = 10;
      float aux[l];
      int ind = 0;

      MPI_Recv(&ind, 1,
        MPI_INT, slave_rank, TASK_INDEX_TAG,
        WORLD, &status);

      MPI_Recv(&aux, l,
        MPI_FLOAT, slave_rank, TASK_SETUP_TAG,
        WORLD, &status);

      for(int i = 0; i < l; i++){
        array[ind + i] = aux[i];
      }

      //Iterate slave
      slave_rank = (slave_rank % (size - 1) ) + 1;
      iteration += 1;

      if (iteration >= number_of_tasks){
        break;
      }

    }


    keep = 0;

    printf(".\n");

    //Stop all slaves
    for(int slave = 0; slave < size; slave++){
      send_integer(&keep, 1,
        slave, SHOULD_CONTINUE);
    }

    printf(".\n");

    //show_all(array);
    show_smaller_and_bigger(array);

    //free(array);

  }else{
    // Slave
    int length = 0;
    int index = 0;
    float* array;
    int keep = 1;

    while(1){

      MPI_Recv(&keep, 1,
        MPI_INT, ROOT, SHOULD_CONTINUE,
        WORLD, &status);

      if (keep == 0){
        //Break the chains and be Free!
        //printf("\nslave%d is leaving.\n", rank);
        break;
      }

      //Listen Number of arguments from root
      MPI_Recv(&length, 1,
        MPI_INT, ROOT, TASK_ARG_COUNT_TAG,
        WORLD, &status);

      int l = length; // Value were losing reference, needed to be put in var

      //printf("rank%d read: %d\n", rank, length);

      float array[l];

      MPI_Recv(&index, 1,
        MPI_INT, ROOT, TASK_INDEX_TAG,
        WORLD, &status);

      int ind = index;

      MPI_Recv(&array, l,
        MPI_FLOAT, ROOT, TASK_SETUP_TAG,
        WORLD, &status);

      //Calculate
      for(int i = 0; i < l; i++){
        array[i] = (float) sqrt(array[i]);
      }

      send_integer(&ind, 1,
        ROOT, TASK_INDEX_TAG);

      send_float(&array, l,
        ROOT, TASK_SETUP_TAG);

    }
  }


  error = MPI_Finalize();

  return 0;
}





void send_integer(int* value_pointer, int count, int destination, int tag){

  int er = MPI_Send(value_pointer, count,
    MPI_INT, destination,
    tag, WORLD);

  //Check if message were sended properly
  if (er == MPI_SUCCESS){
    //printf("sended.\n");
  }else{
    //printf("not sended! %d \n", er);
  }
}

void send_float(float* value_pointer, int count, int destination, int tag){

  int er = MPI_Send(value_pointer, count,
    MPI_FLOAT, destination,
    tag, WORLD);

  //Check if message were sended properly
  if (er == MPI_SUCCESS){
    //printf("sended.\n");
  }else{
    //printf("not sended! %d \n", er);
  }
}


float* initialize_array(){

  float* array = (float*) malloc(sizeof(float) * VEC_LENGTH);

  for(int i = 0; i < VEC_LENGTH; i++){
    array[i] = (float) pow(i - VEC_LENGTH/2, 2);
    //printf("%d - %.2f\n", i, array[i]);
  }

  return array;
}

void show_smaller_and_bigger(float* array){

  float smaller, bigger = array[0];

  //Calculating manually
  for(int i = 0; i < VEC_LENGTH; i++){
    if(array[i] > bigger){
      bigger = array[i];
    }

    if(array[i] < smaller){
      smaller = array[i];
    }
  }

  printf("Smaller: %.2f\n", smaller);
  printf("Bigger: %.2f\n", bigger);
}

void show_all(float* array){
  for(int i = 0; i < VEC_LENGTH; i++){
    printf("%d - %.2f\n", i, array[i]);
  }
}
