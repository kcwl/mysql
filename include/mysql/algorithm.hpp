#pragma once
#include <algorithm>
#include <codecvt>
#include <sstream>
#include <string>
#include <string_view>
#include <boost/mysql.hpp>
#include "mysql/reflect.hpp"

#pragma warning(disable : 4996)

namespace mysql
{
	template<typename _Ty>
	struct cast
	{
		static auto invoke(const boost::mysql::field_view& field)
		{
			std::stringstream ss{};
			ss << field;

			using type = std::remove_cvref_t<_Ty>;

			type result{};

			ss >> result;

			return result;
		}
	};
	

	template<typename _Ty>
	struct cast<std::vector<_Ty>>
	{
		static auto invoke(const boost::mysql::field_view& field)
		{
			std::vector<_Ty> result{};

			return result;
		}
	};

	template <typename T, std::size_t... I>
	auto to_struct_impl(const boost::mysql::row& row, std::index_sequence<I...>)
	{
		return T{ cast<decltype(reflect::get<I>(std::declval<T>()))>::invoke(row[I])...};
	}

	template <typename T>
	auto to_struct(const boost::mysql::row& row)
	{
		return to_struct_impl<T>(row, std::make_index_sequence<reflect::tuple_size_v<T>>{});
	}

	template <const std::string_view&... args>
	struct concat
	{
		constexpr static auto impl() noexcept
		{
			constexpr auto len = (args.size() + ... + 0);
			std::array<char, len + 1> arr{};

			auto f = [i = 0, &arr](auto const& str) mutable
			{
				for (auto s : str)
					arr[i++] = s;

				return arr;
			};

			(f(args), ...);

			arr[len] = '\0';

			return arr;
		}

		static constexpr auto arr = impl();

		static constexpr std::string_view value{ arr.data(), arr.size() - 1 };
	};

	template <std::string_view const&... args>
	constexpr static auto concat_v = concat<args...>::value;

	template<bool flag, std::size_t N>
	struct int_to_string2
	{

	};

	inline std::string to_uft8(const std::string& str)
	{
		std::vector<wchar_t> buff(str.size());

#ifdef _MSC_VER
		std::locale loc("zh-CN");
#else
		std::locale loc("zh_CN.GB18030");
#endif

		wchar_t* wnext = nullptr;
		const char* next = nullptr;
		mbstate_t state{};

		int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).in(
			state, str.data(), str.data() + str.size(), next, buff.data(), buff.data() + buff.size(), wnext);
		if (res != std::codecvt_base::ok)
			return "";

		std::wstring_convert<std::codecvt_utf8<wchar_t>> cuft8;

		return cuft8.to_bytes(std::wstring(buff.data(), wnext));
	}

	inline std::string to_gbk(const std::string& str)
	{
		std::vector<char> buff(str.size() * 2);

		std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
		std::wstring wtmp = cutf8.from_bytes(str);

#ifdef _MSC_VER
		std::locale loc("zh-CN");
#else
		std::locale loc("zh_CN.GB18030");
#endif

		const wchar_t* wnext = nullptr;
		char* next = nullptr;

		mbstate_t state{};
		int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).out(
			state, wtmp.data(), wtmp.data() + wtmp.size(), wnext, buff.data(), buff.data() + buff.size(), next);
		if (res != std::codecvt_base::ok)
			return {};

		return std::string(buff.data(), next);
	}
} // namespace mysql