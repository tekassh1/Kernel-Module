# PageTable walk linux kernel module

Загружаемый модуль ядра Linux, выводящий пользователю всё виртуальное пространоство процесса по заданному PID.

Модуль вывод иерархию всех уровней таблиц страниц и конечный физический адресс для каждой области памяти процесса.

Вывод в формате:

```
+-----+
| PGD |
+-----+
   |
   |   +-----+
   +-->| P4D |
       +-----+
          |
          |   +-----+
          +-->| PUD |
              +-----+
                 |
                 |   +-----+
                 +-->| PMD |
                     +-----+
                        |
                        |   +-----+
                        +-->| PTE |
                            +-----+
```

Формат взаимодействия:

```
make all
sudo insmod src/kernel_module.ko
sudo src/api
Enter PID: <your PID>
```

Примеры вывода:



> Ядро `5.15.0-43-generic`