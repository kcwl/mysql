#pragma once
#include <concepts>
#include <string>

namespace march
{
	template <typename _Ty>
	struct is_string : std::false_type
	{};

	template <>
	struct is_string<std::string> : std::true_type
	{};

	template <>
	struct is_string<const char*> : std::true_type
	{};

	template <std::size_t N>
	struct is_string<const char(&)[N]> : std::true_type
	{};

	template <>
	struct is_string<char*> : std::true_type
	{};

	template <typename _Ty>
	constexpr static bool is_string_v = is_string<_Ty>::value;
}