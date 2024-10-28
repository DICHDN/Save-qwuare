/*
Курсовой проект «Потокобезопасная очередь»
Пул потоков на базе потокобезопасной очереди.

Что нужно сделать:
Создать потокобезопасную очередь, хранящую функции, предназначенные для исполнения.
На основе этой очереди реализовать пул потоков.
Этот пул состоит из фиксированного числа рабочих потоков, равного количеству аппаратных ядер.
Когда у программы появляется какая-то работа, она вызывает функцию, которая помещает эту работу в очередь.
Рабочий поток забирает работу из очереди, выполняет указанную в ней задачу, после чего проверяет, есть ли в очереди другие работы.
Реализуемые классы
1. Класс thread_pool — реализация пула потоков.
Минимально требуемые поля класса thread_pool:

вектор потоков, которые инициализируют в конструкторе класса и уничтожают в деструкторе;
потокобезопасная очередь задач для хранения очереди работ;
остальные поля на усмотрение разработчика.
Минимально требуемые методы класса thread_pool:

метод work — выбирает из очереди очередную задачу и исполняет её. Этот метод передаётся конструктору потоков для исполнения;
метод submit — помещает в очередь задачу. В качестве аргумента метод может принимать или объект шаблона std::function, или объект шаблона package_task;
остальные методы на усмотрение разработчика.
2. Шаблонный класс safe_queue — реализация очереди, безопасной относительно одновременного доступа из нескольких потоков.
Минимально требуемые поля класса safe_queue:

очередь std::queue для хранения задач,
std::mutex для реализации блокировки,
std::condtional_variables для уведомлений.

Минимально требуемые методы класса safe_queue:

метод push — записывает в начало очереди новую задачу. При этом захватывает мьютекс, а после окончания операции нотифицируется условная переменная;
метод pop — находится в ожидании, пока не придёт уведомление на условную переменную. При нотификации условной переменной данные считываются из очереди;
остальные методы на усмотрение разработчика.
Алгоритм работы
Объявить объект класса thread_pool.
Описать несколько тестовых функций

Раз в секунду класть в пул одновременно 2 функции и проверять их исполнение.
*/







#include<chrono>
#include<thread>
#include<execution>
#include <iostream>
#include <windows.h>
#include<vector>
#include<mutex>
#include <functional>
#include <future>
#include <stdexcept>
#include <queue>
#include <condition_variable>
std::mutex m;

template<typename T>
class safe_queue 
{
 public:
    safe_queue() = default;

    // Записывает элемент в конец очереди
    void push(const T& value) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        lock.unlock();
        cond_var_.notify_one();
    }

    // Извлекает элемент из начала очереди
    bool pop(T& value) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });

        if (queue_.empty()) 
        {
            return false;
        }

        value = queue_.front();
        queue_.pop();
        return true;
    }

    bool empty() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

 private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_var_;
};

class thread_pool {
public:
    thread_pool(size_t num_threads)
        : stop(false) 
    {
        for (size_t i = 0; i < num_threads; ++i) 
        {
            workers.emplace_back([this] { work(); });
        }
    }

    ~thread_pool() 
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) 
        {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> 
    {
        using return_type = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) {
                throw std::runtime_error("submit on stopped thread_pool");
            }

            tasks.push([task]() { (*task)(); });
        }
        condition.notify_one();
        return result;
    }

private:
    std::vector<std::thread> workers;
    safe_queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    void work() 
    {
        while (true) 
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });

                if (stop && tasks.empty()) 
                {
                    return;
                }

                if (!tasks.pop(task)) 
                {
                    continue;
                }
            }
            try 
            {
                task();
            }
            catch (const std::exception& e) 
            {
                // Обработка исключений в задачах
                std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
            }
        }
    }
};

void drawProgressBar(int x, int y)
{
    size_t hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    int u = 0;
    bool flag = true;
    int k = 50; // количество символов в прогрессбаре
    for (int i = 0; i < k; ++i)
    {
        if (flag)
        {
            // блок для вывода id потока под своими параметрами вывода
            m.lock();
            COORD id_thred;
            id_thred.X = x - 19;
            id_thred.Y = y;
            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), id_thred);
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
            std::cout << "  ID THREAD: " << std::this_thread::get_id();
            // блок рисования прогресса
            COORD bar;
            bar.X = x;
            bar.Y = y;
            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), bar);
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_GREEN);

            int pos = (i * i) / 100; // условный прогресс до 100%            
            for (int j = 0; j < i; ++j)
            {
                if (flag)
                {
                    std::cout << " ";

                    if (u > 895) // условие иметации ошибки
                    {
                        bar.X = x + i + 1;
                        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), bar);
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
                        std::cout << "ERROR!!!" << std::endl;
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
                        flag = false;
                    }
                }
                if (j == k - 2)
                {
                    bar.X = x + i + 1;
                    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), bar);
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
                    std::cout << "COMPLITE!!!"<< std::endl;
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
                }
            }
        }
        m.unlock();
        u = (rand() * hash) % 900;
        Sleep(u);
        if (!flag) break;
    }
}




int main() 
{
    
    size_t num_threads = std::thread::hardware_concurrency();
    thread_pool pool(num_threads);// количество одновременных потокоы не больше чем количесво физических ядер
    std::cout << "hardware_concurrency = " << num_threads << std::endl;
    int k = 4;//множитель задач
    for (int i=0;i< num_threads*k;++i)
    {
        auto future1 = pool.submit(drawProgressBar, 19,  i+1);
        if (i%2==1) Sleep(1000);//задержка формирования очереди согласно условию 
    };
       
    return 0;
}
