#pragma once
#include "algorithm.hpp"
#include "error_code.hpp"
#include "io_service_pool.hpp"
#include "results.hpp"
#include <boost/mysql/tcp_ssl.hpp>
#include <string>
#include <vector>

namespace march
{
	class db_service final
	{
	public:
		template <typename _Endpoint, typename _Param>
		explicit db_service(boost::asio::io_service& ios, _Endpoint&& host, _Param&& param)
			: io_service_(ios)
			, ssl_ctx_(boost::asio::ssl::context::tls_client)
			, mysql_ptr_(new boost::mysql::tcp_ssl_connection(io_service_, ssl_ctx_))
			, endpoint_(host)
			, params_(param)
		{
			run();
		}

		~db_service()
		{
			close();
		}

	public:
		void close()
		{
			boost::mysql::error_code ec;

			boost::mysql::diagnostics dg;

			mysql_ptr_->quit(ec, dg);

			mysql_ptr_->close(ec, dg);
		}

		results execute(const std::string& sql, error_code& ec)
		{
			boost::mysql::results result{};
			boost::mysql::diagnostics diag{};

			mysql_ptr_->execute(sql, result, ec, diag);

			has_busy_ = false;

			return results(std::move(result));
		}

		template <typename _Func>
		auto async_execute(std::string_view sql, _Func&& f)
		{
			boost::mysql::results result{};

			return mysql_ptr_->async_execute(sql, result,
				[&, func = std::move(f)](const error_code& ec)
				{
					func(results(result), ec);

					has_busy_ = false;
				});
		}

		results query(const std::string& sql, error_code& ec)
		{
			boost::mysql::results result{};
			boost::mysql::diagnostics diag{};

			mysql_ptr_->query(sql, result, ec, diag);

			has_busy_ = false;

			return results(result);
		}

		template <typename _Ty, typename _Func>
		void async_query(const std::string& sql, _Func&& f)
		{
			boost::mysql::results res{};

			return mysql_ptr_->async_query(sql, res,
				[&, func = std::move(f)](const error_code& ec) mutable
				{
					func(results(res), ec);

					has_busy_ = false;
				});
		}

		void busy()
		{
			has_busy_ = true;
		}

		bool idle() const
		{
			return !has_busy_;
		}

	private:
		void run()
		{
			mysql_ptr_->async_connect(*endpoint_.begin(), *params_, 
				[](auto ec) 
				{
					if (ec)
					{
						std::cout << "maybe connect mysql occur error: " << ec.message() << std::endl;
					}
				});
		}

	private:
		boost::asio::io_service& io_service_;

		boost::asio::ssl::context ssl_ctx_;

		std::unique_ptr<boost::mysql::tcp_ssl_connection> mysql_ptr_;

		boost::asio::ip::tcp::resolver::results_type endpoint_;

		std::shared_ptr<boost::mysql::handshake_params> params_;

		bool has_busy_;
	};

	constexpr static auto default_port_string = boost::mysql::default_port_string;

} // namespace march