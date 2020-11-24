#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>

#define MAX_CMD_ARG 20
#define BUFSIZ 256

const char *prompt = "myshell> ";
char cmdline[BUFSIZ];
pid_t fgpid = 0;

void fatal(char *str)
{
  perror(str);
  exit(1);
}

int makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
{
  int numtokens = 0;
  char *snew = NULL;

  if ((s == NULL) || (delimiters == NULL))
    return -1;

  snew = s + strspn(s, delimiters); /* Skip delimiters */
  if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
    return numtokens;

  numtokens = 1;

  while (1)
  {
    if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
      break;
    if (numtokens == (MAX_LIST - 1))
      return -1;
    numtokens++;
  }
  return numtokens;
}

// chdir() 시스템 콜을 수행하는 함수 (HOME 디렉토리 이동 기능 적용)
void newchdir(char **cargv, int cargc)
{
  uid_t uid = getuid();               // 프로세스를 실행한 사용자 id 저장
  struct passwd *pwd = getpwuid(uid); // uid의 여러 정보 저장
  char *pathname;

  if (cargc == 1 || !strcmp(cargv[1], "~")) // 입력된 경로가 없거나 ~인 경우
    pathname = pwd->pw_dir;                 // pwd에서 Home 디렉토리 경로를 꺼내어 저장
  else
    pathname = cargv[1];

  if (chdir(pathname) != 0)
    perror("newchdir()");
}

//command line의 뒤에서부터 가장 처음 등장하는 공백과 탭을 제외한 문자가 &인지 확인
int bgfinder()
{
  int bg = 0;
  int index = strlen(cmdline) - 1;

  while (1)
  {
    if (cmdline[index] != ' ' && cmdline[index] != '\t')
    {
      if (cmdline[index] == '&' && cmdline[index - 1] != '&') //&&인 경우 skip
      {
        bg = 1;               // 백그라운드 실행
        cmdline[index] = ' '; // &문자를 공백으로 대체
      }
      break;
    }
    index--;
  }

  return bg;
}

static void sigchldhandler(int signo)
{
  pid_t pid;
  pid = wait(NULL);
  if (fgpid == pid)
    fgpid = 0;
}

// redirect 핸들러
int redihandler(int *cargc, char **cargv)
{
  int fd, direction;
  int i = 0;
  for (; i < *cargc; i++)
  {
    if (!strcmp(cargv[i], "<"))
    {
      direction = 0;
      break;
    }
    if (!strcmp(cargv[i], ">"))
    {
      direction = 1;
      break;
    }
  }
  if (i < *cargc) // < 또는 > 가 존재할 때
  {
    if (i < *cargc - 1) //문법 오류 판별
    {
      if (direction == 0) //redirection이 input일 때
      {
        if ((fd = open(cargv[i + 1], O_RDONLY)) == -1)
          fatal("redihandler()");
      }
      else //redirection이 output일 때
      {
        if ((fd = open(cargv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
          fatal("redihandler()");
      }
      dup2(fd, direction);
      close(fd);

      for (; i < *cargc; i++)
      {
        if (i < *cargc - 2)
          cargv[i] = cargv[i + 2];
        else
          cargv[i] = NULL;
      }
      *cargc -= 2;

      return 0;
    }
    else
    {
      fputs("redihandler(): Syntax error\n", stderr);
      exit(1);
    }
  }
  else
    return 1;
}

void connpipe(int *argclist, char ***argvlist, int *pfd, int count, int idx)
{
  pid_t pid = fork();

  switch (pid)
  {
  case -1:
    fatal("connpipe()");
  case 0:
    dup2(pfd[1], 1);
    close(pfd[1]);
    close(pfd[0]);
    execvp(argvlist[idx][0], argvlist[idx]);
    break;
  default:
    while (1)
    {
      if (redihandler(&argclist[idx + 1], argvlist[idx + 1]))
        break;
    }
    dup2(pfd[0], 0);
    close(pfd[0]);
    close(pfd[1]);
    if (count > idx + 1)
    {
      if (pipe(pfd) == -1) //또 다른 연결을 위한 파이프 재생성
        fatal("connpipe()");
      connpipe(argclist, argvlist, pfd, count, idx + 1); //재귀
    }
    else
      execvp(argvlist[idx + 1][0], argvlist[idx + 1]); //마지막 실행
  }
}

int pipehandler(int *cargc, char **cargv)
{
  pid_t pid;
  int count = 0; //파이프 개수
  int start = 0;
  int subc[MAX_CMD_ARG];
  char **subv[MAX_CMD_ARG];
  int pfd[2];

  for (int i = 0; i < *cargc; i++)
  {
    if (!strcmp(cargv[i], "|"))
    {
      subc[count] = i - start;
      subv[count] = (char **)malloc(subc[count] * sizeof(char *));
      for (int j = 0; j < subc[count]; j++)
        subv[count][j] = cargv[j + start];

      start = i + 1;
      count++;
    }
  }

  if (count == 0)
    return 1; //pipe가 존재하지 않음

  subc[count] = *cargc - start;
  subv[count] = (char **)malloc(subc[count] * sizeof(char *));
  for (int j = 0; j < subc[count]; j++)
    subv[count][j] = cargv[j + start];

  while (1)
  {
    if (redihandler(&subc[0], subv[0]))
      break;
  }
  if (pipe(pfd) == -1) //파이프 생성
    fatal("connpipe()");

  connpipe(subc, subv, pfd, count, 0); //파이프 연결 함수
}

int main(int argc, char **argv)
{
  char *cmdvector[MAX_CMD_ARG];
  int cmdargc;
  int bg = 0;
  pid_t pid;

  if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    fatal("can't catch SIGINT");
  if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
    fatal("can't catch SIGQUIT");
  if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
    fatal("can't catch SIGTSTP");
  if (signal(SIGCHLD, sigchldhandler) == SIG_ERR)
    fatal("can't catch SIGCHLD");

  while (1)
  {
    fputs(prompt, stdout);
    fgets(cmdline, BUFSIZ, stdin);
    cmdline[strlen(cmdline) - 1] = '\0';

    if (strcmp("exit", cmdline) == 0) //exit 입력 시 종료
      break;

    bg = bgfinder(); //백그라운드 프로세스 여부 판별

    if ((cmdargc = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG)) <= 0) //탭이나 공백만 입력 시
      continue;

    if (!strcmp("cd", cmdvector[0])) //chdir명령 실행 시 자식 프로세스를 생성하지 않음(불필요)
    {
      newchdir(cmdvector, cmdargc);
      continue;
    }

    switch (pid = fork())
    {
    case 0:
      if (bg == 0)
      {
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
          fatal("can't catch SIGINT");
        if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
          fatal("can't catch SIGQUIT");
        if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
          fatal("can't catch SIGTSTP");
      }
      if (pipehandler(&cmdargc, cmdvector))
      {
        while (1)
        {
          if (redihandler(&cmdargc, cmdvector))
            break;
        }
        execvp(cmdvector[0], cmdvector);
      }
      fatal("main()");
    case -1:
      fatal("main()");
    default:
      if (bg == 0)
        fgpid = pid;
      while (fgpid)
        pause();

      bg = 0;
    }
  }
  return 0;
}
