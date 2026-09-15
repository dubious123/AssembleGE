#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::texconv::detail
{
	constexpr const char*
	to_dxgi_format_name(graphics::e::texture_format f) noexcept
	{
		using enum graphics::e::texture_format;
		switch (f)
		{
		case unknown:
			return nullptr;

		// 128 bit
		case rgba32_typeless:
			return nullptr;
		case rgba32_float:
			return "R32G32B32A32_FLOAT";
		case rgba32_uint:
			return "R32G32B32A32_UINT";
		case rgba32_sint:
			return "R32G32B32A32_SINT";

		// 96 bit
		case rgb32_typeless:
			return nullptr;
		case rgb32_float:
			return "R32G32B32_FLOAT";
		case rgb32_uint:
			return "R32G32B32_UINT";
		case rgb32_sint:
			return "R32G32B32_SINT";

		// 64 bit
		case rgba16_typeless:
			return nullptr;
		case rgba16_float:
			return "R16G16B16A16_FLOAT";
		case rgba16_unorm:
			return "R16G16B16A16_UNORM";
		case rgba16_uint:
			return "R16G16B16A16_UINT";
		case rgba16_snorm:
			return "R16G16B16A16_SNORM";
		case rgba16_sint:
			return "R16G16B16A16_SINT";
		case rg32_typeless:
			return nullptr;
		case rg32_float:
			return "R32G32_FLOAT";
		case rg32_uint:
			return "R32G32_UINT";
		case rg32_sint:
			return "R32G32_SINT";
		case r32g8x24_typeless:
			return nullptr;
		case d32_float_s8x24_uint:
			return "D32_FLOAT_S8X24_UINT";
		case r32_float_x8x24_typeless:
			return nullptr;
		case x32_typeless_g8x24_uint:
			return nullptr;

		// 32 bit
		case rgb10a2_typeless:
			return nullptr;
		case rgb10a2_unorm:
			return "R10G10B10A2_UNORM";
		case rgb10a2_uint:
			return "R10G10B10A2_UINT";
		case r11g11b10_float:
			return "R11G11B10_FLOAT";
		case rgba8_typeless:
			return nullptr;
		case rgba8_unorm:
			return "R8G8B8A8_UNORM";
		case rgba8_unorm_srgb:
			return "R8G8B8A8_UNORM_SRGB";
		case rgba8_uint:
			return "R8G8B8A8_UINT";
		case rgba8_snorm:
			return "R8G8B8A8_SNORM";
		case rgba8_sint:
			return "R8G8B8A8_SINT";
		case rg16_typeless:
			return nullptr;
		case rg16_float:
			return "R16G16_FLOAT";
		case rg16_unorm:
			return "R16G16_UNORM";
		case rg16_uint:
			return "R16G16_UINT";
		case rg16_snorm:
			return "R16G16_SNORM";
		case rg16_sint:
			return "R16G16_SINT";
		case r32_typeless:
			return nullptr;
		case d32_float:
			return "D32_FLOAT";
		case r32_float:
			return "R32_FLOAT";
		case r32_uint:
			return "R32_UINT";
		case r32_sint:
			return "R32_SINT";
		case r24g8_typeless:
			return nullptr;
		case d24_unorm_s8_uint:
			return "D24_UNORM_S8_UINT";
		case r24_unorm_x8_typeless:
			return nullptr;
		case x24_typeless_g8_uint:
			return nullptr;

		// 16 bit
		case rg8_typeless:
			return nullptr;
		case rg8_unorm:
			return "R8G8_UNORM";
		case rg8_uint:
			return "R8G8_UINT";
		case rg8_snorm:
			return "R8G8_SNORM";
		case rg8_sint:
			return "R8G8_SINT";
		case r16_typeless:
			return nullptr;
		case r16_float:
			return "R16_FLOAT";
		case d16_unorm:
			return "D16_UNORM";
		case r16_unorm:
			return "R16_UNORM";
		case r16_uint:
			return "R16_UINT";
		case r16_snorm:
			return "R16_SNORM";
		case r16_sint:
			return "R16_SINT";

		// 8 bit
		case r8_typeless:
			return nullptr;
		case r8_unorm:
			return "R8_UNORM";
		case r8_uint:
			return "R8_UINT";
		case r8_snorm:
			return "R8_SNORM";
		case r8_sint:
			return "R8_SINT";
		case a8_unorm:
			return "A8_UNORM";
		case r1_unorm:
			return "R1_UNORM";

		// packed
		case rgb9e5_sharedexp:
			return "R9G9B9E5_SHAREDEXP";
		case rg8_bg8_unorm:
			return "R8G8_B8G8_UNORM";
		case gr8_gb8_unorm:
			return "G8R8_G8B8_UNORM";

		// block compressed
		case bc1_typeless:
			return nullptr;
		case bc1_unorm:
			return "BC1_UNORM";
		case bc1_unorm_srgb:
			return "BC1_UNORM_SRGB";
		case bc2_typeless:
			return nullptr;
		case bc2_unorm:
			return "BC2_UNORM";
		case bc2_unorm_srgb:
			return "BC2_UNORM_SRGB";
		case bc3_typeless:
			return nullptr;
		case bc3_unorm:
			return "BC3_UNORM";
		case bc3_unorm_srgb:
			return "BC3_UNORM_SRGB";
		case bc4_typeless:
			return nullptr;
		case bc4_unorm:
			return "BC4_UNORM";
		case bc4_snorm:
			return "BC4_SNORM";
		case bc5_typeless:
			return nullptr;
		case bc5_unorm:
			return "BC5_UNORM";
		case bc5_snorm:
			return "BC5_SNORM";

		// legacy bgra
		case b5g6r5_unorm:
			return "B5G6R5_UNORM";
		case b5g5r5a1_unorm:
			return "B5G5R5A1_UNORM";
		case bgra8_unorm:
			return "B8G8R8A8_UNORM";
		case bgrx8_unorm:
			return "B8G8R8X8_UNORM";
		case rgb10_xr_bias_a2_unorm:
			return "R10G10B10_XR_BIAS_A2_UNORM";
		case bgra8_typeless:
			return nullptr;
		case bgra8_unorm_srgb:
			return "B8G8R8A8_UNORM_SRGB";
		case bgrx8_typeless:
			return nullptr;
		case bgrx8_unorm_srgb:
			return "B8G8R8X8_UNORM_SRGB";

		// block compressed, d3d11
		case bc6h_typeless:
			return nullptr;
		case bc6h_ufloat16:
			return "BC6H_UF16";
		case bc6h_sfloat16:
			return "BC6H_SF16";
		case bc7_typeless:
			return nullptr;
		case bc7_unorm:
			return "BC7_UNORM";
		case bc7_unorm_srgb:
			return "BC7_UNORM_SRGB";

		// 4 bit bgra / abgr
		case bgra4_unorm:
			return "B4G4R4A4_UNORM";
		case abgr4_unorm:
			return "A4B4G4R4_UNORM";

		// sampler feedback
		case sampler_feedback_min_mip_opaque:
			return nullptr;
		case sampler_feedback_mip_region_used:
			return nullptr;
		}
		AGE_UNREACHABLE();
	}

	constexpr const char*
	to_intermediate_format_name(graphics::e::texture_format f) noexcept
	{
		using enum graphics::e::texture_format;
		switch (f)
		{
		// srgb: keep the srgb tag so texconv filters mips in linear space
		case rgba8_unorm_srgb:
		case bgra8_unorm_srgb:
		case bgrx8_unorm_srgb:
		case bc1_unorm_srgb:
		case bc2_unorm_srgb:
		case bc3_unorm_srgb:
		case bc7_unorm_srgb:
			return "R8G8B8A8_UNORM_SRGB";

		// 32 bit float
		case rgba32_float:
		case rgb32_float:
		case rg32_float:
		case r32_float:
		case d32_float:
		case d32_float_s8x24_uint:
			return "R32G32B32A32_FLOAT";

		// 16 bit float and hdr packed
		case rgba16_float:
		case rg16_float:
		case r16_float:
		case r11g11b10_float:
		case rgb9e5_sharedexp:
		case bc6h_ufloat16:
		case bc6h_sfloat16:
			return "R16G16B16A16_FLOAT";

		// 16 bit unorm
		case rgba16_unorm:
		case rg16_unorm:
		case r16_unorm:
		case d16_unorm:
		case d24_unorm_s8_uint:
		case rgb10a2_unorm:
		case rgb10_xr_bias_a2_unorm:
			return "R16G16B16A16_UNORM";

		// 16 bit snorm
		case rgba16_snorm:
		case rg16_snorm:
		case r16_snorm:
			return "R16G16B16A16_SNORM";

		// 8 bit snorm, bc4 / bc5 snorm come from 8 bit sources
		case rgba8_snorm:
		case rg8_snorm:
		case r8_snorm:
		case bc4_snorm:
		case bc5_snorm:
			return "R8G8B8A8_SNORM";

		// integer targets keep their own class
		case rgba32_uint:
		case rgb32_uint:
		case rg32_uint:
		case r32_uint:
			return "R32G32B32A32_UINT";
		case rgba32_sint:
		case rgb32_sint:
		case rg32_sint:
		case r32_sint:
			return "R32G32B32A32_SINT";
		case rgba16_uint:
		case rg16_uint:
		case r16_uint:
		case rgb10a2_uint:
			return "R16G16B16A16_UINT";
		case rgba16_sint:
		case rg16_sint:
		case r16_sint:
			return "R16G16B16A16_SINT";
		case rgba8_uint:
		case rg8_uint:
		case r8_uint:
			return "R8G8B8A8_UINT";
		case rgba8_sint:
		case rg8_sint:
		case r8_sint:
			return "R8G8B8A8_SINT";

		// everything else: 8 bit unorm class (rgba8, bgra, bc1 / bc2 / bc3 / bc4 / bc5 / bc7 unorm, 4 / 5 / 1 bit, a8, packed yuv-ish)
		default:
			return "R8G8B8A8_UNORM";
		}
	}

	bake_options
	to_bake_options(const asset::texture_bake_option& opt, const char* output_dir) noexcept
	{
		auto res = bake_options{};

		res.output_dir = output_dir;

		res.dxgi_format_name = to_dxgi_format_name(opt.format);
		AGE_ASSERT(res.dxgi_format_name != nullptr, "format is not a valid texconv target (typeless / view / sampler feedback)");

		res.srgb_both			= graphics::texture_format_is_srgb(opt.format);
		res.intermediate_format = to_intermediate_format_name(opt.format);

		AGE_ASSERT((opt.is_cube and opt.is_3d) is_false);

		const auto need_assemble = opt.is_cube or opt.is_3d or opt.array_or_depth_count > 1u;

		if (need_assemble is_false)
		{
			res.assemble_kind = "";
		}
		else if (opt.is_cube and opt.array_or_depth_count == 1u)
		{
			res.assemble_kind = "cube";
		}
		else if (opt.is_cube and opt.array_or_depth_count > 1u)
		{
			res.assemble_kind = "cubearray";
		}
		else if (opt.is_3d)
		{
			res.assemble_kind = "volume";
		}
		else
		{
			res.assemble_kind = "array";
		}

		res.assemble_output_filename = opt.output_filename;

		res.width	 = opt.width;
		res.height	 = opt.height;
		res.fit_pow2 = opt.fit_pow2;

		res.mip_count = opt.mip_count;

		switch (opt.filter)
		{
		case asset::e::mip_filter_kind::point:
		{
			res.image_filter = "POINT";
			break;
		}
		case asset::e::mip_filter_kind::linear:
		{
			res.image_filter = "LINEAR";
			break;
		}
		case asset::e::mip_filter_kind::cubic:
		{
			res.image_filter = "CUBIC";
			break;
		}
		case asset::e::mip_filter_kind::box:
		{
			res.image_filter = "BOX";
			break;
		}
		case asset::e::mip_filter_kind::triangle:
		{
			res.image_filter = "TRIANGLE";
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		switch (opt.wrap)
		{
		case asset::e::wrap_mode_kind::wrap:
		{
			res.wrap = true;
			break;
		}
		case asset::e::wrap_mode_kind::mirror:
		{
			res.mirror = true;
			break;
		}
		case asset::e::wrap_mode_kind::clamp:
		{
			break;
		}
		}

		res.hflip	 = opt.hflip;
		res.vflip	 = opt.vflip;
		res.invert_y = opt.invert_y;

		res.separate_alpha	= opt.separate_alpha;
		res.alpha_threshold = opt.alpha_threshold;
		res.keep_coverage	= opt.keep_coverage;

		return res;
	}
}	 // namespace age::external::texconv::detail

namespace age::external::texconv
{
	bool
	bake_texture(std::span<const char* const> src, const char* output_dir, const asset::texture_bake_option& opt) noexcept
	{
		AGE_ASSERT(src.empty() == false);
		AGE_ASSERT(opt.array_or_depth_count >= 1u);

		return age::external::texconv::bake_texture(
			src.data(),
			static_cast<uint32>(src.size()),
			detail::to_bake_options(opt, output_dir));
	}

	bool
	bake_texture(const char* const p_src, const char* output_dir, const asset::texture_bake_option& opt) noexcept
	{
		return bake_texture(std::span<const char* const>{ std::addressof(p_src), 1u }, output_dir, opt);
	}
}	 // namespace age::external::texconv