#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics
{
	bool
	texture_format_is_srgb(e::texture_format format) noexcept
	{
		using enum e::texture_format;
		switch (format)
		{
		case rgba8_unorm_srgb:
		case bgra8_unorm_srgb:
		case bgrx8_unorm_srgb:
		case bc1_unorm_srgb:
		case bc2_unorm_srgb:
		case bc3_unorm_srgb:
		case bc7_unorm_srgb:
			return true;
		default:
			return false;
		}
	}

	bool
	texture_format_is_block_compressed(e::texture_format format) noexcept
	{
		using enum e::texture_format;
		c_auto v = to_idx(format);
		return (v >= to_idx(bc1_typeless) and v <= to_idx(bc5_snorm))
			or (v >= to_idx(bc6h_typeless) and v <= to_idx(bc7_unorm_srgb));
	}

	// channels the shader can read as data. bc1 counts 3 (its 1 bit alpha is a cutout, not a channel),
	// depth / stencil / typeless / sampler feedback count 0.
	uint32
	get_texture_format_channel_count(e::texture_format format) noexcept
	{
		using enum e::texture_format;
		switch (format)
		{
		case rgba32_float:
		case rgba32_uint:
		case rgba32_sint:
		case rgba16_float:
		case rgba16_unorm:
		case rgba16_uint:
		case rgba16_snorm:
		case rgba16_sint:
		case rgb10a2_unorm:
		case rgb10a2_uint:
		case rgba8_unorm:
		case rgba8_unorm_srgb:
		case rgba8_uint:
		case rgba8_snorm:
		case rgba8_sint:
		case b5g5r5a1_unorm:
		case bgra8_unorm:
		case bgra8_unorm_srgb:
		case rgb10_xr_bias_a2_unorm:
		case abgr4_unorm:
		case bgra4_unorm:
		case bc2_unorm:
		case bc2_unorm_srgb:
		case bc3_unorm:
		case bc3_unorm_srgb:
		case bc7_unorm:
		case bc7_unorm_srgb:
			return 4;

		case rgb32_float:
		case rgb32_uint:
		case rgb32_sint:
		case r11g11b10_float:
		case rgb9e5_sharedexp:
		case rg8_bg8_unorm:
		case gr8_gb8_unorm:
		case b5g6r5_unorm:
		case bgrx8_unorm:
		case bgrx8_unorm_srgb:
		case bc1_unorm:
		case bc1_unorm_srgb:
		case bc6h_ufloat16:
		case bc6h_sfloat16:
			return 3;

		case rg32_float:
		case rg32_uint:
		case rg32_sint:
		case rg16_float:
		case rg16_unorm:
		case rg16_uint:
		case rg16_snorm:
		case rg16_sint:
		case rg8_unorm:
		case rg8_uint:
		case rg8_snorm:
		case rg8_sint:
		case bc5_unorm:
		case bc5_snorm:
			return 2;

		case r32_float:
		case r32_uint:
		case r32_sint:
		case r16_float:
		case r16_unorm:
		case r16_uint:
		case r16_snorm:
		case r16_sint:
		case r8_unorm:
		case r8_uint:
		case r8_snorm:
		case r8_sint:
		case a8_unorm:
		case r1_unorm:
		case bc4_unorm:
		case bc4_snorm:
			return 1;

		default:
			return 0;
		}
	}

	bool
	texture_format_has_alpha(e::texture_format format) noexcept
	{
		using enum e::texture_format;
		switch (format)
		{
		case bc1_unorm:
		case bc1_unorm_srgb:
			return true;
		default:
			return get_texture_format_channel_count(format) == 4;
		}
	}

	// bits per pixel for size estimates. block compressed formats report their average.
	uint32
	get_texture_format_bpp(e::texture_format format) noexcept
	{
		using enum e::texture_format;
		c_auto v = to_idx(format);

		if (v >= to_idx(rgba32_typeless) and v <= to_idx(rgba32_sint)) { return 128; }
		if (v >= to_idx(rgb32_typeless) and v <= to_idx(rgb32_sint)) { return 96; }
		if (v >= to_idx(rgba16_typeless) and v <= to_idx(x32_typeless_g8x24_uint)) { return 64; }
		if (v >= to_idx(rgb10a2_typeless) and v <= to_idx(x24_typeless_g8_uint)) { return 32; }
		if (v >= to_idx(rg8_typeless) and v <= to_idx(r16_sint)) { return 16; }
		if (v >= to_idx(r8_typeless) and v <= to_idx(a8_unorm)) { return 8; }
		if (format == r1_unorm) { return 1; }
		if (format == rgb9e5_sharedexp or format == rg8_bg8_unorm or format == gr8_gb8_unorm) { return 32; }
		if (v >= to_idx(bc1_typeless) and v <= to_idx(bc1_unorm_srgb)) { return 4; }
		if (v >= to_idx(bc2_typeless) and v <= to_idx(bc3_unorm_srgb)) { return 8; }
		if (v >= to_idx(bc4_typeless) and v <= to_idx(bc4_snorm)) { return 4; }
		if (v >= to_idx(bc5_typeless) and v <= to_idx(bc5_snorm)) { return 8; }
		if (format == b5g6r5_unorm or format == b5g5r5a1_unorm or format == bgra4_unorm or format == abgr4_unorm) { return 16; }
		if (v >= to_idx(bgra8_unorm) and v <= to_idx(bgrx8_unorm_srgb)) { return 32; }
		if (v >= to_idx(bc6h_typeless) and v <= to_idx(bc7_unorm_srgb)) { return 8; }
		return 0;
	}
}	 // namespace age::graphics