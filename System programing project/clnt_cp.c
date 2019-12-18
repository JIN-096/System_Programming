#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include <termios.h>
#include <curses.h>
#include <fcntl.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define BLANK "                                                    "
#define LEFT 75
#define RIGHT 77

void *send_msg(void *arg);
void *recv_msg(void *arg);
void *send_game(void *arg);
void *recv_game(void *arg);

void set_nodelay_mode();
void error_handling(char *msg);
void set_cr_noecho_mode();
void set_cr_echo_mode();
void draw();
void redraw();
void start(int);
void game(int);
void end(int);

char flag=0;
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int chat_row = 30;
int main(int argc, char *argv[])
{
	int chat_sock, game_sock;
	struct sockaddr_in serv_chat_addr, serv_game_addr;
	pthread_t snd_thread, rcv_thread;
	pthread_t snd_game_thread, rcv_game_thread;
	void *thread_return;
	if(argc!=4)
	{
		printf("Usage : %s <IP> <port> <name>\n",argv[0]);
		exit(1);
	}
	sprintf(name,"[%s]",argv[3]);

	chat_sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_chat_addr,0,sizeof(serv_chat_addr));
	serv_chat_addr.sin_family=AF_INET;
	serv_chat_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_chat_addr.sin_port=htons(atoi(argv[2]));

    game_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_game_addr,0,sizeof(serv_game_addr));
    serv_game_addr.sin_family=AF_INET;
    serv_game_addr.sin_addr.s_addr=inet_addr(argv[1]);
        serv_game_addr.sin_port=htons(atoi(argv[2])+100);

	printf("%x %x\n", serv_chat_addr.sin_port, serv_game_addr.sin_port);
	//socket 할당 

	if(connect(chat_sock,(struct sockaddr*)&serv_chat_addr,sizeof(serv_chat_addr))==-1)
		error_handling("chat connect() error");

	if(connect(game_sock,(struct sockaddr*)&serv_game_addr, sizeof(serv_game_addr)) == -1)
		error_handling("game connect() error");
	// connect 
	initscr();
	clear();
	set_cr_noecho_mode();
	//screen 셋팅 noechomode()
	pthread_create(&snd_thread,NULL,send_msg,(void*)&chat_sock);
	pthread_create(&rcv_thread,NULL,recv_msg,(void*)&chat_sock);
	pthread_create(&snd_game_thread,NULL,send_game,(void*)&game_sock);
	pthread_create(&rcv_game_thread, NULL, recv_game,(void*)&game_sock);
	// 4개의 thread를 생성
	// 2개는 채팅의 listen write 용 2개는 게임의 listen 과 write용 
	noecho(); // screen 의 noecho 
	pthread_join(snd_game_thread, &thread_return);
	pthread_join(rcv_game_thread, &thread_return);
	pthread_join(snd_thread,&thread_return);
	pthread_join(rcv_thread,&thread_return);
	// 4개의 쓰레드가 종료될때까지 대기한다 
	close(chat_sock);
	close(game_sock);
	set_cr_echo_mode();
	// echo on 
	return 0;
}

void* send_game(void *arg){
	int sock=*((int*)arg);
	write(sock, name, strlen(name));
	while(1){
		if(flag == 'R' || flag == 'r'){
			write(sock, "r", 1);
			break;
		}
	}
	while(1){
		if(flag==32){
			write(sock, "h\0", 2);
			flag =1;
		}
		else if(flag =='e'||flag=='E'){
			write(sock,"e",1);
			move(28,0);
			addstr("Wait other player....");
			refresh();
			flag =1;
		}
	}
}

void* recv_game(void *arg){
	int sock=*((int*)arg);
		start(sock);
		game(sock);
		end(sock);
}

void *send_msg(void *arg)
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1)
	{
		flag = getch();
		if(flag== 10){ // 엔터입력시에만 채팅 가능 
			echo();
			move(41, 0);
			addstr(name);
			refresh();
			getstr(msg);
			move(41,0);
			addstr(BLANK);
			refresh();
			if(!strcmp(msg,"q\0")||!strcmp(msg,"Q\0"))
			{
				set_cr_echo_mode();
				close(sock);
				exit(0);
			}
			sprintf(name_msg,"%s %s",name,msg);
			write(sock,name_msg,strlen(name_msg));//서버로 자신의 메시지를 전송
		}
		fflush(stdin);
		noecho();
	}
	return NULL;
}
void *recv_msg(void *arg)
{
	int sock=*((int*)arg);
	int i=0;
	int j, start=0;
	char name_msg[100][NAME_SIZE+BUF_SIZE];
	int str_len;
	int system_row=0;
	int count=0;
	draw();
	while(1)
	{
		str_len= read(sock, name_msg[i], NAME_SIZE+BUF_SIZE-1); //read 
		name_msg[i][str_len]=0;
		if(str_len == -1)
			return(void*)-1;
		chat_row = 30;
		move(chat_row, 0); // 좌표이동 
		for(j=start; j<start+10; j++){
			move(chat_row,0);
			addstr(BLANK);
			move(chat_row, 0);
			addstr(name_msg[j]);
			chat_row++;
		}
		move(41,0);
		refresh(); //  화면에 출력 
		i++;
		if(i>9)
			start++;
	}
	return NULL;
}
void error_handling(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
} 

