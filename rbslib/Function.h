#pragma once
#include <functional>
#include <list>
namespace RbsLib::Function 
{
    template<typename T>
    class Function;

    template<typename R, typename... Args>
    class Function<R(Args...)>
    {
    private:
        std::list<std::function<R(Args...)>> collection;
    public:
        void Add(const std::function<R(Args...)>& function)
        {
            this->collection.push_back(function);
        }
        void Remove(const std::function<R(Args...)>& function)
        {
            this->collection.remove_if([&function](const std::function<R(Args...)>& f) {return f.target_type() == function.target_type(); });
        }
		void operator+=(const std::function<R(Args...)>& function)
        {
			this->Add(function);
		}
        void operator-=(const std::function<R(Args...)>& function)
        {
            this->Remove(function);
        }
		void Clear(void)
		{
			this->collection.clear();
		}
		void Pop(void)
		{
			this->collection.pop_back();
		}
        void operator()(Args... args) const
        {
            for (const auto& func : collection)
            {
                func(args...);
            }
        }
    };
}