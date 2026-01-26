export module DebouncedAction;

import std;
import std.compat;

export class DebouncedAction
{
public:
    explicit DebouncedAction(std::function<void()> action) :
        action(std::move(action)),
        workerThread(&DebouncedAction::ExecuteLoop, this)
    {
    }

    ~DebouncedAction()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            isExit = true;
        }
        condition.notify_one();

        if (workerThread.joinable())
            workerThread.join();
    }

    void Request()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            isPending = true;
        }
        condition.notify_one();
    }

private:
    void ExecuteLoop()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (true)
        {
            condition.wait(lock, [this]() { return isPending || isExit; });
            if (isExit)
                break;

            isPending = false;

            lock.unlock();
            action();
            lock.lock();
        }
    }

private:
    std::function<void()> action;
    std::thread workerThread;
    std::mutex mutex;
    std::condition_variable condition;
    bool isPending = false;
    bool isExit = false;
};