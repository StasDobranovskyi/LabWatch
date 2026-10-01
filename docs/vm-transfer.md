# Передача в Ubuntu 24.04

Основна копія знаходиться на Mac у `Lab01/Work/LabWatch`. Порядок відповідає `Study/VM-transfer.md`: спочатку перегляд, без `--delete`, результат назад через окремий inbox, перед заміною порівняння. Наведені команди для VM ще не виконано.

Підготовлений архів лежить поряд із репозиторієм у Work: `LabWatch-vm-2026-10-03.tar.gz`, поруч `.sha256`. У ньому немає `.git`, `firmware/build` або сценарію відео. Це копія джерел, її не потрібно ініціалізувати як інший репозиторій. Git для захисту показуйте з основної копії на Mac.

## Нова копія через архів

Запустіть VM до демонстрації, перевірте, що це Ubuntu 24.04 та Docker працює без sudo. Передайте архів і файл SHA256 у `~/Downloads` зручним доступним каналом. Адреса VM, користувач і канал передачі ще невідомі. У локальному VirtualBox 03.10 список VM був порожній, тому `ubuntu-vm` не вважаємо перевіреним хостом.

```sh
cd ~/Downloads
sha256sum -c LabWatch-vm-2026-10-03.tar.gz.sha256
tar -tzf LabWatch-vm-2026-10-03.tar.gz
test ! -e ~/study/2026-autumn/SP/Lab01/Dobranovskyi/LabWatch
```

Якщо остання команда повертає ненульовий код, зупиніться та порівняйте наявні файли. Для нової порожньої копії:

```sh
mkdir -p ~/study/2026-autumn/SP/Lab01/Dobranovskyi
tar -xzf ~/Downloads/LabWatch-vm-2026-10-03.tar.gz -C ~/study/2026-autumn/SP/Lab01/Dobranovskyi
cd ~/study/2026-autumn/SP/Lab01/Dobranovskyi/LabWatch
cat /etc/os-release
uname -m
docker version
docker info
mkdir -p ~/labwatch-evidence
set -o pipefail
make clean
make build 2>&1 | tee ~/labwatch-evidence/ubuntu-build-1.log
sha256sum firmware/build/firmware.elf firmware/build/firmware.hex firmware/build/firmware.bin > ~/labwatch-evidence/ubuntu-build-1.sha256
make clean
make build 2>&1 | tee ~/labwatch-evidence/ubuntu-build-2.log
sha256sum firmware/build/firmware.elf firmware/build/firmware.hex firmware/build/firmware.bin > ~/labwatch-evidence/ubuntu-build-2.sha256
diff -u ~/labwatch-evidence/ubuntu-build-1.sha256 ~/labwatch-evidence/ubuntu-build-2.sha256
```

Не продовжуйте після помилки збірки. Збережіть також фактичні ОС, platform, image ID та `arm-none-eabi-gcc --version` з контейнера. Однакові hashes на одному host не гарантують ідентичності на іншій архітектурі або після зміни пакунків у Docker-образі.

VS Code/Wokwi можна запускати у VM або на хості, де є розширення й доступна ліцензія. Відкрийте саме відповідну копію й переконайтеся, що `wokwi.toml` бачить її ELF/HEX. Якщо запускаєте Wokwi на іншому комп'ютері, спочатку передайте свіжі ELF/HEX із VM і перевірте hashes. Наявність збірки на Mac не замінює виконання цих дій.

## Оновлення та повернення результатів

Якщо копія у VM вже існує, `rsync -avn` показує заплановані зміни. Після перевірки списку використайте `-av`; не додавайте `--delete`. Вкажіть власні перевірені SSH-адресу й шляхи, не вважайте приклад `ubuntu-vm` налаштованим.

З VM копіюйте логи та скриншоти в окрему папку `Lab01/Work/VM-Inbox`, спочатку `rsync -avn`, потім `-av`. Порівняйте з основною копією. Зміни прошивки з VM не переносіть поверх основних файлів без diff. Не редагуйте одну роботу паралельно на Mac та у VM.
