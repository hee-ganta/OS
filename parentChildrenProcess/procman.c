/**
 * OS Assignment #1
 **/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)
#define handle_error(msg)\
	do{perror(msg); exit(EXIT_FAILURE);}while(0)//signalfd 함수 사용시 에러문 호출시 사용

#define ID_MIN 2
#define ID_MAX 8
#define COMMAND_LEN 256

/*Task의 action의 분류*/
typedef enum
{
  ACTION_ONCE,
  ACTION_RESPAWN,

} Action;

/*Task 속성을 담을 구조체의 선언*/
typedef struct _Task Task;
struct _Task
{
  Task          *next;

  volatile pid_t pid;
  int            piped;
  int            pipe_a[2];
  int            pipe_b[2];

  char           id[ID_MAX + 1];
  char           pipe_id[ID_MAX + 1];
  Action         action;
  char           command[COMMAND_LEN];
  int 			 order;//순서를 저장할 변수<추가>
};

/*여러개의 Task들을 관리할 포인터 변수의 선언*/
static Task *tasks;

static volatile int running;

/*
 *문자열의 앞,뒤 공백문자들을 제거해 주는 변수
 *
 *@param : 공백문자들을 없애고자 하는 문자열
 *
 *@return : 공백문자들을 없앤 문자열
 */
static char *
strstrip (char *str)
{
  char  *start;
  size_t len;

  len = strlen (str);
  while (len--)//문자열의 길이만큼 문자 하나하나를 조사
    {
      if (!isspace (str[len]))//문자열 끝이 공백이 아닌경우
        break;//반복문을 빠져나옴
      str[len] = '\0';//공백문자이면 문자열의 끝을 지정하고 그 이전 문자가 공백문자인지 확인
    }
  /*문자열 앞에 공백문자들을 제거해 주는 작업*/
  for (start = str; *start && isspace (*start); start++)//문자열의 첫 부분부터 문자가 없거나 공백문자가 아닌 곳까지 start포인트를 이동
    ;
  memmove (str, start, strlen (start) + 1);//공백문자가 아닌 문자를 가리키는 곳 부터 문자열을 str포인터가 가리키는 위치에다가 문자열을 다시 세팅해줌

  return str;//공백문자들을 없애준 문자열을 리턴
}

/*
 *Task id가 유효한지 판별해 주는 함수
 *
 *@param : Task id가 적혀 있는 문자열
 *
 *@return : 유효 : 0
 *          유효하지 않음 : -1
 */
static int
check_valid_id (const char *str)
{
  size_t len;
  int    i;

  len = strlen (str);//문자열의 길이를 가져옴
  if (len < ID_MIN || ID_MAX < len)//id길이가 최소 지정 길이보다 작거나 최대 지정 길이보다 크면 유효하지 않음
    return -1;

  for (i = 0; i < len; i++)//문자열에 있는 문자들을 조사
    if (!(islower (str[i]) || isdigit (str[i])))//영어 소문자 이거나 숫자가 아니라면 유효하지 않음
      return -1;

  return 0;
}

/*
 *해당 id에 해당하는 Task를 리스트에서 가져옴(문자열의 data)
 *
 *@param : 찾고자 하는 Task id
 *
 *@return : 해당하는 Task (존재하지 않다면 NULL을 리턴)
 */
static Task *
lookup_task (const char *id)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)//리스트를 순회하면서 조사
    if (!strcmp (task->id, id))//Task id와 찾고자 하는 id가 일치하면 그 task를 리턴
      return task;

  return NULL;//리스트를 다 순회하였는데 발견하지 못하면 NULL을 리턴
}

/*
 *해당 id에 해당하는 Task를 리스트에서 가져옴(pid_t의 data)
 *
 *@param : 찾고자 하는 Task pid
 *
 *@return : 해당하는 Task (존재하지 않다면 NULL을 리턴)
 */
static Task *
lookup_task_by_pid (pid_t pid)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)//리스트를 순회하면서 조사
    if (task->pid == pid)//Task pid와 찾고자 하는 pid가 일치하면 그 task를 리턴
      return task;

  return NULL;//리스트를 다 순회하였는데 발견하지 못하면 NULL을 리턴
}

