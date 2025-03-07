// src/ObserverHelper.cpp
#include "reaction/observerHelper.h"

namespace reaction {

void ObserverHelper::addObserver(Observer observer) {
    observers_.push_back(observer);
}

void ObserverHelper::notifyObservers() {
    for (auto& observer : observers_) {
        observer();
    }
}

} // namespace reaction