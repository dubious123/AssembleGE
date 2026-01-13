#pragma once

namespace age::request
{
	enum class type : uint8
	{
		window_resize = 0,

		count
	};

	enum class state : uint8
	{
		init = 0,
		pending,
		done,

		count
	};

}	 // namespace age::request

namespace age::request
{
	using t_phase = uint8;

	using t_handle_id = uint32;

	struct handle
	{
		t_handle_id id;
	};
}	 // namespace age::request

namespace age::request
{
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

namespace age::request
{
	void
	push_init(request::type, param, subsystem::flags listeners) noexcept;

	void
	push_pending() noexcept;

	void
	push_done() noexcept;

	void
		pop(age::subsystem::type) noexcept;
}	 // namespace age::request

namespace age::request::g
{
	// g queue<info>
	// g queue<t_handle_id> [ subsystem::count ]
}