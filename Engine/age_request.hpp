#pragma once

namespace age::request::detail
{
	struct sync_state
	{
		uint8 required_ack_count;
	};
}	 // namespace age::request::detail

namespace age::request
{
	AGE_DEFINE_ENUM(type, uint8, window_close, window_resize, count)

	using t_phase		   = uint8;
	using t_sync_state_idx = uint16;

	namespace detail
	{
		template <typename t>
		concept cx_request_payload =
			sizeof(t) <= sizeof(uint32)
			and alignof(t) <= alignof(uint32)
			and std::is_trivially_copyable_v<std::remove_cvref_t<t>>;
	}

	struct param
	{
		alignas(uint32)
			std::byte storage[sizeof(uint32)];

		constexpr param(detail::cx_request_payload auto&& arg) noexcept
		{
			age::util::write_bytes(storage, FWD(arg));
		};

		template <detail::cx_request_payload t>
		constexpr t
		as() const noexcept
		{
			std::array<std::byte, sizeof(t)> tmp{};
			std::memcpy(tmp.data(), storage, sizeof(t));
			return std::bit_cast<t>(tmp);
		}
	};
}	 // namespace age::request

// meta
namespace age::request::detail
{

	template <subsystem::flags v_listeners>
	struct phase_meta
	{
		static constexpr subsystem::flags listeners = v_listeners;
		constexpr phase_meta() noexcept				= default;
	};

	template <request::type v_type, typename t_param, typename... t_phase>
	struct request_meta
	{
		using param_type = t_param;

		static constexpr request::type	  type		  = v_type;
		static constexpr request::t_phase phase_count = static_cast<request::t_phase>(sizeof...(t_phase));

		static_assert(phase_count > 0);

		static constexpr subsystem::flags
		get_listeners(request::t_phase phase)
		{
			switch (phase)
			{
#define X(N)                                                           \
	case N:                                                            \
	{                                                                  \
		if constexpr (N < phase_count)                                 \
		{                                                              \
			return age::meta::variadic_at_t<N, t_phase...>::listeners; \
		}                                                              \
		else                                                           \
		{                                                              \
			[[fallthrough]];                                           \
		}                                                              \
	}
				__X_REPEAT_LIST_512
#undef X
			default:
			{
				return subsystem::flags{ 0 };
			}
			}
		}

		static constexpr auto
		required_ack_count(request::t_phase phase = 0)
		{
			return age::util::popcount(std::to_underlying(get_listeners(phase)));
		}

		static constexpr bool
		sync_required(request::t_phase phase = 0)
		{
			return required_ack_count(phase) > 1;
		}
	};

#define COMBINE_FLAGS(x) age::subsystem::flags::x |

#define PHASE(...) \
	phase_meta<FOR_EACH(COMBINE_FLAGS, __VA_ARGS__) age::subsystem::flags{ 0 }>
	// phase_meta<flag_combine(FOR_EACH_ARG(ADD_NAMESPACE, __VA_ARGS__))>

#define DEFINE_REQUEST_BEGIN               \
	template <age::request::type req_type> \
	constexpr auto get_req_meta()          \
	{                                      \
		if constexpr (false)               \
		{                                  \
		}

#define DEFINE_REQUEST(req_type_name, param_type, ...)                                     \
	else if constexpr (req_type == age::request::type::req_type_name)                      \
	{                                                                                      \
		return request_meta<age::request::type::req_type_name, param_type, __VA_ARGS__>{}; \
	}

#define DEFINE_REQUEST_END                            \
	else                                              \
	{                                                 \
		static_assert(false, "invalid request type"); \
	}                                                 \
	}
}	 // namespace age::request::detail

namespace age::request::detail
{
	DEFINE_REQUEST_BEGIN

	DEFINE_REQUEST(window_close,
				   platform::window_handle,
				   PHASE(graphics),
				   PHASE(platform))
	DEFINE_REQUEST(window_resize,
				   platform::window_handle,
				   PHASE(platform, graphics))

	DEFINE_REQUEST_END

#undef COMBINE_FLAGS
#undef PHASE
#undef DEFINE_REQUEST_BEGIN
#undef DEFINE_REQUEST
#undef DEFINE_REQUEST_END
}	 // namespace age::request::detail

namespace age::request
{
	struct info
	{
		request::param	 req_param;
		t_sync_state_idx sync_state_idx;

		request::t_phase phase;
		request::type	 type;
	};
}	 // namespace age::request

namespace age::request
{
	template <request::type req_type, request::t_phase v_phase = 0>
	FORCE_INLINE constexpr void
	create(auto req_param) noexcept;

	template <subsystem::type sys_type>
	FORCE_INLINE constexpr decltype(auto)
	for_each() noexcept;

	template <subsystem::type sys_type, request::type req_type, request::t_phase v_phase>
	FORCE_INLINE void
	set_done(info& req) noexcept;
}	 // namespace age::request

namespace age::request::g
{
	inline auto sync_state_vec	 = data_structure::sparse_vector<request::detail::sync_state>{};
	inline auto request_info_vec = std::array<data_structure::vector<request::info>, (std::size_t)subsystem::type::count>{};
}	 // namespace age::request::g