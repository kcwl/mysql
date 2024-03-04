#pragma once
#include <boost/test/unit_test_suite.hpp>
#include <chrono>
#include <march.hpp>

using namespace std::chrono_literals;

using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(march)

struct products
{
	int prod_id;
	std::string prod_name;
	int prod_price;
	int vend_id;
};

BOOST_AUTO_TEST_CASE(sync)
{
	march::io_service_pool io_pool{ 5 };

	march::db_service_pool<march::db_service> pool(io_pool, "172.26.4.15", march::default_port_string, "kcwl", "123456",
												   "test_mysql");

	std::thread t([&] { io_pool.run(); });

	auto result = march::insert(pool, products{ 1, "pro", 2, 3 });

	BOOST_CHECK_EQUAL(result.affected_rows(), 1);

	auto select_result = march::select_if<products>(pool, MAR_EXPR(prod_id) == 1).to_vector<products>();

	if (!select_result.empty())
	{
		auto& last_one = select_result.back();

		BOOST_CHECK_EQUAL(last_one.prod_id, 1);
		BOOST_CHECK_EQUAL(last_one.prod_name, "pro");
		BOOST_CHECK_EQUAL(last_one.prod_price, 2);
		BOOST_CHECK_EQUAL(last_one.vend_id, 3);
	}

	auto remove_result = march::remove_if<products>(pool, MAR_EXPR(prod_id) == 1);

	BOOST_CHECK_EQUAL(remove_result.affected_rows(), 1);

	using trans_t = transaction<march::db_service>;

	auto res = pool.transactions(
		[&]
		{
			auto result1 = march::insert(pool, products{ 2, "pro", 2, 3 });

			if (!result1.affected_rows())
				return false;

			auto result2 = march::insert(pool, products{ 3, "pro", 2, 3 });

			if (!result2.affected_rows())
				return false;

			auto result3 = march::insert(pool, products{ 4, "pro", 2, 3 });

			if (!result3.affected_rows())
				return false;

			return true;
		});

	BOOST_CHECK(res == true);

	std::this_thread::sleep_for(1s);

	io_pool.stop();
	t.join();
}

BOOST_AUTO_TEST_CASE(async)
{
	march::io_service_pool io_pool{ 5 };

	march::db_service_pool<march::db_service> pool(io_pool, "172.26.4.15", march::default_port_string, "kcwl", "123456",
												   "test_mysql");

	std::thread t([&] { io_pool.run(); });

	auto result = march::insert(pool, products{ 1, "pro", 2, 3 });

	BOOST_CHECK_EQUAL(result.affected_rows(), 1);

	march::async_insert(pool, products{ 1, "pro", 2, 3 },
						[](auto result, auto ec)
						{
							BOOST_CHECK(!ec);
							BOOST_CHECK_EQUAL(result.affected_rows(), 1);
						});

	march::async_select_if<products>(pool, MAR_EXPR(prod_id) == 1,
									 [](auto result, auto ec)
									 {
										 BOOST_CHECK(!ec);

										 auto& top_one = result.top<1, products>().back();

										 BOOST_CHECK_EQUAL(top_one.prod_id, 1);
										 BOOST_CHECK_EQUAL(top_one.prod_name, "pro");
										 BOOST_CHECK_EQUAL(top_one.prod_price, 2);
										 BOOST_CHECK_EQUAL(top_one.vend_id, 3);
									 });

	march::async_remove_if<products>(pool, MAR_EXPR(prod_id) == 1,
									 [](auto result, auto ec)
									 {
										 BOOST_CHECK(!ec);

										 BOOST_CHECK_EQUAL(result.affected_rows(), 1);
									 });

	std::this_thread::sleep_for(1s);

	using trans_t = transaction<march::db_service>;

	pool.async_transactions(
		[&]()
		{
			auto result1 = march::insert(pool, products{ 2, "pro", 2, 3 });

			if (!result1.affected_rows())
				return false;

			auto result2 = march::insert(pool, products{ 3, "pro", 2, 3 });

			if (!result2.affected_rows())
				return false;

			auto result3 = march::insert(pool, products{ 4, "pro", 2, 3 });

			if (!result3.affected_rows())
				return false;

			return true;
		},
		[&](auto result, auto ec)
		{
			BOOST_CHECK(!ec);

			BOOST_CHECK(result.affected_rows() == 3);
		});

	std::this_thread::sleep_for(1s);

	io_pool.stop();
	t.join();
}

