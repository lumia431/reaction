#ifndef REACTION_TRIGGERMODE_H
#define REACTION_TRIGGERMODE_H

namespace reaction
{
    template <typename F, typename... A>
    inline auto createGetFunRef(F &&f, A &&...args)
    {
        return [f = std::forward<F>(f), &...args = *args]() mutable
        {
            return std::invoke(f, args.get()...);
        };
    }

    template <typename F, typename... A>
    inline auto createUpdateFunRef(F &&f, A &&...args)
    {
        return [f = std::forward<F>(f), &...args = *args]() mutable
        {
            return std::invoke(f, args.getUpdate()...);
        };
    }

    struct AlwaysTrigger
    {
    protected:
        template <typename... Args>
        bool checkTrigger(Args... args)
        {
            return true;
        }
    };

    // // ValueChangeTrigger 策略：仅在值发生变化时触发更新
    struct ValueChangeTrigger
    {
    protected:
        template <typename... Args>
            requires(sizeof...(Args) == 1) && (std::same_as<std::tuple_element_t<0, std::tuple<Args...>>, bool>)
        bool checkTrigger(Args... args)
        {
            return (... && args);
        }
    };

    // // ThresholdTrigger 策略：当满足阈值条件时触发
    struct ThresholdTrigger
    {
        template <typename F, typename... A>
        void setThreshold(F &&f, A &&...args)
        {
            if (m_repeatDependent)
            {
                m_thresholdFun = createUpdateFunRef(std::forward<F>(f), std::forward<A>(args)...);
            }
            else
            {
                m_thresholdFun = createGetFunRef(std::forward<F>(f), std::forward<A>(args)...);
            }
        }

    protected:
        template <typename... Args>
        bool checkTrigger(Args... args)
        {
            return std::invoke(m_thresholdFun);
        }

        void setRepeatDependent(bool repeat)
        {
            m_repeatDependent = repeat;
        }

    private:
        std::function<bool()> m_thresholdFun = []()
        { return true; };
        bool m_repeatDependent = false;
    };

} // namespace reaction

#endif // REACTION_TRIGGERMODE_H