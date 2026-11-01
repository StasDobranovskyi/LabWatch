# LabWatch

Прототип контролю лабораторного приладу: MPU6050, PIR і аналоговий LDR, режими очікування, роботи та обслуговування. PIR створює інформаційну подію й не визначає особу або намір. [Вимоги продукту](docs/PRD.md) · [Архітектура](docs/architecture.md).

ЛР3 використовує один bare-metal super loop без RTOS на STM32C031C6. Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Роздільність часу прошивки 1 мс (HAL SysTick); рядок BOOT із TIM3 1 MHz застарілий. Виміряна збірка має source fingerprint `d7c3f569f1d1dfb638999a4578bc940a164f577837d2776d9796671d594cf9fa` та ELF SHA-256 `fbb10f8e971e68c7f920f759c0a21c769ec9440fff2cd9f60147864f338789f2`.

Проведено S1, S2 та S3 у Wokwi. S2 тривав 3620,999 с за шкалою прошивки, але помилка IMU анулювала робочий еталон. S3 зафіксував 55 цифрових пар фронт–реакція у вибраних вікнах видачі журналу і фактичні storage fault та втрати черги. Фізична плата, power cut і частина граничних перевірок не підтверджені. [Задачі](docs/tasks.md) · [Метрики](docs/metrics/README.md) · [Обмеження](docs/limitations.md) · [База ЛР4](docs/baseline-lab4.md).

```sh
make build
make test
make sanitize
make test-linux
make coverage
```

UART 115200 8N1: `status`, `selftest`, `calibrate`, `mode idle|work|service`, `ack`, `imuaddr 0x68|0x69`, `version`, `stats`, `storage`, `journal dump`, `mark`. Wokwi використовує RFC2217 localhost:4537; новий інтерактивний запуск потребує окремого особистого GUI-допуску. Не активувати trial.

[Історичний звіт ЛР2](docs/lab2/report.pdf) · [Захвати ЛР2](docs/captures/README.md). Старі результати не є вимірами поточної прошивки.
