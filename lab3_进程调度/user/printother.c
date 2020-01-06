#include<stdio.h>
#include<ulib.h>

int main(void){
	fork();
	fork();
	int i;
	for(i=0;i<10;i++){
		cprintf("%d %d\n",getpid(),i);
		yield();
	}
	cprintf("Process: %d, enqueue_times: %d\n", getpid(), getenqueue_times());
	return 0;
}