/*
 *설정파일에서 추가할 Task의 정보들을 받아와서 리스트에 추가하는 함수
 *
 *order부분 순서에 맞게 리스트에 추가<추가기능>
 *
 *@param : 리스트에 추가하고자 하는 Task
 */
static void
append_task (Task *task)
{
  Task *new_task;
  Task *prev = NULL;

  new_task = malloc (sizeof (Task));//새로 추가하조가 하는 task를 받을 공간
  if (!new_task)//동적할당 실패시 에러문 호출 후 함수 종료
    {
      MSG ("failed to allocate a task: %s\n", STRERROR);
      return;
    }

  /*새롭게 넣어줄 Task설정*/
  *new_task = *task;
  new_task->next = NULL;

  if (!tasks)//리스트가 비어있을 때는 리스트의 첫부분으로 설정
    tasks = new_task;
  else//리스트가 비어있지 않다면 다음과 같이 설정
    {
      Task *t;

	  t = tasks;//삽입하고자 하는 위치를 찾기 위해 리스트 첫 부부분부터 order값을 비교하여 탐색
	  prev = NULL;//삽입하고자 하는 위치의 전 tasks의 리스트 부분은 알아야 연결이 가능하기 때문에 따로 저장해 둠

	  while(1)
	  {
		  /*리스트의 끝일 경우*/
		  if(t->next == NULL)
		  {
			 if(t->order <= new_task->order)//new_task의 order가 큰 경우 나중에 수행<같을 경우에도 상위 행에 있는 프로그램을 먼저 수행> -> 리스트릐 뒷부분
			 {
				 t->next = new_task;
				 break;
			 }
			 else//new_task의 order가 작은 경우 먼저 수행 -> 리스트의 앞 부분
			 {
				if(prev != NULL)//리스트 크기가 2이상일 경우에는 prev가 NULL이 아니기 때문에 연결을 시켜줌
				{
					prev->next = new_task;//넣고자 하는 위치의 이전 Task와 연결을 해 줌
					new_task->next = t;//new_task의 next 설정을 해 줌
				}
				else
				{
					new_task->next = t;//new_task의 next  설정을 해 줌
					tasks = new_task;//헤더의 설정
				}
				break;
			 }

		  }
		  else//끝이 아닐 경우
		  {
			  if(t->order <= new_task->order)//현재의 포인트보다 new_task 의 order가 더 큰 경우 계속 리스트에서 알맞은 위치를 찾아봐야 함
			  {
				  prev = t;
				  t = t->next;
			  }
			  else//만약 현재의 포인트보다 new_task의 order가 작은 경우 리스트에 삽입후 반복문을 빠져나옴
			  {
				  if(prev != NULL)//prev가 NULL이 아닌 경우 연결을 시켜줌 
				  {
					  prev->next = new_task;//넣고자 하는 위치의 이전 Task와 연결을 해 줌
					  new_task->next = t;//new_task의 next 설정을 해 줌
				  }
				  else
				  {
				 	 new_task->next = t;//new_task의 next 설정을 해 줌
					 tasks = new_task;//헤더의 설정
				  }
				  break;
			  }
		  }

	  }

    }
}

/*
 * order 부분이 양식에 맞는지 판별해주는 함수<추가가능>
 *
 *@param : 파일에서 읽어들인 order 문자열
 *
 *@return : 성공 -> 1을 반환
 *			실패 -> 0을 반환 
 */

static int
order_check(const char* str)
{
	int i;
	size_t size = strlen(str);

	/*문자열 길이가 4이하인지 확인*/
	size = strlen(str);
	if(size > 4)
	{
		return 0;
	}

	/*문자열에 숫자가 아닌 수가 들어있는지 확인*/
	for(i = 0; i < (int)size; i++)
	{
		if(!isdigit(str[i]))
		{
			return 0;
		}
	}
	return 1;
}


/*
 *설정파일을 읽어들이는 작업을 하는 함수
 *
 *@param : 읽어들일 설정파일의 이름
 *
 *@return : 성공 -> 0을 반환
 *          실패 -> -1을 반
 */
