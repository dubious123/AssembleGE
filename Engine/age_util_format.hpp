#pragma once

template <std::size_t n_size>
struct std::formatter<std::array<char, n_size>> : formatter<std::string_view>
{
	auto
	format(const std::array<char, n_size>& arr, auto& ctx) const
	{
		return formatter<std::string_view>::format(
			std::string_view{ arr.data() }, ctx);
	}
};

template <>
struct std::formatter<float2>
{
	constexpr auto
	parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto
	format(const float2& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(),
							  "[{:.6f}, {:.6f}]",
							  vec[0], vec[1]);
	}
};

template <>
struct std::formatter<float3>
{
	constexpr auto
	parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto
	format(const float3& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(),
							  "[{:.6f}, {:.6f}, {:.6f}]",
							  vec[0], vec[1], vec[2]);
	}
};

template <>
struct std::formatter<float4>
{
	constexpr auto
	parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto
	format(const float4& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(),
							  "[{:.6f}, {:.6f}, {:.6f}, {:.6f}]",
							  vec[0], vec[1], vec[2], vec[3]);
	}
};

template <typename t>
requires requires(t handle) { {handle.id} -> std::convertible_to<uint64>; }
struct std::formatter<t> : std::formatter<uint64>
{
	auto
	format(const t& h, auto& ctx) const
	{
		return std::formatter<uint64>::format(h.id, ctx);
	}
};