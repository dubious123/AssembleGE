#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	bool
	validate_header<e::kind::material>(const file_header& header) noexcept
	{
		auto res = true;
		{
			c_auto tmp = header.asset_kind == e::kind::material;
			AGE_ASSERT(tmp);
			res &= tmp;
		}

		return res;
	}

	std::array<char, config::max_asset_path_len>&
	entry<e::kind::material>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::material>::is_loaded() const noexcept
	{
		return AGE_IS_INVALID_ID(render_id) is_false;
	}

	std::array<const handle*, 5>
	entry<e::kind::material>::all_textures() const noexcept
	{
		return std::array<const handle*, 5>{
			&h_tex_base_color,
			&h_tex_metallic_roughness,
			&h_tex_normal,
			&h_tex_occlusion,
			&h_tex_emissive,
		};
	}

	std::array<handle*, 5>
	entry<e::kind::material>::all_textures() noexcept
	{
		return std::array<handle*, 5>{
			&h_tex_base_color,
			&h_tex_metallic_roughness,
			&h_tex_normal,
			&h_tex_occlusion,
			&h_tex_emissive,
		};
	}
}	 // namespace age::asset

namespace age::asset::material
{
	void
	add_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::material>();
		AGE_ASSERT(entry.ref_counter < std::numeric_limits<BARE_OF(entry.ref_counter)>::max());
		++entry.ref_counter;
	}

	void
	remove_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::material>();
		AGE_ASSERT(entry.ref_counter > 0);
		--entry.ref_counter;
	}
}	 // namespace age::asset::material

namespace age::asset::material
{
	namespace detail
	{
		std::array<char, config::max_asset_path_len>
		get_tex_path(asset::handle h_tex) noexcept
		{
			if (runtime::is_handle_invalid(h_tex))
			{
				return std::array<char, config::max_asset_path_len>{};
			}
			else
			{
				return h_tex.get_path<e::kind::texture>();
			}
		}
	}	 // namespace detail

	void
	build(std::string_view mat_path, const material_desc& desc) noexcept
	{
		auto buf = byte_buf::gen_reserved(config::max_asset_path_len * 5 + sizeof(entry<e::kind::material>));

		buf.write(
			desc.base_color_factor,
			desc.metallic_factor,
			desc.roughness_factor,
			desc.emissive_factor,
			desc.normal_scale,
			desc.occlusion_strength,
			desc.alpha_cutoff,
			desc.alpha_mode,
			detail::get_tex_path(desc.h_tex_base_color),
			detail::get_tex_path(desc.h_tex_metallic_roughness),
			detail::get_tex_path(desc.h_tex_normal),
			detail::get_tex_path(desc.h_tex_occlusion),
			detail::get_tex_path(desc.h_tex_emissive));

		c_auto f_header = get_default_file_header<e::kind::material>(buf.size());
		write_asset_file(mat_path.data(), f_header, buf.data());
	}

	void
	save(handle h_mat) noexcept
	{
		if (runtime::is_handle_invalid(h_mat))
		{
			AGE_ASSERT(false);
			return;
		}

		auto& entry = h_mat.get_entry<e::kind::material>();

		auto buf = byte_buf::gen_reserved(config::max_asset_path_len * 5 + sizeof(entry));

		buf.write(
			entry.base_color_factor,
			entry.metallic_factor,
			entry.roughness_factor,
			entry.emissive_factor,
			entry.normal_scale,
			entry.occlusion_strength,
			entry.alpha_cutoff,
			entry.alpha_mode,
			detail::get_tex_path(entry.h_tex_base_color),
			detail::get_tex_path(entry.h_tex_metallic_roughness),
			detail::get_tex_path(entry.h_tex_normal),
			detail::get_tex_path(entry.h_tex_occlusion),
			detail::get_tex_path(entry.h_tex_emissive));

		c_auto f_header = get_default_file_header<e::kind::material>(buf.size());
		write_asset_file(h_mat.get_path<e::kind::material>().data(), f_header, buf.data());
	}
}	 // namespace age::asset::material