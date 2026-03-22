#!/bin/bash
# Вариант 1: сбор системной информации и отправка по UDP
# Адрес и порт сервера указываются преподавателем на занятии
SERVER_IP="${1:-192.168.1.100}"
SERVER_PORT="${2:-9999}"

{
    echo "=== vmstat ==="
    vmstat 1 3
    echo "=== free ==="
    free -h
    echo "=== ps (топ 10 по CPU) ==="
    ps aux --sort=-%cpu | head -11
    echo "=== ip addr ==="
    ip addr show eth0
} | nc -u -w 3 "$SERVER_IP" "$SERVER_PORT"

echo "Данные отправлены на $SERVER_IP:$SERVER_PORT"
