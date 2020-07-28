/**
 * OS Assignment #1 Test Task.
 **/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MSG(x...) fprintf (stderr, x)

static char        *name = "Task";
static volatile int looping;

static void
signal_handler (int signo)
{
  if (!looping)//이미 이전에 SIGINT 혹은 SIGTERM 시그널 발생이 되었더라면 밑의 작업을 수행하지 않고 처리가 끝남
    return;

  looping = 0;//SIGINT  혹은 SIGTERM 시그널 발생시 looping이 일어나지 않도록 변수를 0으로 지정 
  MSG ("'%s' terminated by SIGNAL(%d)\n", name, signo);//task가 끝났음을 알림
}

int
main (int    argc,
      char **argv)
{
  int   timeout    = 0;
  int   read_stdin = 0;
  char *msg_stdout = NULL;

  /* Parse command line arguments. */
  {
    int opt;

    while ((opt = getopt (argc, argv, "n:t:w:r")) != -1)//검색하려는 옵션들 -> n,t,w,r (이때, ,n,t,w에는 :가 붙어 있으므로 뒤에 인자 값이 없다면 오류)
      {
	switch (opt)
	  {
	  case 'n'://n은 task의 이름 재정의 
	    name = optarg;//이때,optarg는 옵션 뒤에 오는 파라미터
	    break;
	  case 't'://t는 timedout 정의
	    timeout = atoi (optarg);
	    break;
	  case 'r'://읽는다는 옵션을 추가
	    read_stdin = 1;
	    break;
	  case 'w'://쓰는 모드를 추가, 이때 optarg는 쓰고자 하는 문장
	    msg_stdout = optarg;
	    break;
	  default://해당 지정 옵션들이 아닌 다른 옵션이 온 경우
	    MSG ("usage: %s [-n name] [-t timeout] [-r] [-w msg]\n", argv[0]);
	    return -1;
	  }
      }
  }

  /* Register SIGINT / SIGTERM signal handler. */
  {
    struct sigaction sa;

    sigemptyset (&sa.sa_mask);//셋을 비워줌
    sa.sa_flags = 0;//flags속성은 없는 것으로 설정
    sa.sa_handler = signal_handler;//처리 지침을 설정
    if (sigaction (SIGINT, &sa, NULL))//SIGINT 시그널에 대한 처리 지침을 바꿔주고 오류가 나면 메세지를 띄우고 종료
      {
	MSG ("'%s' failed to register signal handler for SIGINT\n", name);
	return -1;
      }

    sigemptyset (&sa.sa_mask);//셋을 비워줌
    sa.sa_flags = 0;//flags속성은 없는 것으로 설정
    sa.sa_handler = signal_handler;//처리 지침을 설정
    if (sigaction (SIGTERM, &sa, NULL))//SIGTERM 시그널에 대한 처리 지침을 바꿔주고 오류가 나면 메세지를 띄우고 종료 
      {
	MSG ("'%s' failed to register signal handler for SIGTERM\n", name);
	return -1;
      }
  }

  MSG ("'%s' start (timeout %d)\n", name, timeout);//task가 시작이 되었음을 알림

  looping = 1;

  /* Write the message to standard outout. */
  if (msg_stdout)//쓰고자 하는 메세지를 redirect된 곳에다가 씀
    {
      char msg[256];

      snprintf (msg, sizeof (msg), "%s from %s", msg_stdout, name);//msg에가가 3번째 파라미터에 있는 문자열을 저장 
      write (1, msg, strlen (msg) + 1);//메세지를 쓰는 작업
    }

  /* Read the message from standard input. */
  if (read_stdin)
    {
      char    msg[256];
      ssize_t len;

      len = read (0, msg, sizeof (msg));//0번 파일 디스크립터에 있는 내용을 msg에다가 저장
      if (len > 0)
	{
	  msg[len] = '\0';//문자열의 끝 지정
	  MSG ("'%s' receive '%s'\n", name, msg);//받은 메세지를 터미널에 띄워줌
	}
    }

  /* Loop */
  while (looping && timeout != 0)//looping 이 1이면 아직 SIGINT나 SIGTERM 시그널을 받지 않았다는 소리이고 timeout이 0가 되야 task가 종료 
    {
      if (0) MSG ("'%s' timeout %d\n", name, timeout);
      usleep (1000000);//1000000마이크로 초 동안 대기->timeout이 발생하는 간격은 1000000마이크로 초 라고 설정해 놓음
      if (timeout > 0)//timeout을 하나 줄여줌
	timeout--;
    }

  MSG ("'%s' end\n", name);//task가 끝났다고 알림

  return 0;
}
