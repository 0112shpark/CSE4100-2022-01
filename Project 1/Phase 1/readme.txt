phase1 readme
make 명령어로 compile후 ./myshell로 실행
example code의 shellex를 참고하여 구현.

기본 linux shell과 비슷하게 기본 명령어만 구현했다. cd는 home으로 가는 명령어를 제외하고 구현했다. " 와 ' 사이에 있는 값들은 input값으로 받았으며 그 외의 모든 공백은 무시했다.

- main
	command입력을 받고 eval함수로 넘겨주었다.
- eval 
	command의 값을 parseline함수에 넘겨주어 명령어와 인자를 구별했다.
	builtin_command는 exit과 cd명령어를 따로 처리해준 함수로써 다른 명령어는 execvp로 실행했다. 
	fork를 사용해 child를 생성후 child에서 execvp함수로 명령어를 수행했다.	
	명령어들이 /bin/에 저장되어있기때문에 자동으로 path를 찾아서 실행하는 execvp함수를 사용했다.
	명령실행이 끝나고 waitpid함수로 fork된 child process를 reap시켜주었다.
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
-del_colon
	"이나'를 지우는 함수이다. 문자들을 앞으로 한칸씩 당겨 맨 앞 " 이나 ' 를 지워주고 마지막 중복되는 문자는 		NULL로 할당한다. 이로써 ' 혹은 " 사이의 문자들만 str에 저장된다.