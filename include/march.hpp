#pragma once
#include "march/db_service.hpp"
#include "march/service_pool.hpp"
#include "march/sql.hpp"

namespace
{
	using mysql_pool = march::db_service_pool<march::db_service>;
}

namespace march
{
	template <typename _Ty, string_literal... Args>
	std::vector<_Ty> select(mysql_pool& pool)
	{
		error_code ec;

		return select_chain(pool).select<_Ty, Args...>().query(ec);
	}

	template <typename _Ty, typename _Func>
	auto async_select(mysql_pool& pool, _Func&& f)
	{
		return select_chain(pool).select<_Ty>().async_query(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	auto select_if(mysql_pool& pool, _Attr&& attr)
	{
		error_code ec;

		return select_chain(pool).select<_Ty>().where(std::forward<_Attr>(attr)).query(ec);
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_select_if(mysql_pool& pool, _Attr&& attr, _Func&& f)
	{
		return select_chain(pool)
			.select<_Ty>()
			.where(std::forward<_Attr>(attr))
			.async_query<_Ty>(std::forward<_Func>(f));
	}

	template <typename _Ty>
	auto insert(mysql_pool& pool, _Ty&& t)
	{
		error_code ec;
		return chain_sql(pool).insert(std::forward<_Ty>(t)).execute(ec);
	}

	template <typename _Ty, typename _Func>
	void async_insert(mysql_pool& pool, _Ty&& t, _Func&& f)
	{
		return chain_sql(pool).insert(std::forward<_Ty>(t)).async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty>
	bool remove(mysql_pool& pool)
	{
		error_code ec;
		return chain_sql(pool).remove<_Ty>().execute(ec);
	}

	template <typename _Ty, typename _Func>
	void async_remove(mysql_pool& pool, _Func&& f)
	{
		return chain_sql(pool).remove<_Ty>().async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	auto remove_if(mysql_pool& pool, _Attr&& attr)
	{
		error_code ec;

		return chain_sql(pool).remove<_Ty>().where(std::forward<_Attr>(attr)).execute(ec);
	}

	template <typename _Ty, typename _Attr, typename _Func>
	auto async_remove_if(mysql_pool& pool, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool)
			.remove<_Ty>()
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty>
	auto update(mysql_pool& pool, _Ty&& t)
	{
		error_code ec;
		return chain_sql(pool).update(std::forward<_Ty>(t)).execute(ec);
	}

	template <typename _Ty, typename _Func>
	auto async_update(mysql_pool& pool, _Ty&& t, _Func&& f)
	{
		return chain_sql(pool).update(std::forward<_Ty>(t)).async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	auto update_if(mysql_pool& pool, _Ty&& t, _Attr&& attr)
	{
		error_code ec;

		return chain_sql(pool).update(std::forward<_Ty>(t)).where(std::forward<_Attr>(attr)).execute(ec);
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_update_if(mysql_pool& pool, _Ty&& t, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool)
			.update(std::forward<_Ty>(t))
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty>
	auto replace(mysql_pool& pool, _Ty&& t)
	{
		error_code ec;

		return chain_sql(pool).update(std::forward<_Ty>(t)).execute(ec);
	}

	template <typename _Ty, typename _Func>
	void async_replace(mysql_pool& pool, _Ty&& t, _Func&& f)
	{
		return chain_sql(pool).update(std::forward<_Ty>(t)).async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	auto replace_if(mysql_pool& pool, _Ty&& t, _Attr&& attr)
	{
		error_code ec;

		return chain_sql(pool).update(std::forward<_Ty>(t)).where(std::forward<_Attr>(attr)).execute(ec);
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_replace_if(mysql_pool& pool, _Ty&& t, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool)
			.update(std::forward<_Ty>(t))
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}
} // namespace march
