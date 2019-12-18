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
	init(); // ī�� �����̶� ����
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
		pthread_create(&t_id[clnt_cnt], NULL, handle_clnt, (void*)&clnt_chat_sock); // chating thread ��� ����� ������ sock ������ ����������
		pthread_create(&t_game[clnt_cnt++], NULL, handle_game, (void*)&clnt_game_sock); // ��ݿ���� ������ sock ������ ������
	}
		for(i=0; i<MAX_CLNT; i++){
			pthread_detach(t_game[i]);
			pthread_detach(t_id[i]);
		} // ������ ���� ���� �����Լ� ���
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
		usleep(200000); // usleep() tcp --> �������� ��谡 ����. ó���� 1234 �ι�°�� 4567 ����°�� 1234 ù���� �����͸� �ް� �۾��� �ϴµ��� 2,3��° �����Ͱ� ���̿Ծ� �׷��� �ѹ��� �о�
		send_game_msg(buf, len); // ä���̶� ������ game �������� �����ذ�
	}
}


void *handle_game(void *arg){
	int sock = *((int*)arg);
	int str_len,i;
	int flag=0;
	char msg[BUF_SIZE];
	str_len = read(sock,p_name[clnt_cnt-1], sizeof(p_name[BUF_SIZE])); // �÷��̾� �̸��� �޾�
	p_name[clnt_cnt-1][str_len]=0;
	r_state[clnt_cnt-1] =0; // ready state --> ready or unready [1, 0]
	send_cur_player();
	//���� �÷��̾� ��Ȳ�� ����
		for(i=0; i<MAX_CLNT; i++)
			r_state[i] = 0; // ���߿� �����ҰԿ�  ������ part 
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
		} // ���� ���Ͽ��� 'r' �̶�� �޽����� ������ read ���¸� ��ȯ�Ѵ�. 
		while(flag!=1){
			flag = 1;
			for(i=0; i<clnt_cnt; i++){
				if(r_state[i] == 0)
					flag = 0;
			}
			if(clnt_cnt <4)
				flag =0;
		} // �÷��̾ ��� ready�̸� while �� ���� �ƴϸ� ��� ��ȯ
		sleep(1); // ������ ��������
		write(sock,"end\0",4); // ���ӽ��� 
		game(sock); // ���ӽ��� 

}

void init(){
	int i,j,x,y,temp,count=0;
	srand((unsigned int)time(NULL));
  	for(i=0; i<52; i++)
                card[i] = i+1; //ī�� �Ҵ�
        for(i=0; i<100; i++){
                x = rand() % 52;
                y = rand() % 52;
                if(x !=y){
                        temp = card[x];
                        card[x] = card[y];
                        card[y] = temp;
                }
        } // ī�� ����

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
        } // �������� ���� ī�带 �� ��� 17�̻� �Ǵ� 21������ ������ ��ӻ̾�

	       for(i =0; i<4; i++){
                player[i+1][0] = card[count++];
                player[i+1][1] = card[count++];
                player[i+1][2] = card[count++];
                player[i+1][3] = card[count++];
                player[i+1][4] = card[count++];
                cnt[i+1] = 2;
        }
		    // ī�带 �Ҵ��� ���°� 



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
	} // 0 == ���� �������� �÷��̾� 
	pthread_mutex_unlock(&mutx);
	usleep(100000); // ������ ���� ����
	write(sock,"end\0", 4);
	// ����
	for(i=1; i<5; i++){
		for(j=0; j<cnt[i]; j++){
			usleep(100000);
			trans(sock,player[i][j],1);
		}
		usleep(10000);
		write(sock,"next\0",5);
	} // ī�� 0~51 --> ��Ʈ ���̾� Ŭ�ι� ������ ��ȯ�ؼ� �޽��� ���·� Ŭ���̾�Ʈ�� ����

	// cnt[i]�� �� �÷��̾���� ī�� ����
	// ���� ����, �������� ��� 2�徿 �ִ� ����

	while((str_len = read(sock, buf, sizeof(buf)))!=0){ // �÷��̾ spacebar�� �Է��ϸ� ������ ���� 
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
				} // ���� 
				usleep(100000);
				send_game_msg("next\0",5);
			}
		}
		if(!strncmp(buf,"e\0",2)){
			for(i=0; i<clnt_cnt; i++){
				if(sock == clnt_game_socks[i])
					g_state[i] = 1;
			} // e�� �Է��ϸ� �÷��̾��� ���¸� �����·� �ٲ���
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
		} // ��ΰ� �������϶� ī�� �����ִ� �� ����
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
	} // �÷��̾ ������ �ִ� ī�� �հ� ������ ī�� ���� �� 

	trans(sock,player[0][0],1);
	 // Hiddenī���� ������ �÷��̾�鿡�� ����
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
//if read buffer�� �����Ͱ� ������ --> �޽����� �а� send_msg �Լ��� �޽��� ����
	// ctrl + c ������ ���� �� �Ʒ��� ����

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
	} // �迭�� ��ĭ �� �����ְ� ������ ��ĭ�� ���°� 
	clnt_cnt--; // ������ �ϳ� �ٿ��ָ� 
	pthread_mutex_unlock(&mutx);
	send_cur_player(); // ��ȭ�� �÷��̾���� ����
	close(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len){
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len);
	pthread_mutex_unlock(&mutx);
} // pthread_mutex_lock --> clint �迭�� �Ҵ�� clint���� �޽��� ���� --> mutex_unlock

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
