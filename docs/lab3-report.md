# ЛР3 LabWatch: проміжний результат

> Історичний стан 28.10.2026 для старішого firmware; поточні результати d7 наведено у `docs/baseline-lab4.md`. Застарілі заяви про відсутність S1–S3 або TIM3 1 MHz нижче не описують d7.

Firmware888ef98d: TIM3 raw16 extension без overflow IRQ, verified flash erase, factory imageFF,8 host suites, ASan/UBSan macOS/Linux PASS, portable coverage100%/82.39%. Code/data27520 B flash, journal4096 B окремо, RAM7784 B із резервами. Дві clean builds ідентичні.

Реально виконано Wokwi pilot d9c97958. Виявлено невалідні TIM3 durations і проблему initial/erase state journal. UART/VCD/screens збережено; незалежний VCD дає6919 IMU marker pulses і2867 ADC pulses, одноразовий UART→FLASH marker1757.3us. Runtime pilot stack1180/2048, heap0 належать лише старій d9 версії. Стан internal flash erase потребує повторного all-FF read-back.

888 firmware ще не запускалася на MCU/Wokwi. Повні S1/S2>=1h/S3>=30events, final runtime memory, independent method comparison і power-cut незавершені. Звіт draft; ЛР3 не готова. Деталі: [tasks](tasks.md), [metrics](metrics/README.md), [captures](captures/lab3/README.md), [limitations](limitations.md), [baseline](baseline-lab4.md).
