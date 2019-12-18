#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <sys/socket.h>

 

void error_handling(char *message);

typedef struct {

	int order;

	int number;

	char shape[3];

}trump;
int dealer_total=0;
void winlose(int total);
int dealer(trump card[]);
void display(trump card[],int start_num,int end_num);
void display_hidden(trump card[]);
int cal_num(int num);
char space[13][100] = {

		"┌──────────────┐ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"│              │ ",
		"└──────────────┘ "
};


int main(int argc, char *argv[])

{
	system("clear");	
	
        int sock;

        struct sockaddr_in serv_addr;

       // char message[30];

        trump card[52];

 

        if (argc != 3)

        {

               printf("Usage: %s <IP> <port>\n", argv[0]);

               exit(1);

        }

 

        sock = socket(PF_INET, SOCK_STREAM, 0); //소켓을 생성

        if (sock == -1)

               error_handling("socket() error");

 

        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;

        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

        serv_addr.sin_port = htons(atoi(argv[2]));

 

        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) //connect 함수호출

               error_handling("connect() error");

 

        int n = read(sock, card, sizeof(card) - 1);
	
        if (n == -1)

               error_handling("read() error");
	
 	char c;
	char arr[5];
	int dealer_cardnum;
	int total=cal_num(card[0].number)+cal_num(card[1].number);

	arr[0]=2;
	while(1)
	{
		display_hidden(card);
		display(card,0,arr[0]);	
		printf("total is : %d\n stop을 원하시면 q를 누르세요\n",total);	
		scanf("%c",&c);
		if(c=='q')
			break;
		system("clear");
		total += cal_num(card[arr[0]++].number);		
	}

	write(sock,arr,sizeof(int));
	system("clear");
	
	dealer_cardnum=dealer(card);
	
	display(card,dealer_cardnum,52);
	printf("─-─-─-─-─-─-─-─-─-─-─-─-─-─-─-─-─\n");
	display(card,0,arr[0]);
	printf("your total : %d\n",total);
	
	winlose(total);
        close(sock);

        return 0;

}
void winlose(int total)//승리 확인
{
	if((total<dealer_total && dealer_total<22)||(total>21&&dealer_total<22))
		printf("Computer win!!\n");
	else if((total>dealer_total && total < 22) ||(total<22&&dealer_total>21))
		printf("User win!!\n");
	else if(total == dealer_total || (total>21 && dealer_total>21))
		printf("Due!!\n");
}
int dealer(trump card[]) // 딜러 계산
{
	int i=50;
	dealer_total=cal_num(card[51].number)+cal_num(card[50].number);
	while(dealer_total<17)
		dealer_total+=cal_num(card[--i].number);
	printf("dealer total : %d\n", dealer_total);
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
void display_hidden(trump card[])//가로로 출력
{
	int i,j;
	printf("Dealer Deck\n");
	printf("%s",space[0]);
	printf("%s",space[0]);
	printf("\n");
	//printf("%d",card[i].number);
	if (card[50].number == 10) 
		printf("│ 10           │ ");
	else
		printf(card[50].number > 10 ? "│ %c            │ " : "│ %d            │ ",card[50].number);
	
	printf("│  ?           │ ");
	printf("\n");
	
	for(j=2;j<6;j++)	
	{
		printf("%s%s",space[j],space[j]);		
		printf("\n");
	}

		if (strcmp(card[50].shape, "a") == 0)
			printf("│      ♠       │ ");

		else if (strcmp(card[50].shape, "b") == 0)
			printf("│      ◆       │ ");

		else if (strcmp(card[50].shape, "c") == 0)
			printf("│      ♥       │ ");
		else if (strcmp(card[50].shape, "d") == 0)
			printf("│      ♣       │ ");
	printf("│    Hidden    │ ");
	printf("\n");
	for(j=7;j<11;j++)	
	{
		printf("%s%s",space[j],space[j]);		
		printf("\n");
	}	
	if (card[50].number == 10)
		printf("│ 10           │ ");
	else
		printf(card[50].number > 10 ? "│            %c │ " : "│            %d │ ",card[50].number);
	printf("│            ? │ ");
	printf("\n");
	printf("%s%s",space[12],space[12]);
	printf("\n");
	
	printf("─-─-─-─-─-─-─-─-─-─-─-─-─-─-─-─-─\n");
}
void display(trump card[],int start_num,int end_num)//가로로 출력
{
	int i,j;
	for(i=start_num;i<end_num;i++)
		printf("%s",space[0]);
	printf("\n");
	for(i=start_num;i<end_num;i++)
	{
		if (card[i].number == 10) 
			printf("│ 10           │ ");
		else
			printf(card[i].number > 10 ? "│ %c            │ " : "│ %d            │ ",card[i].number);
	}
	printf("\n");
	
	for(j=2;j<6;j++)	
	{
		for(i=start_num;i<end_num;i++)
			printf("%s",space[j]);		
		printf("\n");
	}
	for(i=start_num;i<end_num;i++)	
	{
		if (strcmp(card[i].shape, "a") == 0)
			printf("│      ♠       │ ");

		else if (strcmp(card[i].shape, "b") == 0)
			printf("│      ◆       │ ");

		else if (strcmp(card[i].shape, "c") == 0)
			printf("│      ♥       │ ");
		else if (strcmp(card[i].shape, "d") == 0)
			printf("│      ♣       │ ");
	}
	printf("\n");
	for(j=7;j<11;j++)	
	{
		for(i=start_num;i<end_num;i++)
		{
			printf("%s",space[j]);		
		}
		printf("\n");
	}	
	for(i=start_num;i<end_num;i++)
	{
		if (card[i].number == 10)
			printf("│ 10           │ ");
		else
			printf(card[i].number > 10 ? "│            %c │ " : "│            %d │ ",card[i].number);
	}
	printf("\n");
	for(i=start_num;i<end_num;i++)
		printf("%s",space[12]);
	printf("\n");

}
 

void error_handling(char *message)

{

        fputs(message, stderr);

        fputc('\n', stderr);

        exit(1);

}
