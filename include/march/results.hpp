#pragma once
#include "algorithm.hpp"
#include <boost/mysql/results.hpp>
#include <vector>

namespace march
{
	class results final
	{
	public:
		explicit results(boost::mysql::results mysql_result)
			: result_(mysql_result)
		{}

		~results() = default;

	public:
		results(const results& other)
			: result_(other.result_)
		{

		}

		results& operator=(const results& other)
		{
			if (&other != this)
			{
				result_ = other.result_;
			}

			return *this;
		}

	public:
		template <typename _Ty>
		std::vector<_Ty> to_vector()
		{
			return make_result<_Ty>();
		}

		template <typename _Ty>
		_Ty top_one()
		{
			return top<1, _Ty>();
		}

		template <std::size_t N, typename _Ty>
		std::vector<_Ty> top()
		{
			auto result_size = result_.rows().size();

			N < result_size ? result_size = N : 0;

			return make_result<_Ty>(result_size);
		}

		std::size_t last_id() const
		{
			return result_.last_insert_id();
		}

		std::size_t affected_rows()
		{
			if (!result_.has_value())
				return 0;

			return result_.affected_rows();
		}

	private:
		template <typename _Ty>
		std::vector<_Ty> make_result(std::size_t result_size = 0)
		{
			std::vector<_Ty> results{};

			if (!result_.has_value())
				return results;

			result_size == 0 ? result_size = result_.rows().size() : 0;

			for (std::size_t i = 0; i < result_size; ++i)
			{
				auto column = result_.rows().at(i);

				results.push_back(detail::to_struct<_Ty>(column));
			}

			return results;
		}

	private:
		boost::mysql::results result_;
	};
} // namespace march