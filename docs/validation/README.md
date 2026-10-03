# Фактична перевірка 03.10.2026

Підготовку і збірки виконано 03.10.2026. Git author/committer dates встановлено за погодженим графіком; часові мітки цих логів відповідають фактичному виконанню.

## Підтверджено

Host: macOS 27.0.1 ARM64. Docker client darwin/arm64, Docker Desktop server linux/arm64, kernel `6.12.76-linuxkit`. Компілятор у контейнері `arm-none-eabi-gcc 10.3.1 20210621`. Target прошивки Cortex-M0+, STM32C031C6. Це Docker-збірка, виконана з Mac, а не робота у студентській Ubuntu VM.

- `main.c`, `stm32c0xx_hal_msp.c`, `syscalls.c`, `.ioc` перевірено: USART2 PA2/PA3, 115200 8N1, `printf` через HAL; LED PA5; затримка 1000 мс.
- Файли прошивки, `.ioc`, `diagram.json`, `wokwi.toml` і Dockerfile звірено з SP-Boilerplate без змін. README шаблону збережено в `docs/template-readme.md`.
- Обидва запуски `make clean` / `make build` завершилися з кодом 0. Build перед кожною компіляцією видалено.
- ELF/HEX/BIN непорожні й побайтно однакові між двома збірками. Linker size: text 13464, data 120, bss 1808 байтів. `.elf` містить debug-інформацію, його файловий розмір не дорівнює обсягу Flash.

| Артефакт | SHA256 обох збірок |
|---|---|
| firmware.elf | `0f7b87f63097adfc290c57ea018a65e5a05da061aaa48fd520c14fbfe05f85da` |
| firmware.hex | `20c68952fd3c7ab6640f8ca53935729d6ca2c54d55dbba98597a92c2e0248415` |
| firmware.bin | `c21e312881deefbec24a06e9808d3651367530ac8ea930d3b37cc011daa7f7df` |

Початок першого збереженого запуску 16:53:13 +03:00, другого 16:53:19 +03:00. Перед ними також виконано успішну попередню збірку; її окремий повний лог у репозиторії не збережений. Наведена пара має повні [build-1.log](build-1.log) і [build-2.log](build-2.log), [hashes 1](build-1.sha256), [hashes 2](build-2.sha256), [машинний результат](result.json). `docker build` використовував кеш образу, компіляція прошивки була чистою. Пакунки Dockerfile не закріплені за версіями, тому результат не доводить ідентичності на іншому host або після оновлення образу.

Метадані: [host](host.txt), [Docker version](docker-version.txt), [server platform](docker-server.txt), [image ID](image.txt), [toolchain](toolchain.txt).

## Не підтверджено

Ubuntu 24.04 VM не запущена. На Mac знайдено VirtualBox, але `VBoxManage list vms` і `list runningvms` повернули порожні списки. Це не виключає VM на іншому комп'ютері.

На Mac є `/Applications/Visual Studio Code.app` і розширення `wokwi.wokwi-vscode`. Спроба відкрити LabWatch не дала стабільного окремого вікна для керування: активне вікно змінювалось між іншими проєктами. Перевірку припинено, щоб не впливати на чужі вікна. Наявність доступної ліцензії повідомив користувач, її застосування в цьому проєкті не перевірене. Trial не активовано.

Немає підтвердженого Wokwi UART-виводу або спостереження LED, CubeMX-скриншотів, датчиків, вимірювань затримки, PDF-звіту чи відео. Код і компіляція не замінюють ці докази. Тестові профілі PRD залишаються очікуваними результатами.
