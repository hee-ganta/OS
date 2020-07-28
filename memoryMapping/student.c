#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#define MSG(x...) fprintf (stderr,x);

char filename[100];
char value[100];
char attr_name[100];

void insert(int check);
void show(int check);

/*
 *file을 Student 구조체의 메모리와 매핑 후에 값을 넣는 함수
 *
 *@param : check의 값에 따라 -f 속성이 지정이 되어 있는지 안되어 있는지 구별
 */
void insert(int check)
{
	char *buf;
	buf = malloc(sizeof(Student));
	char *r_buf;
	r_buf = malloc(sizeof(Student));

	int fd;
	struct stat sb;
	int flag = PROT_WRITE | PROT_READ;

	static Student *student;
	char *offset;
	int count;
	int i;
	ssize_t f_size;
	int full_check = 0;

	
	if(check == 1)//filename이 지정되어 있는 경우
	{
		if((fd = open(filename,O_RDWR|O_CREAT, 0644)) < 0)
		{
			perror("파일 열기 에러\n");
			exit(1);
		}
	}
	else if(check == 3)//filename이 지정이 안 되어 있는 경우 
	{
		if((fd = open("./student.dat",O_RDWR|O_CREAT,0644)) < 0)
		{
			perror("파일 열기 에러\n");
			exit(1);
		}
	}

	if(fstat(fd,&sb) < 0)//파일의 정보를 얻어오며 오류시 오류문을 출력
	{
		perror("파일 정보 얻기 에러\n");
		exit(1);
	}

	f_size = read(fd,r_buf,sizeof(Student));//파일을 읽어들임

	if(f_size == 0)//만약 읽어들인 파일의 내용 크가가 0이면 새로 생성된 파일이므로 구조체의 크기만큼 0x00값을 써줌
	{
		memset(buf,0x00,sizeof(Student));
		write(fd,buf,sizeof(Student));
	}

	student = mmap(0,sizeof(Student),flag,MAP_SHARED,fd,0);//해당 파일을 메모리와 매핑함

	if(student == (Student *)-1)//매핑오류시 오류문을 출력
	{
		perror("메모리 매핑 에러\n");
		exit(1);
	}
	
	offset = (char*)student;//메모리의 맨 앞 부분은 offset이 가리키게 함
	
	count = student_get_offset_of_attr(attr_name);//해당 필드의 맨 앞부분의 offset값을 가져옴
	
	if(count == 0 && (strcmp(attr_name,"name") != 0))//만약 Student구조체에 없는 필드라면 오류문 출력후 종료
	{
		MSG("해당 속성은 존재하지 않음\n");
		exit(1);
	}

	for(i=0; i < count;i++)//offset을 해당 필드 앞에다가 위치시켜줌
	{
		offset = offset+1;
	}

	for(i = 0 ; i < strlen(value);i++)//값을 써주는 작업
	{
		if(i != 0 && (strcmp(studnet_lookup_attr_with_offset(i+count), "unknown") != 0))
		{
			full_check = 1;
			break;
		}
		*offset = value[i];
		offset = offset+1;
	}
	
	if(full_check == 0)
	{
		*offset = 0x00;
	}
	

	if(munmap(student,sizeof(Student)) == -1)//파일과 메모리의 매핑을 해제함
	{
		perror("메모리 매핑 해제 에러\n");
		exit(1);
	}
	
	free(buf);
	free(r_buf);
	close(fd);
	
	show(check+1);//결과값을 출력
}

/*
 *file을 Student구조체의 메모리에 매핑 후에 값을 읽어오는 함수
 *
 *@param : check의 값에 따라 -f 속성이 지정이 되어 있는지 안되어 있는지 구별 
 */
