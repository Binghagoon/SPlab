#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define N_OF_PORT 5
#define BUF_SIZE 65536 // server.c의 message 크기
#define SERVER_ADDR "127.0.0.1"

void *run(void *data)
{
    struct sockaddr_in *serv_addr = (struct sockaddr_in *)data; // pthread 루틴으로 넘어온 포인터를 sockaddr 형식으로 캐스팅
    int port_n = htons(serv_addr->sin_port);                    // 포트번호 엔디안 변환

    // 포트번호.txt에 쓰는 fd
    char filename[10] = "";
    sprintf(filename, "%d", port_n);
    strcat(filename, ".txt");
    FILE *file_fd = fopen(filename, "w");

    // IPv4 TCP 연결 지향 소켓을 열어 fd를 가져옴
    int socket_fd;
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Client(port %d): failed to open socket.\n", port_n);
        exit(-1);
    }

    // 서버로 연결 시도하고 소켓 fd에 연결
    if (connect(socket_fd, (struct sockaddr *)serv_addr, sizeof(struct sockaddr)) == -1)
    {
        printf("Client(port %d): connect failed.\n", port_n);
        exit(-1);
    }
    else
    {
        printf("Client(port %d): connect succeed!\n", port_n);
    }

    // 사용자가 클라이언트를 종료할때까지 서버 write 대기 반복
    while (1)
    {
        // 소켓 수신용 버퍼를 만들고 서버에서 write가 올때까지 대기
        char buffer[BUF_SIZE] = "";
        int recv_len = recv(socket_fd, buffer, BUF_SIZE, 0);

        // 타임스탬프 로깅을 위해 시간 가져옴
        struct timeval now;
        gettimeofday(&now, NULL);
        struct tm *tm = localtime(&now.tv_sec);

        // H:M:S.ms length buffer 형식으로 로깅
        fprintf(file_fd, "%d:%d:%d.%ld %d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, now.tv_usec / 100, recv_len, buffer);
    }

    // 파일, 소켓 fd 정리
    fclose(file_fd);
    close(socket_fd);
}

int main()
{
    // 포트 수만큼 pthread 준비
    pthread_t thread[N_OF_PORT];

    for (int i = 0; i < N_OF_PORT; i++)
    {
        // sockaddr_in 구조체 할당
        struct sockaddr_in* serv_addr = malloc(sizeof(struct sockaddr_in));
        serv_addr->sin_family = AF_INET; // IPv4
        serv_addr->sin_addr.s_addr = inet_addr(SERVER_ADDR); // 서버 주소 변환
        serv_addr->sin_port = htons((i + 1) * 2000); // 포트번호 엔디안 변환, 포트번호는 임의로 2000의 배수로 지정

        // run 루틴으로 pthread 분기, serv_addr를 payload로 전달
        int res = pthread_create(&thread[i], NULL, run, (void *)serv_addr);
    }

    return 0;
}
