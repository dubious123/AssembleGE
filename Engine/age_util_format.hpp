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
requires(age::meta::variadic_contains_v<t, float2, float3, float4> is_false
		 and (age::meta::is_specialization_of_v<t, vec2>
			  or age::meta::is_specialization_of_v<t, vec3>
			  or age::meta::is_specialization_of_v<t, vec4>))
struct std::formatter<t>
{
	constexpr auto
	parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto
	format(const t& v, std::format_context& ctx) const
	{
		auto out = ctx.out();
		*out++	 = '[';
		[&]<size_t... i>(std::index_sequence<i...>) {
			((out = std::format_to(out, "{}{}", (i == 0 ? "" : ", "), v[i])), ...);
		}(std::make_index_sequence<t::size()>{});
		*out++ = ']';
		return out;
	}
};

template <typename t>
requires(age::meta::is_specialization_of_v<t, mat22>
		 or age::meta::is_specialization_of_v<t, mat33>
		 or age::meta::is_specialization_of_v<t, mat44>
		 or age::meta::is_specialization_of_v<t, mat34>
		 or age::meta::is_specialization_of_v<t, mat22a>
		 or age::meta::is_specialization_of_v<t, mat33a>
		 or age::meta::is_specialization_of_v<t, mat44a>)
struct std::formatter<t>
{
	constexpr auto
	parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto
	format(const t& m, std::format_context& ctx) const
	{
		using elem_t = std::remove_cvref_t<decltype(m[0][0])>;

		auto out = ctx.out();
		*out++	 = '[';
		[&]<size_t... r>(std::index_sequence<r...>) {
			((out = format_row(out, m, r, (r == 0))), ...);
		}(std::make_index_sequence<t::rows()>{});
		*out++ = ']';
		return out;
	}

  private:
	static auto
	format_row(auto out, const t& m, size_t r, bool first)
	{
		using elem_t = std::remove_cvref_t<decltype(m[0][0])>;

		if (not first) { out = std::format_to(out, ", "); }
		*out++ = '[';
		[&]<size_t... c>(std::index_sequence<c...>) {
			((out = [&] {
				 if constexpr (std::is_floating_point_v<elem_t>)
				 {
					 return std::format_to(out, "{}{:.6f}", (c == 0 ? "" : ", "), m[r][c]);
				 }
				 else
				 {
					 return std::format_to(out, "{}{}", (c == 0 ? "" : ", "), m[r][c]);
				 }
			 }()),
			 ...);
		}(std::make_index_sequence<t::cols()>{});
		*out++ = ']';
		return out;
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