/*
 * Тестовый ZMQ-издатель (имитация сервера преподавателя)
 * Рассылает сонет Шекспира (перевод) в текстовом и сжатом виде
 *
 * Компиляция: gcc ./lab7_publisher.c -o ./lab7_publisher.bin -lz -lzmq
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <zmq.h>
#include <zlib.h>

#define PORT    5563
#define BUFSIZE 8192

/* Перевод Сонета №1 Шекспира */
const char *SONNET_RU =
    "Мы все хотим, чтоб не угасли\n"
    "Созданья редкой красоты,\n"
    "И пусть не вянут жизни краски\n"
    "И вечно зеленеют цветы.\n"
    "\n"
    "Но ты, плененный образом своим,\n"
    "Питаешь этим пламенем себя,\n"
    "И, расточая свой запас родным,\n"
    "С врагом живешь в раздоре, сам губя.\n"
    "\n"
    "Ты — украшенье мира, вестник мая,\n"
    "Закладчик красоты твоей в ростовщика;\n"
    "Расточишь сам свое богатство, зарывая\n"
    "Наследство для потомков и века.\n"
    "\n"
    "Будь к миру щедрым! Иль к могиле путь\n"
    "Унесет с собой твою суть.\n";

int main(void) {
    printf("=== ZMQ Publisher (тест сервер) ===\n");
    printf("Порт: %d\n\n", PORT);

    void *context = zmq_ctx_new();
    assert(context);

    void *publisher = zmq_socket(context, ZMQ_PUB);
    assert(publisher);

    char endpoint[64];
    snprintf(endpoint, sizeof(endpoint), "tcp://*:%d", PORT);
    int rc = zmq_bind(publisher, endpoint);
    assert(rc == 0);

    printf("Издатель запущен. Ждём подписчиков 2 секунды...\n");
    sleep(2); /* Даём время подписчику подключиться */

    /* 1. Отправляем текстовое сообщение с ключом "RU" */
    char msg[BUFSIZE];
    int msg_len = snprintf(msg, sizeof(msg), "RU%s", SONNET_RU);
    zmq_send(publisher, msg, msg_len, 0);
    printf("Отправлено текстовое сообщение (RU), %d байт\n", msg_len);
    sleep(1);

    /* 2. Сжимаем и отправляем с ключом "ZU" */
    unsigned char compressed[BUFSIZE];
    uLongf comp_len = BUFSIZE;
    int zret = compress(compressed, &comp_len,
                        (const Bytef *)SONNET_RU, strlen(SONNET_RU));
    if (zret == Z_OK) {
        char out[BUFSIZE];
        memcpy(out, "ZU", 2);
        memcpy(out + 2, compressed, comp_len);
        zmq_send(publisher, out, 2 + comp_len, 0);
        printf("Отправлено сжатое сообщение (ZU), оригинал: %zu байт, сжато: %lu байт\n",
               strlen(SONNET_RU), comp_len);
    }

    sleep(1);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    printf("Готово.\n");
    return 0;
}
