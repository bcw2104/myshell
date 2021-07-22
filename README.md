# myshell
# 프로젝트 1
요구사항
1.	cd명령이 제대로 동작하지 않는 버그를 수정
2.	exit 명령 구현 – exit 명령 입력 시 myshell 종료
3.	백그라운드 실행 구현 - 명령어라인 뒤에 &를 추가하면 백그라운드에서 동작

구현 방법
1.	cd 명령 동작 구조 수정
•	문제점
▪	myshell이 생성하는 자식 프로세스에서 cd명령이 실행되고 종료되므로 실질적으로 myshell에게는 어떠한 영향도 주지 않음
•	해결 방법
▪	cd 명령어 입력 시 자식 프로세스 생성을 차단하고  myshell에서 직접 실행하도록 동작 구조 변경
▪	소스코드
•	newchdir 함수 : 명령어 백터를 입력받아 chdir을 호출
◦	유닉스의 cd동작과 동일하게 경로를 입력하지 않거나 ~ 입력 시 myshell을 실행한 user의 홈 디렉토리로 이동
◦	존재하지 않는 경로 입력시 perror를 호출하여 알림	
•	main 함수 수정
◦	makelist를 myshell에서 실행하게 위치 변경
▪	fork() 호출 이전에 makelist 실행 시 자식 프로세스에도 적용되므로 cd 이외 다른 동작에 영향을 미치지 않는다.
◦	cd명령 입력이 감지되면 newchdir함수를 실행 후 다시 입력모드로 전환 – fork() 호출 불필요
2.	exit 명령 구현
•	구현 방법
	작동 순서
1.	command line을 입력 받은 후 문자열 “exit”과 비교
2.	동일하다면 break를 통해 무한루프 탈출
3.	곧 바로 return 0이 실행되며 프로세스 종료
	소스코드
•	main 함수 수정

3. 백그라운드 실행 구현
•	구현 방법
▪	작동 순서
1.	while 루프를 통해 cmdline의 마지막 지점부터 공백,탭을 제외한 문자가 나올 때까지 탐색
2.	&가 포함되어 있다면 &를 공백문자로 치환 후 bg = 1
•	단,  &&은 제외
3.	bg == 0 이면 myshell은 자식프로세스를 기다림, bg == 1 이면 myshell은 자식프로세스를 기다리지 않음
•	waitpid사용으로 기다릴 프로세스 직접 지정
4.	bg를 0으로 초기화
▪	소스코드
•	main 함수 수정	
동작 확인
1.	cd 동작

2.	exit 동작
	
3.	백그라운드 동작
	
	고찰
•	좀비프로세스가 생기는 이유
◦	부모 프로세스가 자식 프로세스가 종료되었음에도 wait를 통해 회수하지 않을 때 좀비 프로세스가 생긴다.
•	해당 테스트의 문제점
◦	wait(NULL)를 사용할 경우 만약 자식 background 프로세스가 종료된 후 다른 자식 foreground프로세스를 실행 시 wait(NULL)는 이전의 background 프로세스를 회수하게 된다. (우선적으로 종료되는 프로세스를 회수하기 때문) 따라서 회수 되어야하는 foreground프로세스가 좀비 프로세스가 된다. 따라서 waitpid(pid,NULL,0)를 사용해 회수해야 할 프로세스를 지정해야 한다.

실행 테스트 - 백그라운드 프로세스 확인 (sleep 1000 &, sleep 10)

Psuedo-code - 전체 코드

define MAX_CMD_ARG is 10
define BUFSIZ is 256

prompt  ← "myshell> "  : const char*
cmdvector[MAX_CMD_ARG]  : char*			// 공백 또는 탭으로 나뉘어진 command line argument 저장
cmdline[BUFSIZ]  : char				// command line 저장

// 오류 제어
function fatal(char *str)
 	print error message
 	exit with error

