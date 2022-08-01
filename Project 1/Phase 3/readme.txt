phase3 readme
make 명령어로 compile후 ./myshell로 실행
phase 2의 코드에 추가로 작성하는 형식으로 구현

&를 명령어 젤 뒤에 붙여서 background에서 명령을 실행할 수 있다.
ctrl+c 를 실행하면 현재 진행중인 fg job이 종료되며 진행중인 job이 없을경우 아무일도 일어나지 않는다.
ctrl+z 를 실행하면 현재 진행중인 fg job이 정지되며 진행중인 job이 없을경우 아무일도 일어나지 않는다.
jobs 명령어를 사용해 현재 bg에서 정지되었거나 진행중인 job을 출력한다.
	- 이미 종료된 job은 출력하지 않는다.
fg %#를 이용해 #에 해당하는 job을 fg에서 다시 시작시킨다.
bg %#를 이용해 #에 해당하는 job을 bg에서 다시 시작시킨다.
kill %#를 이용해 #에 해당하는 job을 강제종료시킨다.
job이 종료될 경우, bg의 job만 종료되었다는 message를 출력시켜 종료되었다는것을 알수 있게 하였다.

각 함수에 대한 설명은 아래에 기술되어 있다.

job_l 구조체를 사용하여 job들의 정보를 저장

- main
	command입력을 받고 eval함수로 넘겨주었다.
	여러가지 signal handling함수를 install한다.
-initjobtable
	job table을 만든다. 
-sigstp_handler
	crtl+z입력을 받았을때 실행된다. job table에서 실행되고 있는 foreground job을 찾아 정보를 업데이트하고 SIGSTP signal을 보낸다.
-sigchld_handler
	child가 종료 혹은 멈춤되었을때 실행된다. 만약 어떤 signal에 의해 멈췄다면 어떤 job이 어떤 signal에 의해 멈췄는지 출력하고 그 job의 state를 멈춤으로 setting한다. 만약 background에 돌아가던 process가 종료되면  종료되었다는 문	구를 출력한다. foreground일경우엔 출력하지않는다. 종료되면 그 job을 table에서 지운다. 만약 kill이나 crtl+c로 종료되었을경우 아무런 출력없이 job을 지운다.
-sigint_handler
	crtl+c입력을 받았을때 실행중인 process를 종료시킨다.
-getfgpid
	현재 foreground에서 돌아가고있는 job의 pid를 넘겨받는다.
-insertjb
	job의 정보를 받아 job table에 넣어준다.
-deljob
	전달받은 pid값의 job을 초기화시켜준다.
-printjob
	jobs command를 입력받았을때 실행된다. 현재 bg에 있는 모든 job들을 출력한다.
-prsinglejob
	fg나 bg가 실행되었을때 실행된다. 전달받은 pid의 job을 출력한다.
- eval 
	입력받은 command를 pipeparse로 넘겨 | 문자로 구별하여 argv에 저장했다. fork를 수행하여 child를 생성	후, pipe	가 없으면 ' '으로 parsing후 바로 실행한다. pipe가 발견되면 (count>0) execpipe함수로 첫번째 명령			어와 두번째 명령어를 같이 넘겨준다. return후 wait함수로 child의 termination을 기다린다.
	eval에서 fork된 child가 종료되었을경우, wait 함수로 child를 종료시킨다.
-execpipe
	file descriptor fds[2]를 생성 후, pipe함수로 값을 할당해준다. 인자로 받은 첫번째 명령어들을 공백으로 나	눠준 후, 	다시한번 fork를 한다. 여기서 실행되는 명령어는 pipe로 값을 넣어야하기때문에 dup2함수로 표준 	출력을 pipe의 쓰기에 할당해주고 pipe의 읽기 부분은 쓰지 않기에 닫아준다. 그리고 명령어를 수행한다. 이 함수에서의 parents는 명령어를 실행하고 출력해주는 역할이다. execpipe함수는 항상 두개의 인자, 즉 첫번 째 명령어와 두번째 명령어를 받는데 출력은 두번째 명령어이기때문에 여기서는 표준입력을 pipe에 할당해 	
	pipe에서 값을 읽어올수 있게 한다. 그리고 pipe의 쓰기는 닫는다. 만약 두번째 명령어가 count-1, 즉 마지막 	명령어일  경우 함수를 수행하고 출력한다.  
-execfg
	fg %#를 실행한다. 알맞은 format으로 입력받았을 경우 그 job의 정보를 update해주고 SIGCONT signal을 보내 job을 	
	재가동 시킨다. 그리고 그 job이 끝날때까지 기다린다.
-waitfg
	fg job이 끝날때 까지 기다린다. 
-execbg
	bg %#를 실행한다. 정보를 업데이트하고 SIGCONT signal을 보낸다.
-execkill
	전달받을 job id의 process를 종료시킨다. 
-getjbid
	전달받은 job id 와 동일한 id를 가진 job을 return.
-getjbpid
	전달받은 pid와 동일한 pid를 가진 job을 return.
-getidpid
	전달받은 pid와 동일한 pid를 가진 job의 id를 return.
- builtin_command
	exit입력은 프로그램 종료
	cd 입력은 chdir함수로 directory를 변경시켜주었다.
- parseline
	command입력을 구분해주는 함수이다.
	명령어를 구분하기위해 첫 공백을 찾아 NULL값으로 구분한다.
	첫 명령어가 cd일 경우 '/'로 구분되어 있는 directory를 argv[]배열에 대입한다.
	'/'주위에 공백이 존재할 경우 첫번째 directory만 인식하여 이동한다. (기존 LINUX system에서의 구동결과랑 
	동일하게 구현)
	cd명령어가 아닐경우 계속 공백으로 구분되어있는 명령어들을 argv배열에 하나하나 넣어준다.
	모든 공백은 무시한다.
	'이나"가 있을경우 del_colon함수를 수행해 지워준다.
-pipeparse
	parseline과 동일하지만 command를 단지 공백이 아닌 | 로 나눠준다. |로 나눠진 문자열들이 return된다.
-del_colon
	"이나'를 지우는 함수이다. 문자들을 앞으로 한칸씩 당겨 맨 앞 " 이나 ' 를 지워주고 마지막 중복되는 문자는 	NULL로 할당한다. 이로써 ' 혹은 " 사이의 문자들만 str에 저장된다.