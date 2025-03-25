#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/invalidStrategy.h"
#include "reaction/expression.h"
#include <atomic>

namespace reaction {

// Trait to determine the expression type for DataSource
template <typename... Args>
struct SourceTraits {
    using ExprType = Expression<Args...>;
};

// Specialization of SourceTraits for DataSource with a single type
template <typename TriggerPolicy, typename InvalidStrategy, typename Type>
struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type>> {
    using ExprType = Expression<TriggerPolicy, Type>;
};

// Specialization of SourceTraits for DataSource with multiple types
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>> {
    using ExprType = Expression<TriggerPolicy, Type, Args...>;
};

// DataSource class template that handles the value, observers, and invalidation strategies
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
class DataSource : public SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>>::ExprType,
                   private InvalidStrategy {
public:
    // Using the ExprType from SourceTraits
    using ExprType = typename SourceTraits<DataSource>::ExprType;
    using InvStrategy = InvalidStrategy;
    using ExprType::ExprType; // Inherit constructors from ExprType

    // Set new value and notify observers
    template <typename T, typename... A>
    bool set(T &&t, A &&...args) {
        return this->setSource(std::forward<T>(t), std::forward<A>(args)...);
    }

    // Close the DataSource and remove it from the observer graph
    void close() {
        ObserverGraph::getInstance().closeNode(this->getShared());
    }

    // Assignment operator to update the value and notify observers
    template <typename T>
    DataSource &operator=(T &&t)
        requires(!ConstCC<Type>)
    {
        this->updateValue(std::forward<T>(t));
        this->notifyObservers(this->getValue() != t);
        return *this;
    }

    // Addition assignment operator
    DataSource &operator+=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() + rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Subtraction assignment operator
    DataSource &operator-=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() - rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Multiplication assignment operator
    DataSource &operator*=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() * rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Division assignment operator
    DataSource &operator/=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() / rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix increment operator
    DataSource &operator++()
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() + 1);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix decrement operator
    DataSource &operator--()
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() - 1);
        this->notifyObservers(true);
        return *this;
    }

    // Getter functions
    auto get() const {
        return this->getValue();
    }
    auto getUpdate() const {
        return this->evaluate();
    }
    auto getRaw() const {
        return this->getRawPtr();
    }
    auto &getRef() const {
        return this->getReference();
    }

private:
    // Methods for managing weak references
    void addWeakRef() {
        m_weakRefCount++;
    }

    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

    // Friend class to allow access to private methods for weak reference management
    template <typename T>
    friend class DataWeakRef;

    // Atomic counter for weak references
    std::atomic<int> m_weakRefCount{0};
};

// DataWeakRef class template that manages weak references to DataSource objects
template <typename DataType>
class DataWeakRef {
public:
    // Type alias for the invalidation strategy of the DataSource
    using InvStrategy = typename DataType::InvStrategy;

    // Constructor that locks the weak pointer and adds a weak reference
    DataWeakRef(std::shared_ptr<DataType> ptr) :
        m_weakPtr(ptr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    // Destructor that releases the weak reference
    ~DataWeakRef() {
        if (auto p = m_weakPtr.lock())
            p->releaseWeakRef();
    }

    // Copy constructor, adding a weak reference
    DataWeakRef(const DataWeakRef &other) :
        m_weakPtr(other.m_weakPtr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    // Move constructor
    DataWeakRef(DataWeakRef &&other) noexcept :
        m_weakPtr(std::move(other.m_weakPtr)) {
    }

    // Copy assignment operator, adding a weak reference
    DataWeakRef &operator=(const DataWeakRef &other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = other.m_weakPtr;
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }
        return *this;
    }

    // Move assignment operator
    DataWeakRef &operator=(DataWeakRef &&other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = std::move(other.m_weakPtr);
        }
        return *this;
    }

    // Arrow operator to access raw pointer
    auto operator->() const {
        return getPtr()->getRaw();
    }

    // Dereference operator
    DataType &operator*() const {
        return *getPtr();
    }

    // Conversion to bool to check if the weak pointer is valid
    explicit operator bool() const {
        return !m_weakPtr.expired();
    }

    // Getter functions for the value and raw data
    auto get() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getValue();
    }
    auto getUpdate() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->evaluate();
    }
    auto getRaw() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getRaw();
    }
    auto &getRef() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getRef();
    }
    auto getShared() {
        return getPtr()->getShared();
    }

    // Set a new value to the source
    template <typename T, typename... A>
    bool set(T &&t, A &&...args) {
        return getPtr()->setSource(std::forward<T>(t), std::forward<A>(args)...);
    }
    // Set a threshold for the source
    template <typename F, typename... A>
    void setThreshold(F &&f, A &&...args) {
        getPtr()->setThreshold(std::forward<F>(f), std::forward<A>(args)...);
    }
    // Close the source
    void close() {
        getPtr()->close();
    }
    // Set and get name for the source
    void setName(const std::string &name) {
        getPtr()->setName(name);
    }
    std::string getName() const {
        return getPtr()->getName();
    }

private:
    // Helper function to get the shared pointer from weak pointer
    std::shared_ptr<DataType> getPtr() const {
        if (m_weakPtr.expired())
            throw std::runtime_error("Null weak pointer");
        return m_weakPtr.lock();
    }

    // Weak pointer to the DataSource
    std::weak_ptr<DataType> m_weakPtr;
};

// Helper trait to extract the underlying type from DataWeakRef
template <typename T>
struct ExtractDataWeakRef {
    using Type = T;
};

// Specialization for DataWeakRef to extract the underlying type
template <typename T>
struct ExtractDataWeakRef<DataWeakRef<T>> {
    using Type = T;
};

// Field alias for DataWeakRef to work with DataSource
template <typename MetaTypePtr, CompareCC SourceType>
using Field = DataWeakRef<DataSource<AlwaysTrigger, FieldStrategy, FieldIdentity<MetaTypePtr, std::decay_t<SourceType>>>>;

// Function to create a Field DataSource
template <typename MetaTypePtr, CompareCC SourceType>
auto makeFieldSource(MetaTypePtr *meta, SourceType &&value) {
    auto ptr = std::make_shared<DataSource<AlwaysTrigger, FieldStrategy, FieldIdentity<MetaTypePtr, std::decay_t<SourceType>>>>(FieldIdentity<MetaTypePtr, std::decay_t<SourceType>>{meta, std::forward<SourceType>(value)});
    FieldGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr};
}

// Function to create a constant DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectFailureStrategy, typename SourceType>
auto makeConstantDataSource(SourceType &&value) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, const std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    ObserverGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr};
}

// Function to create a meta DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectFailureStrategy, CompareCC SourceType>
auto makeMetaDataSource(SourceType &&value) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    if constexpr (HasFieldCC<std::decay_t<SourceType>>) {
        ptr->setField();
    }
    ObserverGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr};
}

// Function to create a variable DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectFailureStrategy, typename Fun, ArgSizeOverZeroCC... Args>
auto makeVariableDataSource(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, Fun, typename ExtractDataWeakRef<std::decay_t<Args>>::Type...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return DataWeakRef{ptr};
}

// Function to create an action DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectFailureStrategy, typename Fun, ArgSizeOverZeroCC... Args>
auto makeActionSource(Fun &&fun, Args &&...args) {
    return makeVariableDataSource<TriggerPolicy, InvalidStrategy>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

} // namespace reaction

#endif // REACTION_DATASOURCE_H
