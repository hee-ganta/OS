/**
 * OS Assignment #2
 **/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#define MSG(x...) fprintf(stderr,x)
#define MSGOUT(x...) fprintf(stdout,x)
#define STRERROR strerror (errno)
#define ID_LEN 3
#define INPUT_SIZE 256

typedef struct _Task Task;

/*프로세스를 담을 큐를 리스트로 관리하기 위한 포인터 변수*/

/*프로세스를 담을 큐의 구조*/
typedef struct ready_queue{
	Task *front;
	Task* rear;
	int cnt;
	char state;//무슨 큐인지 저장
}ready_queue;

/*Task의 정보를 저장할 구조체*/
struct _Task
{
	Task *next;

	char id[ID_LEN];
	char process_type;
	
	int arrive_time;
	int service_time;
	int priority;

	int task_num;//몇번째로 들어온 Task인지 저장
	int remain_service_time;//서비스를 하고 받아야 하는 남은 서비스 타임을 저장
	int start_state;//큐로 들어갔는지 확인해주는 변수
	int remain_arrive_time;//남아있는 도착 시간을 저장
};

static Task *tasks = NULL; //기존의 Task를 어떤 식으로 받아왔는지 관리해 주기 위한 리스트
static ready_queue H;//H큐
static ready_queue M;//M큐
static ready_queue L;//L큐

/*
 *문자열의 앞뒤 공백을 없재기 위한 함수
 *
 *@param : 앞뒤 공백을 제거해주기 위한 문자열
 *
 *@return : 공백을 제거해준 문자열
 */
static char* strstrip(char *str)
{
	char *start;
	size_t len;

	len = strlen(str);
	while(len--)//문자열의 뒷부분의 공백을 제거해 주는 작업
	{
		if(!isspace(str[len]))
			break;
		str[len]= '\0';
	}

	for(start = str; *start && isspace(*start);start++);//문자열의 앞부분의 공백을 제거해 주는 작업
	memmove(str,start,strlen(start) + 1);
	return str;
}

/*
 *유효한 Task ID인지 확인
 *첫째 자리 -> 알파벳 대문자, 둘째 자리 -> 숫자
 *
 *@param : Task ID
 *
 *@return : 만족 : 0/ 불만족 : -1
 */
static int check_valid_id(const char *str)
{
	size_t len;
	int i;

	len =strlen(str);
	if(len != 2)//길이가 2 이상이면 유효한 ID가 아님
		return -1;

	if(!(isupper(str[0]) && isdigit(str[1])))//첫째자리가 알파벳 대문자, 둘째자리가 숫자인지 확인해 주는 작업
		return -1;

	return 0;
}

/*
 *이미 Task가 큐에 들어있는지 확인해 주는 함수
 *
 *@param : Task ID
 *
 *@return : 존재 : 해당 Task/ 없음 : NULL
 */
static Task *lookup_task(const char *id)
{
	Task *t;

	for(t = tasks; t!= NULL; t= t->next)
	{
		if(!strcmp(t->id,id))
			return t;
	}
	return NULL;
}

/*
 *유효한 process-type인지 확인
 *(H,M,L 중 하나의 타입)
 *
 *@param : process-type
 *
 *@return : 만족 : 0/ 불만족 : -1
 */
static int check_process_type(const char *str)
{
	size_t len;

	len  =strlen(str);

	/*길이가 1이 아닌 경우 지정된 프로세스 타입이 아님*/
	if(len != 1)
		return -1;

	/* process-type이 지정되지 않은 경우 */
	if(len == 0)
		return -1;

	/*'H', 'M', 'L'이외의 문자가 오는 경우 유효하지 않음*/
	if(str[0] != 'H' && str[0] != 'M' && str[0] != 'L')
		return -1;

	return 0;
}

/*
 *유효한 arrive-time인지 확인
 *0 이상 30 이하 정수로 명시된 프로세스 도착 시간(밀리초)
 *
 *@param : arrive-time
 *
 *@return : 만족 : 0/ 불만족 : -1
 */