void show(int check)
{
	char *buf;
	buf = malloc(sizeof(Student));
	char *r_buf;
	r_buf = malloc(sizeof(Student));

	int fd;
	struct stat sb;
	int flag = PROT_READ;

	static Student *student;
	char *offset;
	int count;
	int i;
	ssize_t f_size;
	int c = 0;

	if(check == 2)//filename이 지정되어 있는 경우
	{
		if((fd = open(filename,O_RDWR|O_CREAT,0644)) < 0)
		{
			perror("파일 열기 에러\n");
			exit(1);
		}
	}
	else if(check == 4)//filename이 지정이 안 되어있는 경우
	{
		if((fd = open("./student.dat",O_RDWR|O_CREAT,0644)) < 0)
		{
			perror("파일 열기 에러\n");
			exit(1);
		}
	}

	if(fstat(fd,&sb) < 0)//파일을 조사하고 정상적으로 끝나지 않았을시 오류문 출력
	{
		perror("파일 정보 얻기 에러\n");
		exit(1);
	}

	f_size = read(fd,r_buf,sizeof(Student));//파일을 읽음

	if(f_size == 0)//만약 읽어들인 파일의 내용 크가가 0이면 새로 생성된 파일이므로 구조체의 크기만큼 0x00값을 써줌
	{
		memset(buf,0x00,sizeof(Student));
		write(fd,buf,sizeof(Student));
	}

	student = mmap(0,sizeof(Student),flag,MAP_SHARED,fd,0);//파일을 메모리와 매핑을 함

	if(student == (Student *)-1)//매핑오류시 오류문을 출력
	{
		perror("메모리 매핑 에러\n");
		exit(1);
	}

	offset = (char *)student;//Student구조체의 맨 앞부분에 offset을 가져다 놓음

	count = student_get_offset_of_attr(attr_name);//해당 필드의 맨 앞부분의 offset값을 가져옴

	if(count == 0 && (strcmp(attr_name,"name") != 0))//만약 Student구조체에 없는 필드라면 오류문 출력후 종료
	{
		MSG("해당 속성은 존재하지 않음\n");
		exit(1);
	}

	for(i = 0 ; i < count; i++)//해당 필드 앞부분에 offset을 가져다 놓음
	{
		offset = offset +1;
	}

	memset(r_buf,0x00,sizeof(Student));//읽어들인 정보를 저장할 변수의 배열 초기화 작업

	while(1)
	{
		//해당 필드 전체 혹은 끝부분이 나타날때 까지 정보를 읽음
		if(*offset == 0x00 || (c != 0 && (strcmp(studnet_lookup_attr_with_offset(c+count) , "unknown") != 0)))
		{
			break;
		}

		r_buf[c] = *offset;//정보를 문자열에 저장
		offset = offset+1;
		c++;
	}

	r_buf[c] = '\0';
	
	//정수인지 문자열인지 판단 후 해당 형식에 맞게 출력 
	if(student_attr_is_integer(attr_name) == 0)
	{
		printf("%s\n",r_buf);
	}
	else
	{
		int v = atoi(r_buf);
		printf("%d\n",v);
	}

	if(munmap(student,sizeof(Student)) == -1)//파일과 메모리의 매핑을 종료
	{
		perror("메모리 매핑 해제\n");
		exit(1);
	}

	free(buf);
	free(r_buf);
	close(fd);
}

int main(int argc, char **argv)
{
	filename[0] = '\0';
	value[0] = '\0';
	attr_name[0] = '\0';

	int check = 0;

	char *buf;
	buf = malloc(sizeof(Student));
	
	int i,j;
	int f_check= 0;
	int s_check = 0;
	int c= 0;

	//인자로 -f , -s이 들어왔는지 판별과 그에 대한 케이스를 분류하는 작업
	//f_check : 1 - "-f"가 들어있음, 0 - "-f"가 들어있지 않음
	//s_check : 1 - "-s"가 들어있음, 0 - "-s"가 들어있지 않음
	for(i = 1; i < argc; i++)
	{
		if(!strcmp("-f",argv[i]))
		{
			f_check = 1;
			
			i++;
			while(1)
			{
				if(i == argc-1) break;
				
				if(!strcmp("-s" , argv[i])) break;

				if(c != 0)
				{
					filename[c] = ' ';
					c++;
				}

				for(j = 0; j < strlen(argv[i]);j++)
				{
					if(argv[i][j] != '"')
					{
						filename[c] = argv[i][j];
						c++;
					}
				}
				i++;
			}
		}

		c = 0;

		if(!strcmp("-s",argv[i]))
		{
			s_check  = 1;
			i++;
			while(1)
			{
				if(i == argc-1) break;
				
				if(c != 0)
				{
					value[c] = ' ';
					c++;
				}

				for(j = 0 ; j < strlen(argv[i]) ; j++)
				{
					if(argv[i][j] != '"')
					{
						value[c] = argv[i][j];
						c++;
					}
				}
				i++;
			}
		}
		
		if(i == argc-1)
		{
			strcpy(attr_name,argv[i]);
		}
	}
	
	//각각의 경우마다 케이스 분류를 해 줌
	if(f_check == 1 && s_check == 1) check = 1;//"-f", "-s"가 다 있는 졍우
	else if(f_check == 1 && s_check == 0) check = 2;//"-f" 만 있는 경우
	else if(f_check == 0 && s_check == 1) check = 3;//"-s" 만 있는 졍우
	else check = 4;//둘 다 없는 경우

	if(check  == 1 || check == 3)//값을 설정하는 작업을 수행
		insert(check);
	else if(check == 2 || check == 4)//값을 가져오는 작업을 수행
		show(check);
	
	return 0;
}

