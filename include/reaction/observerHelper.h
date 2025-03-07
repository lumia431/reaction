// include/reaction/ObserverHelper.h
#ifndef REACTION_OBSERVERHELPER_H
#define REACTION_OBSERVERHELPER_H

#include <vector>
#include <functional>

namespace reaction {

class ObserverHelper {
public:
    using Observer = std::function<void()>;

    void addObserver(Observer observer);
    void notifyObservers();

private:
    std::vector<Observer> observers_;
};

} // namespace reaction

#endif // REACTION_OBSERVERHELPER_H