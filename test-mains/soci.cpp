#include <ctime>
#include <optional>
#include <string>

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include <soci/soci.h>
#include <soci/boost-tuple.h>
#include <soci/boost-optional.h>
#include <soci/postgresql/soci-postgresql.h>


int main()
{
	using namespace soci;

	session ses(postgresql, "user=test dbname=test");
	
	{
		std::tm tm = { 0 };
		::strptime("2024-11-25T15:25:09", "%Y-%m-%dT%H:%M:%S", &tm);
		
		auto t = mktime(&tm);
		auto tp = std::chrono::system_clock::from_time_t(t);
		std::tm dt, dt2, dtpar = tm;
		
		long id = 2;
		int res = -1;
		ses << "select id, dt from dttest where dt = :dt", soci::into(id), soci::into(dt), soci::use(dtpar);
		ses << "select id, dt from dttest where dt between :dt and :dt", soci::into(id), soci::into(dt), soci::use(dtpar), soci::use(dtpar);
		ses << "select id, dt from dttest where id >= :id and id <= :id", soci::into(id), soci::into(dt), soci::use(id), soci::use(id);
		
		ses << "select id, dt, :dt, (case when EXTRACT(MONTH FROM COALESCE(dt, :dt)) = EXTRACT(MONTH FROM cast(:dt as date)) then 1 else 0 end) from dttest"
			   " where EXTRACT(MONTH FROM dt) = EXTRACT(MONTH FROM cast((CASE WHEN :dt = :dt THEN :dt ELSE :dt END) as DATE) )"
				, soci::use(dtpar, "dt")
				, soci::into(id), soci::into(dt), soci::into(dt2), soci::into(res);
		
		char buffer[256];
		std::strftime(buffer, 256, "%Y-%m-%dT%H:%M:%S", &dt);
		
		fmt::println("id = {}, res = {}, dt = {:%F %T}, dt2 = {:%F %T}", id, res, dt, dt2);
	}
	
	{
		soci::rowset<soci::row> rs = ses.prepare << "select * from numtest";
			
		for (auto & row : rs)
		{
			auto colprop1 = row.get_properties(0);
			auto colprop2 = row.get_properties(1);
			
			auto dt1 = colprop1.get_data_type();
			auto db1 = colprop1.get_db_type();
			
			auto dt2 = colprop2.get_data_type();
			auto db2 = colprop2.get_db_type();
			
			fmt::println("dt1 = {}; db1 = {}; name = {}", fmt::underlying(dt1), fmt::underlying(db1), colprop1.get_name());
			fmt::println("dt1 = {}; db1 = {}; name = {}", fmt::underlying(dt2), fmt::underlying(db2), colprop2.get_name());
			
			auto id  = row.get<long long>(0);
			auto num = row.get<double>(1);
			
			fmt::println("id = {}, num = {}", id, num);
		}
	}
	
	{
		soci::rowset<boost::tuple<std::string, std::string>> rs = ses.prepare << "select * from numtest";
		for (auto & row : rs)
		{
			auto [id, num] = row;
			fmt::println("id = {}, num = {}", id, num);
		}
	}
	
	{
		std::optional<std::string> null;
		std::string res, data;
		data = "abc";
		ses << "call echo_proc(:s1, :s2)", use(data), use(null), into(res);
		
		fmt::println("res = {}, null = {}", res, null);
	}
	
	{
		std::string res;
		std::string data = "abc";
		ses << "select echo(:txt)", use(data, "txt"), into(res);
		
		fmt::println("res = {}", res);
	}
	
	{
		std::string sres;
		boost::optional<std::string> ores;
		ses << "select textval, intval as value from test where intval = 12", into(ores);
			
		fmt::println("gotdata = {}; res = {}", ses.got_data(), ores.value());
		
		std::set<std::string> data;
		soci::rowset<std::optional<std::string>> rs = ses.prepare << "select textval from test";
		for (auto & row : rs)
			data.insert(row.value_or(""));
		
		fmt::println("data = {}", data);
	}
	
	return 0;
}