static int check_arrive_time(const char *str)
{
	float f_num;
	int i_num;

	/*정수인지 판별*/
	f_num = atof(str);
	i_num = atoi(str);
	if(f_num - i_num != 0)
		return -1;

	/*범위가 0이상 30이하인지 판별*/
	if(f_num < 0 || f_num > 30)
		return -1;

	return 0;
}

/*
 *유효한 service-time인지 확인
 *1 이상 30 이하 정수로 명시된 프로세스 서비스 시간(밀리초)
 *
 *@param : service-time
 *
 *@return : 만족 : 0/ 불만족 : -1
 */
static int check_service_time(const char *str)
{
	float f_num;
	int i_num;

	/*정수인지 판별*/
	f_num = atof(str);
	i_num = atoi(str);
	if(f_num - i_num != 0)
		return -1;

	/*범위가 1이상 30이하인지 판별*/
	if(f_num < 1 || f_num > 30)
		return -1;

	return 0;
}

/*
 *유효한 priority인지 확인
 *1 이상 10 이하 정수로 명시된 프로세스 우선 순위
 *
 *@param : priority
 *
 *@return : 만족 : 0/ 불만족 : -1
 */
static int check_priority(const char *str)
{
	float f_num;
	int i_num;

	/*정수인지 판별*/
	f_num = atof(str);
	i_num = atoi(str);
	if(f_num - i_num != 0)
		return -1;

	/*범위가 1이상 10이하인지 판별*/
	if(f_num < 1 || f_num > 10)
		return -1;

	return 0;
}

/*
 *받아온 프로세스를 tasks리스트에 저장해주는 작업
 *
 *@param : task
 */
static void append_task (Task *task)
{
  Task *new_task;

  new_task = malloc (sizeof (Task));//넣어줄 Task의 동적할당
  if (!new_task)//동적할당 오류시
    {
      MSG ("failed to allocate a task: %s\n", STRERROR);
      return;
    }

  *new_task = *task;
  new_task->next = NULL;
  /*리스트에 task를 저장*/
  if (!tasks)
    tasks = new_task;
  else
    {
      Task *t;

      for (t = tasks; t->next != NULL; t = t->next) ;
      t->next = new_task;
    }
}

/*
 *H큐에 Task를 저장
 *
 *@param : Task 
 */
static void Hpush(Task *task)
{
	int i;
	Task *insert_task;
	Task *t;
	Task *prev;

	ready_queue a = H;

	insert_task = malloc(sizeof(Task));//넣어줄 Task를 받을 공간을 동적으로 할당

	if(!insert_task)//동적할당의 실패시 오류문 출력
	{
		MSG("failed to allocate a %stask for insert Hqueue : %s\n",task->id,STRERROR);
		return; 
	}

	*insert_task = *task;
	insert_task->next = NULL;
	prev = NULL;
	
	if(H.rear == NULL)//첫 원소일때
	{
		H.front = insert_task;
		H.rear = insert_task;
	}
	else//첫 원소가 아닐때(priority가 큰 것을 큐의 뒷부분에 넣어줌)
	{
		int check = 0;
		/*priority가 낮은 프로세스를 큐의 앞부분에 넣어준다.*/
		if(H.front == H.rear)//프로세스가 하나일때
		{
			if(insert_task->priority < H.rear->priority)
			{
				H.rear->next = insert_task;
				H.front = insert_task;
			}
			else
			{
				insert_task->next = H.front;
				H.rear = insert_task;
			}
		}
		else if(H.front != H.rear)//프로세스가 여러개일때
		{
			prev = H.rear;
			if(prev->priority <= insert_task->priority)
			{
				insert_task->next = prev;
				H.rear = insert_task;
				check = 1;
			}
			else
			{
				for(t = H.rear->next; t!= NULL;t=t->next)
				{
					if(t->priority <= insert_task->priority)
					{
						prev->next = insert_task;
						insert_task->next = t;
						check = 1;
						break;
					}
				}

				if(check == 0)
				{
					H.front->next = insert_task;
					H.front = insert_task;
				}
			}
		}
	}
	H.cnt++;
	return;
}

/*
 *M큐에 Task를 저장
 *
 *@param : Task
 */