// command line을  delimiters를 기준으로 분할
function makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
	numtokens ← 0  :  int 				//  list에 저장된 항목 개수
	snew ← NULL  :  char *

  	if s = NULL OR delimiters = NULL
   		return -1

  	snew ← s + strspn(s, delimiters); 			// 맨 앞의 공백 또는 탭 제거
  	if (list[numtokens] ← strtok(snew, delimiters) = NULL     	// 문자열 snew를 delimiters로 첫번째 분할 (명령 코드) 
   		return numtokens

  	numtokens ← 1

  	while (true)
   		if (list[numtokens] ← strtok(NULL, delimiters)) = NULL 	// 문자열 snew를 delimiters로 분할 (argument) 
    			break
    		if numtokens = (MAX_LIST - 1)
     			return -1
  	 	numtokens ++
 	 return numtokens

// chdir() 시스템 콜을 수행하는 함수 (HOME 디렉토리 이동 기능 적용)
function newchdir(char **list, int argc)				
  	uid ← current user id						// 프로세스를 실행한 사용자 id 저장
  	pwd ← uid’s information						// uid의 여러 정보 저장
  	pathname							// 경로 저장

  	if argc = 1 OR list[1] is not equal with "~"		// 입력된 경로가 없거나 ~인 경우
    		pathname ← uid’s home directory		// pwd에서 Home 디렉토리 경로를 꺼내어 저장
  	else
    		pathname ← list[1]					
  	if chdir(pathname) != 0
    		print error message


function main(int argc, char **argv)
	index ← 0 :  int
	cmd_argc
	bg ← 0			// 백그라운드 실행 유무(1 : 백스라운드 , 0 : 포어그라운드)
	pid			// 프로세스 id 저장

  	while (true)
   		print prompt to stdout
		read cmdline from stdin
    		end of cmdline ← '\0'
    		if cmdline is equal with “exit” 				
      			break				// 무한 루프 탈출 → 프로세스 종료
	
		index ← (length of cmdline – 1)				
		
		// command line의 뒤에서부터 가장 처음 등장하는 공백과 탭을 제외한 문자가 &인지 확인
    		while (true)
     	 		if cmdline[index] != ' ' AND cmdline[index] != '\t'
        				if cmdline[index] = '&' AND cmdline[index - 1] != '&'	// &&인 경우 무시
         				bg ← 1					// 백그라운드 실행
          				cmdline[index] ← ' '			// &문자를 공백으로 대체
        				break
      			index--
  
    		cmd_argc ← makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG)	
   	 	if cmdvector[0] is equal with “cd” 			// 명령이 cd인 경우
     			newchdir(cmdvector, cmd_argc)	
    	  		continue					// 자식 프로세스 생성 생략

    		switch pid ← fork()					//자식 프로세스 생성
    			case 0:
    	  			execvp(cmdvector[0], cmdvector)	// 자식 프로세스 실행 전환
     	 			fatal("main()")
    			case -1:
  	    			fatal("main()")
    			default:
      				if bg = 0
      	  				waitpid(pid, NULL, 0)	//직전에 생성된 자식 프로세스를 기다린다.
      				bg ← 0				//bg 초기화
	 return 0		









#프로젝트 2
요구사항
1.	SIGCHLD로 자식 프로세스 wait() 시 프로세스가 온전하게 수행되도록 구현
2.	^C(SIGINT), ^\(SIGQUIT) 사용시 쉘이 종료되지 않도록, Foreground 프로세스 실행 시 SIGINT를 받으면 프로세스가 끝나는 것을 구현

구현 방법
1.	SIGCHLD로 자식 프로세스 wait() 시 프로세스가 온전하게 수행되도록 구현
•	문제점
◦	단순히 포어그라운드 프로세스에 대하여 wait(NULL)을 사용할 시 이미 실행 중이던 백그라운드 프로세스가 종료되면 wait(NULL)은 백그라운드 프로세스를 회수하게 되어 포어그라운드 프로세스가 회수되지 못하는 문제점이 존재
•	해결방법
1.	SIGCHLD로 모든 자식프로세스 종료 시 사용자 설정 함수를 통해 종료된 자식 프로세스를 회수
2.	포어그라운드 프로세스는 쉘에서 유일하므로 포어그라운드 프로세스 실행 시 해당 process ID를 별도로 저장
3.	포어그라운드 프로세스가 회수될 때까지 pause를 통해 대기
◦	소스코드
▪	fgpid →  포어그라운드 프로세스 id를 저장하는 변수
▪	SIGCHLD 발생 시 호출하는 사용자 정의 함수
▪	시그널 지정 → main함수 내부
▪	포어그라운드 프로세스 생성 시(bg = 0일때) fgpid에 pid저장 후 while문을 통해 해당 프로세스가 종료될 때까지 대기 ( 포어그라운드 프로세스가 종료되면 SIGCHLD이 발생되고 sigchld_handler에 의해 fgpid가 0으로 세팅) 



2.	^C(SIGINT), ^\(SIGQUIT) 사용시 쉘이 종료되지 않도록, Foreground 프로세스 실행 시 SIGINT를 받으면 프로세스가 끝나는 것을 구현
•	문제점 
◦	^C(SIGINT), ^\(SIGQUIT) 사용시 myshell이 종료됨
•	해결방법
◦	myshell 이  SIGINT, SIGQUIT, SIGTSTP를 무시하고 포어그라운드 자식 프로세스가 생성되면 자식 프로세스의 SIGINT, SIGQUIT, SIGTSTP를 기본값으로 세팅한다.
◦	소스코드
▪	myshell이 기본적으로 SIGINT, SIGQUIT, SIGTSTP를 무시하도록 설정
▪	자식 프로세스가 포어그라운드일 때(bg = 0)  SIGINT, SIGQUIT, SIGTSTP를 기본값으로 설정

