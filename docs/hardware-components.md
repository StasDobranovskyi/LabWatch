# Компоненти LabWatch

Стан на 03.10.2026: у схемі тільки Nucleo-C031C6 і Serial Monitor. Таблиця датчиків є планом ЛР2. Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

## Наявна платформа

| Компонент / інтерфейс | Підтверджено | Джерело |
|---|---|---|
| STM32C031C6 | Cortex-M0+, до 48 МГц, 32 КБ Flash, 12 КБ RAM. HAL C, без Arduino. | [ST DS13867 Rev 4, с. 1](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=1); `firmware/Makefile`, `Core/Src/main.c`. |
| USART2 | PA2 TX, PA3 RX, AF1; 115200, 8 біт, 1 стоп-біт, без парності. | `Core/Src/stm32c0xx_hal_msp.c`, `main.c`; [ST DS13867, с. 32, 35](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=32). |
| LED LD4 | PA5, світиться за високого рівня; шаблон перемикає його після кожного `tick`, далі затримка 1000 мс. | [Wokwi, Onboard LED](https://docs.wokwi.com/parts/board-st-nucleo-c031c6); `Core/Inc/main.h`, `Core/Src/main.c`. |
| I²C1 | PB8 SCL, PB9 SDA, AF6; ініціалізація вже є, підключених датчиків немає. Значення Timing `0x20303E5D`. Частоту шини ще не виміряно. | `firmware.ioc`, `Core/Src/stm32c0xx_hal_msp.c`; [ST DS13867, с. 34, 37](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=34). |

Ланцюжок UART у шаблоні: `printf` → `_write` у `syscalls.c` → `__io_putchar` у `main.c` → `HAL_UART_Transmit`. `diagram.json` з'єднує Serial Monitor RX з PA2, TX з PA3. Симуляційне виконання цього ланцюжка ще не підтверджено.

## План підключення у ЛР2

| Датчик | Сигнали і запропоновані піни | Показ / межа інтерпретації | Джерело |
|---|---|---|---|
| `wokwi-pir-motion-sensor` | VCC, GND, OUT → PA1 GPIO input. PA1 ще не налаштований. | Цифровий рух поруч; модель утримує OUT, тому фронт не дорівнює кількості людей. | [Wokwi PIR, Pin names / Using the sensor](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor). |
| `wokwi-mpu6050` | VCC, GND, SCL → PB8, SDA → PB9; AD0 → GND для 7-бітної адреси 0x68. INT у першій реалізації не потрібен. | Прискорення в g, швидкість у °/с; точні координати не визначаються. AD0 до VCC змінює адресу на 0x69. | [Wokwi MPU6050, Pin names / Units](https://docs.wokwi.com/parts/wokwi-mpu6050). |
| `wokwi-photoresistor-sensor` | VCC, GND, AO → PA0 ADC_IN0. DO не використовується. PA0/ADC ще не налаштовані. | Raw ADC і відносна зміна світла. Світло не підтверджує відкриття кожуха; відкриття в темряві може бути непомітним. | [Wokwi LDR, Pin names / Operation](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor); [ST DS13867, с. 32](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=32). |

Це запропонований розподіл, а не виконаний монтаж. Він залишає PA2/PA3 і PA5 для UART/LED. Перед ЛР2 звірити доступність піна на платі, налаштувати CubeMX та перевірити кожний сигнал. GPIO, I²C і ADC дають три різні інтерфейси.

Для MPU6050 початкова проєктна частота шини 100 кГц, після вимірювання SCL. У HAL аргумент адреси має відповідати контракту API, для стандартної 7-бітної адреси використовується зсув на один біт. Джерело API вже в проєкті: `firmware/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_i2c.c`, опис параметра `DevAddress`. Зсунутий аргумент HAL не називати іншою адресою датчика.

## Ідентифікація та регістри MPU6050

[InvenSense RM-MPU-6000A-00 Rev 4.2](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf#page=45), с. 45, описує `WHO_AM_I` 0x75, початкове значення 0x68, незалежно від AD0. Таблиця на с. 7 описує блок прискорення 0x3B–0x40 і гіроскопа 0x43–0x48. Ці сторінки підтверджено індексованим текстом офіційного документа, повний PDF напряму отримати не вдалося. Перед драйвером потрібно повторно отримати документ і звірити налаштування пробудження, діапазонів та фільтра. Читання регістрів на пристрої ще не виконувалось.

Для PIR і LDR register map / WHO_AM_I не застосовуються: вони передають цифровий або аналоговий сигнал, а не регістри.

## Живлення і незакриті питання

Для майбутньої схеми приймаємо рівень 3.3 В, спільний GND, AO у межах 0…VREF+ та I²C pull-up до 3.3 В. Це план, а не перевірка сумісності фізичних модулів. STM32 має 12-бітний ADC; у ЛР2 планується код 0…4095. Електричні умови потрібно звірити з [ST DS13867, с. 42 і розділом ADC](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf#page=42). Не переносити 5-вольтові таблиці Arduino або його 10-бітну шкалу на STM32.

| Компонент | Стан офіційного datasheet / живлення | Що потрібно завершити |
|---|---|---|
| MPU-6050 / breakout | [Product specification TDK](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf) повернув HTTP 403. Старий [PDF InvenSense](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet.pdf) повернув 404. Напругу, струм і сторінки специфікації модуля не видаємо за перевірені. | Отримати повний PDF; указати сторінки pinout, напруг, активного струму, швидкості I²C; відділити чіп від живлення конкретного breakout. |
| PIR | Wokwi описує модель, не конкретний вироблений модуль. Струм і допустима напруга невідомі. | Обрати фізичну модель і знайти manufacturer PDF; перевірити pinout, час прогріву, вихідний рівень та струм. Не приписувати параметри випадкового HC-SR501 усім PIR. |
| LDR | Wokwi описує подільник із резистором 10 кОм. Фізичний виробник і модель LDR невідомі. | Обрати виробника; знайти PDF зі сторінками опору/світла, потужності й геометрії; визначити струм подільника та максимальний AO. Позначення GM5528 з методички ще не визначає виробника. |

Без цих даних неможливо обґрунтувати сумарний струм від 3V3 Nucleo або потребу в зовнішньому живленні. Симуляція не вимірює електричне споживання реального виробу. Вимога PDF ЛР1 щодо повних manufacturer datasheet, струмів і конкретних сторінок для кожного датчика залишається частково відкритою.
