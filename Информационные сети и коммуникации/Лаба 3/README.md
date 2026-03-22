# Лаба 3 — Гайд для одногруппников

## Что нужно поменять под себя

1. **ФИО и дата** — в отчёте замени `Кровин Н.И.` на своё ФИО
2. **IP и MAC** — у тебя будут другие, файлы `.rep` нужно перегенерировать
3. **Вариант** — у лабы 3 нет вариантов, задание одинаковое для всех

---

## Установка пакетов (если не делал для лабы 2)

```bash
sudo apt update && sudo apt install -y net-tools iproute2 iputils-ping gnupg
```

---

## Генерация файлов отчёта

Скачай `run_lab3.sh` (из репозитория или получи у Никиты), скопируй в WSL:
```bash
cp /mnt/c/Users/ТВО_ЮЗЕР/Downloads/run_lab3.sh ~/run_lab3.sh
sed -i 's/\r//' ~/run_lab3.sh
chmod +x ~/run_lab3.sh
~/run_lab3.sh
```

Скрипт создаст все файлы с твоими реальными IP и MAC адресами.

---

## Подпись отчёта GPG

```bash
# Напиши свой отчёт (поменяй ФИО и IP)
nano ~/lab3report.txt

# Подпиши
gpg --clearsign ~/lab3report.txt
```

---

## Файлы для сдачи

| Файл | Описание |
|------|----------|
| `lab3report.txt.asc` | Отчёт с цифровой подписью |
| `config.rep` | Вывод ifconfig и ip addr |
| `ipv6_ip.rep` | Назначение IPv6 через ip |
| `ipv6_ifconfig.rep` | Назначение IPv6 через ifconfig |
| `alt_forms.rep` | Альтернативные формы адресов |
| `ipv6_forms.rep` | IPv4 в формате IPv6 |
| `ping6.rep` | Результаты ping6 |
| `ping6_linklocal.rep` | Ping6 link-local адреса |
| `sysinfo.sh` | Скрипт сбора информации (запускать на паре) |
