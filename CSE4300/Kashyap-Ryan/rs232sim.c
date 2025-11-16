#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


//Argument structure required to invoke sendData through a thread.
struct argData{
	int* tx;
	int* rx;
	int* data;
	FILE* file;
};


//I want a thread that is simulating the clock to actually proc the send data function.
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clockTick =  PTHREAD_COND_INITIALIZER;

//Status of the clock being ready or not
int clock_ready = 0;
//Global variable to keep track of baud rate
int baudRate = 104;
//Should support multiple threads
void* sendData(void* a)
{
	//Recasting to an argData pointer
	struct argData* args = (struct argData*)(a);
	//I did not know about fprintf at the time.
	char outText[50];
	//variable to keep track of time
	int time = 0;
	//Need to keep track of previous int to set the bus value
	int previousInt = 0;
	//Originally designed with these 4 as a single method running in the process.
	//Had to do this to make it multithreaded compatible.
	int* tx = args->tx;
	int* rx = args->rx;
	int* data = args->data;
	FILE* file = args->file;
	//Assuming maximum size of 8 bytes(pretty standard for a packet in digital communications)
	for(int i = 0; i < 8; i++){
		if(data[i] == -1) break;
		//Start transmission
		pthread_mutex_lock(&mtx);
		*tx = 1;
		//again, didn't know about fprintf at this time.
		sprintf(outText, "%d %d\n", *tx, time);
		fputs(outText, file);
		//Wait for the clock to unlock
		clock_ready = 0;
		while(!clock_ready)
			pthread_cond_wait(&clockTick, &mtx);
		pthread_mutex_unlock(&mtx);
		//Update the time
		time += baudRate;
		for(int j = 0; j < 8; j++){
			//Get the first bit(LSB transmission)
			//Same as previous transmission: 1, different:0
			//lock da mutex
			pthread_mutex_lock(&mtx);
			//actual logic for calculating what tx should be
			*tx = ((data[i] >> j) % 2 == previousInt) ? *tx : !(*tx);
			previousInt = (data[i] >> j) % 2;
			//print it to the file
			sprintf(outText, "%d %d\n", *tx, time);
			fputs(outText, file);
			//wait for the clock thread to signal this thread
			clock_ready=0;
			while(!clock_ready)
				pthread_cond_wait(&clockTick, &mtx);
			pthread_mutex_unlock(&mtx);
			//get back after it(update the time value)
			time += baudRate;
		}
		//Stop transmission
		//lock da mutex
		pthread_mutex_lock(&mtx);
		*tx = 0;
		sprintf(outText, "%d %d\n", *tx, time);
		fputs(outText, file);
		//wait for the clock thread to signal this thread
		clock_ready=0;
		while(!clock_ready)
			pthread_cond_wait(&clockTick, &mtx);
		pthread_mutex_unlock(&mtx);
		//continue!
		time += baudRate;
	}
	return NULL;
}

void* clockThread(void* args){
while(1){	
	//Let's simulate clock drift
	int drift = (rand() % 13) - 6;
	usleep(104 + drift);
	//Need to make sure that our updated time in the sendData function is reflective of the actual baud rate.
	baudRate = 104+drift;
	//lock da mutex
	pthread_mutex_lock(&mtx);
	//set clock ready to 1
	clock_ready = 1;
	//broadcast to signal the transmitting thread
	pthread_cond_broadcast(&clockTick);
	//Unlock the mutex
	pthread_mutex_unlock(&mtx);
}

	
return NULL;
}

int main(){
//Initialize RS232 communication bus(integer)
//Internally we will represent negative voltage as 0 and positive voltage as 1
//Externally we will adjust to show the output
int rs232tx = 0;
int rs232rx = 0;
//Our two files
FILE* outputFile;
FILE* clockFile;
//allocate memory for the arguments
struct argData* args = malloc(sizeof(struct argData));
outputFile = fopen("output.txt", "w");
clockFile = fopen("idealClock.txt", "w");
//Simulate ideal clock
//check if it's null
if (clockFile != NULL){
	//Just run 50 cycles of a 9600bps baud rate clock.
	for(int i = 0; i < 50; i++){
		fprintf(clockFile, "%d %d\n", i%2, 52*i);
	}
}
//This way we can plot this against the actual packet and see the clock drift
fclose(clockFile);
//if the files not fucked(should really be checking this on the clock file too)
if(outputFile != NULL){
	//Add your data here! this is what we update every time we want to transmit stuff
	int data[3] = {0x68,0x69, -1};
	//Assigning all of our awesome variables!
	args->data = data;
	args->tx = &rs232tx;
	args->rx = &rs232rx;
	args->file = outputFile;
	//Need to dispatch a thread to do this
	//need to set up a clock thread :D
	pthread_t sender, clocker;
	pthread_create(&clocker, NULL, clockThread, NULL);
	pthread_create(&sender, NULL, sendData, args);
	//We want to keep going after the sender is done	
	pthread_join(sender, NULL);
	//we can just cancel the clock thread because we do not care about it :(
	pthread_cancel(clocker);
	//free the args
	free(args);
	//close the output file
	fclose(outputFile);
}
else{
	printf("Unable to open file!\n");
}
return 0; 
}
