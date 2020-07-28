#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX 10
#define MSG(x...) fprintf(stdout,x)

/*동기화를 위한 mutex 변수*/
pthread_mutex_t mutex[MAX];
/*동기화시 조건을 판별하기 위한 condition변수*/
pthread_cond_t condition[MAX];

/*cycle 수의 판단을 하기 위한 mutex변수*/
pthread_mutex_t count;

/*파라미터를 받을 변수*/
int total_philosopher;
int msec;
int cycle;

/*포크의 소유상태와 현재 철학자들의 상태를 저장하기 위한 변수*/
int pick[MAX] = {0,};
char state[MAX][20];

/*진행된 cycle수를 판단하기 위한 변수*/
int cnt_cycle = 0;

/*
 *철학자의 수 파라미터가 유효한지 판별(오류처리)
 *
 *@param : 받은 파라미터
 *
 *@return : 성공 - 0, 실패 - 1
 */
int check_total_philosopher(char *num)
{
	int len = strlen(num);
	int i;
	
	for(i = 0 ; i < len; i++)//문자가 있는지 확인
	{
		if(!isdigit(num[i]))
		{
			return 1;
		}
	}

	if((atoi(num) < 3) || (atoi(num) > 10))//3~10사이의 정수인지 확인
	{
		return 1;
	}
		
	return 0;
}

/*
 *thinking 또는 eating시간 파라미터가 유효한지 판별(오류처리)
 *
 *@param : 받은 파라미터
 *
 *@return :  성공 - 0, 실패 - 1 
 */
int check_msec(char *num)
{
	int len = strlen(num);
	int i;

	for(i = 0 ; i < len; i++)//문자가 있는지 확인
	{
		if(!isdigit(num[i]))
		{
			return 1;
		}
	}

	if((atoi(num) < 10) || (atoi(num) > 1000))//10~1000사이의 정수인지 확인
	{
		return 1;
	}
	return 0;
}

/*
 *thinking-eating 사이클 수 파라미터 유효한지 판별(오류처리)
 *
 *@param : 받은 파라미터
 *
 *@return : 성공 - 0, 실패 - 1
 */
int check_cycle(char *num)
{
	int len = strlen(num);
	int i;

	for(i = 0 ; i < len; i++)//문자가 있는지 판별
	{
		if(!isdigit(num[i]))
		{
			return 1;
		}
	}

	if((atoi(num) < 1) || (atoi(num) > 100))//1~100사이의 정수인지 판별
	{
		return 1;
	}
	return 0;
}

/*
 *철학자가 포크를 들어 올리는 기능
 *
 *@param : 해당 포크의 번호
 */
void pick_fork(int fork_num)
{
	pthread_mutex_lock(&mutex[fork_num]);//해당 포크 번호에 대하여 lock
	while(pick[fork_num] == 1)//포크가 내려놓은 상태일때까지 대기
	{
		pthread_cond_wait(&condition[fork_num], &mutex[fork_num]);//pthread_cond_signal함수로 인한 시그널을 받을 때까지 대기
	}
	pick[fork_num] = 1;//포크를 집어든 상태이면 pick을 1로 설정
	pthread_mutex_unlock(&mutex[fork_num]);//해당 포크 번호에 대하여 unlock
} 

/*
 *철학자가 포크를 내려놓는 기능
 *
 *@param : 해당 포크의 번호
 */
void return_fork(int fork_num)
{
	pthread_mutex_lock(&mutex[fork_num]);//해당 포크 번호에 대하여 lock
	pick[fork_num] = 0;//포크를 내려놓은 상태이면 pick를 0으로 설정
	pthread_cond_signal(&condition[fork_num]);//pthread_cond_wait함수 쪽으로 시그널을 보내줌
	pthread_mutex_unlock(&mutex[fork_num]);//해당 포크 번호에 대하여 unlock
}

/*
 *싸이클이 몇번 진행되었는지에 대한 카운트를 해 주는 변수
 *(cycle은 누군가 포크를 집어서 eating을 하는 상태가 되면 cycle진행이 한번 되었다고 판단을 해 주웠습니다)
 */
void count_cycle()
{
	pthread_mutex_lock(&count);
	cnt_cycle++;
	pthread_mutex_unlock(&count);
}

/*
 *현재 cycle진행 횟수가 입력된 cycle진행횟수를 초과했는지 판단해 주는 함수
 *
 *@return : 초과시 - 1 / 초과하지 않을시 - 0
 */
int check_cnt_cycle()
{
	int res = 0;
	pthread_mutex_lock(&count);
	if(cnt_cycle > cycle)
	{
		res = 1;
	}
	pthread_mutex_unlock(&count);

	return res;
}

/*
 *중간에 철학자들의 상태를 출력
 */
void print_state()
{
	int i;

	for(i = 0 ; i < total_philosopher;i++)
	{
		MSG("%-10s",state[i]);
	}
	MSG("\n");
}

/*
 *철학자가 생각하는 상태를 나타냄
 *(상태 변환,포크를 내려놓는 기능)
 *
 *@parma : 철학자의 번호
 */
