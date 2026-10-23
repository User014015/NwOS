# NwC

Полный гайд: `NWCTutorial.md`.

## Минимум

```c
#include <nwc.h>

1A main() {
    nw::out << "Hello!" << nw::endl;
    return 0;
}
```

- `1A` = int, `0A` = void (для `main` и функций без результата).
- `int` и `void` не поддерживаются — компилятор подскажет `1A` / `0A`.
- `#include` принимается, но пока игнорируется.

## Компиляция и запуск

```bash
bash compiler/build.sh
compiler/builded/nwc NwDev/hello.nw -o NwDev/hello.nwo
```

В NwOS:

```text
run hello.nwo
```

Максимум `.nwo` — 16384 байта (файловая система хранит 32 сектора).

## Типы

| Тип | Значение |
|---|---|
| `1A` | целое |
| `lng` | длинное целое |
| `flt` | число (дробная часть отбрасывается) |
| `bol` | `true` / `false` |
| `char` | символ |
| `str` | строка |
| `char name[N]` | строка / буфер |
| `const 1A` | const-модификатор |
| `nonzero 1A` | unsigned-модификатор |
| `needed 1A` | volatile-модификатор |
| `lng lng` | 64-битное целое |

```c
1A x = 42;
str name = "NwC";
char cmd[32];
bol ok = true;
char c = 'A';
```

Переменных максимум 32, имя до 31 символа.

Присваивание работает:

```c
1A x = 1;
x = 2;
x = x + 10;
```

`const`, `nonzero`, `needed` пока только принимаются парсером. `lng lng` работает.

## Вывод

```c
nw::out << "x=" << x << nw::endl;
nw::out << "без новой строки";
```

Работают escape-последовательности:

```c
nw::out << "a\nb\r\t\\\"";
```

## Цветной вывод

```c
nw::out!COLOR << "красный" << nw::endl!COLOR:RED;
nw::out!COLOR << "без endl" << no_endl!COLOR:GREEN;
```

Цвета: `BLACK`, `BLUE`, `GREEN`, `CYAN`, `RED`, `MAGENTA`, `BROWN`,
`LIGHT_GRAY`, `DARK_GRAY`, `LIGHT_BLUE`, `LIGHT_GREEN`, `LIGHT_CYAN`,
`LIGHT_RED`, `LIGHT_MAGENTA`, `YELLOW`, `WHITE`.

После вывода цвет восстанавливается.

## Ввод

```c
char cmd[32];
nw::out << "> ";
nw::cin >> cmd;          // вся строка

1A number;
nw::cin >> number;       // число

char text[64];
nw::line(nw::cin, text); // тоже строка
```

## Операторы

`+ - * / == != < > <= >=`

Операторы выполняются **слева направо без приоритета**:

```c
nw::out << 2 + 3 * 2;    // 10, а не 8
nw::out << 2 + (3 * 2);  // 8
```

Числа: `255`, `0xFF`.

Нет: `%`, `&&`, `||`, `!`, `-5`.

## Условия и циклы

```c
if (x > 10) {
    nw::out << "big" << nw::endl;
} else if (x > 5) {
    nw::out << "medium" << nw::endl;
} else {
    nw::out << "small" << nw::endl;
}
```

```c
while (1) {
    nw::cin >> cmd;

    if (strcmp(cmd, "exit") == 0) {
        break;
    }

    if (strcmp(cmd, "skip") == 0) {
        continue;
    }
}
```

`for` пока нет.

## Строки

```c
if (strcmp(cmd, "help") == 0) {
    nw::out << "help" << nw::endl;
}
```

```c
if (strncmp(cmd, "echo", 4) == 0) {
    nw::out << "это echo" << nw::endl;
}
```

## Функции

```c
1A add(1A a, 1A b) {
    return a + b;
}

0A hello(char s[5]) {
    nw::out << "hi " << s << nw::endl;
}

0A main() {
    call hello("Bob");          // вызов-инструкция
    nw::out << add(2, 3);       // вызов в выражении
    return;
}
```

До 16 параметров. Функции можно объявлять до или после `main`.
Параметры и локальные переменные не конфликтуют между вызовами, рекурсия работает.
`return` возвращает только целое; строки возвращать пока нельзя.

## `__asm__` и клавиши

```c
#include <asm.h>

0A main() {
    while (1) {
        __asm__("hlt");     // пока поддерживается только hlt
    }
}
```

```c
1A key;
key = nw::getkey(ESC);       // ждать ESC
nw::out << "KEY=" << key << nw::endl;
```

Клавиши: `NONE`, `ESC`, `ENTER`, `BACKSPACE`, `TAB`, `UP`, `DOWN`, `LEFT`,
`RIGHT`, `SUPER_S`, `SPACE` и алиасы `KEY_*`.
`nw::getkey()` читает следующую клавишу, `nw::getkey(KEY)` ждёт конкретную.

## Пауза, клавиша, графика и время

```c
sleep(1000);                             // пауза ~1 секунда
1A key = nw::getkey();                   // код клавиши
nw::out << "KEY=" << key << nw::endl;

nw::time();                              // ЧЧ:ММ:СС

nw::rect(10, 20, 100, 50, 0x00FF00);     // x, y, w, h, RGB
nw::rect(0, 0, 640, 400, 0x000080);

nw::draw!FRAME << x=10,x2=200; y=10,y2=120; TITLE="Window"; !COLOR=NAVY;
nw::draw!FRAME << x=8,x2=228; y=425,y2=691; frame::no::window; !COLOR=BLACK;
nw::draw!TEXT << x=50,y=40 !COLOR=WHITE << "Hello";
nw::draw!CUBE << auto::func::connect << COLOR=BLUE;

clear_screen();
```

У `FRAME` границы `x/x2` и `y/y2`; по умолчанию это окно с заголовком
`TITLE` и кнопкой закрытия. `frame::no::window` убирает заголовок.
`TEXT` рисует строку в пиксельных координатах. `clear_screen()` очищает весь экран.
Цвета: `RED`, `DARK_RED`, `GREEN`, `BLUE`, `CYAN`, `WHITE`, `GRAY` и т.д.
Строку можно переносить через `\` в конце строки.

`nw::color(n);` меняет цвет текста по номеру палитры, `nw::clear();` очищает экран.

## Готовый пример

```c
#include <nwc.h>

1A main() {
    char cmd[32];

    while (1) {
        nw::out << "> ";
        nw::cin >> cmd;

        if (strcmp(cmd, "help") == 0) {
            nw::out << "help, time, exit" << nw::endl;
        } else if (strcmp(cmd, "time") == 0) {
            nw::time();
        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            nw::out << "unknown: " << cmd << nw::endl;
        }
    }

    return 0;
}
```

## Частые ошибки

- `Int is not supported...` → пиши `1A`.
- `Void is not supported...` → пиши `0A`.
- `char line[32];` → `line` зарезервировано, используй `buf`.
- `Unknown function` / `Wrong argument count` → проверь имя функции и число аргументов.
- `unsupported asm instruction` → сейчас есть только `__asm__("hlt")`.
- `unknown draw color` / `unknown draw shape` → проверь `nw::draw!FRAME|TEXT|CUBE` и имя цвета.
- `unknown color` → проверь имя цвета в `!COLOR:NAME`.

## Пока не работает

Комментарии, `int`, `void`, `for`, `%`, `&&`, `||`, `!`,
индексация массивов, возврат строк из функций, реальная семантика
`const`/`nonzero`/`needed`, VBE/`nw::gfx`.
