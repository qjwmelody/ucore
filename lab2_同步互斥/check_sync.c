#include <stdio.h>
#include <proc.h>
#include <sem.h>
#include <monitor.h>
#include <assert.h>

#define N 5 /* 哲学家数目 */
#define LEFT (i-1+N)%N /* i的左邻号码 */
#define RIGHT (i+1)%N /* i的右邻号码 */
#define THINKING 0 /* 哲学家正在思考 */
#define HUNGRY 1 /* 哲学家想取得叉子 */
#define EATING 2 /* 哲学家正在吃面 */
#define TIMES  4 /* 吃4次饭 */
#define SLEEP_TIME 10

#define M 2 //生产者消费者各自的数目
#define Num 10 //缓冲区大小


//---------- philosophers problem using semaphore ----------------------
int state_sema[N]; /* 记录每个人状态的数组 */
/* 信号量是一个特殊的整型变量 */
semaphore_t mutex; /* 临界区互斥 */
semaphore_t s[N]; /* 每个哲学家一个信号量 */

struct proc_struct *philosopher_proc_sema[N];

void phi_test_sema(i) /* i：哲学家号码从0到N-1 */
{ 
    if(state_sema[i]==HUNGRY&&state_sema[LEFT]!=EATING
            &&state_sema[RIGHT]!=EATING)
    {
        state_sema[i]=EATING;
        up(&s[i]);
    }
}

void phi_take_forks_sema(int i) /* i：哲学家号码从0到N-1 */
{ 
        down(&mutex); /* 进入临界区 */
        state_sema[i]=HUNGRY; /* 记录下哲学家i饥饿的事实 */
        phi_test_sema(i); /* 试图得到两只叉子 */
        up(&mutex); /* 离开临界区 */
        down(&s[i]); /* 如果得不到叉子就阻塞 */
}

void phi_put_forks_sema(int i) /* i：哲学家号码从0到N-1 */
{ 
        down(&mutex); /* 进入临界区 */
        state_sema[i]=THINKING; /* 哲学家进餐结束 */
        phi_test_sema(LEFT); /* 看一下左邻居现在是否能进餐 */
        phi_test_sema(RIGHT); /* 看一下右邻居现在是否能进餐 */
        up(&mutex); /* 离开临界区 */
}

int philosopher_using_semaphore(void * arg) /* i：哲学家号码，从0到N-1 */
{
    int i, iter=0;
    i=(int)arg;
    cprintf("I am No.%d philosopher_sema\n",i);
    while(iter++<TIMES)
    { /* 无限循环 */
        cprintf("Iter %d, No.%d philosopher_sema is thinking\n",iter,i); /* 哲学家正在思考 */
        do_sleep(SLEEP_TIME);
        phi_take_forks_sema(i); 
        /* 需要两只叉子，或者阻塞 */
        cprintf("Iter %d, No.%d philosopher_sema is eating\n",iter,i); /* 进餐 */
        do_sleep(SLEEP_TIME);
        phi_put_forks_sema(i); 
        /* 把两把叉子同时放回桌子 */
    }
    cprintf("No.%d philosopher_sema quit\n",i);
    return 0;    
}

//-----------------philosopher problem using monitor ------------
/*PSEUDO CODE :philosopher problem using monitor
 * monitor dp
 * {
 *  enum {thinking, hungry, eating} state[5];
 *  condition self[5];
 *
 *  void pickup(int i) {
 *      state[i] = hungry;
 *      if ((state[(i+4)%5] != eating) && (state[(i+1)%5] != eating)) {
 *        state[i] = eating;
 *      else
 *         self[i].wait();
 *   }
 *
 *   void putdown(int i) {
 *      state[i] = thinking;
 *      if ((state[(i+4)%5] == hungry) && (state[(i+3)%5] != eating)) {
 *          state[(i+4)%5] = eating;
 *          self[(i+4)%5].signal();
 *      }
 *      if ((state[(i+1)%5] == hungry) && (state[(i+2)%5] != eating)) {
 *          state[(i+1)%5] = eating;
 *          self[(i+1)%5].signal();
 *      }
 *   }
 *
 *   void init() {
 *      for (int i = 0; i < 5; i++)
 *         state[i] = thinking;
 *   }
 * }
 */

struct proc_struct *philosopher_proc_condvar[N]; // N philosopher
int state_condvar[N];                            // the philosopher's state: EATING, HUNGARY, THINKING  
monitor_t mt, *mtp=&mt;                          // monitor

void phi_test_condvar (i) { 
    if(state_condvar[i]==HUNGRY&&state_condvar[LEFT]!=EATING
            &&state_condvar[RIGHT]!=EATING) {
        cprintf("phi_test_condvar: state_condvar[%d] will eating\n",i);
        state_condvar[i] = EATING ;
        cprintf("phi_test_condvar: signal self_cv[%d] \n",i);
        cond_signal(&mtp->cv[i]) ;
    }
}


