Задача:<br>
Написать быстрый пакетный конвертер картинок из Png в Jpeg.<br>
На вход - директория с png-картинками. Количество картинок - любое, размер одной картинки - ~20-50 мб.<br>
Требуется - записать в эту же директорию картинки в формате Jpeg (с такими же именами).<br>
Оценка качества программы - скорость работы.<br><br>

Мои комментарии:<br>
Основные моменты алгоритма оптимизации:<br>
1. Чтение/запись на диск осуществялется в одном потоке последовательно, т.к. это быстрее чем несколько потоков будут дёргать диск.<br>
2. Конвертация PNG-JPEG осуществляется многопоточно в памяти. Кол-во потоков в зависимости от кол-ва процессоров/ядер в системе.<br>
3. Дальнейшая оптимизация возможна с изпользованием паттерна "пул объектов".<br><br>

Иструменты: C++, QT, QT Creator
