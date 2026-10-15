# Звіт ЛР2 LabWatch

Добрановський Станіслав Вадимович, ІО-36.

Повний звіт: [PDF](lab2/report.pdf).

В одному стенді перевірено MPU6050/I2C, PIR/GPIO та LDR/ADC. WHO_AM_I=0x68, спокійний вектор1g; після створеного address NACK0x69 повернення0x68 відновлює обмін. Канали мають окрему валідність і події, прошивка не зупиняє ADC/PIR через IMU fault.

Виміряно SCL 100 кГц, період IMU 20.008575 мс, період ADC 49.992450 мс, ADC wrapper65.225мкс. Остання величина включає програмні накладні витрати й не є часом тільки conversion. [Сигнали й ручний аналіз](captures/README.md), [числа](lab2/measurements.json).

Core/Platform/Devices/App розділено; vendor ADC checksums перевіряються автоматично. П’ять host suites та captured vectors дають 96.96% lines і 79.08% branches для переносних App+Devices. [Результати](lab2/validation/result.json), [CI](https://github.com/StasDobranovskyi/LabWatch/actions/runs/38726348661).

Це цифрова перевірка формиB Wokwi. Фізична точність, живлення, струмові піки, фронти й промислова надійність потребують окремого стенда. Звіт містить pinout, power budget, component diagram, аналіз інтерфейсів, журнал шести проблем із commit links та відповідальність за напрями.
