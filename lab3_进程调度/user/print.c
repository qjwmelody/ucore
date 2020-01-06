#include<stdio.h>
#include<ulib.h>

int main(void){
	fork();
	fork();
	int i;
	for(i=0;i<3;i++){
		cprintf("%d %d\n",getpid(),i);
		yield();
	}
	return 0;
}
