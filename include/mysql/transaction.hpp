#pragma once
#include <string>
#include <vector>
#include <memory>

namespace mysql
{
	template<typename _Service>
	class transaction
	{
		class transaction_guard
		{
		public:
			transaction_guard(transaction& trans)
				: trans_(trans)
			{

			}

			~transaction_guard()
			{
				trans_.close();
			}

		private:
			transaction& trans_;
		};

		friend class transaction::transaction_guard;
		
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
		explicit transaction(_Service* conn_ptr, isolation_level level, isolation_scope scope, bool consistant = true)
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

			boost::mysql::error_code ec;
			
			if(conn_service_ptr_)
				conn_service_ptr_->execute(ss.str(), ec);
		}

	public:
		template<typename _Func>
		bool execute(_Func&& f)
		{
			transaction_guard lk(*this);

			return finish_ = f();
		}

	private:
		void commit()
		{
			boost::mysql::error_code ec;

			if (!conn_service_ptr_)
				return;

			conn_service_ptr_->execute("commit", ec);
		}

		void roll_back()
		{
			boost::mysql::error_code ec;

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