#pragma once
#include <mysql/io_service_pool.hpp>
#include <mysql/algorithm.hpp>
#include <boost/mysql.hpp>
#include <string>
#include <vector>

namespace mysql
{
	class mysql_connect final
	{
	public:
		template <typename _Endpoint, typename _Param>
		explicit mysql_connect(boost::asio::io_service& ios, _Endpoint&& host, _Param&& param)
			: io_service_(ios)
			, ssl_ctx_(boost::asio::ssl::context::tls_client)
			, mysql_ptr_(new boost::mysql::tcp_ssl_connection(io_service_, ssl_ctx_))
			, endpoint_(host)
			, params_(param)
		{
			run();
		}

		~mysql_connect()
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

		void set_charset(const std::string& charset = "utf8mb4")
		{
			boost::mysql::results result;

			mysql_ptr_->async_execute("SET NAMES " + charset, result,
				[this, charset](auto)
				{
				});
		}

		bool execute(const std::string& sql, boost::mysql::error_code& ec)
		{
			boost::mysql::results result{};
			boost::mysql::diagnostics diag{};

			mysql_ptr_->execute(sql, result, ec, diag);

			has_busy_ = false;

			return result.has_value();
		}

		template <typename _Func>
		auto async_excute(std::string_view sql, _Func&& f)
		{
			boost::mysql::results result{};

			return mysql_ptr_->async_execute(sql, result,
				[&](const boost::mysql::error_code& ec)
				{
					if (ec)
					{
						f(false);
						return;
					}

					f(true);

					has_busy_ = false;
				});
		}

		template <typename _Ty>
		bool query(const std::string& sql, std::vector<_Ty>& t, boost::mysql::error_code& ec)
		{
			boost::mysql::results result{};
			boost::mysql::diagnostics diag{};

			mysql_ptr_->query(sql, result, ec, diag);

			if (!result.has_value())
				return false;

			t = make_result<_Ty>(result);

			has_busy_ = false;

			return true;
		}

		template <typename _Ty, typename _Func>
		auto async_query(const std::string& sql, _Func&& f)
		{
			boost::mysql::results result{};

			return mysql_ptr_->async_query(sql, result,
				[&, func = std::move(f)](const boost::mysql::error_code& ec) mutable
				{
					if (ec)
					{
						return;
					}

					func(make_result<_Ty>(result));

					has_busy_ = false;
				});
		}

		void busy()
		{
			has_busy_ = true;
		}

		bool idle()
		{
			return !has_busy_;
		}

	private:
		void run()
		{
			mysql_ptr_->async_connect(*endpoint_.begin(), *params_,
				[this](const boost::mysql::error_code& ec)
				{
					if (ec)
					{
						return;
					}

					// set_charset();
				});
		}

		template <typename _Ty>
		std::vector<_Ty> make_result(const boost::mysql::results& result)
		{
			std::vector<_Ty> results{};

			if (!result.has_value())
				return results;

			for (auto column : result.rows())
			{
				results.push_back(to_struct<_Ty>(column));
			}

			return results;
		}

	private:
		boost::asio::io_service& io_service_;

		boost::asio::ssl::context ssl_ctx_;

		std::unique_ptr<boost::mysql::tcp_ssl_connection> mysql_ptr_;

		boost::asio::ip::tcp::resolver::results_type endpoint_;

		std::shared_ptr<boost::mysql::handshake_params> params_;

		bool has_busy_;
	};
} // namespace mysql