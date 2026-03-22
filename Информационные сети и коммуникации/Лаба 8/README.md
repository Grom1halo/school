# Лаба 8 — Гайд для одногруппников

## Тема: UDP аудио + эксперименты со сжатием и потерями

## Варианты

| Вариант | Задание |
|---------|---------|
| **1** | UDP пакеты, метка RAW ← этот файл |
| 2 | ZMQ подписка, метка RAW |
| 3 | UDP пакеты, метка Z08 (LZW) |
| 4 | ZMQ подписка, метка Z08 |
| 5 | UDP пакеты, метка Z16 |
| 6 | ZMQ подписка, метка Z16 |
| 7 | ZMQ подписка Z08 + лог потерь |

---

## Что нужно поменять под себя

1. **ФИО и дата** — в отчёте
2. **Порт** — уточни у преподавателя

---

## Установка пакетов

```bash
sudo apt install -y gcc speex sox iproute2
```

---

## Запуск всех экспериментов

```bash
cp /mnt/c/Users/ТВО_ЮЗЕР/Downloads/run_lab8.sh ~/run_lab8.sh
sed -i 's/\r//' ~/run_lab8.sh
chmod +x ~/run_lab8.sh
~/run_lab8.sh
```

Скрипт автоматически:
- Создаст тестовый RAW аудиофайл
- Сожмёт SPEEX и gzip, сравнит коэффициенты
- Проведёт эксперимент с 10% потерей пакетов (netem)
- Проведёт эксперимент с задержкой 100ms

---

## Эксперименты вручную

```bash
# Потеря 10% пакетов
sudo tc qdisc add dev lo root netem loss 10%
# ... запусти тест ...
sudo tc qdisc del dev lo root

# Задержка 100ms
sudo tc qdisc add dev lo root netem delay 100ms
# ... запусти тест ...
sudo tc qdisc del dev lo root
```

---

## На паре

```bash
./lab8_receiver.bin <порт_преподавателя> audio_out.raw
aplay -r 8000 -f U8 -c 1 audio_out.raw
```

---

## Файлы для сдачи

| Файл | Описание |
|------|----------|
| `lab8report.txt.asc` | Отчёт с цифровой подписью |
| `lab8_receiver.c` | Исходный код приёмника |
| `loss_experiment.rep` | Эксперимент с потерями |
| `delay_experiment.rep` | Эксперимент с задержкой |
