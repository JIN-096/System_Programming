#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_CLNT 4

void *handle_clnt(void *arg);
void *handle_game(void *arg);
void send_msg(char* msg, int len);
void send_game_msg(char*, int);
void error_handling(char *msg);
void send_start(int);
void send_cur_player();
void game(int);
void trans(int ,int,int);
void init();

int sum=0;
int size=0;
int player[MAX_CLNT+1][5];
int card[52];
int cnt[5] = {0};
int clnt_cnt =0;
int clnt_socks[MAX_CLNT];
int clnt_game_socks[MAX_CLNT];
char p_name[MAX_CLNT][BUF_SIZE];
int r_state[MAX_CLNT] = {0};
int g_state[MAX_CLNT] = {0};
pthread_mutex_t mutx;

int main(int argc, char* argv[]){
	int serv_chat_sock, clnt_chat_sock;
	int serv_game_sock, clnt_game_sock;
	int i;
	struct sockaddr_in serv_chat_adr, clnt_chat_adr, serv_game_adr, clnt_game_adr;
	int clnt_adr_sz;
	pthread_t t_id[MAX_CLNT], t_game[MAX_CLNT];
	init(); // 카드 셔플이랑 셋팅
	if(argc != 2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_chat_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_chat_adr, 0, sizeof(serv_chat_adr));
	serv_chat_adr.sin_family = AF_INET;
	serv_chat_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_chat_adr.sin_port = htons(atoi(argv[1]));

        serv_game_sock = socket(PF_INET, SOCK_STREAM, 0);
        memset(&serv_game_adr, 0, sizeof(serv_game_adr));
        serv_game_adr.sin_family = AF_INET;
        serv_game_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_game_adr.sin_port = htons(atoi(argv[1])+100);

	if(bind(serv_chat_sock, (struct sockaddr*) &serv_chat_adr, sizeof(serv_chat_adr))==-1)
		error_handling("bind() error");
	if(bind(serv_game_sock, (struct sockaddr*)&serv_game_adr,sizeof(serv_game_adr)) == -1)
		error_handling("bind() error");

	if(listen(serv_chat_sock, 5) == -1)
		error_handling("listen() error");
	if(listen(serv_game_sock, 5) == -1)
		error_handling("listen() error");
	// socket setting
	while(1){
		clnt_adr_sz = sizeof(clnt_chat_adr);
		clnt_chat_sock = accept(serv_chat_sock, (struct sockaddr*)&clnt_chat_adr, &clnt_adr_sz);
		clnt_game_sock = accept(serv_game_sock, (struct sockaddr*)&clnt_game_adr, &clnt_adr_sz);
		// socket accept --> connect
		pthread_mutex_lock(&mutx); // mutex lock --> clnt_count fix
		clnt_socks[clnt_cnt] = clnt_chat_sock; //clnt count arrary for chatting
		clnt_game_socks[clnt_cnt] = clnt_game_sock; //client count arry for gaming
		pthread_mutex_unlock(&mutx); // mutex unlock 
		pthread_create(&t_id[clnt_cnt], NULL, handle_clnt, (void*)&clnt_chat_sock); // chating thread 방금 연결된 아이의 sock 정보를 가지고있지
		pthread_create(&t_game[clnt_cnt++], NULL, handle_game, (void*)&clnt_game_sock); // 방금연결된 아이의 sock 정보를 가진다
	}
		for(i=0; i<MAX_CLNT; i++){
			pthread_detach(t_game[i]);
			pthread_detach(t_id[i]);
		} // 쓰레드 종료 까지 메인함수 대기
	//	printf("Connect client IP address : %s %d\n", inet_ntoa(clnt_game_adr.sin_addr), clnt_game_adr.sin_port);
	close(serv_chat_sock);
	close(serv_game_sock);
	return 0;
}


