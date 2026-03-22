/*
 * Тестовый UDP-отправитель аудиопотока (имитация сервера преподавателя)
 * Генерирует синусоидальный тон 440 Hz и отправляет UDP пакетами
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT      9000
#define SAMPLERATE 8000
#define CHUNK     512   /* сэмплов за пакет */

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : PORT;
    int packets = (argc > 3) ? atoi(argv[3]) : 20;

    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    int bcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_aton(host, &addr.sin_addr);

    printf("Отправляю %d UDP пакетов на %s:%d...\n", packets, host, port);

    char buf[8 + CHUNK];

    for (int p = 0; p < packets; p++) {
        /* Заголовок: метка R08 + timestamp */
        memcpy(buf, "R08", 4);
        uint32_t ts = htonl((uint32_t)time(NULL) + p);
        memcpy(buf + 4, &ts, 4);

        /* Генерируем синус 440 Hz, 8-bit unsigned */
        for (int i = 0; i < CHUNK; i++) {
            double t = (double)(p * CHUNK + i) / SAMPLERATE;
            uint8_t sample = (uint8_t)(128 + 127 * sin(2.0 * M_PI * 440.0 * t));
            buf[8 + i] = (char)sample;
        }

        sendto(sock, buf, 8 + CHUNK, 0,
               (struct sockaddr *)&addr, sizeof(addr));
        printf("Пакет %d отправлен (%d байт аудио)\n", p + 1, CHUNK);
        usleep(64000); /* ~64ms между пакетами */
    }

    close(sock);
    printf("Готово.\n");
    return 0;
}
