phase2 readme
make 명령어로 compile후 ./myshell로 실행
phase 1의 코드에 추가로 작성하는 형식으로 구현

명령어을 여러개 실행할 수 있다. | 문자를 기준으로 앞 명령어의 output값을 뒤 명령어의 input값으로 넣어서 실행한다.

각 함수에 대한 자세한 사항은 아래에 기술하였다.

- main
	command입력을 받고 eval함수로 넘겨주었다.
- eval 
	입력받은 command를 pipeparse로 넘겨 | 문자로 구별하여 argv에 저장했다. fork를 수행하여 child를 생성	후, pipe가 없으면 ' '으로 parsing후 바로 실행한다. pipe가 발견되면 (count>0) execpipe함수로 첫번째 명령		어와 두번째 명령어를 같이 넘겨준다. return후 wait함수로 child의 termination을 기다린다.
	eval에서 fork된 child가 종료되었을경우, wait 함수로 child를 종료시킨다.
-execpipe
	file descriptor fds[2]를 생성 후, pipe함수로 값을 할당해준다. 인자로 받은 첫번째 명령어들을 공백으로 나	눠준 후, 다시한번 fork를 한다. 여기서 실행되는 명령어는 pipe로 값을 넣어야하기때문에 dup2함수로 표준 	출력을 pipe의 쓰기에 할당해주고 pipe의 읽기 부분은 쓰지 않기에 닫아준다. 그리고 명령어를 수행한다. 이 	
	함수에서의 parents는 명령어를 실행하고 출력해주는 역할이다. execpipe함수는 항상 두개의 인자, 즉 첫번 	째 명령어와 두번째 명령어를 받는데 출력은 두번째 명령어이기때문에 여기서는 표준입력을 pipe에 할당해 	
	pipe에서 값을 읽어올수 있게 한다. 그리고 pipe의 쓰기는 닫는다. 만약 두번째 명령어가 count-1, 즉 마지막 	명령어일  경우 함수를 수행하고 출력한다.  
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
	"이나'를 지우는 함수이다. 문자들을 앞으로 한칸씩 당겨 맨 앞 " 이나 ' 를 지워주고 마지막 중복되는 문자는 		NULL로 할당한다. 이로써 ' 혹은 " 사이의 문자들만 str에 저장된다.