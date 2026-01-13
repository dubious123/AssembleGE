#include "age_pch.hpp"
#include "age.hpp"

namespace age::request::detail
{
	struct info
	{
		request::param	 req_param;
		request::t_phase phase;
		subsystem::flags listeners;
		request::type	 req_type;
		request::state	 req_state;
	};
}	 // namespace age::request::detail

namespace age::request
{
	void
	push_init(request::type req_type, param req_param, subsystem::flags listeners) noexcept
	{
		for (auto [idx, flag] : age::util::each_set_bit_pair(listeners))
		{
			int a = 2;
		}
	}
}	 // namespace age::request