export module EnumComponent;

import std;

export
template<auto V>
consteval std::string_view GetEnumString()
{
    static_assert(std::is_enum_v<decltype(V)>);

#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    std::string_view name = signature;

    name.remove_prefix(name.find("V = ") + 4);
    name = name.substr(0, name.find_first_of(";]"));

#elif defined(_MSC_VER)
    constexpr std::string_view signature = __FUNCSIG__;
    std::string_view name = signature;

    constexpr std::string_view prefix = "GetEnumString<";
    static_assert(
        signature.find(prefix) != std::string_view::npos &&
        signature.rfind(">(void)") != std::string_view::npos,
        "Unsupported function signature format"
    );
    name.remove_prefix(name.find(prefix) + prefix.size());
    name = name.substr(0, name.rfind(">(void)"));

#else
#error Unsupported compiler
#endif

    if (name.find('(') != std::string_view::npos)
        return {};

    const auto pos = name.rfind("::");
    return pos == std::string_view::npos ? name : name.substr(pos + 2);
}

// 需要对应的枚举类型的最大值不超过Max，否则需要传入更大的Max
// 不要设置太大的Max否则会拖慢编译速度
export
template<int Max = 100, int Min = 0, typename TEnum>
std::string_view GetEnumString(TEnum value)
{
    static_assert(std::is_enum_v<TEnum>);
    static_assert(Min <= Max);

    static constexpr auto names = []<std::size_t... I>(std::index_sequence<I...>)
    {
        return std::array<std::string_view, sizeof...(I)>
        {
            GetEnumString<static_cast<TEnum>(Min + static_cast<int>(I))>()...
        };
    }(std::make_index_sequence<static_cast<std::size_t>(Max - Min) + 1>{});

    const auto raw = std::to_underlying(value);
    if (std::cmp_less(raw, Min) || std::cmp_greater(raw, Max))
        return "[Unknown]";

    const auto name = names[static_cast<std::size_t>(static_cast<int>(raw) - Min)];

    return name.empty() ? std::string_view{ "[Unknown]" } : name;
}