static void Mpush(Task *task)
{
	int i;
	Task *insert_task;
	Task *t;
	Task *prev;

	insert_task = malloc(sizeof(Task));//넣어줄 Task를 받을 공간을 동적으로 할당

	if(!insert_task)//동적할당 실패시 오류문 출력
	{
		MSG("failed to allocate a %stask for insert Mqueue : %s\n",task->id,STRERROR);
		return;
	}

	*insert_task = *task;
	insert_task->next = NULL;
	prev = NULL;

	if(M.rear == NULL)//첫 원소일때
	{
		M.front = insert_task;
		M.rear = insert_task;
	}
	else//첫 원소가 아닐때(remain_service-time이 짧은 것을 큐의 뒷부분에 넣어줌)
	{
		int check = 0;
		/*service-time이 짧은 것을 큐의 앞부분에 넣어준다.*/
		if(M.front == M.rear)//프로세스가 하나일때
		{
			if(insert_task->remain_service_time < M.rear->remain_service_time)
			{
				M.rear->next = insert_task;
				M.front = insert_task;
			}
			else
			{
				insert_task->next = M.front;
				M.rear = insert_task;
			}
		}
		else if(M.front != M.rear)//프로세스가 여러개일때
		{
			prev = M.rear;

			if(prev->remain_service_time <= insert_task->remain_service_time)
			{
				insert_task->next = prev;
				M.rear = insert_task;
				check = 1;
			}
			else
			{
				for(t = M.rear->next; t != NULL; t = t->next)
				{
					if(t->remain_service_time <= insert_task->remain_service_time)
					{
						prev->next = insert_task;
						insert_task->next = t;
						check = 1;
						break;
					}
				}

				if(check == 0)
				{
					M.front->next = insert_task;
					M.front = insert_task;
				}
			}
		}
	}
	M.cnt++;
	return;
}

/*
 *L큐에 Task를 저장
 *
 *@param : Task
 */
static void Lpush(Task *task)
{
	int i;
	Task *insert_task;
	Task *t;

	insert_task=malloc(sizeof(Task));//넣어줄 Task를 받을 공간을 동적으로 할당

	if(!insert_task)//동적할당 실패시 오류문 출력
	{
		MSG("failed to allocated a %stask for insert Lqueue : %s",task->id,STRERROR);
		return;
	}
	
	*insert_task = *task;
	insert_task->next = NULL;
	/*큐에 빨리 도착한 프로세스를 앞부분에 넣어준다.*/
	if(L.front == NULL)//큐가 빈 경우
	{
		L.front = insert_task;
		L.rear = insert_task;
	}
	else//큐에 프로세스가 있는 경우
	{
		insert_task->next = L.rear;
		L.rear = insert_task;
	}
	L.cnt++;
	return;
}

/*
 *큐의 맨 앞에 있는 프로세스를 빼내어 주는 함수
 *
 *@param : 빼내고자 하는 큐
 *
 *@return : 성공 : 해당 프로세스/ 실패 : NULL
 */
static Task *pop_task(ready_queue *q)
{
	Task *t;
	Task *pop;
	pop = NULL;

	if(q->cnt == 0)
	{
		MSG("fail to %cpop: %s\n",q->state,STRERROR);
		return NULL;
	}

	if(q->front == q->rear)
	{
		pop = q->front;
		q->front = NULL;
		q->rear = NULL;
	}
	else if(q->front != q->rear)
	{
		pop = q->front;
		for(t = q->rear; t->next != q->front; t = t->next);
		q->front = t;
	}
	q->cnt--;
	return pop;
}


/*
 *입력 파일을 읽어 Task를 받아오는 작업
 *
 *@param : 입력파일
 *
 *@return : 성공 : 0, 실패 : -1
 */
