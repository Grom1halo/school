# Лаба 2 — Гайд для одногруппников

## Что нужно поменять под себя

1. **ФИО и дата** — открой `lab2report.txt.asc` (или создай свой `lab2report.txt`) и замени `Кровин Н.И.` и дату на свои
2. **Вариант** — если у тебя не вариант 1, скрипт `script.sh` нужно переделать (напиши в чат)
3. **Файлы `.rep`** — у тебя будет другой IP, поэтому их нужно перегенерировать (см. ниже)

---

## Установка WSL и пакетов

Открой PowerShell и выполни:
```powershell
wsl --install -d Ubuntu
```
После установки открой Ubuntu и выполни:
```bash
sudo apt update && sudo apt install -y net-tools iproute2 tcpdump traceroute iputils-ping gcc netcat-openbsd libzmq3-dev gnupg
```

---

## Генерация файлов отчёта

Скачай `run_lab2.sh` из этой папки, скопируй в WSL и запусти:
```bash
cp /mnt/c/Users/ТВО_ЮЗЕР/Downloads/run_lab2.sh ~/run_lab2.sh
sed -i 's/\r//' ~/run_lab2.sh
chmod +x ~/run_lab2.sh
~/run_lab2.sh
```
Скрипт сам создаст все файлы: `dump.rep`, `gateping.rep`, `arp.rep`, `trace.dmp`, `logfile.rep`

---

## Подпись отчёта GPG

```bash
# Импортируй свой приватный ключ
gpg --import путь/к/privkey.sec

# Подпиши отчёт (замени fingerprint на свой)
gpg -u ТВО_FINGERPRINT --clearsign ~/lab2report.txt
```
Fingerprint своего ключа можно узнать командой:
```bash
gpg --list-secret-keys --keyid-format LONG
```

---

## Файлы для сдачи
| Файл | Описание |
|------|----------|
| `lab2report.txt.asc` | Отчёт с цифровой подписью |
| `script.sh` | Shell-скрипт (вариант 1) |
| `dump.rep` | Дамп трафика tcpdump |
| `gateping.rep` | Ping шлюза |
| `arp.rep` | Таблица ARP |
| `trace.dmp` | Трассировка до 8.8.8.8 |
| `logfile.rep` | Лог мониторинга 192.168.1.1 |
