# bomzhur

Демо-репозиторий к статье на Хабре: <!-- TODO: ссылка на статью -->

Узел шины на CH32V003: JSON-RPC, настройки во флеше, обновление по шине (A/B)
и резидентный загрузчик в 16 КБ flash и 2 КБ RAM.

## Клонирование

Зависимости ([Zhele](https://github.com/azhel12/Zhele) и
[wch_cmake](https://github.com/azhel12/wch_cmake)) подключены сабмодулями:

```sh
git clone --recursive https://github.com/azhel12/bomzhur.git
```

## Сборка

Нужен вендорный тулчейн `riscv32-wch-elf`: обработчики
прерываний помечены атрибутом `interrupt("WCH-Interrupt-fast")`, который обычный
GCC не понимает.

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

В `build/` появятся:

- `bomzhur_boot_app0_app1.bin` — полный образ (загрузчик + оба слота) для первой прошивки программатором;
- `bomzhur_app0_app1.bin` — файл обновления по шине.
