#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	graphics::e::texture_format
	migrate_texture_format(uint16 legacy) noexcept
	{
		using enum graphics::e::texture_format;

		constexpr auto lut = std::array{
			rgba8_unorm,		 // 0  rgba8_unorm
			rgba8_unorm_srgb,	 // 1  rgba8_unorm_srgb
			rgba16_float,		 // 2  rgba16_float
			rgba16_unorm,		 // 3  rgba16_unorm
			rgba32_float,		 // 4  rgba32_float
			r8_unorm,			 // 5  r8_unorm
			rg8_unorm,			 // 6  r8g8_unorm
			r16_float,			 // 7  r16_float
			rg16_float,			 // 8  r16g16_float
			bc1_unorm,			 // 9  bc1_unorm
			bc1_unorm_srgb,		 // 10 bc1_unorm_srgb
			bc3_unorm,			 // 11 bc3_unorm
			bc3_unorm_srgb,		 // 12 bc3_unorm_srgb
			bc4_unorm,			 // 13 bc4_unorm
			bc4_snorm,			 // 14 bc4_snorm
			bc5_unorm,			 // 15 bc5_unorm
			bc5_snorm,			 // 16 bc5_snorm
			bc6h_ufloat16,		 // 17 bc6h_uf16
			bc6h_sfloat16,		 // 18 bc6h_sf16
			bc7_unorm,			 // 19 bc7_unorm
			bc7_unorm_srgb,		 // 20 bc7_unorm_srgb
			r32_float,			 // 21 r32_float
			rg32_uint,			 // 22 r32g32_uint
			rgba16_float,		 // 23 r16g16b16a16_float (duplicate of 2 in v1)
			r8_uint,			 // 24 r8_uint
			d32_float,			 // 25 d32_float
			d16_unorm,			 // 26 d16_unorm
			r11g11b10_float,	 // 27 r11g11b10_float
			rg16_snorm,			 // 28 r16g16_snorm
			rgba8_typeless,		 // 29 rgba8_typeless
		};

		return legacy < lut.size() ? lut[legacy] : unknown;
	}
}	 // namespace age::asset