static int read_input(const char *filename)
{
	FILE *fp;
	char line[INPUT_SIZE * 2];
	int line_nr;
	int process_cnt;//유효한 프로세스의 갯수
	int check_arrive =0;//다음 행의 도착시간이 같거나 나중인지 확인해주는 변수

	fp = fopen(filename,"r");
	if(!fp)
	{
		MSG("fail to load input file '%s'",filename);
		return -1;
	}

	line_nr =0;
	process_cnt = 0;
	//파일을 읽는 작업
	while(fgets(line,sizeof(line),fp))
	{
		Task task;
		char *p;
		char *s;
		size_t len;
		int space_count = 0;//공백문자의 갯수를 판단해주는 변수 
		
		line_nr++;
		memset(&task,0x00,sizeof(task));


		len = strlen(line);
		if(line[len - 1] == '\n')
			line[len - 1] = '\0';

 		/*한 행에 필요 이상의 정보가 있는지 확인*/
		for(int i = 0 ; i < len-1; i++)
		{
			if(line[i] == ' ')
				space_count++;
		}
		
		//확인 메세지
		if(0)
			MSG("data[%3d] %s\n",line_nr,line);

		if(process_cnt >= 260)//프로세스는 최대 260개를 받을 수 있음
		{
			MSG("the number of process is the maximum value\n");
			break;
		}

		strstrip(line);

		if(line[0] == '#' || line[0] =='\0')
			continue;

		//id process-type arrive-time service-time priority 나누는 작업

		/*id*/
		s = line;
		p = strchr(s,' ');
		if(!p)
			goto invalid_line;
		*p ='\0';
		strstrip(s);
		if(check_valid_id(s))//유효한 id인지 체크
		{
			MSG("invalid process id '%s' in line %d,ignored\n",s,line_nr);
			continue;
		}
		if(lookup_task(s))//중첩된 id인지 체크
		{
			MSG("duplicate process id '%s' in line %d,ignored\n",s,line_nr);
			continue;
		}
		strcpy(task.id,s);//구조체에 id 저장

		/*process-type*/
		s = p+1;
		p = strchr(s, ' ');
		if(!p)
			goto invalid_line;
		*p = '\0';
		strstrip(s);
		if(check_process_type(s))//유효한 process-type인지 체크
		{
			MSG("invalid format in line %d,ignored \n",line_nr);
			continue;
		}
		
		task.process_type = s[0];//구조체에 process-tyoe저장

		/*arrive-time*/
		s = p+1;
		p = strchr(s,' ');
		if(!p)
			goto invalid_line;
		*p = '\0';
		strstrip(s);
		if(check_arrive_time(s))//유효한 arrive-time인지 체크
		{
			MSG("invalid arrive-time '%s' in line %d,ignored\n",s,line_nr);
			continue;
		}
		task.arrive_time = atoi(s);//구조체에 arrive-time과 초기 remain_arrive_time저장
		task.remain_arrive_time = atoi(s);

		/*service-time*/
		s = p+1;
		p = strchr(s,' ');
		if(!p)
			goto invalid_line;
		*p = '\0';
		strstrip(s);
		if(check_service_time(s))//유효한 service-time인지 체크
		{
			MSG("invalid service-time '%s' in line %d,ignored\n",s,line_nr);
			continue;
		}

		task.service_time = atoi(s);//구조체에 service-time과 초기 remain-service-time저장
		task.remain_service_time = atoi(s);

		/*priority*/
		s = p+1;
		if(s[0] == '\0')
			goto invalid_line;
		*p = '\0';
		strstrip(s);
		if(check_priority(s))//유효한 priority인지 체크
		{
			MSG("invalid priority '%s' in line %d,ignored\n",s,line_nr);
			continue;
		}

		task.priority = atoi(s);//구조체에 priority 저장

   		if(check_arrive > task.arrive_time)//받은 행의 도착시간이 같거나 나중인지 확인
			goto invalid_line;
		check_arrive = task.arrive_time;

    	/*한 행에 필요 이상의 정보가 있으면 오류처리*/
		if(space_count > 4)
			goto invalid_line;

		process_cnt++;//프로세스의 갯수 카운트
		task.task_num = process_cnt;//프로세스의 번호를 구조체에 저장
		task.start_state = 0;//큐에 들어갔는지 확인해주는 변수 설정

		append_task(&task);//리스트에다가 받아온 task저장(출력시 순서를 알기 위하여 저장을 해 놓음)
		continue;

		invalid_line://유효하지 않은 프로세스에 대한 오류 메세지
			MSG("invalid format in line %d, ignored\n",line_nr);
	}
	fclose(fp);
	return 0;

}

