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
    struct IValueSource
    {
        virtual ~IValueSource() = default;
        virtual T Get() const = 0;
    };

    template<typename T>
    struct IPublisher :virtual IValueSource<T>
    {
        virtual ~IPublisher() = default;
        virtual bool Set(T value) = 0;
        virtual void Notify() = 0;
    };

    // 订阅者应禁用拷贝构造、拷贝赋值、移动构造、移动赋值，避免Observer中捕获的this被销毁后变成野指针
    // 如果触发通知操作和销毁订阅者操作不在同一线程，应确保先取消订阅再销毁订阅者
    template<typename T>
    struct IObservable :virtual IValueSource<T>
    {
        using Observer = std::function<void(const T&)>;
        virtual ~IObservable() = default;
        [[nodiscard]] virtual Subscription Subscribe(Observer observer, bool notifyImmediately = false) = 0;
    };

    // 类型T需要实现==操作符
    template<typename T>
    class Observable :public IObservable<T>, public IPublisher<T>
    {
    public:
        using Observer = std::function<void(const T&)>;

        explicit Observable(T initialValue, bool notifyOnSameValue = false) :
            value(std::move(initialValue)),
            notifyOnSameValue(notifyOnSameValue),
            registry(std::make_shared<SubscriptionRegistry>())
        {
        }
        Observable(const Observable&) = delete;
        Observable& operator=(const Observable&) = delete;
        Observable(Observable&&) = delete;
        Observable& operator=(Observable&&) = delete;

        [[nodiscard]]
        Subscription Subscribe(Observer observer, bool notifyImmediately = false) override
        {
            if (observer == nullptr)
                return {};

            std::lock_guard<std::recursive_mutex> lock(registry->Mutex);

            if (notifyImmediately && notifyingCount == 0)
            {
                ++notifyingCount;
                observer(value);
                --notifyingCount;
            }

            std::weak_ptr<SubscriptionRegistry> weakRegistry = registry;
            auto iter = registry->Observers.insert(registry->Observers.end(), std::move(observer));
            return Subscription([weakRegistry, iter]()
            {
                if (auto registry = weakRegistry.lock())
                {
                    std::lock_guard<std::recursive_mutex> lock(registry->Mutex);
                    registry->Observers.erase(iter);
                }
            });
        }

        T Get() const override
        {
            std::lock_guard<std::recursive_mutex> lock(registry->Mutex);
            return value;
        }

        bool Set(T value) override
        {
            std::lock_guard<std::recursive_mutex> lock(registry->Mutex);
            if (notifyingCount > 0)
                return false;

            if (notifyOnSameValue || this->value != value)
            {
                this->value = std::move(value);
                NotifySubscribers();
            }
            return true;
        }

        void Notify() override
        {
            std::lock_guard<std::recursive_mutex> lock(registry->Mutex);
            if (notifyingCount == 0)
                NotifySubscribers();
        }

    private:
        void NotifySubscribers()
        {
            ++notifyingCount;
            auto observers = registry->Observers;
            for (const auto& observer : observers)
            {
                observer(value);
            }
            --notifyingCount;
        }

    private:
        struct SubscriptionRegistry
        {
            std::recursive_mutex Mutex;
            std::list<Observer> Observers;
        };

        T value;
        bool notifyOnSameValue;
        int notifyingCount = 0;
        std::shared_ptr<SubscriptionRegistry> registry;
    };
}