void send_cur_player(){ 
	int i; char buf[BUF_SIZE]={0}; 
	int len;
	for(i=0; i<4; i++){
		if(i < clnt_cnt){
			if(r_state[i] == 1) // if read == 1 
				len = sprintf(buf, "player %d : %-10s State : Ready\n",i+1, p_name[i]);
			else // ready == 0 
				len = sprintf(buf, "player %d : %-10s State : Unready\n", i+1, p_name[i]);
		}
		else
			len = sprintf(buf, "player %d : unconnected\n",i+1);
		buf[len] = 0;
		usleep(200000); // usleep() tcp --> 데이터의 경계가 없어. 처음에 1234 두번째로 4567 세번째로 1234 첫번재 데이터를 받고 작업을 하는동안 2,3번째 데이터가 같이왔어 그러면 한번에 읽어
		send_game_msg(buf, len); // 채팅이랑 같은데 game 소켓한테 보내준거
	}
}


void *handle_game(void *arg){
	int sock = *((int*)arg);
	int str_len,i;
	int flag=0;
	char msg[BUF_SIZE];
	str_len = read(sock,p_name[clnt_cnt-1], sizeof(p_name[BUF_SIZE])); // 플레이어 이름을 받아
	p_name[clnt_cnt-1][str_len]=0;
	r_state[clnt_cnt-1] =0; // ready state --> ready or unready [1, 0]
	send_cur_player();
	//현재 플레이어 상황을 전송
		for(i=0; i<MAX_CLNT; i++)
			r_state[i] = 0; // 나중에 수정할게요  최현석 part 
		while((str_len = read(sock, msg, sizeof(msg))) !=0){
			if(!strcmp(msg,"r")){
				for(i=0; i<clnt_cnt; i++){
					if(clnt_game_socks[i] ==sock){
						r_state[i] = 1; 
						break;
					}
				}
				send_cur_player();
			break;
			}
		} // 게임 소켓에서 'r' 이라는 메시지가 들어오면 read 상태를 변환한다. 
		while(flag!=1){
			flag = 1;
			for(i=0; i<clnt_cnt; i++){
				if(r_state[i] == 0)
					flag = 0;
			}
			if(clnt_cnt <4)
				flag =0;
		} // 플레이어가 모두 ready이면 while 문 종료 아니면 계속 순환
		sleep(1); // 데이터 오류방지
		write(sock,"end\0",4); // 게임시작 
		game(sock); // 게임시작 

}

void init(){
	int i,j,x,y,temp,count=0;
	srand((unsigned int)time(NULL));
  	for(i=0; i<52; i++)
                card[i] = i+1; //카드 할당
        for(i=0; i<100; i++){
                x = rand() % 52;
                y = rand() % 52;
                if(x !=y){
                        temp = card[x];
                        card[x] = card[y];
                        card[y] = temp;
                }
        } // 카드 셔플

        for(i=0; i<5; i++){
                player[0][i] = card[count++];
                sum += ((player[0][i]%13)+1);
                size++;
                if(sum>=17 && sum <=21)
                        break;
                else if(sum>21){
                        i=-1;
                        size=0;
                        sum=0;
                }
        } // 딜러한테 먼저 카드를 줘 얘는 17이상 또는 21이하일 때까지 계속뽑아

	       for(i =0; i<4; i++){
                player[i+1][0] = card[count++];
                player[i+1][1] = card[count++];
                player[i+1][2] = card[count++];
                player[i+1][3] = card[count++];
                player[i+1][4] = card[count++];
                cnt[i+1] = 2;
        }
		    // 카드를 할당해 놓는것 



}

