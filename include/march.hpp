#pragma once
#include "march/db_service.hpp"
#include "march/service_pool.hpp"
#include "march/sql.hpp"
#include "march/transaction.hpp"

namespace
{
	using transaction_t = march::sql_transaction<march::db_service>;
} // namespace

namespace march
{
	using sync_pool = march::db_sync_pool<march::db_service, march::sql_transaction>;

	using async_pool = march::db_async_pool<march::db_service, march::sql_transaction>;

	template <typename _Ty>
	std::size_t insert(sync_pool& pool, _Ty&& t)
	{
		error_code ec;

		auto result = chain_sql(pool).insert(std::forward<_Ty>(t)).execute(ec);

		if (ec)
			return 0;

		return result.last_id();
	}

	template <typename _Ty, typename _Func>
	void async_insert(async_pool& pool, _Ty&& t, _Func&& f)
	{
		return chain_sql(pool).insert(std::forward<_Ty>(t)).async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty>
	std::size_t remove(sync_pool& pool)
	{
		error_code ec;

		auto result = chain_sql(pool).remove<_Ty>().execute(ec);

		if (ec)
			return 0;

		return result.affected_rows();
	}

	template <typename _Ty, typename _Func>
	void async_remove(async_pool& pool, _Func&& f)
	{
		return chain_sql(pool).remove<_Ty>().async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	std::size_t remove_if(sync_pool& pool, _Attr&& attr)
	{
		error_code ec;

		auto result = chain_sql(pool).remove<_Ty>().where(std::forward<_Attr>(attr)).execute(ec);

		if (ec)
			return 0;

		return result.affected_rows();
	}

	template <typename _Ty, typename _Attr, typename _Func>
	auto async_remove_if(async_pool& pool, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool).remove<_Ty>().where(std::forward<_Attr>(attr)).async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	std::size_t update_if(sync_pool& pool, _Ty&& t, _Attr&& attr)
	{
		error_code ec;

		auto result = chain_sql(pool).update(std::forward<_Ty>(t)).where(std::forward<_Attr>(attr)).execute(ec);

		if (ec)
			return 0;

		return result.affected_rows();
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_update_if(async_pool& pool, _Ty&& t, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool)
			.update(std::forward<_Ty>(t))
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	std::size_t replace_if(sync_pool& pool, _Ty&& t, _Attr&& attr)
	{
		error_code ec;

		auto result = chain_sql(pool).update(std::forward<_Ty>(t)).where(std::forward<_Attr>(attr)).execute(ec);

		if (ec)
			return 0;

		return result.affected_rows();
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_replace_if(async_pool& pool, _Ty&& t, _Attr&& attr, _Func&& f)
	{
		return chain_sql(pool)
			.update(std::forward<_Ty>(t))
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, string_literal... Args>
	std::vector<_Ty> select(sync_pool& pool)
	{
		error_code ec;

		auto result = select_chain(pool).select<_Ty, Args...>().execute(ec);

		if (ec)
			return {};

		return result.to_vector<_Ty>();
	}

	template <typename _Ty, typename _Func>
	void async_select(async_pool& pool, _Func&& f)
	{
		return select_chain(pool).select<_Ty>().async_execute(std::forward<_Func>(f));
	}

	template <typename _Ty, typename _Attr>
	std::vector<_Ty> select_if(sync_pool& pool, _Attr&& attr)
	{
		error_code ec;

		auto result = select_chain(pool).select<_Ty>().where(std::forward<_Attr>(attr)).execute(ec);

		if (ec)
			return {};

		return result.to_vector<_Ty>();
	}

	template <typename _Ty, typename _Attr, typename _Func>
	void async_select_if(async_pool& pool, _Attr&& attr, _Func&& f)
	{
		return select_chain(pool)
			.select<_Ty>()
			.where(std::forward<_Attr>(attr))
			.async_execute(std::forward<_Func>(f));
	}

	template <typename _Func>
	bool transaction(sync_pool& pool, _Func&& f,
					 transaction_t::isolation_level level = transaction_t::isolation_level::no_repeated_read,
					 transaction_t::isolation_scope scope = transaction_t::isolation_scope::current,
					 bool consistant = true)
	{
		pool.transactions(level, scope, consistant, std::forward<_Func>(f));
	}

	template <typename _Func>
	bool async_transaction(async_pool& pool, _Func&& f,
						   transaction_t::isolation_level level = transaction_t::isolation_level::no_repeated_read,
						   transaction_t::isolation_scope scope = transaction_t::isolation_scope::current,
						   bool consistant = true)
	{
		pool.async_transaction(level, scope, consistant, std::forward<_Func>(f));
	}

} // namespace march
