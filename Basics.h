/*
 * Copyright (c) 2022, Ali Mohammad Pur <mpfard@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// NTTP String
template<auto N>
struct String {
    constexpr String(char const (&s)[N])
    {
        for (auto i = 0; i < N; i++)
            chars[i] = s[i];
    }

    template<auto M>
    constexpr bool operator==(String<M> const& other) const
    {
        if constexpr (N != M) {
            return false;
        } else {
            for (auto i = 0; i < N; i++)
                if (chars[i] != other.chars[i])
                    return false;
            return true;
        }
    }

    static constexpr auto length = N;
    char chars[N];
};

template<String S>
struct s_impl {
    static constexpr auto value = S;
};

template<String S>
constexpr inline auto s = s_impl<S>::value;

template<auto left, auto right>
constexpr inline String concat_strings_impl = [] {
    constexpr auto left_length = left.length;
    constexpr auto right_length = right.length;
    constexpr auto total_length = left_length + right_length - 1;
    char result[total_length];
    for (auto i = 0; i < left_length; ++i)
        result[i] = left.chars[i];
    for (auto i = 0; i < right_length; ++i)
        result[i + left_length - 1] = right.chars[i];
    return String<total_length>(result);
}();

template<auto first, auto... rest>
constexpr inline String concat_strings = concat_strings_impl<first, concat_strings<rest...>>;

template<auto first>
constexpr inline String concat_strings<first> = first;

template<unsigned N, typename... T>
struct TemplateParameterAtIndexImpl;

template<typename T0, typename... Ts>
struct TemplateParameterAtIndexImpl<0, T0, Ts...> {
    using Type = T0;
};

template<unsigned N, typename T0, typename... T>
struct TemplateParameterAtIndexImpl<N, T0, T...> {
    using Type = typename TemplateParameterAtIndexImpl<N - 1, T...>::Type;
};

template<template<typename...> typename T, typename Specialised, auto Index>
struct TemplateParameterAtIndexDetail;

template<template<typename...> typename T, typename... Ts, auto Index>
struct TemplateParameterAtIndexDetail<T, T<Ts...>, Index> {
    using Type = TemplateParameterAtIndexImpl<Index, Ts...>;
};

template<template<typename...> typename T, typename Specialised, auto Index>
using TemplateParameterAtIndex = typename TemplateParameterAtIndexDetail<T, Specialised, Index>::Type;

template<typename T>
struct TypeWrapper {
    using Type = T;
};

template<typename Error>
void compiletime_fail(Error);

template<bool F, auto Value>
inline constexpr auto DependentBool = F;

template<auto Value>
inline constexpr auto DependentFalse = DependentBool<false, Value>;

template<bool Condition, typename True, typename False>
struct ConditionalImpl;

template<typename True, typename False>
struct ConditionalImpl<true, True, False> {
    using Type = True;
};

template<typename True, typename False>
struct ConditionalImpl<false, True, False> {
    using Type = False;
};

template<bool Condition, typename True, typename False>
using Conditional = typename ConditionalImpl<Condition, True, False>::Type;

template<auto Value>
struct Type {
    constexpr static auto value = Value;
};

template<auto value, auto... Cases>
struct SwitchImpl;

template<auto value, auto Case, auto Result, auto... Rest>
struct SwitchImpl<value, Case, Result, Rest...> {
    static constexpr auto result = Conditional<value == Case, Type<Result>, Type<SwitchImpl<value, Rest...>::result>>::value;
};

template<auto value, auto Default>
struct SwitchImpl<value, Default> {
    static constexpr auto result = Default;
};

template<auto value, auto... Cases>
constexpr inline auto Switch = SwitchImpl<value, Cases...>::result;

template<typename T, T... Ts>
struct IntegerSequence {
    using Type = T;
    static constexpr unsigned size() noexcept { return sizeof...(Ts); };
};

template<typename T, T N, T... Ts>
auto make_integer_sequence_impl()
{
    if constexpr (N == 0)
        return IntegerSequence<T, Ts...> {};
    else
        return make_integer_sequence_impl<T, N - 1, N - 1, Ts...>();
}

template<typename T, T N>
using MakeIntegerSequence = decltype(make_integer_sequence_impl<T, N>());

template<auto V>
static constexpr String to_string = "some number";

template<>
constexpr String to_string<0> = "0";

template<>
constexpr String to_string<1> = "1";

template<>
constexpr String to_string<2> = "2";

template<>
constexpr String to_string<3> = "3";

template<typename T, typename U>
static constexpr auto IsSame = false;

template<typename T>
static constexpr auto IsSame<T, T> = true;
