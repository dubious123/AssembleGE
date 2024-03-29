#include "pch.h"
#include "timing.h"

namespace editor::utilities::timing
{
	std::string now_str()
	{
		const auto now = std::chrono::zoned_time { std::chrono::current_zone(), std::chrono::system_clock::now() }.get_local_time();
		const auto dp  = std::chrono::floor<std::chrono::days>(now);

		std::chrono::year_month_day ymd { dp };
		std::chrono::hh_mm_ss		hms { std::chrono::floor<std::chrono::milliseconds>(now - dp) };
		return std::format("{}-{:02}-{:02} {:02}:{:02}", ymd.year(), (unsigned int)ymd.month(), (unsigned int)ymd.day(), hms.hours().count(), hms.minutes().count());
	}
}	 // namespace editor::utilities::timing