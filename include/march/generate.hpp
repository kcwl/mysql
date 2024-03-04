#pragma once
#include "algorithm.hpp"
#include "keyword.hpp"
#include "string_literal.hpp"
#include <array>
#include <functional>
#include <typeinfo>

using namespace std::string_view_literals;

namespace march
{
	template <std::string_view const& Keyword, typename _Ty>
	void make_input_sql(std::string& sql, _Ty&& t)
	{
		constexpr static std::string_view table_name = reflect::title<_Ty>();

		constexpr auto temp_sql_prev = detail::concat_v<Keyword, SPACE, INTO, SPACE, table_name, SPACE, VALUES, LEFT_BRACKET>;

		sql.append(temp_sql_prev.data());

		reflect::for_each(std::forward<_Ty>(t),
						   [&](auto&& value)
						   {
							   using type = std::remove_cvref_t<decltype(value)>;
							   if constexpr (std::same_as<type, std::string>)
							   {
								   sql += "'";
								   sql += value;
								   sql += "'";
							   }
							   else
							   {
								   sql += std::to_string(value);
							   }

							   sql += ",";
						   });

		if (sql.back() == ',')
			sql.pop_back();

		sql += RIGHT_BRACKET;
	}

	template <typename _From, const std::string_view& Keyword, std::string_view const&... Args>
	void make_select_sql(std::string& sql)
	{
		constexpr static auto table_name = reflect::title<_From>();

		if constexpr (sizeof...(Args) != 0)
		{
			constexpr std::string_view temp_sql =
				detail::concat_v<SELECT, SPACE, Keyword, detail::concat_v<Args, COMMA, SPACE>..., FROM, SPACE, table_name>;

			constexpr auto pos = temp_sql.find(FROM);

			constexpr static std::string_view left = temp_sql.substr(0, pos - 2);

			constexpr static std::string_view right = temp_sql.substr(pos - 1);

			constexpr auto temp_sql_s = detail::concat_v<left, right>;

			sql = std::string(temp_sql_s.data(), temp_sql_s.size());
		}
		else
		{
			constexpr auto temp_sql = detail::concat_v<SELECT, SPACE, ASTERISK, SPACE, FROM, SPACE, table_name>;

			sql = std::string(temp_sql.data(), temp_sql.size());
		}
	}

	template <typename _Ty>
	void make_remove_sql(std::string& sql)
	{
		constexpr static auto table_name = reflect::title<_Ty>();

		constexpr auto temp_sql_prev = detail::concat_v<REMOVE, SPACE, FROM, SPACE, table_name>;

		sql += temp_sql_prev;
	}

	template <typename _Ty>
	void make_update_sql(std::string& sql, _Ty&& t)
	{
		constexpr static std::string_view table_name = reflect::title<_Ty>();

		constexpr auto temp_sql_prev = detail::concat_v<UPDATE, SPACE, table_name, SPACE>;

		sql.append(temp_sql_prev.data());

		reflect::for_each(std::forward<_Ty>(t),
						   [&](auto&& value)
						   {
							   sql += detail::concat_v<SET, SPACE, SPACE>;

							   using type = std::remove_cvref_t<decltype(value)>;
							   if constexpr (std::same_as<type, std::string>)
							   {
								   sql += "'";
								   sql += value;
								   sql += "'";
							   }
							   else
							   {
								   sql += std::to_string(value);
							   }

							   sql += ",";
						   });

		if (sql.back() == ',')
			sql.pop_back();

		sql += RIGHT_BRACKET;
	}

	template <const std::string_view& Keyword, std::string_view const&... Args>
	void make_cat(std::string& sql)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			sql += Keyword;
		}
		else
		{
			constexpr auto temp_sql = detail::concat_v<Keyword, detail::concat_v<Args, COMMA, SPACE>...>;

			sql += temp_sql.substr(0, temp_sql.size() - 2);
		}
	}

} // namespace march