static int
read_config (const char *filename)
{
  FILE *fp;
  char  line[COMMAND_LEN * 2];
  int   line_nr;

  fp = fopen (filename, "r");//설정파일을 read모드로 열음
  if (!fp)//파일 열기 실패시 -1을 반환
    return -1;

  tasks = NULL;//리스트를 초기화 

  line_nr = 0;
  while (fgets (line, sizeof (line), fp))//최대 line의 크기만큼 한 문장씩 설정파일에서 받아옴
    {
      Task   task;
      char  *p;
      char  *s;
      size_t len;

      line_nr++;//라인의 수를 증가
      memset (&task, 0x00, sizeof (task));//task의 메모리에 있는 값을 0X00으로 지정해줌

      len = strlen (line);//라인의 크기를 구해냄
      if (line[len - 1] == '\n')//만약 문자열의 끝 부분이 개행문자라면 문자열의 끝을 표시해주는 '\0'문자로 바꿔줌
        line[len - 1] = '\0';

      if (0)
        MSG ("config[%3d] %s\n", line_nr, line);

      strstrip (line);//문자열 앞 뒤 공백문자들을 없애주는 작업 수행

      /* comment or empty line */
      if (line[0] == '#' || line[0] == '\0')//만약 주석문이거나 줄바꿈 행이면 다음 행을 조사하러 감
        continue;

	  /*id:action:order:pipe-id:command를 분리하는 작업*/

      /* id */
      s = line;
      p = strchr (s, ':');//해당 문자열 안에서 처음으로 나온 ':' 문자 검사
      if (!p)//만약 없다면 다음 행을 조사하러 감
        goto invalid_line;
      *p = '\0';//':'문자를 문자열의 끝을 의미하는 '\0'문자로 바꿔줌<이로써 해당 id만 한 문자열로써 s가 그 문자열을 가리키게 됨>
      strstrip (s);//문자열의 앞뒤 공백문자들을 제거
      if (check_valid_id (s))//유효한 id인지 확인(오류이면 오류 메세지를 출력)
        {
          MSG ("invalid id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      if (lookup_task (s))//해당 id에 해당하는 task가 있는지 확인(없으면 오류 메세지 출력)
        {
          MSG ("duplicate id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      strcpy (task.id, s);//있으면 구조체에 해당 id를 넣어줌

      /* action */
      s = p + 1;//id 다음 부분부터 포인터를 바꿈
      p = strchr (s, ':');//해당 문자열 안에서 처음으로 나온 ':' 문자 검사
      if (!p)//만약 없다면 다음 행을 조사하러 감
        goto invalid_line;
      *p = '\0';//':'문자를 문자열의 끝을 의미하는 '\0'문자로 바꿔줌<이로써 해당 action만 한 문자열로써 s가 그 문자열을 가리키게 됨>
      strstrip (s);//문자열의 앞뒤 공백문자를 제거
      if (!strcasecmp (s, "once"))//대소문자를 비교하지 않고 문자열이 같은지 다른지 비교
        task.action = ACTION_ONCE;
      else if (!strcasecmp (s, "respawn"))
        task.action = ACTION_RESPAWN;
      else//두 문자열 전부 해당하지 않으면 오류 메세지를 출력하고 다음 행을 조사하러 감
        {
          MSG ("invalid action '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }

	  /*order <추가기능>*/
	  s = p+1;//action 다음 부분부터 포인터를 바꿈
	  p = strchr(s,':');//해당 문자열 안에서 처음으로 나온 ':' 문자 검사
	  if(!p)//만약 없다면 다음 행을 조사하러 감
		  goto invalid_line;
	  *p = '\0';//':'문자를 문자열의 끝을 의미하는 '\0'문자로 바꿔줌<이로써 해당 order만 한 문자열로써 s가 그 문자열을 가리키게 됨>
	  strstrip(s);//문자열의 앞뒤 공백문자를 제거
	  if(!order_check(s))//해당 order가 조건에 맞는 order인지 체크하고 아닐 경우 오류메세지를 출력하고 다음 행을 조사하러 감
	  {
		  MSG("invaild order '%s' in line %d, ignored\n",s,line_nr);
		  goto invalid_line;
	  }
	  //공백인 경우 임의의 순서를 rand함수를 통하여 임의의 순서를 생성<이때, 조건에 order의 자리수는 최대4자리 임으로 0부터 9999까지의 숫자중 임의의 숫자를 생성해줌>
	  if(strlen(s) == 0)
	  {
		  task.order = rand() % 10000;
	  }
	  else
	  {
	 	 task.order = atoi(s);//공백이 아닌 경우 해당 순서값을 저장
	  }


      /* pipe-id */
      s = p + 1;//order 다음 부분부터 포인터를 바꿈
      p = strchr (s, ':');//해당 문자열 안에서 처음으로 나온 ':' 문자 검사
      if (!p)//만약 없다면 다음 행을 조사하러 감
        goto invalid_line;
      *p = '\0';//':'문자를 문자열의 끝을 의미하는 '\0'문자로 바꿔줌<이로써 해당 pipe-id만 한 문자열로써 s가 그 문자열을 가리키게 됨>
      strstrip (s);//문자열의 앞뒤 공백문자를 제거
      if (s[0] != '\0')//만약 공백이 아니라면
        {
          Task *t;

          if (check_valid_id (s))//유효한 id인지 체크
            {
              MSG ("invalid pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }

          t = lookup_task (s);//해당 id가 리스트에 있는지 확인
          if (!t)//리스트에 없으면 오류 메세지
            {
              MSG ("unknown pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }
          if (task.action == ACTION_RESPAWN || t->action == ACTION_RESPAWN)//task의 action이 respawn일때는  오류 메세지 
            {
              MSG ("pipe not allowed for 'respawn' tasks in line %d, ignored\n", line_nr);
              continue;
            }
          if (t->piped)//pipe하고자 하는 task가 이미 다른 task와 연결되어 있는 경우 오류메세지
            {
              MSG ("pipe not allowed for already piped tasks in line %d, ignored\n", line_nr);
              continue;
            }

          strcpy (task.pipe_id, s);//pipe-id를 구조체에 저장
          task.piped = 1;//연결 되어 있음을 체크
          t->piped = 1;//pipe-id에 해당하는 task도 연결 되어 있음을 체크
        }

      /* command */
      s = p + 1;//pipe-id 다음 부분부터 포인터를 바꿈
      strstrip (s);//문자열의 앞뒤 공백문자를 제거
      if (s[0] == '\0')//만약 첫 부분이 문자열 끝 문자라면 오류 메세지 출력
        {
          MSG ("empty command in line %d, ignored\n", line_nr);
          continue;
        }
      strncpy (task.command, s, sizeof (task.command) - 1);//command를 구조체에 저장
      task.command[sizeof (task.command) - 1] = '\0';//문자열의 끝 설정

      if (0)
        MSG ("id:%s pipe-id:%s order:%d action:%d command:%s\n",
             task.id, task.pipe_id,task.order, task.action, task.command);

      append_task (&task);//받아온 task를 append_task 함수를 거쳐서 리스트에 저장
      continue;

    invalid_line:
      MSG ("invalid format in line %d, ignored\n", line_nr);
    }
  
  fclose (fp);//설정 파일을 닫아줌

  return 0;
}

/*
 *command 부분에서 명령어 처리를 하기 위한 분할 작업
 *
 *@param : 분할 작업을 하기 위한 command부분
 *
 *@return : 분할한 결과물 
 */
static char **
make_command_argv (const char *str)
{
  char      **argv;
  const char *p;
  int         n;

  for (n = 0, p = str; p != NULL; n++)//command를 처음부터 끝까지 탐색
    {
      char *s;

      s = strchr (p, ' ');//공백문자가 있는지 검사
      if (!s)//더이상 없으면 탐색을 종료
        break;
      p = s + 1;//pf를 공백문자 다음으로 위치시켜 놓음
    }
  n++;//명령문의 갯수를 n에다가 세팅

  argv = calloc (sizeof (char *), n + 1);//n+1만큼 명령문을 담을 만큼 argv 문자열 갯수 동적할당
  if (!argv)//동적할당 실패시 오류문 출력
    {
      MSG ("failed to allocate a command vector: %s\n", STRERROR);
      return NULL;
    }
  /*명령문 하나하나마다 argv에다가 저장을 해 주는 작업*/
  for (n = 0, p = str; p != NULL; n++)
    {
      char *s;

      s = strchr (p, ' ');//공백분자마다 분리
      if (!s)//공백문자가 존재하지 않으면 더이상 작업을 하지 않음
        break;
      argv[n] = strndup (p, s - p);//p부터 s-p바이트 만큼 명령문을 저장
      p = s + 1;//다음 조사를 위한 세팅
    }
  argv[n] = strdup (p);//마지막 명령문을 저장

  if (0)
    {

      MSG ("command:%s\n", str);
      for (n = 0; argv[n] != NULL; n++)
        MSG ("  argv[%d]:%s\n", n, argv[n]);
    }

  return argv;//명령문들을 리턴
}

/*
 *부모 프로세스를 수행하면서 fork함수를 시스템 콜을 하면서 자식 프로세스를 수행
 *
 *자식 프로세스에 대해서는 pipe작업을 수행
 *
 *@param : 수행하고자 하는 task
 */
static void
spawn_task (Task *task)
{
  if (0) MSG ("spawn program '%s'...\n", task->id);
  
  /*설정파일에 pipe_id가 공백 -> 만약에 pipe_id */
  /*piped 상태가 1인 경우 해당 작업 수행*/
  if (task->piped && task->pipe_id[0] == '\0')//이 경우는 다른 프로세스의 piped_pid로 지목이 당한 프로세스에 해당이 된다.
    {
      if (pipe (task->pipe_a))//pipe_a통신 파이프를 생성하고 실패시 piped변수에 상태 설정과 오류 메세지를 띄워줌
        {
          task->piped = 0;
          MSG ("failed to pipe() for prgoram '%s': %s\n", task->id, STRERROR);
        }
      if (pipe (task->pipe_b))//pipe_b통신 파이프를 생성하고 실패시 piped변수에 상태 설정과 오류 메세지를 띄워줌
        {
          task->piped = 0;
          MSG ("failed to pipe() for prgoram '%s': %s\n", task->id, STRERROR);
        }
    }

  /*자식 프로세스생성 -> fork함수 시스템 콜*/
  task->pid = fork ();
  if (task->pid < 0)//fork함수의 리턴값이 0 미만이면 오류발생 임으로 오류 메세지 띄워줌
    {
      MSG ("failed to fork() for program '%s': %s\n", task->id, STRERROR);
      return;
    }

  /* child process */
  if (task->pid == 0)
    {
      char **argv;

      argv = make_command_argv (task->command);//command에 있는 명령문을 분리하고 argv변수에 넣어줌
      if (!argv || !argv[0])//받아온 명령문이 존재하지 않을 때 오류 메세지(이때, 0번째 명령문은 실행할 파일 이름)
        {
          MSG ("failed to parse command '%s'\n", task->command);
          exit (-1);
        }

      if (task->piped)//piped 상태가 1일 경우에 해당 작업을 수행
        {
		  //0 -> stdin
		  //1 -> stdout
		  //2 -> stderror
		  /*redirect 작업*/	
          if (task->pipe_id[0] == '\0')//만약에 pipe_id가 비어있는 경우 자식의 stdin과 stdout을 다음과 같이 redirect 시켜줌
            {
              dup2 (task->pipe_a[1], 1);//stdout을 pipe_a[1] 쓰기 전용 파일로 redirect시켜줌
              dup2 (task->pipe_b[0], 0);//stdin을 pipe_b[0] 읽기 전용 파일로 redirect시켜줌
              close (task->pipe_a[0]);//파일들을 닫아주는 작업
              close (task->pipe_a[1]);
              close (task->pipe_b[0]);
              close (task->pipe_b[1]);
            }
          else
            {/*만약에 pipe_id가 존재를 하는 경우*/
              Task *sibling;

              sibling = lookup_task (task->pipe_id);//해당 id에 해당하는 task를 찾음
				if (sibling && sibling->piped)//찾은 task가 존재하고 piped상태가 1이면 redirect 시켜줌
                {
                  dup2 (sibling->pipe_a[0], 0);//stdin을 pipe_id에 해당하는 task의 pipe_a[0] 읽기 전용 파일로 redirect 시켜줌
                  dup2 (sibling->pipe_b[1], 1);//stdout을 pipe_id에 해당하는 task의 pipe_b[1] 쓰기 전용 파일로 redirect 시켜줌
                  close (sibling->pipe_a[0]);//파일들을 닫아주는 작업
                  close (sibling->pipe_a[1]);
                  close (sibling->pipe_b[0]);
                  close (sibling->pipe_b[1]);
                }
            }
        }

      execvp (argv[0], argv);//argv[0] 에 해당하는 프로세스를 수행하고 argv를 인자로 넘겨줌
      MSG ("failed to execute command '%s': %s\n", task->command, STRERROR);//exec계열 함수 실행 실패시 오류 메세지를 생성하고 종료시킴
      exit (-1);
    }
}

/*
 *리스트들에 존재하는 task들을 수행시키는 함수
 */
static void
spawn_tasks (void)
{
  Task *task;

  for (task = tasks; task != NULL && running; task = task->next)
    spawn_task (task);
}

/*
 *SIGSHLD 시그널의 처지리침을 바꾸기 위한 함수
 *
 *@param : 시그널 번호 -> 이 프로그램에서는 SIGCHLD시그널 번호를 넘겨줌
 */
static void
wait_for_children (int signo)
{
  Task *task;
  pid_t pid;

 rewait:
  pid = waitpid (-1, NULL, WNOHANG);//부모프로세스보다 자식 프로세스가 먼저 끝나는 경우 좀비프로세스가 생성되지 않도록 부모프로세스가 끝나고 자식 프로세스를 회수하기 위해 wait를 해줌
  if (pid <= 0)//만약 기다린 자식 프로세스의 pid 가 -1이면 실패이고 0이면 자식 프로세스가 종료되지 않은 것임
    return;

  task = lookup_task_by_pid (pid);//자식프로세스의 pid를 이용하여 리스트에서 찾아옴
  if (!task)//pid에 대한 프로세스가 없다면 오류 메세지 출력
    {
      MSG ("unknown pid %d", pid);
      return;
    }

  if (0) MSG ("program[%s] terminated\n", task->id);

  if (running && task->action == ACTION_RESPAWN)//만약 task의 action부분이 respawn부분이라면 다시 spawn_task함수를 수행 -> 무한반복적으로 수행이 됨, running 변수는 SIGINT, SIGTERM시그널이 올 때만 0이고 그러지 않으면 1이기 때문에 두 시그널이 발생하지 않으면 해당 작업을 수행
    spawn_task (task);//spawn_task작업을 수행
  else
    task->pid = 0;//만약 action 부분이 once라면 pid를 0로 세팅하고 빠져나옴 -> 무한반복문을 탈출하게 됨

  /* some SIGCHLD signals is lost... */
  goto rewait;
}

/*
 *SIGINT, SIGTERM 시그널들의 처리지침을 바꾸기 위한 함수
 * 
 *@param : 시그널 번호 -> 이 프로스램에서는 SIGINT, SIGTERM 시그널들의 번호를 넘겨줌
 */
static void
terminate_children (int signo)
{
  Task *task;

  if (1) MSG ("terminated by SIGNAL(%d)\n", signo);

  running = 0;//running 변수를 0로 세팅 -> 더이상 spawn_task 함수를 부르지 않기 위한 처리

  for (task = tasks; task != NULL; task = task->next)//리스트에 있는 task들을 탐색
    if (task->pid > 0)//아직 pid가 0으로 세팅이 안된 task에 있어서(예를 들어 action 이 respawn인 task)
      {
        if (0) MSG ("kill program[%s] by SIGNAL(%d)\n", task->id, signo);//메세지 출력
        kill (task->pid, signo);//해당 task에 시그널 번호를 보냄(파라미터가 SIGINT, SIGTERM 에 따라서 맞는 시그널을 보냄)
      }

  exit (1);//종료
}
/*  
 *signalfd함수를 이용하여 시그널을 재정의 해주는 함수<추가기능>
 */
static void
action_signalfd()
{
	sigset_t mask;
	int sfd;
	struct signalfd_siginfo fdsi;
	ssize_t s;
	Task* task;
	
	sigemptyset(&mask);//시그널 셋을 비워둠

    /*셋에 정의하고 싶은 시그널들을 추가 -> 이번 프로그램에서는 SIGCHLD, SIGINT, SIGTERM 이 3개의 시그널들의 처리 지침을 바꾸기 위해 셋에 추가 */
	sigaddset(&mask,SIGCHLD);
	sigaddset(&mask,SIGINT);
	sigaddset(&mask,SIGTERM);

    /*시그널들을 블록화*/
	if(sigprocmask(SIG_BLOCK,&mask,NULL) == -1)//실패시 오류 메세지 출력
  	handle_error("sigprocmask");

	/*
	 *signalfd 함수의 사용
	 *첫번째 인자가 -1 이므로 새 파일디스크립터를 생성
	 *두번째 인자가 mask이므로 시그널 집합이 mask로 교체
	 *세번째 인자가 0이므로 flags 옵션을 지정하지 않음
	 */
	sfd = signalfd(-1,&mask,0);
	if(sfd == -1)//오류가 생기면 오류 메세지 출력
		handle_error("signalfd");

	while(1)//시그널이 발생할때마다 해당 작업을 수행
	{
		int check = 0;

		s = read(sfd,&fdsi,sizeof(struct signalfd_siginfo));//signalfd파일 디스크립터를 받아 fdsi구조체에 읽어드림

		if(s == -1)//실패시 오류 메세지 출력
	    {
			MSG("Error : %s\n",STRERROR);
	    }

	    /*SIGCHLD 처리 지침 바꾸기*/
	 	if(fdsi.ssi_signo == SIGCHLD)
	    {
			wait_for_children(SIGCHLD);
	    }
		/*SIGINT 처리 지침 바꾸기*/
		else if(fdsi.ssi_signo== SIGINT)
		{
			terminate_children(SIGINT);
		}
		/*SIGTERM 처리 지침 바꾸기*/
		else if(fdsi.ssi_signo == SIGTERM)
		{
			terminate_children(SIGTERM);
		}
		
		/*
		 * 리스트에 있는 Task들이 모두 종료 되었을 시에는 더이상 시그널 처리를 할 필요가 없고
		 * 무한 반복문을 빠져나와 main()함수가 처리가 되도록 하기 위한 처리  
		 */
	    for (task = tasks; task != NULL && running; task = task->next)//리스트를 순회
		{
			if(task->pid != 0)//pid가 모두 0인지를 체크
			{
				check = 1;
			}
		}

		if(check == 0)//만약 리스트에 있는 모든 pid가 0로 세팅이 되어져 있다면 running변수를 0으로 만듬
		{
			running = 0;
		}

		if(running == 0)//무한 반목문을 빠져 나옴
		{
			break;
		}
  }

}

int
main (int    argc,
      char **argv)
{
  int terminated;

  if (argc <= 1)//읽어들일 설정파일이 없음으로 오류문 출력 후 종료 
    {
      MSG ("usage: %s config-file\n", argv[0]);
      return -1;
    }

  if (read_config (argv[1]))//설정파일을 read_config함수로 읽어들임, 실패시 오류 메세지 출력 후 종료
    {
      MSG ("failed to load config file '%s': %s\n", argv[1], STRERROR);
      return -1;
    }

  running = 1;//running 변수를 1로 설정 -> SIGINT, SIGTERM 신호시 0으로 바꾸게끔 처리 지침을 바꿈

  spawn_tasks ();//리스트들에 있는 task들을 수행

  action_signalfd();//<추가기능의 구현> signalfd함수를 이용하여 SIGCHLD, SIGINT, SIGTERM 들의 처리 지침을 바꿔줌

  terminated = 0;
  while (!terminated)
    {
      Task *task;

      terminated = 1;
      for (task = tasks; task != NULL; task = task->next)//리스트들을 순외
        if (task->pid > 0)//아직 task->pid 가 0으로 세팅이 안되어 있으면 반복문을 재실행(task의 action이 respawn인 경우에는 0으로 세팅이 안되어 있음, once인 경우 부모 프로세스가 끝나면 0로 세팅)
          {
            terminated = 0;
            break;
          }

      usleep (100000);//100000마이크로초 동안 대기
    }
  
  return 0;
}
