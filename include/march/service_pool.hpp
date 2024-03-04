#pragma once
#include "error_code.hpp"
#include "io_service_pool.hpp"
#include "results.hpp"
#include "transaction.hpp"
#include <algorithm>
#include <format>
#include <memory>
#include <mutex>
#include <vector>

namespace march
{
	template <typename _Service>
	class db_service_pool
	{
		using service_t = _Service;

		using unique_service_ptr = std::unique_ptr<service_t>;

		using service_ptr = service_t*;

		using transaction_t = transaction<_Service>;

		static constexpr std::size_t connect_number = 2 * 3;

	public:
		template <typename... _Args>
		explicit db_service_pool(io_service_pool& pool, _Args&&... Args)
			: pool_(pool)
		{
			make_service_pool(pool_, std::forward<_Args>(Args)...);
		}

		~db_service_pool() = default;

		results execute(const std::string& sql, error_code& ec)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return results{ boost::mysql::results{} };

			return conn_ptr->execute(sql, ec);
		}

		template <typename _Func>
		void async_execute(const std::string& sql, _Func&& f)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return;

			return conn_ptr->async_execute(sql, std::forward<_Func>(f));
		}

		results query(const std::string& sql, error_code& ec)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return results{ boost::mysql::results{} };

			return conn_ptr->query(sql, ec);
		}

		template <typename _Ty, typename _Func>
		void async_query(const std::string& sql, _Func&& f)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return;

			return conn_ptr->template async_query<_Ty>(sql, std::forward<_Func>(f));
		}

		template <typename _Func>
		bool transactions(_Func&& f,
						  transaction_t::isolation_level level = transaction_t::isolation_level::no_repeated_read,
						  transaction_t::isolation_scope scope = transaction_t::isolation_scope::current,
						  bool consistant = true)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return false;

			return transaction(conn_ptr, level, scope, consistant).execute(std::forward<_Func>(f));
		}

		template <typename _Func, typename _Token>
		void async_transactions(
			_Func&& f, _Token&& token,
			transaction_t::isolation_level level = transaction_t::isolation_level::no_repeated_read,
			transaction_t::isolation_scope scope = transaction_t::isolation_scope::current, bool consistant = true)
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return;

			return transaction(conn_ptr, level, scope, consistant)
				.async_execute(std::forward<_Func>(f), std::forward<_Token>(token));
		}

	private:
		template <typename... _Args>
		void make_service_pool(io_service_pool& pool, _Args&&... Args)
		{
			make_param(std::forward<_Args>(Args)...);

			for (std::size_t i = 0; i < connect_number; i++)
			{
				connectors_.push_back(std::make_unique<_Service>(pool.get_io_service(), endpoint_, params_));
			}
		}

		service_ptr get_service()
		{
			std::lock_guard lk(free_mutex_);

			if (connectors_.empty())
				return nullptr;

			for (auto& service : connectors_)
			{
				if (!service->idle())
					continue;

				return service.get();
			}

			return nullptr;
		}

		template <typename _Host, typename _Passwd, typename... _Args>
		void make_param(_Host&& host, _Passwd&& psw, _Args&&... Args)
		{
			boost::asio::ip::tcp::resolver resolve(pool_.get_io_service());

			endpoint_ = resolve.resolve(host, psw);

			params_.reset(new boost::mysql::handshake_params(std::forward<_Args>(Args)...));
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

		std::vector<unique_service_ptr> connectors_;

		std::mutex free_mutex_;

		boost::asio::ip::tcp::resolver::results_type endpoint_;

		std::shared_ptr<boost::mysql::handshake_params> params_;
	};
} // namespace march