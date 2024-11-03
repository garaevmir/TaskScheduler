#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

template <typename function_type>
class TaskScheduler {
public:
    TaskScheduler() {
        scheduler = std::thread([&] { run(); });
    }

    ~TaskScheduler() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop = true;
        }

        cond.notify_one();
        if (scheduler.joinable()) {
            scheduler.join();
        }

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void Add(function_type task, std::time_t timestamp) {
        std::unique_lock<std::mutex> lock(mutex);
        tasks.emplace(std::move(task), timestamp);
        cond.notify_one();
    }

private:
    struct Task {
        Task(function_type _func, std::time_t _time) : func(std::move(_func)), time(_time){};

        function_type func;
        std::time_t time;

        bool operator<(const Task& other) const {
            return other.time < time;
        }
    };

    bool stop = false;
    std::thread scheduler;
    std::mutex mutex;
    std::condition_variable cond;
    std::priority_queue<Task> tasks;
    std::vector<std::thread> workers;

    void run() {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex);

            if (stop && tasks.empty()) {
                break;
            } else if (tasks.empty()) {
                cond.wait(lock);
            } else {
                auto curr_time = std::time(nullptr);
                auto& nearest_task = tasks.top();

                if (nearest_task.time <= curr_time) {
                    workers.emplace_back(nearest_task.func);
                    tasks.pop();
                }
            }
        }
    }
};

int main() {
    TaskScheduler<std::function<void()>> scheduler;

    scheduler.Add([] { std::cout << "Task 1\n"; }, std::time(nullptr) + 2);
    scheduler.Add([] { std::cout << "Task 2\n"; }, std::time(nullptr) + 1);
    scheduler.Add([] { std::cout << "Task 3\n"; }, std::time(nullptr) + 5);
    scheduler.Add([] { std::cout << "Task 4\n"; }, std::time(nullptr) + 3);

    return 0;
}
