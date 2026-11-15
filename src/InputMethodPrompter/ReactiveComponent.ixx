export module ReactiveComponent;

import std;

export namespace Reactive
{
    class Subscription
    {
    public:
        Subscription() = default;
        Subscription(std::function<void()> cancel) :cancel(std::move(cancel)) {}
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;
        Subscription(Subscription&& other) noexcept :cancel(std::exchange(other.cancel, {})) {}
        Subscription& operator=(Subscription&& other) noexcept
        {
            if (this != &other)
            {
                Unsubscribe();
                cancel = std::exchange(other.cancel, {});
            }
            return *this;
        }

        ~Subscription()
        {
            Unsubscribe();
        }

        void Unsubscribe()
        {
            if (cancel != nullptr)
            {
                auto cancelFunction = std::exchange(cancel, {});
                cancelFunction();
            }
        }

    private:
        std::function<void()> cancel;
    };

    template<typename T>
    struct IPublisher
    {
        virtual ~IPublisher() = default;
        virtual void Set(T value) = 0;
        virtual void Notify() = 0;
    };

    // 订阅者应禁用拷贝构造、拷贝赋值、移动构造、移动赋值，避免Observer中捕获的this被销毁后变成野指针
    // 如果触发通知操作和销毁订阅者操作不在同一线程，应确保先取消订阅再销毁订阅者
    template<typename T>
    struct IObservable
    {
        using Observer = std::function<void(const T&)>;
        virtual ~IObservable() = default;
        [[nodiscard]] virtual Subscription Subscribe(Observer observer, bool notifyImmediately = false) = 0;
        virtual T Get() const = 0;
    };

    template<typename T>
    class Observable :public IObservable<T>, public IPublisher<T>
    {
    public:
        using Observer = std::function<void(const T&)>;

        explicit Observable(T initialValue) :
            value(std::move(initialValue)),
            registry(std::make_shared<SubscriptionRegistry>())
        {
        }
        Observable(const Observable&) = delete;
        Observable& operator=(const Observable&) = delete;
        Observable(Observable&&) = delete;
        Observable& operator=(Observable&&) = delete;

        // 回调内不要调用此对象的所有public函数以及Unsubscribe否则会死锁
        // 回调内不能销毁、重新赋值订阅句柄，因为它们也会自动取消订阅、重复加锁
        // 回调内抛出异常会中止本轮剩余通知，已经更新的值不会回滚
        [[nodiscard]] Subscription Subscribe(Observer observer, bool notifyImmediately = false) override
        {
            if (observer == nullptr)
                return {};

            std::lock_guard<std::mutex> lock(registry->Mutex);

            if (notifyImmediately)
                observer(value);

            std::weak_ptr<SubscriptionRegistry> weakRegistry = registry;
            auto itor = registry->Observers.insert(registry->Observers.end(), std::move(observer));
            return Subscription([weakRegistry, itor]()
            {
                if (auto registry = weakRegistry.lock())
                {
                    std::lock_guard<std::mutex> lock(registry->Mutex);
                    registry->Observers.erase(itor);
                }
            });
        }

        T Get() const override
        {
            std::lock_guard<std::mutex> lock(registry->Mutex);
            return value;
        }

        void Set(T value) override
        {
            std::lock_guard<std::mutex> lock(registry->Mutex);
            if (this->value != value)
            {
                this->value = std::move(value);
                NotifySubscribers();
            }
        }

        void Notify() override
        {
            std::lock_guard<std::mutex> lock(registry->Mutex);
            NotifySubscribers();
        }

    private:
        void NotifySubscribers()
        {
            for (const auto& observer : registry->Observers)
            {
                observer(value);
            }
        }

    private:
        struct SubscriptionRegistry
        {
            std::mutex Mutex;
            std::list<Observer> Observers;
        };

        T value;
        std::shared_ptr<SubscriptionRegistry> registry;
    };
}