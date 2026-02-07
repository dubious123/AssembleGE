#include "age_pch.hpp"
#include "age.hpp"

namespace age::request::detail
{
	constexpr inline auto sync_state_idx_npos = std::numeric_limits<t_sync_state_idx>::max();
}	 // namespace age::request::detail

namespace age::request
{
	template <request::type req_type, request::t_phase v_phase>
	FORCE_INLINE constexpr void
	create(auto req_param) noexcept
	{
		using t_meta = decltype(detail::get_req_meta<req_type>());

		static_assert(
			std::is_same_v<decltype(req_param), typename t_meta::param_type>
				or std::is_same_v<decltype(req_param), param>,
			"request::create(): parameter type mismatch");

		constexpr auto req_meta = detail::get_req_meta<req_type>();

		auto sync_state_idx = detail::sync_state_idx_npos;

		if constexpr (req_meta.sync_required())
		{
			g::sync_state_vec.emplace_back(detail::sync_state{
				.required_ack_count = req_meta.required_ack_count(v_phase) });
		}

		for (auto subsystem_type_idx : age::views::each_set_bit_idx(req_meta.get_listeners(v_phase)))
		{
			g::request_info_vec[std::to_underlying(subsystem_type_idx)].emplace_back(request::info{
				.req_param		= req_param,
				.sync_state_idx = sync_state_idx,
				.phase			= v_phase,
				.type			= req_type });
		}
	}

	template <subsystem::type sys_type>
	FORCE_INLINE constexpr decltype(auto)
	for_each() noexcept
	{
		return std::views::iota(0)
			 | std::views::take(g::request_info_vec[std::to_underlying(sys_type)].size())
			 | std::views::reverse
			 | std::views::transform([](auto idx) -> auto& { return g::request_info_vec[std::to_underlying(sys_type)][idx]; });
	}

	template <subsystem::type sys_type, request::type req_type, request::t_phase v_phase>
	FORCE_INLINE void
	set_done(info& req) noexcept
	{
		constexpr auto req_meta		  = detail::get_req_meta<req_type>();
		constexpr auto next_listeners = req_meta.get_listeners(v_phase + 1);
		constexpr auto has_next_phase = next_listeners != subsystem::flags{ 0 };

		auto& vec		 = g::request_info_vec[std::to_underlying(sys_type)];
		auto  backup_req = req;
		req				 = vec.back();
		vec.pop_back();

		if constexpr (req_meta.sync_required(v_phase))
		{
			auto& sync_state = g::sync_state_vec[backup_req.sync_state_idx];

			AGE_ASSERT(sync_state.required_ack_count > 0);
			AGE_ASSERT(sync_state.required_ack_count <= req_meta.required_ack_count(v_phase));

			--sync_state.required_ack_count;
			if (bool is_done = sync_state.required_ack_count == 0)
			{
				g::sync_state_vec.remove(backup_req.sync_state_idx);

				if constexpr (has_next_phase)
				{
					request::create<req_type, v_phase + 1>(backup_req.req_param);
				}
			}
		}
		else if constexpr (has_next_phase)
		{
			request::create<req_type, v_phase + 1>(backup_req.req_param);
		}
	}
}	 // namespace age::request