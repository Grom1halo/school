#!/bin/bash
# Лаба 4, Вариант 1 — Мониторинг памяти через MQTT
# Использование: ./lab4_monitor.sh [MQTT_HOST] [MQTT_PORT] [THRESHOLD_MB]

MQTT_HOST="${1:-localhost}"
MQTT_PORT="${2:-1883}"
THRESHOLD="${3:-100}"   # Порог свободной памяти в МБ
TOPIC_BASE="monitoring/$(hostname)/memory"
INTERVAL=10             # Интервал публикации (секунды)

echo "=== MQTT Memory Monitor ==="
echo "Брокер:   $MQTT_HOST:$MQTT_PORT"
echo "Топик:    $TOPIC_BASE"
echo "Порог:    ${THRESHOLD} MB"
echo "Интервал: ${INTERVAL}s"
echo "Ctrl+C для остановки"
echo ""

pub() {
    mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "$1" -m "$2" 2>/dev/null
}

while true; do
    # Получаем данные vmstat (пропускаем 2 строки заголовка)
    VMSTAT=$(vmstat -SM 1 1 | tail -1)

    # Парсим поля vmstat: r b swpd free buff cache si so bi bo in cs us sy id wa st
    FREE_MB=$(echo "$VMSTAT" | awk '{print $4}')
    USED_SWAP=$(echo "$VMSTAT" | awk '{print $3}')
    CPU_US=$(echo "$VMSTAT" | awk '{print $13}')
    CPU_SY=$(echo "$VMSTAT" | awk '{print $14}')
    CPU_ID=$(echo "$VMSTAT" | awk '{print $15}')

    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

    # Публикуем данные по топикам
    pub "${TOPIC_BASE}/free_mb"    "$FREE_MB"
    pub "${TOPIC_BASE}/swap_mb"    "$USED_SWAP"
    pub "${TOPIC_BASE}/cpu_user"   "$CPU_US"
    pub "${TOPIC_BASE}/cpu_system" "$CPU_SY"
    pub "${TOPIC_BASE}/cpu_idle"   "$CPU_ID"
    pub "${TOPIC_BASE}/timestamp"  "$TIMESTAMP"
    pub "${TOPIC_BASE}/vmstat_raw" "$VMSTAT"

    # Проверяем порог памяти
    if [ "$FREE_MB" -lt "$THRESHOLD" ] 2>/dev/null; then
        WARN="ВНИМАНИЕ! Мало памяти: ${FREE_MB}MB < ${THRESHOLD}MB ($(date '+%H:%M:%S'))"
        pub "${TOPIC_BASE}/warning" "$WARN"
        echo "[WARNING] $WARN"
    else
        pub "${TOPIC_BASE}/warning" "OK: ${FREE_MB}MB свободно"
    fi

    echo "[$TIMESTAMP] Free: ${FREE_MB}MB | Swap: ${USED_SWAP}MB | CPU: us=${CPU_US}% sy=${CPU_SY}% id=${CPU_ID}%"

    sleep "$INTERVAL"
done
