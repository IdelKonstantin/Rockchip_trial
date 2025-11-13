#ifndef __THREAD_POOL_HEADER_FILE__
#define __THREAD_POOL_HEADER_FILE__

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <exception>
#include <future>
#include <mutex>
#include <queue>



// Пул потоков для выполнения пользовательских функций с сигнатурой
//      ret func(int id, other_params)
// где  id - индекс потока, в котором будет выполнена функция
//      ret - результат выполнения функции
namespace threadpool
{

    namespace detail
    {
        template<typename T>
        class CQueuePool
        {
        public:
            bool push(T const &value)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->q.push(value);
                return true;
            }

            // удаление извлекаемого элемента
            bool pop(T &v)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                if(this->q.empty())
                    return false;
                v = this->q.front();
                this->q.pop();
                return true;
            }

            bool empty()
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                return this->q.empty();
            }

        private:
            std::queue<T> q;
            std::mutex mutex;
        };
    }

    class CThreadPool
    {

    public:

        CThreadPool()
        { this->init(); }

        CThreadPool(int nThreads)
        {
            this->init();
            this->resize(nThreads);
        }


        // деструктор ожидает завершение выполнения всех функций из пула
        ~CThreadPool()
        {
            this->stop(true);
        }


        // получение количества выполняющихся потоков в пуле
        int size()
        { return static_cast<int>(this->threads.size()); }

        // количество простаивающих потоков
        int n_idle()
        { return this->nWaiting; }

        std::thread &get_thread(int i)
        { return *this->threads[i]; }

        // изменение количества потоков в пуле
        // функция потоконебезопасна, должна вызываться из одного потока одновременно и синхронно с this->stop()
        // значение nThreads должно быть >= 0
        void resize(int nThreads)
        {
            if(!this->isStop && !this->isDone)
            {
                int oldNThreads = static_cast<int>(this->threads.size());
                if(oldNThreads <= nThreads)
                {  // if the number of threads is increased
                    this->threads.resize(nThreads);
                    this->flags.resize(nThreads);

                    for (int i = oldNThreads; i < nThreads; ++i)
                    {
                        this->flags[i] = std::make_shared<std::atomic<bool>>(false);
                        this->set_thread(i);
                    }
                }
                else
                {
                    // количество потоков уменьшается
                    for (int i = oldNThreads - 1; i >= nThreads; --i)
                    {
                        *this->flags[i] = true;  // this thread will finish
                        this->threads[i]->detach();
                    }
                    {
                        // останавливаем отключенные потоки, находящиеся в состоянии ожидания
                        std::unique_lock<std::mutex> lock(this->mutex);
                        this->cv.notify_all();
                    }
                    this->threads.resize(nThreads);  // удаление безопасно, поскольку птоки отключены от пула
                    this->flags.resize(
                            nThreads);  // удаление безопасно, поскольку потоки содержат копии shared_ptr для flags
                }
            }
        }

        // очистка очереди
        void clear_queue()
        {
            std::function<void(int id)> *_f;
            while (this->q.pop(_f))
                delete _f; // чистим очередь
        }

        // извлечение функциональной обертки  оригинальной функции
        std::function<void(int)> pop()
        {
            std::function<void(int id)> *_f = nullptr;
            this->q.pop(_f);
            std::unique_ptr<std::function<void(int id)>> func(
                    _f); // при возврате из функции, удаляем ее даже в случае исключения
            std::function<void(int)> f;
            if(_f)
                f = *_f;
            return f;
        }


        // ожидание завершения выполнения всех потоков и их остановка
        // функция может выполняться асинхронно без блокировки вызывающего потока
        // если isWait == true, все функции из очереди запускаются на выполнение,
        // в противном случае, очередь очищается без запуска функций
        void stop(bool isWait = false)
        {
            if(!isWait)
            {
                if(this->isStop)
                    return;
                this->isStop = true;
                for (int i = 0, n = this->size(); i < n; ++i)
                {
                    *this->flags[i] = true;  // command the threads to stop
                }
                this->clear_queue();  // чистим очередь
            } else
            {
                if(this->isDone || this->isStop)
                    return;
                this->isDone = true;  // даем команду завершения для всех ожидающих потоков
            }
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->cv.notify_all();  // останавливаем все ожидающие потоки
            }
            for (int i = 0; i < static_cast<int>(this->threads.size()); ++i)
            {
                // ожидаем завершения выполняющихся потоков
                if(this->threads[i]->joinable())
                    this->threads[i]->join();
            }

            this->clear_queue();
            this->threads.clear();
            this->flags.clear();
        }

        template<typename F, typename... Rest>
        auto push(F &&f, Rest &&... rest) -> std::future<decltype(f(0, rest...))>
        {
            auto pck = std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(
                    std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Rest>(rest)...)
            );
            auto _f = new std::function<void(int id)>([pck](int id)
                                                      {
                                                          (*pck)(id);
                                                      });
            this->q.push(_f);
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.notify_one();
            return pck->get_future();
        }

        // запуск пользовательской функции, принимающей целочисленный аргумент id - идентификатор выполняющегося потока
        // возвращаемое значение  - std::future, с помощью которой пользователь получает результат и возможные исключения
        template<typename F>
        auto push(F &&f) -> std::future<decltype(f(0))>
        {
            auto pck = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(std::forward<F>(f));
            auto _f = new std::function<void(int id)>([pck](int id)
                                                      {
                                                          (*pck)(id);
                                                      });
            this->q.push(_f);
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.notify_one();
            return pck->get_future();
        }


    private:


        CThreadPool(const CThreadPool &);// = delete;
        CThreadPool(CThreadPool &&);// = delete;
        CThreadPool &operator=(const CThreadPool &);// = delete;
        CThreadPool &operator=(CThreadPool &&);// = delete;

        void set_thread(int i)
        {
            std::shared_ptr<std::atomic<bool>> flag(this->flags[i]); // копия shared ptr, содержащего флаг
            auto f = [this, i, flag/*  копия shared ptr, содержащего флаг */]()
            {
                std::atomic<bool> &_flag = *flag;
                std::function<void(int id)> *_f;
                bool isPop = this->q.pop(_f);
                while (true)
                {
                    while (isPop)
                    {  // проверяем есть ли что-то в очереди
                        std::unique_ptr<std::function<void(int id)>> func(
                                _f); // при возврате из функции, удаляем ее даже в случае исключения
                        (*_f)(i);
                        if(_flag)
                            return;  // запрошена остановка потока  - выходим даже если очередь не пуста
                        else
                            isPop = this->q.pop(_f);
                    }

                    // очередь пуста, ожидаем следующей команды
                    std::unique_lock<std::mutex> lock(this->mutex);
                    ++this->nWaiting;
                    this->cv.wait(lock, [this, &_f, &isPop, &_flag]()
                    {
                        isPop = this->q.pop(_f);
                        return isPop || this->isDone || _flag;
                    });
                    --this->nWaiting;
                    if(!isPop)
                        return;  // если очередь пуста и this->isDone == true или *flag, то выходим

                }
            };
            this->threads[i].reset(new std::thread(f));
        }

        void init()
        {
            this->nWaiting = 0;
            this->isStop = false;
            this->isDone = false;
        }

        std::vector<std::unique_ptr<std::thread>> threads;
        std::vector<std::shared_ptr<std::atomic<bool>>> flags;
        detail::CQueuePool<std::function<void(int id)> *> q;
        std::atomic<bool> isDone;
        std::atomic<bool> isStop;
        std::atomic<int> nWaiting;  // количество ожидающих потоков

        std::mutex mutex;
        std::condition_variable cv;
    };

}

#endif // __THREAD_POOL_HEADER_FILE__
