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
	template <typename _Service, template<typename> typename _Transaction>
	class service_pool
	{
	public:
		using service_t = _Service;

		using transaction_t = _Transaction<service_t>;

	public:
		service_pool() = default;

	public:
		virtual results execute(std::string_view, error_code&)
		{
			return results{ boost::mysql::results{} };
		}

		virtual void async_execute(std::string_view, std::function<void(results, error_code)>)
		{
			return;
		}

		virtual bool transactions(transaction_t::isolation_level, transaction_t::isolation_scope, bool,
								  std::function<bool()>)
		{
			return false;
		}

		virtual void async_transaction(transaction_t::isolation_level, transaction_t::isolation_scope, bool,
									   std::function<bool()>, std::function<void(results, error_code)>)
		{
			return;
		}

	protected:
		template <typename _Host, typename _Passwd, typename... _Args>
		void make_param(boost::asio::io_service& io_service, _Host&& host, _Passwd&& psw, _Args&&... Args)
		{
			boost::asio::ip::tcp::resolver resolve(io_service);

			endpoint_ = resolve.resolve(host, psw);

			params_.reset(new boost::mysql::handshake_params(std::forward<_Args>(Args)...));
		}

	protected:
		boost::asio::ip::tcp::resolver::results_type endpoint_;

		std::shared_ptr<boost::mysql::handshake_params> params_;
	};

	template <typename _Service, template<typename> typename _Transaction>
	class db_sync_pool : public service_pool<_Service, _Transaction>
	{
		using base_type = service_pool<_Service, _Transaction>;

		using service_ptr = typename base_type::service_t*;

		using unique_service_ptr = std::unique_ptr<typename base_type::service_t>;

		static constexpr std::size_t connect_number = 2 * 3;

	public:
		template <typename... _Args>
		explicit db_sync_pool(io_service_pool& pool, _Args&&... Args)
			: pool_(pool)

		{
			make_service_pool(pool_, std::forward<_Args>(Args)...);
		}

		~db_sync_pool() = default;

	public:
		void stop()
		{
			for (auto& service : connectors_)
			{
				if (!service)
					continue;

				service->close();
			}
		}

		virtual results execute(std::string_view sql, error_code& ec) override
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return results{ boost::mysql::results{} };

			return conn_ptr->execute(sql, ec);
		}

		virtual bool transactions(base_type::transaction_t::isolation_level level, base_type::transaction_t::isolation_scope scope, bool consistant,
						  std::function<bool()> f) override
		{
			auto conn_ptr = get_service();

			if (!check_service(conn_ptr))
				return false;

			return base_type::transaction_t(conn_ptr, level, scope, consistant).execute(std::move(f));
		}

	private:
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

		template <typename... _Args>
		void make_service_pool(io_service_pool& pool, _Args&&... Args)
		{
			this->make_param(pool_.get_io_service(), std::forward<_Args>(Args)...);

			for (std::size_t i = 0; i < connect_number; i++)
			{
				auto service_ptr = std::make_unique<_Service>(pool.get_io_service(), this->endpoint_, this->params_);

				service_ptr->connect();

				connectors_.push_back(std::move(service_ptr));
			}
		}

		bool check_service(service_ptr service)
		{
			if (service == nullptr)
				service = new _Service(pool_.get_io_service(), this->endpoint_, this->params_);

			if (!service->idle())
				return false;

			service->busy();

			return true;
		}

	private:
		io_service_pool& pool_;

		std::vector<unique_service_ptr> connectors_;

		std::mutex free_mutex_;
	};

	template <typename _Service, template<typename> typename _Transaction>
	class db_async_pool : public service_pool<_Service, _Transaction>
	{
		using base_type = service_pool<_Service, _Transaction>;

		using service_ptr_t = std::unique_ptr<typename base_type::service_t>;

	public:
		template <typename... _Args>
		db_async_pool(boost::asio::io_service& io_service, _Args&&... Args)
			: io_service_(io_service)
			, service_ptr_(nullptr)
		{
			make_service(std::forward<_Args>(Args)...);
		}

	public:
		void stop()
		{
			service_ptr_->async_close(
				[](error_code ec)
				{
					if (ec)
					{
						std::cout << "maybe async close occur error: " << ec.message() << std::endl;
					}
				});
		}

		virtual void async_execute(std::string_view sql, std::function<void(results, error_code)> f) override
		{
			return service_ptr_->async_execute(sql, std::move(f));
		}

		void async_transaction(base_type::transaction_t::isolation_level level, base_type::transaction_t::isolation_scope scope,
							   bool consistant, std::function<bool()> f, std::function<void(results, error_code)> token)
		{
			return base_type::transaction_t(service_ptr_.get(), level, scope, consistant)
				.async_execute(std::move(f), std::move(token));
		}

	private:
		template<typename... _Args>
		void make_service(_Args&&... Args)
		{
			this->make_param(io_service_, std::forward<_Args>(Args)...);

			service_ptr_ = std::move(std::make_unique<_Service>(io_service_, this->endpoint_, this->params_));

			service_ptr_->async_connect(
				[](error_code ec)
				{
					if (ec)
					{
						std::cout << "maybe async connect occur error: " << ec.message() << std::endl;
					}
				});
		}

	private:
		boost::asio::io_service& io_service_;

		service_ptr_t service_ptr_;
	};
} // namespace march