# Задачі LabWatch, ЛР3

> **Актуальний стан d7, 01.11.2026:** годинний S2 дав IMU error/recovery та втратив робочий еталон; S3 має 55 цифрових пар у вікнах виводу журналу та фактичні storage fault/queue loss. Час прошивки — SysTick 1 мс, не TIM3 1 МГц. Актуальні умови й метрики: `docs/baseline-lab4.md`; старі значення нижче історичні. Фізичні випробування не заявляються.


STM32C031C6, 48 MHz, bare metal, один MSP, без RTOS. Фазові releases на HAL SysTick з кроком 1 мс; пропущені releases відкидаються та рахуються. Дедлайн перевіряється від найстарішого release цього виклику до кінця обробки. Це лічильник епізодів порушення, а skipped окремо рахує невиконані releases.

| Задача | FR | Період / подія | Дедлайн | Робота та межа |
|---|---|---|---|---|
| RX | 04, 14 | 1 ms; до 64 байтів і 1 команди | 100 ms від завершення рядка | ISR ring 256; invalid/overflow відкидаються до CR/LF |
| PIR | 05, 12 | 1 ms; EXTI rising/falling | 100 ms від OUT | 7 queued edges, timestamp ISR; PA10 marker початку dispatch |
| IMU retry | 06, 13 | 1 s, поки invalid | 1 s, крім суми таймаутів ініціалізації | ID + конфігурація 50 Hz + read-back |
| IMU | 06–09, 11–13 | 20 ms | 20 ms; shock record ≤100 ms від спостереженої умови | I2C transaction timeout10 ms; PA6 охоплює read+classification |
| ADC | 10–13 | 50 ms; при fault retry1 s | 50 ms | PA4 HAL wrapper; 5-sample mean, hysteresis500/300 raw, 200 ms confirmation |
| Journal | 14–15 | 20 ms; одна queued record | 100 ms до завершення запису за відсутності backlog | queue16, 4 doublewords; page erase при ротації; PA7 |
| Telemetry | 02, 06, 14, 16 | 1 s | 1 s | SAMPLE + HEALTH, async TX ring2048 |
| LED | 03, 14 | кожен loop | наступний loop | error200 ms, alarmHIGH, normal heartbeat1 s |

FR-15 додає bounded persistent audit trail: останні 64–128 записів із CRC32, mode, event, level, value/detail; режим після reset завжди idle, калібрування не відновлюється. Черга16 і flash не є необмеженим архівом. Pending records можуть зникнути при втраті живлення; latest committed page зберігається при ротації.
FR-16 додає `version`, `storage`, `stats`, `journal dump`, `mark`. Статистика включає час задач, loop body, interval jitter, skipped, deadline episodes, PIR ISR→dispatch, command RX→dispatch, foreground workload fraction, stack painting, heap high water. Жодне число з host fake не підтверджує виконання MCU.

Команди: `mode idle|work|service`, `calibrate`, `status`, `ack`, `selftest`, `imuaddr 0x68|0x69`, `version`, `storage`, `stats`, `journal dump`, `mark`. `mark` є діагностичним checkpoint із event=checkpoint; він навантажує реальний journal path, а не додає затримку. Довга операція у ЛР3 є справжнім HAL_FLASHEx_Erase, без HAL_Delay.

Калібрування500 свіжих IMU samples, не менше9980 ms. Еталон idle відхиляє box-diagonal >50 mg, work >500 mg, gyro >3000 mdps. Work envelope300 mg у власному host тесті не створює shock від нормальних ±150 mg. Shock понад envelope+800 mg. Кут понад10° підтверджується500 ms. Service має окремий transient reference при вході; після виходу обидва робочі еталони невалідні.

Часові виміри d7 використовують `HAL_GetTick()*1000`: 1-мс крок, втрати SysTick під час flash stall не визначаються. Після HAL erase перевіряються всі 512 words сторінки. Factory HEX/BIN заповнений FF до 32 KiB; його повторне завантаження не є фізичним відключенням живлення.
