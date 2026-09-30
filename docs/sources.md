# Джерела та рівень перевірки

Перевірка 03.10.2026. Відсутність доступу до PDF прямо позначено, сторінки не відновлювалися з пам'яті.

| Джерело | Для чого використано | Стан |
|---|---|---|
| `Study/Materials/SP/LR1.pdf`, с. 2–9 | Форма B, Docker/HAL, UART, компоненти, PRD, Git, PDF-звіт | Прочитано; с. 3 і 6 перевірено в рендері. |
| `Study/Materials/SP/TOPICS.md`, LabWatch, уточнення викладача | Межі ЛР1/ЛР2 і події приладу | Прочитано. Це локальний запис повідомлень, наданих користувачем. |
| Повідомлення викладача 23.09.2026 13:14 | ЛР1 закінчується планом, середовищем, UART/LED; сенсори в ЛР2 | Використано за наданим текстом; окремої перевірки в системі курсу немає. |
| Повідомлення 28.09.2026 08:51 | Наступний понеділок, 05.10.2026 | Дата є календарним висновком; час і подальші зміни невідомі. |
| `SP-Boilerplate/README.md` та файли шаблону | Docker, Makefile, HAL, UART/LED, ліцензії | Прочитано, копія файлів звірена; [README збережено](template-readme.md). |
| [ST DS13867 Rev 4](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), с. 1, 32, 34–37, 42 | MCU, піни, AF, електричні умови | Офіційний PDF доступний. |
| [Wokwi Nucleo-C031C6](https://docs.wokwi.com/parts/board-st-nucleo-c031c6) | Симуляційна плата й LED PA5 | Первинна документація моделі. |
| [Wokwi PIR](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor) | Цифровий OUT і Simulate Motion | Первинна документація моделі. |
| [Wokwi MPU6050](https://docs.wokwi.com/parts/wokwi-mpu6050) | I²C, адреси, одиниці симуляції | Первинна документація моделі. |
| [Wokwi LDR](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor) | AO, подільник, напрям зміни рівня | Первинна документація моделі. |
| [Wokwi VS Code](https://docs.wokwi.com/vscode/getting-started) | Відкриття проєкту й запуск | Прочитано; не активуємо trial. |
| [InvenSense register map Rev 4.2](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf), с. 7, 45 | Вихідні регістри та WHO_AM_I | Індексований текст первинного PDF доступний; пряме завантаження не вдалося. |
| [TDK product specification](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf) | Майбутня перевірка живлення MPU6050 | HTTP 403, повний зміст не підтверджено. |

Локальні шляхи наведено для основної копії Study. Вони не входять в архів проєкту. Документація симулятора не замінює manufacturer datasheet фізичного модуля.
