#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MSG(x...) fprintf(stdout,x)

/*
 * 리눅스 커널 버전을 얻어옴
 */
static void get_version()
{
	char buff[10000];
	size_t size;

	FILE *version = fopen("/proc/version","r");// /proc/version파일을 열음

	size = fread(buff,1,sizeof(buff),version);
	if(size == 0 ||size == sizeof(buff))
	{
		MSG("version 정보를 정확히 읽어오지 못하였습니다.\n");
		return;
	}

	buff[size] = '\0';

	MSG("리눅스 커널 버전 : %s\n",buff);

	fclose(version);
}

/*
 *시스템의 cpu갯수 ,클럭속도, 코어의 갯수를 얻어옴
 */
static void get_cpuinfo()
{
	char buff[10000];
	int cpu_ids[100];//physical id가 중접이 되는지 확인하기 위한 변수
	char *p;
	char *q;

	int i;
	int cpu_count = 0;
	int check = 0;
	int clock_check = 0;
	int core_check = 0;
	double cpu_clock = 0;
	int cpu_cores;
	
	FILE *cpuinfo = fopen("/proc/cpuinfo","r");// /proc/cpuinof파일을 열음

	for(i = 0 ; i < 100; i++)
	{
		cpu_ids[i] = -100;
	}

	while(fgets(buff,1024,cpuinfo) != NULL)//정보를 한 행씩 받아옴
	{
		p = strstr(buff,"physical id");//물리적 cpu갯수를 카운트 해주는 작업
		if(p != NULL)
		{
			while(!(*p >= 48 && *p <= 57))
				p = p+1;

			q = p;
			while(*q >= 48 && *q <= 57)
				q = q+1;
			*q = '\0';
			
			int id = atoi(p);

			for(i = 0; i < 100; i++) //중첩이 되어 있는지 체크를 해 줌
			{
				if(cpu_ids[i] == id)
				{
					check = 1;
					break;
				}
			}

			if(check == 0)//중접이 되지 않은 id이면 카운트를 해 주고 기록을 해 줌
			{
				cpu_count++;
				for(i = 0 ; i < 100 ; i++)
				{
					if(cpu_ids[i] == -100)
					{
						cpu_ids[i] = id;
						break;
					}
				}
			}

			check = 0;
			continue;
		}
		
		if(clock_check == 0) p = strstr(buff,"cpu MHz");//클럭 속도를 구하는 작업

		if(p != NULL)
		{
			while(!(*p >= 48 && *p <= 57)) 
				p = p+1;
			q = p;
			while((*q >= 48 && *q <= 57) || *q == '.')
				q = q+1;

			*q = '\0';

			cpu_clock = atof(p);

			clock_check = 1;
			continue;
		}

		if(core_check == 0) p = strstr(buff,"cpu cores");//코어의 갯수를 구하는 작업

		if(p != NULL)
		{
			while(!(*p >= 48 &&*p <= 57))
				p = p+1;
			q = p;
			while((*q >= 48 && *q <= 57))
				q = q+1;

			*q = '\0';
			
			cpu_cores = atoi(p);

			core_check = 1;
			continue;
		}
	}

	MSG("시스템의 CPU갯수 : %d  클럭 속도 : %lf MHz  코어의 갯수 : %d\n\n",cpu_count,cpu_clock,cpu_cores);

	fclose(cpuinfo);

}

/*
 *시스템의 사용 중인 메모리 및 사용 가능한 메모리를 구함
 */
static void get_meminfo()
{
	char buff[10000];
	size_t size;

	int tot;
	int free;

	int count = 0;

	char* s;
	char* p;
	char* q;

	FILE *meminfo = fopen("/proc/meminfo","r");// /proc/meminfo파일을 열음

	size = fread(buff,1,sizeof(buff),meminfo);
	if(size == 0 || size == sizeof(buff))
	{
		MSG("meminfo 정보를 정확히 읽어오지 못하였습니다.\n");
		return;
	}

	s = buff;

	p = strstr(buff,"MemTotal");//전체 메모리의 용량을 가져옴

	while(!(*p >= 48 && *p <= 57))
		p = p+1;
	q = p;

	while(*q >= 48 && *q <= 57)
		q = q+1;
	*q = '\0';

	tot = atoi(p);

	s = q+1;

	p = strstr(s,"MemFree");//사용 가능한 메모리의 용량을 가져옴

	while(!(*p>=48 && *p <= 57))
		p = p+1;

	q = p;

	while(*q >= 48 && *q <= 57)
		q = q+1;
	*q = '\0';

	free = atoi(p);

	MSG("사용 중인 메모리 : %dkB 사용 사능한 메모리 : %dkB\n\n",tot-free,free);//사용중인 메모리는 전체 메모리 용량에서 사용 중인 메모리 용량을 빼준 용량이다.
	
	fclose(meminfo);
}

