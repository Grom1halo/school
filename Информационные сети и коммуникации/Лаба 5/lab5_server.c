/*
 * Лабораторная работа №5, Вариант 1
 * TCP-сервер аутентификации
 * Кровин Никита Игоревич
 *
 * Логика:
 *   1. Ждёт подключения клиента
 *   2. Запрашивает Login: и Password:
 *   3. Если admin/123 — отправляет текущее время и закрывает соединение
 *   4. Если неверно — закрывает соединение
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     5555
#define BUFSIZE  256
#define LOGIN    "admin"
#define PASS     "123"

/* Читает строку из сокета до \n, возвращает длину без \r\n */
int read_line(int sock, char *buf, int maxlen) {
    int n = 0;
    char c;
    while (n < maxlen - 1) {
        int r = recv(sock, &c, 1, 0);
        if (r <= 0) break;
        if (c == '\n') break;
        if (c != '\r') buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

int main(void) {
    int srv_fd, cli_fd;
    struct sockaddr_in srv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    char login_buf[BUFSIZE], pass_buf[BUFSIZE], msg[BUFSIZE];
    time_t t;
    struct tm *tm_info;

    /* Создаём TCP сокет */
    srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_fd < 0) { perror("socket"); exit(1); }

    /* SO_REUSEADDR чтобы не ждать после перезапуска */
    int opt = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port        = htons(PORT);

    if (bind(srv_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(srv_fd, 1) < 0) { perror("listen"); exit(1); }

    printf("Сервер запущен на порту %d...\n", PORT);

    while (1) {
        cli_fd = accept(srv_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_fd < 0) { perror("accept"); continue; }
        printf("Подключение: %s\n", inet_ntoa(cli_addr.sin_addr));

        /* Запрашиваем логин */
        send(cli_fd, "Login: ", 7, 0);
        read_line(cli_fd, login_buf, BUFSIZE);
        printf("Login: %s\n", login_buf);

        /* Запрашиваем пароль */
        send(cli_fd, "Password: ", 10, 0);
        read_line(cli_fd, pass_buf, BUFSIZE);
        printf("Password: %s\n", pass_buf);

        /* Проверяем учётные данные */
        if (strcmp(login_buf, LOGIN) == 0 && strcmp(pass_buf, PASS) == 0) {
            t = time(NULL);
            tm_info = localtime(&t);
            strftime(msg, sizeof(msg),
                     "Доступ разрешён. Текущее время: %Y-%m-%d %H:%M:%S\r\n",
                     tm_info);
            send(cli_fd, msg, strlen(msg), 0);
            printf("Аутентификация успешна. Отправлено время.\n");
        } else {
            send(cli_fd, "Доступ запрещён. Неверный логин или пароль.\r\n", 45, 0);
            printf("Аутентификация неудачна.\n");
        }

        close(cli_fd);
        printf("Соединение закрыто.\n\n");
    }

    close(srv_fd);
    return 0;
}