동작 확인
1.	SIGCHLD 동작
•	포어그라운드 프로세스 정상 작동
•	포어그라운드 프로세스 종료 후 확인 → 백그라운드 프로세스도 회수된 것을 확인

2.	^C, ^\ 확인
•	자식 프로세스가 없을 때 : myshell에서 무시
•	자식 프로세스가 존재할 때 : 자식 프로세스 종료

Psuedo-code
1.	SIGCHLD
•	sigchld_handler
		pid ← current finished child process ID ( result value of wait(NULL);)  
		If pid = current foreground process ID
		     foreground process ID ← 0 
•	main – myshell (not child)
 myshell create forground child process 
 fgpid ←  forground child process ID
	        While (Until foreground process doesn’t exist)
	 	wait until signal occur (pause())
2.	^C(SIGINT), ^\(SIGQUIT) 
•	main – myshell (not child)
set if signal SIGINT, SIGQUIT, SIGTSTP occur, Ignore them ( user signal() systam call, SIG_IGN)
•	main – child 
                  if child process is foreground process
		set if signal SIGINT, SIGQUIT, SIGTSTP occur, Do default action ( user signal() systam call , SIG_DFL)











#프로젝트 3 - 소스코드 전반적으로 변수명 수정
요구사항
1.	리다이렉션 구현
2.	파이프 구현

구현 방법
1.	리다이렉션 구현
•	구동 방식
1.	명령어에서 < 또는 > 가 존재하는지 탐색
2.	open함수를 사용하여 <는 파일을 읽기전용, >는 파일을 쓰기 전용 및 파일이 존재하지 않을 시 생성하는 옵션 부여
3.	dup2함수를 사용하여 이전 단계에서 open한 파일 디스크립터를 < 는 stdout, >는 stdin에 복제
•	이 과정을 통해 표준 입출력을 파일에 대한 입출력으로 변경
4.	리다이렉션 기호와 파일정보를 명령어에서 제거
•	소스코드
▪	int redihandler(int *argc, char **argv)
•	argc : 명령어 argument 개수, argv : 명령어 argument 배열
•	반환 값 : < 또는 >가 존재할 때 0, 존재하지 않을 때 1, 문법 오류일 때 프로세스 종료

▪	while()루프를 활용하여 하나의 명령어에 여러 의 리다이렉션이 존재하는 경우를 처리 
2.	파이프 구현
•	요구 조건
▪	리다이렉션과 파이프가 혼합된 명령어 처리 가능
▪	2개이상의 파이프 처리 가능
•	문제점
▪	cat  <  ls.txt | grep ^d | wc –l  >  dir_num.txt 와 같이 긴 명령어는 argument 개수가 10을 초과하여 작동하지 않는 문제가 발생 
•	MAX_CMD_ARG의 값을 10에서 20으로 변경
•	구동 방식 
1.	명령어에 파이프 기호 ‘|’의 존재 및 개수 파악
2.	존재할 시 파이프 기호를 기준으로 여러 명령어를 분할
3.	파이프 생성
4.	선행하는 명령어를 자식 프로세스를 만들어 실행, 이때 dup2함수를 사용하여 표준 출력을 파이프 출력으로 지정
•	이 때 부모 프로세스는 표준입력을 파이프 입력으로 전환 
•	부모와 자식간의 파이프 입출력 close 
5.	부모 프로세스에서는 파이프 기호 개수만큼 실행되었는지 판별
•	완료되면 마지막으로 남은 명령어 실행 후 종료
•	완료되지 않았다면 파이프 재생성 및 재귀호출을 통해  또 다른 파이프 작업 실행
•	소스코드
▪	int pipehandler(int *cargc, char **cargv)
•	명령어에 포함된 파이프 기호 개수 파악 및 명령어 분할 후 파이프를 생성하고 connpipe함수 실행


▪	void connpipe(int *argclist, char ***argvlist, int *pfd, int count, int idx)
•	프로세스간 파이프 연결 함수
•	argclist : 파이프로 분할된 각 명령어들의 argument 개수
•	argvlist : 파이프로 분할된 각 명령어들의 argument 배열
•	pfd : 파이프 디스크립터, count : 파이프 개수 , idx : 분할된 명령어 인덱스

동작 확인
1.	리다이렉션
2.	파이프
•	파이프 / 리다이렉션 연계 작동 확인 
▪	working directory내 디렉토리는 1개 존재
•	백그라운드 작동 확인