void game(int sock){
	char buf[BUF_SIZE];
	int str_len;
	int flag,i,j;
	int p_sum=0;

	pthread_mutex_lock(&mutx); 
	for(i=0; i<size; i++){
		if(i==0){
			usleep(20000);
			write(sock,"HIDDEN\0", 8);
		}
		else{
			usleep(20000);
			trans(sock,player[0][i],1);
		}
	} // 0 == 딜러 나머지는 플레이어 
	pthread_mutex_unlock(&mutx);
	usleep(100000); // 데이터 오류 방지
	write(sock,"end\0", 4);
	// 엔드
	for(i=1; i<5; i++){
		for(j=0; j<cnt[i]; j++){
			usleep(100000);
			trans(sock,player[i][j],1);
		}
		usleep(10000);
		write(sock,"next\0",5);
	} // 카드 0~51 --> 하트 다이아 클로버 등으로 전환해서 메시지 형태로 클라이언트에 전송

	// cnt[i]는 각 플레이어들의 카드 갯수
	// 딜러 셋팅, 나머지도 모두 2장씩 주는 과정

	while((str_len = read(sock, buf, sizeof(buf)))!=0){ // 플레이어가 spacebar를 입력하면 한장을 더줘 
		buf[str_len]=0;
		if(!strncmp(buf,"h\0",2)){
			for(i=0; i<clnt_cnt; i++)
				if(sock == clnt_game_socks[i]){
					cnt[i+1]+=1;
					break;
				}
			for(i=1; i<5; i++){
				for(j=0; j<cnt[i]; j++){
					usleep(100000);
					trans(sock,player[i][j],0);
				} // 전송 
				usleep(100000);
				send_game_msg("next\0",5);
			}
		}
		if(!strncmp(buf,"e\0",2)){
			for(i=0; i<clnt_cnt; i++){
				if(sock == clnt_game_socks[i])
					g_state[i] = 1;
			} // e를 입력하면 플레이어의 상태를 대기상태로 바꿔줌
			break;
		}
		memset(buf,0,sizeof(buf));
	}
		while(flag!=1){
			flag =1;
			for(i=0; i<clnt_cnt; i++){
				if(g_state[i]==0)
					flag = 0;
			}
			if(flag==1)
				break;
		} // 모두가 대기상태일때 카드 나눠주는 걸 종료
	usleep(100000);
	write(sock,"end\0", 4);


	usleep(20000);
	for(i=0; i<4; i++){
		for(j=0; j<cnt[i+1]; j++){
			p_sum+= ((player[i+1][j]%13)+1);
		}
		if(p_sum >= sum && p_sum <=21)
			write(sock,"WIN\0",4);
		else
			write(sock,"LOSE\0",5);
		p_sum=0;
		usleep(20000);
	} // 플레이어가 가지고 있는 카드 합과 딜러의 카드 합을 비교 

	trans(sock,player[0][0],1);
	 // Hidden카드의 정보를 플레이어들에게 전송
}

void trans(int sock, int num, int f){
	char buf[BUF_SIZE];
	switch(num/13){
	case 0:
		sprintf(buf, "Heart %d\n",num%13+1); 
		break;
	case 1:
		sprintf(buf, "Clover %d\n",num%13+1);
		break;
	case 2:
		sprintf(buf, "Spade %d\n", num%13+1);
		break;
	case 3:
		sprintf(buf, "Diamond %d\n", num%13+1);
		break;
	}
	usleep(20000);
	if(f==1)
		write(sock,buf,strlen(buf));
	else if(f==0)
		send_game_msg(buf,strlen(buf));
}


void *handle_clnt(void * arg){ //chatting mange thread
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	while((str_len = read(clnt_sock, msg, sizeof(msg))) != 0){
		msg[str_len]='\0';
		send_msg(msg, str_len);
	}
//if read buffer에 데이터가 들어오면 --> 메시지를 읽고 send_msg 함수로 메시지 전달
	// ctrl + c 들어오면 종료 후 아래로 진행

	pthread_mutex_lock(&mutx); // mutex lock
	for(i=0; i<clnt_cnt; i++){
		if(clnt_sock == clnt_socks[i]){
			while(i< clnt_cnt-1){
				clnt_socks[i] = clnt_socks[i+1];
				clnt_game_socks[i] = clnt_game_socks[i+1];
				strcpy(p_name[i], p_name[i+1]);
				r_state[i] = r_state[i+1];
				i++;
			break;
			}
		}
	} // 배열의 빈칸 을 지워주고 앞으로 한칸씩 댕기는것 
	clnt_cnt--; // 갯수를 하나 줄여주면 
	pthread_mutex_unlock(&mutx);
	send_cur_player(); // 변화된 플레이어수를 전송
	close(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len){
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len);
	pthread_mutex_unlock(&mutx);
} // pthread_mutex_lock --> clint 배열에 할당된 clint에게 메시지 전송 --> mutex_unlock

void send_game_msg(char* msg, int len){
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0;i <clnt_cnt; i++)
		write(clnt_game_socks[i], msg, len);
	pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg){
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
