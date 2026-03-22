#!/bin/bash

LOGFILE="$HOME/logfile.rep"

# 1. Проверка наличия своей запущенной копии
SCRIPT_NAME=$(basename "$0")
RUNNING=$(ps aux | grep "$SCRIPT_NAME" | grep -v grep | grep -v $$ | wc -l)
if [ "$RUNNING" -gt 0 ]; then
    echo "[WARN] Скрипт уже запущен. Завершение."
    exit 1
fi
echo "[OK] Другой копии скрипта не обнаружено."

# 2. Проверка наличия и количества сетевых интерфейсов
IFACE_COUNT=$(ip link show | grep -c '^[0-9]')
if [ "$IFACE_COUNT" -lt 1 ]; then
    echo "[ERROR] Сетевые интерфейсы не найдены. Завершение."
    exit 1
fi
echo "[OK] Найдено сетевых интерфейсов: $IFACE_COUNT"

# 3. Проверка активности сетевого интерфейса eth0
ETH_UP=$(ip link show eth0 2>/dev/null | grep -c 'UP')
if [ "$ETH_UP" -eq 0 ]; then
    echo "[ERROR] Интерфейс eth0 не активен. Завершение."
    exit 1
fi
echo "[OK] Интерфейс eth0 активен."

# 4. Проверка доступа в локальную сеть
GW=$(ip route | grep default | awk '{print $3}' | head -1)
ping -c 2 -W 2 "$GW" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "[OK] Локальная сеть доступна (шлюз $GW отвечает)."
else
    echo "[WARN] Локальная сеть недоступна."
fi

# 5. Проверка доступа в интернет по протоколу http
nc -z -w 3 8.8.8.8 80 > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "[OK] Интернет по протоколу http доступен."
else
    echo "[WARN] Интернет по протоколу http недоступен."
fi

# 6. Цикл: раз в минуту ping 192.168.1.1
echo "[INFO] Запуск цикла мониторинга 192.168.1.1..."
while true; do
    ping -c 4 -W 2 192.168.1.1 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        MSG="[$(date '+%Y-%m-%d %H:%M:%S')] 192.168.1.1 недоступен. Меняю шлюз на 192.168.1.2."
        echo "$MSG"
        echo "$MSG" >> "$LOGFILE"
        sudo ip route replace default via 192.168.1.2
        echo "[INFO] Шлюз изменён. Завершение работы."
        exit 0
    else
        echo "[$(date '+%H:%M:%S')] 192.168.1.1 доступен."
    fi
    sleep 60
done
