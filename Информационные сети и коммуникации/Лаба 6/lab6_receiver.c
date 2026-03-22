/*
 * Лабораторная работа №6, Вариант 1
 * UDP-приёмник аудиопотока с меткой RAW
 * Кровин Никита Игоревич
 *
 * Формат пакета:
 *   Байты 0-3:  метка типа потока ("R08\0" или "R16\0")
 *   Байты 4-7:  временная метка (uint32_t, сетевой порядок)
 *   Байты 8+:   аудиоданные (8-bit mono 8000Hz или 16-bit mono 8000Hz)
 *
 * Использование:
 *   ./lab6_receiver.bin [порт] [выходной_файл]
 *   По умолчанию: порт 9000, вывод в audio_out.raw
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT        9000
#define BUFSIZE     4096
#define HEADER_SIZE 8

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : PORT;
    const char *outfile = (argc > 2) ? argv[2] : "audio_out.raw";

    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    int recv_len;
    int packet_count = 0;
    long total_bytes = 0;

    /* Создаём UDP сокет */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    /* Разрешаем получение широковещательных пакетов */
    int bcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    printf("=== UDP Audio Receiver, Вариант 1 ===\n");
    printf("Порт:    %d\n", port);
    printf("Вывод:   %s\n", outfile);
    printf("Ctrl+C для остановки\n\n");

    /* Открываем выходной файл */
    FILE *fout = fopen(outfile, "wb");
    if (!fout) { perror("fopen"); exit(1); }

    while (1) {
        recv_len = recv(sock, buf, BUFSIZE, 0);
        if (recv_len < 0) { perror("recv"); break; }
        if (recv_len < HEADER_SIZE) {
            fprintf(stderr, "Короткий пакет: %d байт, пропускаем\n", recv_len);
            continue;
        }

        /* Парсим заголовок */
        char label[5];
        memcpy(label, buf, 4);
        label[4] = '\0';

        uint32_t timestamp;
        memcpy(&timestamp, buf + 4, 4);
        timestamp = ntohl(timestamp);

        int audio_len = recv_len - HEADER_SIZE;
        char *audio_data = buf + HEADER_SIZE;

        /* Проверяем метку — только RAW потоки */
        if (strncmp(label, "R08", 3) == 0 || strncmp(label, "R16", 3) == 0) {
            /* Пишем аудиоданные в файл */
            fwrite(audio_data, 1, audio_len, fout);
            fflush(fout);

            packet_count++;
            total_bytes += audio_len;

            printf("[%5d] Метка: %s | Время: %u | Аудио: %d байт | Всего: %ld байт\n",
                   packet_count, label, timestamp, audio_len, total_bytes);
        } else {
            printf("[пропуск] Метка: %s (не RAW)\n", label);
        }
    }

    fclose(fout);
    close(sock);

    printf("\nПринято пакетов: %d, байт аудио: %ld\n", packet_count, total_bytes);
    printf("Воспроизведение: aplay -r 8000 -f U8 -c 1 %s\n", outfile);
    return 0;
}
