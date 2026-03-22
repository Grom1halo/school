# Лаба 4 — Гайд для одногруппников

## Тема: MQTT / IoT мониторинг

Вариантов нет — задание одинаковое, но у вариант 1 мониторинг памяти (vmstat).

## Что нужно поменять под себя

1. **ФИО и дата** — в отчёте
2. **Hostname** — в топиках MQTT автоматически подставится твой (через `hostname`)
3. **Порог памяти** — можно поменять в скрипте (по умолчанию 200 MB)

---

## Установка пакетов

```bash
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl start mosquitto
```

---

## Запуск демонстрации

```bash
cp /mnt/c/Users/ТВО_ЮЗЕР/Downloads/run_lab4.sh ~/run_lab4.sh
sed -i 's/\r//' ~/run_lab4.sh
chmod +x ~/run_lab4.sh
~/run_lab4.sh
```

---

## На паре (с сервером преподавателя)

```bash
# Мониторинг памяти → сервер преподавателя
~/lab4/lab4_monitor.sh <IP_преподавателя> 1883 100

# Подписаться и смотреть данные
mosquitto_sub -h <IP_преподавателя> -t "monitoring/#" -v
```

Мобильное приложение для виджета: **IOT_MQTT_PANEL** (Google Play)
Настройка: подключиться к брокеру преподавателя, добавить виджет с топиком `monitoring/<hostname>/memory/free_mb`

---

## Файлы для сдачи

| Файл | Описание |
|------|----------|
| `lab4report.txt.asc` | Отчёт с цифровой подписью |
| `lab4_monitor.sh` | Скрипт мониторинга памяти |
| `vmstat.rep` | Вывод vmstat |
| `sub_test.rep` | Тест pub/sub |
| `mqtt_output.rep` | Данные MQTT от монитора |
