#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::texconv::detail
{
	constexpr const char*
	to_dxgi_format_name(graphics::e::texture_format f) noexcept
	{
		switch (f)
		{
		case graphics::e::texture_format::rgba8_unorm:
		{
			return "R8G8B8A8_UNORM";
		}
		case graphics::e::texture_format::rgba8_unorm_srgb:
		{
			return "R8G8B8A8_UNORM_SRGB";
		}
		case graphics::e::texture_format::rgba16_float:
		{
			return "R16G16B16A16_FLOAT";
		}
		case graphics::e::texture_format::rgba16_unorm:
		{
			return "R16G16B16A16_UNORM";
		}
		case graphics::e::texture_format::rgba32_float:
		{
			return "R32G32B32A32_FLOAT";
		}
		case graphics::e::texture_format::r8_unorm:
		{
			return "R8_UNORM";
		}
		case graphics::e::texture_format::r8g8_unorm:
		{
			return "R8G8_UNORM";
		}
		case graphics::e::texture_format::r16_float:
		{
			return "R16_FLOAT";
		}
		case graphics::e::texture_format::r16g16_float:
		{
			return "R16G16_FLOAT";
		}
		case graphics::e::texture_format::bc1_unorm:
		{
			return "BC1_UNORM";
		}
		case graphics::e::texture_format::bc1_unorm_srgb:
		{
			return "BC1_UNORM_SRGB";
		}
		case graphics::e::texture_format::bc3_unorm:
		{
			return "BC3_UNORM";
		}
		case graphics::e::texture_format::bc3_unorm_srgb:
		{
			return "BC3_UNORM_SRGB";
		}
		case graphics::e::texture_format::bc4_unorm:
		{
			return "BC4_UNORM";
		}
		case graphics::e::texture_format::bc4_snorm:
		{
			return "BC4_SNORM";
		}
		case graphics::e::texture_format::bc5_unorm:
		{
			return "BC5_UNORM";
		}
		case graphics::e::texture_format::bc5_snorm:
		{
			return "BC5_SNORM";
		}
		case graphics::e::texture_format::bc6h_uf16:
		{
			return "BC6H_UF16";
		}
		case graphics::e::texture_format::bc6h_sf16:
		{
			return "BC6H_SF16";
		}
		case graphics::e::texture_format::bc7_unorm:
		{
			return "BC7_UNORM";
		}
		case graphics::e::texture_format::bc7_unorm_srgb:
		{
			return "BC7_UNORM_SRGB";
		}
		}
		AGE_UNREACHABLE();
	}

	bake_options
	to_bake_options(const asset::texture_bake_option& opt, const char* output_dir) noexcept
	{
		auto res = bake_options{};

		res.output_dir = output_dir;

		res.dxgi_format_name = to_dxgi_format_name(opt.format);

		switch (opt.format)
		{
		case graphics::e::texture_format::rgba8_unorm_srgb:
		case graphics::e::texture_format::bc1_unorm_srgb:
		case graphics::e::texture_format::bc3_unorm_srgb:
		case graphics::e::texture_format::bc7_unorm_srgb:
		{
			res.srgb_both = true;
			break;
		}
		default:
		{
			break;
		}
		}

		switch (opt.format)
		{
		case graphics::e::texture_format::bc6h_uf16:
		case graphics::e::texture_format::bc6h_sf16:
		case graphics::e::texture_format::rgba16_float:
		case graphics::e::texture_format::rgba16_unorm:
		case graphics::e::texture_format::rgba32_float:
		case graphics::e::texture_format::r16_float:
		case graphics::e::texture_format::r16g16_float:
		{
			res.intermediate_format = "R16G16B16A16_FLOAT";
			break;
		}
		case graphics::e::texture_format::rgba8_unorm_srgb:
		case graphics::e::texture_format::bc1_unorm_srgb:
		case graphics::e::texture_format::bc3_unorm_srgb:
		case graphics::e::texture_format::bc7_unorm_srgb:
		{
			res.intermediate_format = "R8G8B8A8_UNORM_SRGB";
			break;
		}
		default:
		{
			res.intermediate_format = "R8G8B8A8_UNORM";
			break;
		}
		}

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