stash
-


Напишите программу, скрывающую тип файла с целью обеспечить невозможность его чтения стандартными средствами. 
Программа должна иметь 2 режима работы - искажение и восстановление. Режим работы и имя файла передайте с помощью 
аргументов командной строки.

### Compilation info:

```
g++ -std=c++17 main.cpp -o stash
```

### Usage

```
Syntax: ./stash <stash | restore> <filePath>

Examples:
        ./stash stash 1.png
        ./stash restore 1.png
```