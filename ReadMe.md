# Курсовой проект «Потокобезопасная очередь»Пул потоков на базе потокобезопасной очереди.
## Что нужно сделать:
1. Создать потокобезопасную очередь, хранящую функции, предназначенные для исполнения.
2. На основе этой очереди реализовать пул потоков. 
3. Этот пул состоит из фиксированного числа рабочих потоков, равного количеству аппаратных ядер.
4. Когда у программы появляется какая-то работа, она вызывает функцию, которая помещает эту работу в очередь.
5. Рабочий поток забирает работу из очереди, выполняет указанную в ней задачу, после чего проверяет, есть ли в очереди другие работы.
   ### Реализуемые классы
   #### 1. Класс thread_pool — реализация пула потоков.Минимально требуемые поля класса thread_pool:
   * вектор потоков, которые инициализируют в конструкторе класса и уничтожают в деструкторе;
   * потокобезопасная очередь задач для хранения очереди работ;
   * остальные поля на усмотрение разработчика.
  Минимально требуемые методы класса thread_pool:
  * метод work — выбирает из очереди очередную задачу и исполняет её. Этот метод передаётся конструктору потоков для исполнения;
  * метод submit — помещает в очередь задачу. В качестве аргумента метод может принимать или объект шаблона std::function, или объект шаблона package_task;
  * остальные методы на усмотрение разработчика.
  #### 2. Шаблонный класс safe_queue — реализация очереди, безопасной относительно одновременного доступа из нескольких потоков.
  Минимально требуемые поля класса safe_queue:
  * очередь std::queue для хранения задач,
  * std::mutex для реализации блокировки,
  * std::condtional_variables для уведомлений.
  Минимально требуемые методы класса safe_queue:
  * метод push — записывает в начало очереди новую задачу. При этом захватывает мьютекс, а после окончания операции нотифицируется условная переменная;
  * метод pop — находится в ожидании, пока не придёт уведомление на условную переменную. При нотификации условной переменной данные считываются из очереди;
  * остальные методы на усмотрение разработчика.
  ## Алгоритм работы
      1. Объявить объект класса thread_pool.
      2. Описать несколько функций, выводящих в "консоль своё имя" (используется ID потока и его прогресс).
      3. Раз в секунду класть в пул одновременно 2 функции и проверять их исполнение. Поскольку сами функции в себе уже имеют задержку - после первого заполенеия очереди до количества ядер - дальшейшее добавление не заметно, но новый поток запускается как только "освобождается одно из ядер".