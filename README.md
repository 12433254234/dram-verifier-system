# Устройство тестирования модулей памяти MU-475 (УЧПУ Kongsberg NC2000)

Устройства для тестирования модулей оперативной памяти `MU-475`, применяемых в устройстве числового программного управления (УЧПУ) `Kongsberg NC2000` токарно-карусельного станка `TDP 70/110`.

## Назначение

Модуль памяти `MU-475` выполнен на динамических микросхемах `2107В` (4K×1 бит, 3 напряжения питания: +12В / +5В / -5В). Устройство позволяет:

- Записывать тестовые данные во все страницы памяти
- Считывать данные и сравнивать с эталоном
- Проверять циклы регенерации
- Выявлять неисправные ячейки или микросхемы

## Аппаратная платформа

| Компонент               | Выбор                                      |
|------------------------|---------------------------------------------|
| ПЛИС                   | Altera Cyclone IV `EP4CE6E22C8N`          |
| Преобразователь уровней| `TXS0108EQPWRQ1` (5В ↔ 3.3В, 8 каналов)   |
| USB → UART             | `CH340G`                                   |
| Тактовая частота       | 50 МГц                                     |
| Интерфейс с ПК         | UART (38400 бод, 8N1)                      |
| Питание                | +12 В → стабилизаторы на 5В и 3.3В         |

## Реализованные режимы работы

- **Запись** – приём 3 байтов (17 бит) от ПК и запись в выбранную ячейку
- **Чтение** – чтение 17 бит из памяти и отправка 3 байтов в ПК
- **Автоматический перебор страниц** (A/B/C/D)
- **Пауза 10 мс** между записью и чтением для стабилизации
- **Регенерация** (сигнал `REFRESH`) по внешнему требованию

## Программное обеспечение (ПК)

Консольное приложение на C++:
- Открытие COM-порта с параметрами `38400-8-N-1`
- **w** – запись данных из файла `w_data_mix.txt` в память
- **r** – чтение данных из памяти и сохранение в `r_data.txt`
- Поэтапное тестирование всех ячеек и страниц

## Печатная плата

Разработана двухстороняя печатная плата, а так же жгут и плата под жгут(на данный момент не добаленны в проект!)
![](docs/Printed circuit board and wiring harness.png)

# MU-475 Memory Module Tester for Kongsberg NC2000 CNC

A hardware and software tool for testing `MU-475` dynamic RAM modules used in the `Kongsberg NC2000` CNC controller of the `TDP 70/110` lathe.

## Purpose

The `MU-475` memory module uses `2107В` DRAM chips (4K×1 bit, +12V / +5V / -5V). This tester can:

- Write test patterns to all memory pages
- Read back data and compare with expected values
- Verify refresh cycles
- Detect faulty cells or chips

## Hardware Platform

| Component               | Selection                                |
|------------------------|------------------------------------------|
| FPGA                   | Altera Cyclone IV `EP4CE6E22C8N`        |
| Level shifter          | `TXS0108EQPWRQ1` (5V ↔ 3.3V, 8‑bit)    |
| USB → UART             | `CH340G`                                |
| Clock frequency        | 50 MHz                                  |
| PC interface           | UART (38400 baud, 8N1)                 |
| Power input            | +12V → onboard 5V and 3.3V regulators  |

## Operating Modes

- **Write** – receives 3 bytes (17 bits) from PC and writes to a selected memory cell
- **Read** – reads 17 bits from memory and sends 3 bytes to PC
- **Automatic page walk** (A / B / C / D)
- **10 ms pause** between write and read for signal stabilization
- **Refresh** (REFRESH signal) on external request

## PC Software (C++ Console App)

- Opens COM port with `38400-8-N-1` settings
- **w** – write data from `w_data_mix.txt` into memory
- **r** – read data from memory and save to `r_data.txt`
- Sequential test of all cells and pages

## Printed circuit board

A two-sided printed circuit board has been developed, as well as a harness and a harness board (currently not included in the project!)