/*
 *시스템이 마지막으로 부팅된 시간을 구함
 */
static void get_uptime()
{
	char buff[10000];
	size_t size;
	time_t current_time;
	char *p;

	time(&current_time);

	FILE *uptime = fopen("/proc/uptime","r");// /proc/uptime파일을 열음

	size = fread(buff,1,sizeof(buff),uptime);
	if(size == 0 || size == sizeof(buff))
	{
		MSG("uptime 정보를 정확히 읽어오지 못하였습니다.\n");
		return;
	}

	buff[size] = '\0';

	p = strtok(buff," ");//마지막으로 부팅이 되고 지난 시간을 받아옴

	current_time -= atol(p);//현재 시각에서 지난 시간을 빼줌

	MSG("시스템이 마지막으로 부팅된 시간 : %s\n",ctime(&current_time));

	fclose(uptime);
}

/*
 *문맥 교환 개수를 구함
 */
static void get_stat()
{
	char buff[10000];
	size_t size;

	char *p;
	char *s;
	char *q;

	FILE *stat = fopen("/proc/stat","r");// /proc/stat파일을 열음

	size = fread(buff,1,sizeof(buff),stat);
	if(size == 0 || size == sizeof(buff))
	{
		MSG("stat 정보를 정확히 읽어오지 못하였습니다.\n");
		return;
	}

	buff[size] = '\0';
	s = buff;

	p = strstr(s,"ctxt");//ctxt에 해당하는 정보가 문맥교환의 개수이다.

	while(!(*p >= 48 &&*p <= 57))
		p = p+1;

	q = p;

	while(*q >= 48 && *q <= 57)
		q = q+1;

	*q = '\0';
	
	MSG("문맥 교환(context switches) 개수 : %s\n\n",p);

	fclose(stat);
}

/*
 *인터럽트 개수
 */
static void get_interrupts()
{
	char buff[10000];
	char *p;

	int tot = 0;

	int i, len;

	int check = 0 ;

	FILE *interrupts = fopen("/proc/interrupts","r");// /proc/interrupts파일을 열음

	while(fgets(buff,1024,interrupts) != NULL)//각각의 cpu에서 인터럽트의 종류마다 발생 갯수를 누적해서 더해줌
	{
		check = 0;

		p = strtok(buff," ");

		len = strlen(p);
		
		for(i = 0 ; i < len ; i++)
		{
			if(p[i] < 48 || p[i] > 57)
			{
				check = 1;
				break;
			}
		}

		if(check == 0) tot += atoi(p);

		while(p = strtok(NULL," "))
		{
			check = 0;
			len = strlen(p);

			for(i = 0 ; i < len; i++)
			{
				if(p[i] < 48 || p[i] > 57)
				{
					check = 1;
					break;
				}
			}

			if(check == 0) tot += atoi(p);
		}

	}

	MSG("인터럽트 개수 : %d\n\n",tot);

	fclose(interrupts);
}

/*
 *최근 15분 동안 부하 평균을 구함
 */
static void get_loadavg()
{
	char buff[10000];
	size_t size;
	char *s;
	char* p;
	int i;

	FILE *loadavg = fopen("/proc/loadavg","r");// /proc/loadavg파일을 열음
	
	size = fread(buff,1,sizeof(buff),loadavg);
	if(size == 0 || size == sizeof(buff))
	{
		MSG("loadavg 정보를 정확히 읽어오지 못하였습니다.\n");
		return;
	}

	buff[size] = '\0';
	s = buff;
	
	//3번째 부분이 15분 동안의 부하 평균이므로 해당 수치를 가져옴
	for(i = 0 ; i < 2; i++)
	{
		p = strchr(s,' ');
		s = p+1;
	}
	
	p = strchr(s,' ');
	*p = '\0';

	
	MSG("최근 15분 동안 부하 평균 : %s\n\n",s);
	fclose(loadavg);
}

int main()
{
	get_version();
	get_cpuinfo();
	get_meminfo();
	get_uptime();
	get_stat();
	get_interrupts();
	get_loadavg();
	return 0;
}