/*
 *받아온 프로세스즐을 실제로 시물레이션을 시켜보는 함수
 */
static void simulation()
{
	Task *t;
	
	/*챠트의 동적할당 크기 설정을 위한 변수*/
	int tot_service_time = 0;
	int tot_arrive_time = 0;
	int process_cnt = 0;//작업이 들어온 프로세스의 갯수

	char **chart;

	int i,j;
	Task *run;
	int cpu_allot = 0;//H,M에 대한 cpu할당량 계산
	int tick = 0;//cpu-time계산
	int check = 0;//테스크 리스트에 있는 모든 프로세스가 준비완료 상태가 되었는지 확인해주는 변수
	double total_turnaround_time = 0;//프로세스마다 종료시각-도착시간을 계산한 값들을 누적해서 더함
	double average_turnaround_time = 0;//평균 완료시간

	double total_waiting_time = 0;//프로세스마다 완료시간 - 실행시간을 계산한 값들을 누적해서 더함
	double average_waiting_time = 0;//평균 대기시간

	/*
	 *리스트에 있는 모든 service-time의 합, 프로세스의 갯수를 구해주는 작업
	 *챠트를 동적할당으로 만들어 주기 위한 작업
	 */
	for(t= tasks; t != NULL; t = t->next)
	{
		tot_service_time += t->service_time;
		tot_arrive_time += t->arrive_time;
		process_cnt++;
	}

	
	/*도표를 만드는 작업*/
	chart = malloc(sizeof(char*) * process_cnt);
	if (!chart)//동적할당 실패시 오류문 출력
	{
		MSG("chart allocated is failed\n");
		return;
	}
	
	for(i = 0 ; i < process_cnt; i++)
	{
		chart[i] = malloc(sizeof(char) * tot_service_time + tot_arrive_time+2);
		if (!chart[i])//동적할당 실패시 오류문 출력
		{
			MSG("chart allocated is failed\n");
			return;
		}
	}

	//챠트 초기화
	for(i = 0 ; i < process_cnt; i++)
	{
		for(j = 0 ; j < tot_service_time+tot_arrive_time+2;j++)
		{
			chart[i][j] = ' ';
		}
		chart[i][j] = '\0';
	}

	run = NULL;
	while(1)
	{
		check = 0;
		Task temp;
		/*도착시간이 0이면 프로세스들을 ready큐에 넣어줌*/
		for(t = tasks; t != NULL; t = t->next)
		{
			temp = *t;
			if(t->start_state == 0)//아직 도착하지 못한 프로세스 존재
				check = 1;

			if(t->remain_arrive_time == 0 && t->start_state == 0)
			{
				if(t->process_type == 'H')
				{
					Hpush(&temp);
				}
				else if(t -> process_type == 'M')
				{
					Mpush(&temp);
				}
				else if(t->process_type == 'L')
				{
					Lpush(&temp);
				}

				t->start_state = 1;
			}
		}
		
		/*실행중인 프로세스가 없을때 모든 큐가 비어있고 테스크 리스트에 있는 모든 작업들이 큐로 들어간 상태이면 시뮬레이션을 종료*/
		if(run == NULL && H.cnt == 0 && M.cnt == 0 && L.cnt == 0 && check == 0) break;


		if(run == NULL)//실행중인 프로세스가 없는 경우 적절한 프로세스를 큐에서 뺴내옴
		{
			if(H.cnt != 0 && M.cnt != 0)//H,M큐는 L큐보다 우선순위가 높기 때문에 비어있는지 먼저 확인
			{
				//cpu할당량에 따라서 뽑음
				if(cpu_allot % 10 < 6)
				{
					run = pop_task(&H);
				}
				else
				{
					run = pop_task(&M);
				}
			}
			else if(H.cnt != 0 && M.cnt == 0)
			{
				run = pop_task(&H);
			}
			else if(H.cnt == 0 && M.cnt != 0)
			{
				run = pop_task(&M);
			}
			else if(H.cnt == 0 && M.cnt == 0 && L.cnt != 0)//H,M큐 둘다 비어 있고 L큐에 프로세스가 있으면 프로세스를 가져옴
			{
				run = pop_task(&L);
			}
		}
		else//실행중인 프로세스가 있는 경우
		{
			//H프로세스인 경우 preemption이 일어날건지 확인
			if(run->process_type == 'H')
			{
				Task *temp2;
				temp2 = NULL;
				if(H.rear != NULL)
				{
					for(t = H.rear; t != NULL; t = t->next)
					{
						if(run->priority >  t->priority && t->remain_arrive_time == 0)
						{
							temp2 = t;
						}
					}

					if(temp2 != NULL)
					{
						Hpush(run);
						run = pop_task(&H);
					}

				}
			}
			//H 혹은 M 프로세스인 경우 cpu할당량이 끝났는지 확인
			if(run->process_type == 'H')
			{
				if(cpu_allot % 10 >= 6 && M.cnt != 0)
				{
					Hpush(run);
					run = pop_task(&M);
				}
			}
			else if(run->process_type == 'M')
			{
				if(cpu_allot%10 < 6 && H.cnt != 0)
				{
					Mpush(run);
					run = pop_task(&H);
				}
			}

		}

		//프로세스 작업을 수행
		if(0)//확인 메세지
		{
			if(run != NULL)
			{
				MSGOUT("run process: %s..\n",run->id);
			}
		}
		/*챠트를 작성해주는 작업*/
		tick++;//틱수를 늘려줌
		if(run != NULL)
		{
			chart[run->task_num-1][tick] = '*';
			run->remain_service_time--;//남아있는 서비스 타임을 없애줌
			if(run -> process_type == 'H' || run->process_type == 'M')//cpu할당량 조절을 위한 작업
			{
				cpu_allot++;
			}
			if(run->remain_service_time == 0)//service-time만큼 수행을 하면 실행중에서 빠져나옴
			{
				for(t = tasks ; t != NULL; t= t->next)
				{
					if(t->task_num == run->task_num) break;
				}
				total_turnaround_time += tick - t->arrive_time;//종료 시간- 도착시간을 누적해서 저장
				total_waiting_time += (tick-t->arrive_time) - t->service_time;//완료시간 - 실행시간을 누적해서 저장
				run = NULL;
			}
		}

		usleep(1000);//1밀리초

		/*아직 도착하지 못한 프로세스의 도착시간을 줄여주는 작업*/
		for(t = tasks; t != NULL; t = t->next)
		{
			if(t->remain_arrive_time != 0 && t->start_state == 0)
			{
				t->remain_arrive_time--;
			}
		}

	}
	
	/*챠트 출력*/
	MSG("[Multilevel Queue Scheduling]\n");
	t = tasks;
	for(i = 0 ; i < process_cnt; i++)
	{
		printf("%s %s\n",t->id,chart[i]);
		t = t->next;
	}
	/*cpu time 출력*/
	MSGOUT("\nCPU TIME : %d\n",tick);

	/*average turnaround time출력*/
	average_turnaround_time = total_turnaround_time / (double)process_cnt;
	MSGOUT("AVERAGE TURNAROUND TIME: %.2lf\n", average_turnaround_time);
	
	/*average waiting time출력*/
	average_waiting_time = total_waiting_time / (double)process_cnt;
	MSGOUT("AVERAGE WAITING TIME: %.2lf\n",average_waiting_time);

	/*동적할당 해제*/
	for(i = 0 ; i < process_cnt; i++)
	{
		free(chart[i]);
	}
	free(chart);
	
	return;
}

int main(int argc, char **argv)
{
	/*큐 관리 구조체의 초기화 작업*/
	memset(&H,0,sizeof(ready_queue));
	memset(&M,0,sizeof(ready_queue));
	memset(&L,0,sizeof(ready_queue));

	H.state = 'H';
	M.state = 'M';
	L.state = 'L';

	/*입력 파일을 읽는 작업(실패시 오류 문구를 출력하고 종료)*/
	if(argc <= 1)
	{
		MSG("input file must specified\n");
		return -1;
	}
	if(read_input(argv[1]))
	{
 		MSG("failed to load input file '%s'\n",argv[1]);
		return -1;
	}

	simulation();//시뮬레이션 수행

	return 0;
}
