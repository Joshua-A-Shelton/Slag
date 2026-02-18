#ifndef SLAG_GENERALUTILITIES_H
#define SLAG_GENERALUTILITIES_H
#include <ranges>
template<typename T> constexpr auto sequentialEnumRange(T begin, T end)
{
    return std::views::iota(static_cast<int>(begin), static_cast<int>(end) + 1)
           | std::views::transform([](int v) { return static_cast<T>(v); });
}


#endif //SLAG_GENERALUTILITIES_H
