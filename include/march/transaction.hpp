#pragma once
#include <string>
#include <vector>
#include <memory>

namespace march
{
	namespace detail
	{
		template<typename _Func>
		struct init_execute
		{
			void operator()(_Func f)
			{
				f();
			}
		};
	}

	template<typename _Service>
	class sql_transaction
	{
		class transaction_guard
		{
		public:
			transaction_guard(sql_transaction& trans)
				: trans_(trans)
			{
				trans_.start();
			}

			~transaction_guard()
			{
				trans_.close();
			}

		private:
			sql_transaction& trans_;
		};

		friend class sql_transaction::transaction_guard;

	public:
		enum class isolation_level
		{
			read_no_commit,
			read_commit,
			no_repeated_read,
			serialization
		};

		enum class isolation_scope
		{
			current,
			session,
			global
		};

	public:
		explicit sql_transaction(_Service* conn_ptr, isolation_level level, isolation_scope scope, bool consistant = true)
			: conn_service_ptr_(conn_ptr)
			, finish_(true)
		{
			std::stringstream ss{};

			ss << "set ";

			if (scope == isolation_scope::global)
				ss << "global ";
			else if (scope == isolation_scope::session)
				ss << "session ";

			ss << "transcation isolation level ";

			switch (level)
			{
			case isolation_level::read_no_commit:
				ss << "read uncommited";
				break;
			case isolation_level::read_commit:
				ss << "read committed";
				break;
			case isolation_level::no_repeated_read:
				ss << "repeatable read";
				break;
			case isolation_level::serialization:
				ss << "serializable";
				break;
			default:
				break;
			}

			if (consistant)
				ss << " with consistant snapshot";

			ss << ";";

			error_code ec;

			if (conn_service_ptr_)
				conn_service_ptr_->execute(ss.str(), ec);
		}

	public:
		template<typename _Func>
		bool execute(_Func&& f)
		{
			transaction_guard lk(*this);

			return finish_ = f();
		}

		template<typename _Func, typename _Token>
		void async_execute([[maybe_unused]]_Func&& f, [[maybe_unused]]_Token&& token)
		{
			//return boost::asio::async_initiate<_Token, void(error_code)>(detail::init_execute<_Func>(std::move(f)), std::forward<_Token>(token));
		}

	private:
		void start()
		{
			error_code ec;

			if (!conn_service_ptr_)
				return;

			conn_service_ptr_->execute("begin", ec);
		}

		void commit()
		{
			error_code ec;

			if (!conn_service_ptr_)
				return;

			conn_service_ptr_->execute("commit", ec);
		}

		void roll_back()
		{
			error_code ec;

			if (!conn_service_ptr_)
				return;

			conn_service_ptr_->execute("rollback", ec);
		}

		void close()
		{
			finish_ ? commit() : roll_back();

			finish_ = true;
		}

	private:
		_Service* conn_service_ptr_;

		bool finish_;
	};
}