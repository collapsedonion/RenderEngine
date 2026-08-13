//
// Created by Onion on 10.08.2026.
//
module;
#include "cmake-build-debug/_deps/glm-src/glm/detail/type_quat.hpp"
#ifdef __APPLE__
#include <ranges>
#endif

export module dynamic_dispatchable_iterator;
#ifdef __linux__
export import std;
#endif

namespace RenderEngine
{
    export template<typename T>
    class Iterator
    {
    public:
        virtual ~Iterator() = default;
        virtual T& next() = 0;
        virtual operator bool() = 0;

        std::generator<T&> to_generator()
        {
            while (static_cast<bool>(*this))
            {
                co_yield this->next();
            }

            co_return;
        }
    };

    export template<std::ranges::input_range T>
    class RangedIterator : public Iterator<std::ranges::range_value_t<T>>
    {
        using ST = std::ranges::range_value_t<T>;

        T _stored_range;
        std::ranges::iterator_t<T> _current;
        std::ranges::iterator_t<T> _end;
    public:
        RangedIterator(T value): _stored_range(value)
        {
            _current = _stored_range.begin();
            _end = _stored_range.end();
        }

        ST& next() override
        {
            auto& res = *_current;
            ++_current;
            return res;
        }


        operator bool() override
        {
            return _current != _end;
        }
    };
}