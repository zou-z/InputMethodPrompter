export module ConflationComponent;

import std;

export
template<typename T>
class ConflatingProcessor
{
public:
    using Handler = std::move_only_function<void(const T&)>;

    explicit ConflatingProcessor(Handler handler) :
        handler(std::move(handler)),
        worker(std::jthread([this]() { Run(); }))
    {
    }

    ConflatingProcessor(const ConflatingProcessor&) = delete;
    ConflatingProcessor& operator=(const ConflatingProcessor&) = delete;
    ConflatingProcessor(ConflatingProcessor&&) = delete;
    ConflatingProcessor& operator=(ConflatingProcessor&&) = delete;

    ~ConflatingProcessor()
    {
        {
            std::lock_guard lock(mutex);
            isStopping = true;
        }
        condition.notify_one();
        worker.join();
    }

    void Submit(T value)
    {
        {
            std::lock_guard lock(mutex);
            pendingValue.emplace(std::move(value));
        }

        condition.notify_one();
    }

private:
    void Run()
    {
        while (true)
        {
            std::optional<T> currentValue;

            {
                std::unique_lock lock(mutex);
                condition.wait(lock, [this]() { return isStopping || pendingValue.has_value(); });

                if (!pendingValue.has_value())
                    return;

                currentValue.emplace(std::move(pendingValue.value()));
                pendingValue.reset();
            }

            handler(currentValue.value());
        }
    }

private:
    Handler handler;
    std::mutex mutex;
    std::condition_variable condition;
    std::optional<T> pendingValue;
    bool isStopping = false;
    std::jthread worker;
};