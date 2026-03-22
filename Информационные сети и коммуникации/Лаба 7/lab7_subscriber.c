/*
 * Лабораторная работа №7, Вариант 1
 * ZMQ подписчик: получение сонета Шекспира (перевод, нечётная команда)
 * Кровин Никита Игоревич
 *
 * Компиляция: gcc ./lab7_subscriber.c -o ./lab7_subscriber.bin -lz -lzmq
 *
 * Использование:
 *   ./lab7_subscriber.bin [IP] [порт]
 *   По умолчанию: 127.0.0.1:5563
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zmq.h>
#include <zlib.h>

#define BUFSIZE  8192
#define KEY_TEXT "RU"    /* ключ подписки: русский перевод */
#define KEY_COMP "ZU"    /* ключ сжатого сообщения */

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 5563;

    char endpoint[128];
    snprintf(endpoint, sizeof(endpoint), "tcp://%s:%d", host, port);

    printf("=== ZMQ Subscriber, Вариант 1 ===\n");
    printf("Сервер:  %s\n", endpoint);
    printf("Ключ:    %s (перевод сонета) / %s (сжатый)\n\n", KEY_TEXT, KEY_COMP);

    /* Инициализация ZMQ контекста */
    void *context = zmq_ctx_new();
    if (!context) { fprintf(stderr, "zmq_ctx_new failed\n"); exit(1); }

    /* Создаём сокет-подписчик */
    void *socketzmq = zmq_socket(context, ZMQ_SUB);
    assert(socketzmq);

    /* Подключаемся к серверу */
    int ret = zmq_connect(socketzmq, endpoint);
    if (ret < 0) { perror("zmq_connect"); return 1; }

    /* Подписываемся на текстовые и сжатые сообщения */
    ret = zmq_setsockopt(socketzmq, ZMQ_SUBSCRIBE, KEY_TEXT, strlen(KEY_TEXT));
    assert(ret == 0);
    ret = zmq_setsockopt(socketzmq, ZMQ_SUBSCRIBE, KEY_COMP, strlen(KEY_COMP));
    assert(ret == 0);

    printf("Ожидание сообщений... (Ctrl+C для выхода)\n\n");

    char buf[BUFSIZE];
    int msg_count = 0;

    while (1) {
        int rc = zmq_recv(socketzmq, buf, BUFSIZE - 1, 0);
        if (rc < 0) { perror("zmq_recv"); break; }
        buf[rc] = '\0';
        msg_count++;

        /* Определяем тип сообщения по ключу */
        if (strncmp(buf, KEY_COMP, strlen(KEY_COMP)) == 0) {
            /* Сжатое сообщение — декомпрессия zlib */
            char *data = buf + strlen(KEY_COMP);
            int data_len = rc - (int)strlen(KEY_COMP);

            unsigned char out[BUFSIZE];
            uLongf out_len = BUFSIZE - 1;

            int zret = uncompress(out, &out_len,
                                  (const Bytef *)data, (uLong)data_len);
            if (zret == Z_OK) {
                out[out_len] = '\0';
                printf("[%d] Сжатое сообщение (ключ %s), распаковано %lu байт:\n",
                       msg_count, KEY_COMP, out_len);
                printf("%s\n\n", (char *)out);
            } else {
                fprintf(stderr, "uncompress error: %d\n", zret);
            }
        } else {
            /* Текстовое сообщение */
            char *text = buf + strlen(KEY_TEXT);
            printf("[%d] Сообщение (ключ %s):\n%s\n\n",
                   msg_count, KEY_TEXT, text);
        }

        fflush(stdout);

        /* Получаем 2 сообщения (текст + сжатый) и выходим */
        if (msg_count >= 2) break;
    }

    zmq_close(socketzmq);
    zmq_ctx_destroy(context);
    printf("Получено сообщений: %d\n", msg_count);
    return 0;
}
