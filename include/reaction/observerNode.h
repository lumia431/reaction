#include <vector>
#include <functional>

class ObserverNode
{
public:
    // virtual void valueChanged() {}

    void addObserver(const std::function<void()> &f)
    {
        m_observers.emplace_back(f);
    }

    template <typename... Args>
    void updateObservers(const std::function<void()> &f, Args &&...args)
    {
        (..., args.addObserver(f));
    }

    void notify()
    {
        // for (auto &observer : m_observers)
        // {
        //     observer->valueChanged();
        // }
        for (auto &observer : m_observers)
        {
            observer();
        }
    }

private:
    // std::vector<ObserverNode *> m_observers;
    std::vector<std::function<void()>> m_observers;
};