BOOST_AUTO_TEST_CASE(sql)
{
	march::io_service_pool io_pool{ 5 };

	march::db_service_pool<march::db_service> pool(io_pool, "172.26.4.15", march::default_port_string, "kcwl", "123456",
												   "test_mysql");

	{
		using mysql_sql = march::select_chain<march::db_service>;

		auto sql = mysql_sql(pool).select<products, SQL_BIND(prod_name)>().sql();

		BOOST_CHECK_EQUAL(sql, "select prod_name from products");

		sql = mysql_sql(pool).select<products, SQL_BIND(prod_id, prod_name, prod_price)>().sql();

		BOOST_CHECK_EQUAL(sql, "select prod_id, prod_name, prod_price from products");

		BOOST_CHECK_EQUAL(mysql_sql(pool).select<products>().sql(), "select * from products");

		sql = mysql_sql(pool).select_distinct<products, SQL_BIND(vend_id)>().sql();
		BOOST_CHECK_EQUAL(sql, "select distinct vend_id from products");

		sql = mysql_sql(pool).select_top<products, 5, SQL_BIND(prod_name)>().sql();

		BOOST_CHECK_EQUAL(sql, "select top 5 prod_name from products");

		sql = mysql_sql(pool).select<products, SQL_BIND(prod_name)>().limit<5>().sql();
		BOOST_CHECK_EQUAL(sql, "select prod_name from products limit 5");

		sql = mysql_sql(pool).select<products, SQL_BIND(prod_name)>().limit<5>().offset<5>().sql();
		BOOST_CHECK_EQUAL(sql, "select prod_name from products limit 5 offset 5");

		sql = mysql_sql(pool).select<products, SQL_BIND(prod_name)>().order_by<SQL_BIND(prod_name)>().sql();
		BOOST_CHECK_EQUAL(sql, "select prod_name from products order by prod_name");

		sql = mysql_sql(pool)
				  .select<products, SQL_BIND(prod_id, prod_price, prod_name)>()
				  .order_by<SQL_BIND(prod_price, prod_name)>()
				  .sql();
		BOOST_CHECK_EQUAL(sql, "select prod_id, prod_price, prod_name from products order by prod_price, prod_name");

		sql = mysql_sql(pool).select<products, SQL_BIND(prod_id, prod_price, prod_name)>().order_by_index<2, 3>().sql();
		BOOST_CHECK_EQUAL(sql, "select prod_id, prod_price, prod_name from products order by 2, 3");

		sql = mysql_sql(pool).select<products, SQL_BIND(vend_id)>().group_by<SQL_BIND(vend_id)>().sql();
		BOOST_CHECK_EQUAL(sql, "select vend_id from products group by vend_id");

		sql = mysql_sql(pool)
				  .select<products, SQL_BIND(vend_id)>()
				  .group_by<SQL_BIND(vend_id)>()
				  .having(MAR_EXPR(vend_id) > 2)
				  .sql();
		BOOST_CHECK_EQUAL(sql, "select vend_id from products group by vend_id having vend_id > 2");

		sql = mysql_sql(pool)
				  .select<products, SQL_BIND(prod_name, prod_price)>()
				  .where(MAR_EXPR(prod_price) == 3.49)
				  .sql();

		BOOST_CHECK_EQUAL(sql, "select prod_name, prod_price from Products where prod_price = 3.49");

		sql = mysql_sql(pool)
				  .select<products, SQL_BIND(prod_name, prod_price)>()
				  .where(MAR_EXPR(prod_name) != "3.49" | MAR_EXPR(prod_name) <= "3.91")
				  .sql();

		BOOST_CHECK_EQUAL(
			sql, "select prod_name, prod_price from products where prod_name != '3.49' or prod_name <= '3.91'");
	}

	{
		using mysql_sql = march::chain_sql<march::db_service>;

		BOOST_CHECK_EQUAL(mysql_sql(pool).insert(products{ 1, "peter", 2, 3 }).sql(),
						  "insert into products values(1,'peter',2,3)");
	}

	{
		using mysql_sql = march::chain_sql<march::db_service>;

		BOOST_CHECK_EQUAL(mysql_sql(pool).remove<products>().sql(), "delete from products");
	}

	{
		using mysql_sql = march::chain_sql<march::db_service>;

		BOOST_CHECK_EQUAL(mysql_sql(pool).update(products{ 1, "candy", 3, 5 }).sql(),
						  "update person set age = 1 and name = 'candy'");
	}

	{
		using mysql_sql = march::chain_sql<march::db_service>;

		BOOST_CHECK_EQUAL(mysql_sql(pool).replace(products{ 1, "ridy", 6, 7 }).sql(),
						  "replace into products values(1,'ridy',6,7)");
	}
}

BOOST_AUTO_TEST_CASE(transactions)
{
	using trans_t = transaction<march::db_service>;

	auto result = trans_t(nullptr, trans_t::isolation_level::no_repeated_read, trans_t::isolation_scope::current)
					  .execute([]() { return true; });

	BOOST_CHECK(result == false);
}

BOOST_AUTO_TEST_SUITE_END()