void think(int philosopher_number)
{
	int left_fork;
	int right_fork;
	
	left_fork = philosopher_number;//철학자의 왼쪽 포크의 번호
	right_fork = (philosopher_number+1) % total_philosopher;//철학자의 오른쪽 포크의 번호
	
	/*판별시 출력은 1번부터 시작되어 지지만 배열로 관리하기 때문에 코드상에서 철학자와 포크의 번호는
	 *0부터 시작하도록 구성해 놓았습니다.
	 */
	if(philosopher_number % 2 == 0)//실제 철학자의 번호가 홀수인 경우(실제 철학자 번호는 1부터 시작)
		{
			return_fork(right_fork);//오른쪽 포크부터 내려놓음 
			return_fork(left_fork);
		}
		else//실제 철학자의 번호가 짝수인 경우
		{
			return_fork(left_fork);//왼쪽 포크부터 내려놓음
			return_fork(right_fork);
		}
	strcpy(state[philosopher_number],"Thinking");//상태를 바꿔줌
	sleep(msec);//msec만큼 작업을 수행
}

/*
 *철학자가 먹는 상태를 나타냄
 *(상태변환, 포크를 집는 행위, 먹는 시간은 msec만큼 시간이 걸림)
 *
 *@param : 철학자의 번호
 */
void eat(int philosopher_number)
{
	int left_fork;
	int right_fork;
	
	left_fork = philosopher_number;//철학자의 왼쪽 포크의 번호
	right_fork = (philosopher_number+1) % total_philosopher;//철학자의 오른쪽 포크의 번호
	if(check_cnt_cycle() == 1)
		return;
	/*판별시 출력은 1번부터 시작되어 지지만 배열로 관리하기 때문에 코드상에서 철학자와 포크의 번호는
	 *0부터 시작하도록 구성해 놓았습니다.
	 */
	if(philosopher_number % 2 == 0)//실제 철학자의 번호가 홀수인 경우(실제 철학자 번호는 1부터 시작)
		{
			pick_fork(left_fork);//왼쪽 포크부터 집어듬 
			pick_fork(right_fork);
		}
		else//실제 철학자의 번호가 홀수인 경우
		{
			pick_fork(right_fork);//오른쪽 포크부터 집어듬
			pick_fork(left_fork);
		}
	strcpy(state[philosopher_number],"Eating");//상태를 바꿔줌
	count_cycle();//cycle의 수를 높혀줌
	if(check_cnt_cycle() == 1)//cycle초과시 출력을 진행하지 않고 반환
		return;
	print_state();//상태를 출력
	sleep(msec);//msec만큼 작업을 수행
}



/*
 *철학자가 행동을 취하는 함수
 *(thread가 생성되면 해당 함수를 실행)
 *
 *@param : 철학자의 번호
 */
void *action(void *arg)
{
	int i;
	int philosopher_number;
	philosopher_number = *(int*)arg;
	
	while(1)//cycle의 수 만큼 thinking-eating행위를 반복
	{
		if(check_cnt_cycle() == 1)//만약 파라미터로 입력된 cycle수를 만족한다면 더이상 행위를 하지 않음
		{
			break;
		}
		eat(philosopher_number);//포크를 집고 먹는 행위
		think(philosopher_number);//포크를 놓고 생각하는 행위
	}

	return NULL;
}


int main(int argc, char *argv[])
{
	int i;
	int print_check = 0;

	/*철학자의 행동을 표현하기 위한 thread변수*/
	pthread_t philosopher[MAX];

	
	/*오류처리*/
	if(argc != 4)//받은 파라미터의 수가 정확하지 않으면 오류문을 출력
	{
		MSG("parameter format is not correct\n");
		exit(1);
	}

	if(check_total_philosopher(argv[1]))//철학자의 수 파라미터의 오류 판별
	{
		MSG("%s is not vaild parameter\n",argv[1]);
		exit(1);
	}

	if(check_msec(argv[2]))//thinking 또는 eating시간 파리미터의 오류 판별
	{
		MSG("%s is not valid parameter\n",argv[2]);
		exit(1);
	}

	if(check_cycle(argv[3]))//thinking-eating 파라미터의 오류 판별
	{
		MSG("%s is not vaild parameter\n",argv[3]);
		exit(1);
	}
	
	/*유효한 값을 변수에 넣어줌*/
	total_philosopher = atoi(argv[1]);
	msec = atoi(argv[2]);
	cycle = atoi(argv[3]);


	int num[total_philosopher];
	
	/*mutex, condition 초기화 작업*/
	for(i = 0 ; i < total_philosopher; i++)
	{
		pthread_mutex_init(&mutex[i],NULL);
		pthread_cond_init(&condition[i],NULL);
		strcpy(state[i],"Thinking");//상태의 초기화(생각하는 상태)
	}
	
	/*철학자들의 번호 출력(상태 출력을 위한 작업)*/
	for(i = 0 ; i < total_philosopher; i++)
	{
		char num[2];
		sprintf(num,"%d",i+1);
		MSG("P");
		MSG("%-8s ",num);
	}
	MSG("\n");
	
	/*해당 철학자의 수 만큼 thread를 생성*/
	for(i = 0; i < total_philosopher;i++)
	{
		num[i] = i;
		int *p = &num[i];
		pthread_create(&philosopher[i], NULL,action, p);
	}
	
	/*생성된 thread들이 모두 종료될때까지 기다림*/
	for(i = 0 ; i < total_philosopher;i++)
	{
		pthread_join(philosopher[i], NULL);
	}

	/*mutex와 condition을 소멸*/
	for(i = 0 ; i < total_philosopher; i++)
	{
		pthread_mutex_destroy(&mutex[i]);
		pthread_cond_destroy(&condition[i]);
	}

	return 0;
}

