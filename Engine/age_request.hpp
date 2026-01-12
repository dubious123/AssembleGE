#pragma once

namespace age::request::detail
{
	struct entry
	{
	};
}	 // namespace age::request::detail

namespace age::request
{
	enum class type : uint32
	{
		window_resize = 0,

		count
	};

	template <typename t>
	concept cx_request_payload =
		sizeof(t) <= sizeof(uint32)
		and alignof(t) <= alignof(uint32)
		and std::is_trivially_copyable_v<std::remove_cvref_t<t>>;

	struct request_param
	{
		alignas(uint32)
			std::byte storage[sizeof(uint32)];

		template <cx_request_payload t>
		constexpr request_param(t&& arg) noexcept
		{
			age::util::write_bytes(storage, FWD(arg));
		};

		template <cx_request_payload t>
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
		push_init(type, request_param, age::subsystem::flags) noexcept;

	void
	push_pending() noexcept;

	void
	push_done() noexcept;
}	 // namespace age::request