#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void error_handling(char *message);

typedef struct {

	int order;

	int number;

	char shape[3];

}trump;
int dealer_total=0;
int cal_num(int num);
int dealer(trump card[]);
void winlose(int total);



void make_card(trump m_card[]) {

	char shape[4][3] = { "a", "b", "c", "d" };

	for (int i = 0; i < 4; i++) {

		for (int j = i * 13; j < i * 13 + 13; j++) {

			m_card[j].order = i;

			strcpy(m_card[j].shape, shape[i]);

			int k = j % 13 + 1;

 

			if (k == 1)

				m_card[j].number = 'A';

			else if (k == 11)

				m_card[j].number = 'J';

			else if (k == 12)

				m_card[j].number = 'Q';

			else if (k == 13)

				m_card[j].number = 'K';

			else
				m_card[j].number = k;
		}
	}
}

 

void shuffle_card(trump m_card[]) {

	int rnd;

	trump temp;

	srand((unsigned)time(NULL));

	for (int i = 0; i < 52; i++) {

		rnd = rand() % 52;

		temp = m_card[rnd];

		m_card[rnd] = m_card[i];

		m_card[i] = temp;;

	}

}

int main(int argc, char* argv[])
{
	 int serv_sock;

        int clnt_sock;

 	trump card[52];

	make_card(card);
	shuffle_card(card);

        struct sockaddr_in serv_addr;

        struct sockaddr_in clnt_addr;

        socklen_t clnt_addr_size;

 
        if (argc != 2)

        {

               printf("Usage:%s <port>\n", argv[0]);

               exit(1);

        }

 

        serv_sock = socket(PF_INET, SOCK_STREAM, 0); //소켓을 생성

        if (serv_sock == -1)

               error_handling("socket() error");

 

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;

        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        serv_addr.sin_port = htons(atoi(argv[1]));

 

        if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) //bind 함수호출 IP주소와 PORT번호를 할당

               error_handling("bind() error");

 

        if (listen(serv_sock, 5) == -1) //listen 함수를 호출

               error_handling("listen() error");

 

        clnt_addr_size = sizeof(clnt_addr);

        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); //accept 함수를 호출

        if (clnt_sock == -1)

               error_handling("accept() error");

 

        write(clnt_sock, card, sizeof(card)); //write 함수 데이터를 전송하는 기능

	int total=0,dealer_cardnum;
	char num[5];

	read(clnt_sock,num,sizeof(int));
	printf("num : %d\n",num[0]);
	for(int k=0;k<num[0];k++)
	{
		total+=cal_num(card[k].number);
	}
	dealer_cardnum=dealer(card);
	printf("total : %d\n",total);
	
	printf("dealer_cardnum : %d\n",dealer_cardnum);

	close(clnt_sock);

        close(serv_sock);

	winlose(total);
	
	
        return 0;	
}
void winlose(int total)//승리 확인
{
	char *win="User win!!!";
	char *lose="Dealer win!! User lose!!";
	char *due="Due!!!";
	int fd;
	if(-1==(fd=creat("./result.txt",0644)))
		return;
	if((total<dealer_total && dealer_total<22)||(total>21&&dealer_total<22))
	{
		printf("Computer win!!\n");
		write(fd,lose,strlen(lose));
	}
	else if((total>dealer_total && total < 22) ||(total<22&&dealer_total>21))
	{
		printf("User win!!\n");
		write(fd,win,strlen(win));
	}
	else if(total == dealer_total || (total>21 && dealer_total>21))
	{
		printf("Due!!\n");
		write(fd,due,strlen(due));
	}
	close(fd);
}
int dealer(trump card[])
{
	int i=50;
	dealer_total=cal_num(card[51].number)+cal_num(card[50].number);
	while(dealer_total<17)
		dealer_total+=cal_num(card[--i].number);
	return i;
}
		
int cal_num(int num)//A,K,Q,J 변환
{
	switch(num)
	{
	case 65://A
		return 1;
	case 75://K
		return 10;
	case 81://Q
		return 10;
	case 74://J
		return 10;
	default:
		return num;
	}
}

void error_handling(char *message)

{

        fputs(message, stderr);

        fputc('\n', stderr);

        exit(1);

}