void set_nodelay_mode(){
	int termflags;
	termflags = fcntl(0,F_GETFL);
	termflags |= O_NDELAY;
	fcntl(0,F_SETFL, termflags);
}

void set_cr_noecho_mode(void){
        struct termios ttystate;

        tcgetattr(0, &ttystate);
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(0, TCSANOW, &ttystate);
}

void set_cr_echo_mode(void){
	struct termios ttystate;

        tcgetattr(0, &ttystate);
        ttystate.c_lflag |= ICANON;
        ttystate.c_lflag |= ECHO;
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(0, TCSANOW, &ttystate);
}

void draw(){
	move(1,0);
	addstr("Welcome to BlackJack Game!!!!");
	move(25,0);
	addstr("If you want ready, Press <r>");
	move(29,0);
	addstr("--------------------chat room--------------------");
	move(41,0);
	refresh();
}

void start(int sock){
	char msg[BUF_SIZE];
	int row=4;
	int str_len;
	while((str_len = read(sock, msg, sizeof(msg))) != 0){
		if(!strncmp(msg,"end\0",4)){
			memset(msg,0,sizeof(msg));
			break;
		} //player 정보를 계속해서 read 하고 서버쪽에서 모두 ready가 되면 end 보내니까 그거를 받아서 종료 
		msg[str_len] =0;
		move(row, 0);
		addstr(BLANK);
		move(row, 0);
		addstr(msg);
		refresh();
		row +=4;
		if(row > 16)
			row = 4;
	}
}

void game(int sock){
	int str_len;
	int pos =4;
	int card = 0;
	int test=0;
	char gmsg[BUF_SIZE];
	
	char buf[100];

	redraw();

	while((str_len = read(sock, gmsg, sizeof(gmsg))) != 0){
		if(!strncmp(gmsg,"end\0",4))
			break;
		gmsg[str_len] = 0;
		move(pos,20);
		addstr(gmsg);
		refresh();
		pos++;
	} //화면에 카드 띄어주는 것 // 카드를 다 나줘줬어 --> end 메시지가 와서 끝나 
	memset(gmsg,0,sizeof(gmsg));
	pos = 11;
	while(1){
		str_len = read(sock, gmsg, sizeof(gmsg));
		if(!strncmp(gmsg,"end\0",4)){
			memset(gmsg,0,sizeof(gmsg));
			break;
		}
		else if(!strncmp(gmsg,"next\0",5)){
			pos = 11;
			card += 15;
			if(card >45){
				card = 0;
			}
			memset(gmsg,0,sizeof(gmsg));
			continue;
		}
		gmsg[str_len] = 0;
		move(pos, card);
		addstr("          ");
		move(pos,card);
		addstr(gmsg);
		refresh();
		pos++;
		memset(gmsg,0,sizeof(gmsg));
	} // e를 눌러서 end메시지가 오면 종료 
}

void end(int sock){
	int str_len;
	str_len = read(sock,msg, sizeof(msg)); //win lose
	msg[str_len]=0;
	move(15,3);
	addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg)); // win lose 
        msg[str_len]=0;
        move(15,18);
        addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg)); // win lose 
        msg[str_len]=0;
        move(15,33);
        addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg)); // winlose 
        msg[str_len]=0;
        move(15,48);
        addstr(msg);
	refresh();
	str_len = read(sock, msg, sizeof(msg)); // hidden 카드 정보를 읽어와
	msg[str_len] =0 ;
	move(20,0);
	addstr("Hidden : "); 
	addstr(msg); // 히든 카드 출력 
	refresh();
}

void redraw(){
	int i=0;
	for(i=0; i<29; i++){
		move(i,0);
		addstr(BLANK);
	}
	move(1,0);
	addstr("--------------------GAME START--------------------");
	move(3,20);
	addstr("Dealer");
	move(10,3);
	addstr("P1");
	move(10,18);
	addstr("P2");
	move(10,33);
	addstr("P3");
	move(10,48);
	addstr("P4");
	move(25,0);
	addstr("If you want more card, Press <space>");
	move(26,0);
	addstr("If you want stop, Press <e>");
	refresh();
}