void phi_take_forks_condvar(int i) {
     down(&(mtp->mutex));
//--------into routine in monitor--------------
     // LAB7 EXERCISE1: 16307130293
     // I am hungry
	 state_condvar[i] = HUNGRY;
     // try to get fork
	 phi_test_condvar(i);
	 while (state_condvar[i] != EATING) {
		 cprintf("phi_take_forks_condvar: %d failed to get forks\n", i);
		 cond_wait(&(mtp->cv[i]));//得不到叉子就睡眠
	 }
//--------leave routine in monitor--------------
      if(mtp->next_count>0)//唤醒睡眠进程
         up(&(mtp->next));
      else
         up(&(mtp->mutex));
}

void phi_put_forks_condvar(int i) {
     down(&(mtp->mutex));

//--------into routine in monitor--------------
     // LAB7 EXERCISE1: 16307130293
     // I ate over
	 state_condvar[i] = THINKING;
     // test left and right neighbors
	 phi_test_condvar(LEFT);
	 phi_test_condvar(RIGHT);
//--------leave routine in monitor--------------
     if(mtp->next_count>0)
        up(&(mtp->next));
     else
        up(&(mtp->mutex));
}

//---------- philosophers using monitor (condition variable) ----------------------
int philosopher_using_condvar(void * arg) { /* arg is the No. of philosopher 0~N-1*/
  
    int i, iter=0;
    i=(int)arg;
    cprintf("I am No.%d philosopher_condvar\n",i);
    while(iter++<TIMES)
    { /* iterate*/
        cprintf("Iter %d, No.%d philosopher_condvar is thinking\n",iter,i); /* thinking*/
        do_sleep(SLEEP_TIME);
        phi_take_forks_condvar(i); 
        /* need two forks, maybe blocked */
        cprintf("Iter %d, No.%d philosopher_condvar is eating\n",iter,i); /* eating*/
        do_sleep(SLEEP_TIME);
        phi_put_forks_condvar(i); 
        /* return two forks back*/
    }
    cprintf("No.%d philosopher_condvar quit\n",i);
    return 0;    
}

//-----------------philosopher problem using monitor ------------
int buff[Num] = { 0 };//缓冲区初始化为0
int in = 0;//生产者放置产品的位置
int out = 0;//消费者取产品的位置

semaphore_t mutex2;//互斥信号量
semaphore_t full;//记录满缓冲区个数
semaphore_t empty;//记录空缓冲区个数
struct proc_struct *produce_proc[M];
struct proc_struct *purchase_proc[M];

void print() {
	int i;
	for (i = 0; i != Num; i++)
		cprintf("%d", buff[i]);
}

int produce_using_semaphore(void *arg) {
	int t = (int)arg;//生产者编号
	int iter = 0;
	while (iter++ < TIMES) {
		do_sleep(SLEEP_TIME);
		down(&empty);
		down(&mutex2);//进入缓冲区
		buff[in] = 1;//生产产品到缓冲区in
		cprintf("Producer%d in%d.like:  ", t, in);
		print();
		cprintf("\n");
		in = (++in) % Num;
		up(&mutex2);//离开临界区
		up(&full);//增加已用缓冲区数目
	}
	return 0;
}

int purchase_using_semaphore(void *arg) {
	int t = (int)arg;//消费者编号
	int iter = 0;
	while (iter++ < TIMES) {
		do_sleep(SLEEP_TIME);
		down(&full);
		down(&mutex2);//进入缓冲区
		buff[out] = 0;//从缓冲区out取产品
		cprintf("Consumer%d in%d.like: ", t, out);
		print();
		cprintf("\n");
		out = (++out) % Num;
		up(&mutex2);//离开临界区
		up(&empty);//增加剩余缓冲区数目
	}
	return 0;
}

void check_sync(void){

    int i;

    //check semaphore
    sem_init(&mutex, 1);
    for(i=0;i<N;i++){
        sem_init(&s[i], 0);
        int pid = kernel_thread(philosopher_using_semaphore, (void *)i, 0);
        if (pid <= 0) {
            panic("create No.%d philosopher_using_semaphore failed.\n");
        }
        philosopher_proc_sema[i] = find_proc(pid);
        set_proc_name(philosopher_proc_sema[i], "philosopher_sema_proc");
    }

    //check condition variable
    monitor_init(&mt, N);
    for(i=0;i<N;i++){
        state_condvar[i]=THINKING;
        int pid = kernel_thread(philosopher_using_condvar, (void *)i, 0);
        if (pid <= 0) {
            panic("create No.%d philosopher_using_condvar failed.\n");
        }
        philosopher_proc_condvar[i] = find_proc(pid);
        set_proc_name(philosopher_proc_condvar[i], "philosopher_condvar_proc");
    }
    /*
	//check semaphore
	sem_init(&mutex2, 1);
	sem_init(&full, 0);
	sem_init(&empty, Num);

	for (i = 0; i<M; i++) {
		int pid = kernel_thread(produce_using_semaphore, (void *)i, 0);
		if (pid <= 0) {
			panic("create No.%d produce_using_semaphore failed.\n");
		}
		produce_proc[i] = find_proc(pid);
		set_proc_name(produce_proc[i], "produce_sema_proc");
	}

	for (i = 0; i<M; i++) {
		int pid = kernel_thread(purchase_using_semaphore, (void *)i, 0);
		if (pid <= 0) {
			panic("create No.%d purchase_using_semaphore failed.\n");
		}
		purchase_proc[i] = find_proc(pid);
		set_proc_name(purchase_proc[i], "purchase_sema_proc");
	}
	*/
}
