#pragma once
#include "attributes.hpp"
#include "generate.hpp"
#include "reflect.hpp"
#include "to_string.hpp"
#include <vector>

using namespace std::string_view_literals;

namespace march
{
	template <typename _Pool>
	class basic_sql
	{
		using service_t = typename _Pool::service_t;

	public:
		explicit basic_sql(_Pool& pool)
			: pool_(pool)
		{}

		~basic_sql() = default;

	public:
		auto execute()
		{
			error_code ec{};

			auto result = execute(ec);

			if(ec)
				throw std::exception(ec.message());

			return result;
		}

		auto execute(error_code& ec)
		{
			sql_str_ += ";";

			return pool_.execute(sql_str_, ec);
		}

		template <typename _Func>
		void async_execute(_Func&& f)
		{
			sql_str_ += ";";

			return pool_.async_execute(sql_str_, std::forward<_Func>(f));
		}

		std::string sql()
		{
			return sql_str_;
		}

	protected:
		std::string sql_str_;

	private:
		_Pool& pool_;
	};

	template <typename _Pool>
	class chain_sql : public basic_sql<_Pool>
	{
	public:
		chain_sql(_Pool& pool)
			: basic_sql<_Pool>(pool)
		{}

	public:
		template <typename _Ty>
		chain_sql& remove()
		{
			make_remove_sql<_Ty>(this->sql_str_);

			return *this;
		}

		template <typename _Ty>
		chain_sql& insert(_Ty&& t)
		{
			make_input_sql<INSERT>(this->sql_str_, std::forward<_Ty>(t));

			return *this;
		}

		template <typename _Ty>
		chain_sql& update(_Ty&& t)
		{
			make_update_sql(this->sql_str_, std::forward<_Ty>(t));

			return *this;
		}

		template <typename _Ty>
		chain_sql& replace(_Ty&& t)
		{
			make_input_sql<REPLACE>(this->sql_str_, std::forward<_Ty>(t));

			return *this;
		}

		template <typename _Ty>
		chain_sql& where(_Ty&& f)
		{
			this->sql_str_ += " where" + f.sql();

			return *this;
		}

		template <typename _Ty, _Ty left_value, _Ty right_value>
		chain_sql& where_between()
		{
			constexpr auto sql =
				detail::concat_v<SPACE, WHERE, SPACE, BETWEEN, SPACE, left_value, SPACE, AND, SPACE, right_value, SEPARATOR>;

			if (this->sql_str_.empty())
				return *this;

			if (this->sql_str_.back() == ';')
				this->sql_str_.pop_back();

			this->sql_str_ += sql;

			return *this;
		}

		template <typename _Ty>
		chain_sql& where_is_null()
		{
			constexpr auto sql = detail::concat_v<SPACE, WHERE, SPACE, name<_Ty>(), SPACE, IS_NULL, SEPARATOR>;

			if (this->sql_str_.empty())
				return *this;

			if (this->sql_str_.back() == ';')
				this->sql_str_.pop_back();

			this->sql_str_ += sql;

			return *this;
		}
	};

	template <typename _Pool>
	class select_chain : public chain_sql<_Pool>
	{
		using base_type = chain_sql<_Pool>;

	public:
		explicit select_chain(_Pool& pool)
			: base_type(pool)
		{}

	public:
		template <typename _From, string_literal... Args>
		select_chain& select()
		{
			make_select_sql<_From, bind_param<"">::value, bind_param<Args>::value...>(this->sql_str_);

			return *this;
		}

		template <typename _From, string_literal... Args>
		select_chain& select_distinct()
		{
			make_select_sql<_From, detail::concat_v<DISTINCT, SPACE>, bind_param<Args>::value...>(this->sql_str_);

			return *this;
		}

		template <typename _From, std::size_t N, string_literal... Args>
		select_chain& select_top()
		{
			make_select_sql<_From, detail::concat_v<TOP, SPACE, to_string<N>::value, SPACE>, bind_param<Args>::value...>(
				this->sql_str_);

			return *this;
		}

		template <std::size_t N>
		select_chain& limit()
		{
			make_cat<detail::concat_v<SPACE, LIMIT, SPACE, to_string<N>::value>>(this->sql_str_);

			return *this;
		}

		template <std::size_t N>
		select_chain& offset()
		{
			make_cat<detail::concat_v<SPACE, OFFSET, SPACE, to_string<N>::value>>(this->sql_str_);

			return *this;
		}

		template <string_literal... Args>
		select_chain& order_by()
		{
			make_cat<detail::concat_v<SPACE, ORDER, SPACE, BY, SPACE>, bind_param<Args>::value...>(this->sql_str_);

			return *this;
		}

		template <std::size_t... I>
		select_chain& order_by_index()
		{
			make_cat<detail::concat_v<SPACE, ORDER, SPACE, BY, SPACE>, to_string<I>::value...>(this->sql_str_);

			return *this;
		}

		template <string_literal... Args>
		select_chain& group_by()
		{
			make_cat<detail::concat_v<SPACE, GROUP, SPACE, BY, SPACE>, bind_param<Args>::value...>(this->sql_str_);

			return *this;
		}

		template <typename _Attr>
		select_chain& having(_Attr&& attr)
		{
			this->sql_str_ += " having";
			this->sql_str_ += attr.sql();
			return *this;
		}
	};
} // namespace march