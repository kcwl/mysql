#pragma once
#include <algorithm>
#include "mysql/io_service_pool.hpp"
#include <boost/mysql.hpp>
#include <deque>
#include <format>
#include <memory>
#include <mutex>
#include "transaction.hpp"

namespace mysql
{
	template <typename _Service>
	class service_pool
	{
		using service_t = _Service;

		using unique_service_ptr = std::unique_ptr<service_t>;

		using service_ptr = service_t*;

		using transaction_t = transaction<_Service>;

		static constexpr std::size_t connect_number = 2 * 3;

	public:
		template <typename... _Args>
		explicit service_pool(io_service_pool& pool, _Args&&... args)
			: pool_(pool)
		{
			make_service_pool(pool_, std::forward<_Args>(args)...);
		}

		~service_pool() = default;

		bool execute(const std::string& sql)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return false;

			boost::mysql::error_code ec;

			conn_ptr->execute(sql, ec);

			return true;
		}

		template <typename _Func>
		auto async_execute(const std::string& sql, _Func&& f)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return;

			return conn_ptr->async_execute(sql,
				[&, ptr = std::move(conn_ptr), func = std::move(f)](bool value) mutable
				{
					func(std::move(value));
				});
		}

		template <typename _Ty>
		std::vector<_Ty> query(const std::string& sql)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return{};

			boost::mysql::error_code ec;

			std::vector<_Ty> result{};
			conn_ptr->query<_Ty>(sql, result, ec);

			return result;
		}

		template <typename _Ty, typename _Fmt, typename... _Args>
		auto async_pquery(_Fmt&& f, _Args&&... args)
		{
			return async_query<_Ty>(std::format(std::forward<_Fmt>(f), std::forward<_Args>(args)...));
		}

		template <typename _Ty, typename _Func>
		auto async_query(const std::string& sql, _Func&& f)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return;

			return conn_ptr->template async_query<_Ty>(sql,
				[&, ptr = std::move(conn_ptr), func = std::move(f)](const std::vector<_Ty>& value) mutable
				{
					func(value);
				});
		}

		template<typename _Func>
		bool transactions(_Func&& f, transaction_t::isolation_level level = transaction_t::isolation_level::no_repeated_read,
			transaction_t::isolation_scope scope = transaction_t::isolation_scope::current, bool consistant = true)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return false;

			return transaction(conn_ptr, level, scope, consistant).execute(std::forward<_Func>(f));
		}

	private:
		template <typename... _Args>
		void make_service_pool(io_service_pool& pool, _Args&&... args)
		{
			make_param(std::forward<_Args>(args)...);

			for (std::size_t i = 0; i < connect_number; i++)
			{
				free_queue_.push_back(std::make_unique<_Service>(pool.get_io_service(), endpoint_, params_));
			}
		}

		service_ptr get_service()
		{
			std::lock_guard lk(free_mutex_);

			if (free_queue_.empty())
				return nullptr;

			for (auto& service : free_queue_)
			{
				if (!service->idle())
					continue;

				return service.get();
			}

			return nullptr;
		}

		template <typename _Host, typename _Passwd, typename... _Args>
		void make_param(_Host&& host, _Passwd&& psw, _Args&&... args)
		{
			boost::asio::ip::tcp::resolver resolve(pool_.get_io_service());

			endpoint_ = resolve.resolve(host, psw);

			params_.reset(new boost::mysql::handshake_params(std::forward<_Args>(args)...));
		}

		bool check_service(service_ptr service)
		{
			if (service == nullptr)
				service = new _Service(pool_.get_io_service(), endpoint_, params_);

			if (!service->idle())
				return false;

			service->busy();

			return true;
		}

	private:
		io_service_pool& pool_;

		std::vector<unique_service_ptr> free_queue_;

		std::mutex free_mutex_;

		boost::asio::ip::tcp::resolver::results_type endpoint_;

		std::shared_ptr<boost::mysql::handshake_params> params_;
	};
} // namespace mysql