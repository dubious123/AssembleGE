#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	bool
	validate_header<e::kind::env_light>(const file_header& header) noexcept
	{
		auto res = true;
		{
			c_auto tmp = header.asset_kind == e::kind::env_light;
			AGE_ASSERT(tmp);
			res &= tmp;
		}
		{
			c_auto tmp = header.blob_alignment_log2 > 0
					 and header.blob_alignment_log2 == static_cast<uint8>(std::countr_zero(alignof(entry<e::kind::env_light>::header)));
			AGE_ASSERT(tmp);
			res &= tmp;
		}

		return res;
	}

	std::array<char, config::max_asset_path_len>&
	entry<e::kind::env_light>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::env_light>::is_cpu_loaded() const noexcept
	{
		return p_blob is_not_nullptr;
	}

	bool
	entry<e::kind::env_light>::is_gpu_loaded() const noexcept
	{
		return AGE_IS_INVALID_ID(render_id) is_false;
	}

	const entry<e::kind::env_light>::header&
	entry<e::kind::env_light>::get_header() const noexcept
	{
		return *reinterpret_cast<const header*>(p_blob);
	}

	entry<e::kind::env_light>::header&
	entry<e::kind::env_light>::get_header() noexcept
	{
		return *reinterpret_cast<header*>(p_blob);
	}

	const entry<e::kind::env_light>::info_bake&
	entry<e::kind::env_light>::get_bake_info() const noexcept
	{
		return get_header().bake_info;
	}

	entry<e::kind::env_light>::info_bake&
	entry<e::kind::env_light>::get_bake_info() noexcept
	{
		return get_header().bake_info;
	}

	const entry<e::kind::env_light>::info_runtime&
	entry<e::kind::env_light>::get_runtime_info() const noexcept
	{
		return get_header().runtime_info;
	}

	entry<e::kind::env_light>::info_runtime&
	entry<e::kind::env_light>::get_runtime_info() noexcept
	{
		return get_header().runtime_info;
	}

	const void*
	entry<e::kind::env_light>::get_radiance_texture_buffer() const noexcept
	{
		return p_blob + sizeof(header);
	}

	const void*
	entry<e::kind::env_light>::get_prefilter_texture_buffer() const noexcept
	{
		return p_blob + sizeof(header) + get_header().bake_info.prefilter_texture_buffer_offset;
	}

	const void*
	entry<e::kind::env_light>::get_irradiance_texture_buffer() const noexcept
	{
		return p_blob + sizeof(header) + get_header().bake_info.irradiance_texture_buffer_offset;
	}
}	 // namespace age::asset

namespace age::asset::env_light
{
	void
	cpu_unload(handle h_env_light) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();
		AGE_ASSERT(entry.is_cpu_loaded());

		using t_entry  = BARE_OF(entry);
		auto allocator = t_entry::allocator_type(alignof(t_entry::header));
		allocator.deallocate(entry.p_blob);
		entry.p_blob = nullptr;

		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	cpu_load(handle h_env_light) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob = buf.release();
			return;
		}
	}

	handle
	cpu_load(std::string_view tex_name) noexcept
	{
		c_auto h_env_light = asset::detail::load_common<e::kind::env_light>(tex_name);

		cpu_load(h_env_light);

		return h_env_light;
	}

	void
	add_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::env_light>();
		AGE_ASSERT(entry.ref_counter < std::numeric_limits<BARE_OF(entry.ref_counter)>::max());
		++entry.ref_counter;
	}

	void
	remove_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::env_light>();
		AGE_ASSERT(entry.ref_counter > 0);
		--entry.ref_counter;
	}
}	 // namespace age::asset::env_light

namespace age::asset::env_light
{
	bool
	bake(const std::array<char, config::max_asset_path_len>& src,
		 const std::array<char, config::max_asset_path_len>& dst,
		 const env_light_desc&								 desc) noexcept
	{
		constexpr decltype(auto) tmp_dir = "__env_light_temp__";
		constexpr decltype(auto) tmp_tex = "__tmp__";

		constexpr decltype(auto) tmp_tex_radiance_dds	= ".\\__env_light_temp__\\tmp_radiance_tex.dds";
		constexpr decltype(auto) tmp_tex_prefilter_dds	= ".\\__env_light_temp__\\tmp_prefilter_tex.dds";
		constexpr decltype(auto) tmp_tex_irradiance_dds = ".\\__env_light_temp__\\tmp_irradiance_tex.dds";

		auto temp_dir	  = std::filesystem::create_directories(tmp_dir);
		auto tex_bake_res = texture::bake(std::array{ src.data() },
										  asset::get_asset_full_path<e::kind::texture>(std::format("__env_light_temp__\\{}", tmp_tex).data()).data(),
										  texture_bake_option{
											  .format		   = graphics::e::texture_format::rgba16_float,
											  .is_cube		   = false,
											  .is_3d		   = false,
											  .output_filename = nullptr,
											  .fit_pow2		   = false,
											  .mip_count	   = 1 });

		if (tex_bake_res is_false)
		{
			std::filesystem::remove_all(tmp_dir);
			return false;
		}

		auto h_tex = texture::cpu_load(std::format("__env_light_temp__\\{}", tmp_tex));

		auto env_light_bake_res = graphics::bake::env_light(h_tex, desc);

		c_auto radiance_tex_size   = graphics::resource::calc_readback_size(env_light_bake_res.h_radiance);
		c_auto prefilter_tex_size  = graphics::resource::calc_readback_size(env_light_bake_res.h_prefilter);
		c_auto irradiance_tex_size = graphics::resource::calc_readback_size(env_light_bake_res.h_irradiance);


		auto buf = byte_buf::gen_reserved(sizeof(entry<e::kind::env_light>::header) + radiance_tex_size + prefilter_tex_size + irradiance_tex_size);

		buf.move_write_pos(sizeof(entry<e::kind::env_light>::header));

		c_auto radiance_tex_offset	 = buf.size();
		c_auto prefilter_tex_offset	 = radiance_tex_offset + radiance_tex_size;
		c_auto irradiance_tex_offset = prefilter_tex_offset + prefilter_tex_size;

		graphics::resource::readback_texture(std::span{ buf.data() + radiance_tex_offset, radiance_tex_size }, env_light_bake_res.h_radiance);
		graphics::resource::readback_texture(std::span{ buf.data() + prefilter_tex_offset, prefilter_tex_size }, env_light_bake_res.h_prefilter);
		graphics::resource::readback_texture(std::span{ buf.data() + irradiance_tex_offset, irradiance_tex_size }, env_light_bake_res.h_irradiance);
		graphics::resource::release(env_light_bake_res.h_radiance);
		graphics::resource::release(env_light_bake_res.h_prefilter);
		graphics::resource::release(env_light_bake_res.h_irradiance);

		texture::bake_dds_texture_2d(std::span{ buf.data() + radiance_tex_offset, radiance_tex_size },
									 tmp_tex_radiance_dds,
									 extent_2d<uint32>{ desc.cubemap_size, desc.cubemap_size },
									 graphics::e::texture_format::rgba16_float,
									 static_cast<uint32>(std::bit_width(desc.cubemap_size)),
									 1,
									 true,
									 false);

		texture::bake_dds_texture_2d(std::span{ buf.data() + prefilter_tex_offset, prefilter_tex_size },
									 tmp_tex_prefilter_dds,
									 extent_2d<uint32>{ desc.prefilter_size, desc.prefilter_size },
									 graphics::e::texture_format::rgba16_float,
									 desc.prefilter_mip_count,
									 1,
									 true,
									 false);

		texture::bake_dds_texture_2d(std::span{ buf.data() + irradiance_tex_offset, irradiance_tex_size },
									 tmp_tex_irradiance_dds,
									 extent_2d<uint32>{ desc.irradiance_size, desc.irradiance_size },
									 graphics::e::texture_format::rgba16_float,
									 1,
									 1,
									 true,
									 false);
		{
			auto env_light_entry = entry<e::kind::env_light>::header{
				.bake_info = {
					.cubemap_size		 = desc.cubemap_size,
					.prefilter_size		 = desc.prefilter_size,
					.prefilter_mip_count = desc.prefilter_mip_count,
					.irradiance_size	 = desc.irradiance_size,
					.format				 = desc.format,
				},
				.runtime_info = {}
			};

			auto tex_conv_res = true;
			{
				tex_conv_res &= external::texconv::bake_texture(tmp_tex_radiance_dds, tmp_dir, texture_bake_option{
																								   .format			= desc.format,
																								   .is_cube			= false,	// don't assemble
																								   .is_3d			= false,
																								   .output_filename = "tex_radiance_reformat.dds",
																								   .fit_pow2		= false,
																								   .mip_count		= static_cast<uint32>(std::bit_width(desc.cubemap_size)),
																							   });

				auto temp = read_raw_file(".\\__env_light_temp__\\tex_radiance_reformat.dds");

				tex_conv_res &= temp.size() > texture::detail::dds_header_size();

				if (tex_conv_res)
				{
					buf.write_bytes(temp.data() + texture::detail::dds_header_size(), temp.size() - texture::detail::dds_header_size());
				}
			}

			{
				tex_conv_res &= external::texconv::bake_texture(tmp_tex_prefilter_dds, tmp_dir, texture_bake_option{
																									.format			 = desc.format,
																									.is_cube		 = false,	 // don't assemble
																									.is_3d			 = false,
																									.output_filename = "tex_prefilter_reformat.dds",
																									.fit_pow2		 = false,
																									.mip_count		 = desc.prefilter_mip_count,
																								});

				auto temp = read_raw_file(".\\__env_light_temp__\\tex_prefilter_reformat.dds");

				tex_conv_res &= temp.size() > texture::detail::dds_header_size();

				if (tex_conv_res)
				{
					env_light_entry.bake_info.prefilter_texture_buffer_offset = buf.size();
					buf.write_bytes(temp.data() + texture::detail::dds_header_size(), temp.size() - texture::detail::dds_header_size());
				}
			}

			{
				tex_conv_res &= external::texconv::bake_texture(tmp_tex_irradiance_dds, tmp_dir, texture_bake_option{
																									 .format		  = desc.format,
																									 .is_cube		  = false,	  // don't assemble
																									 .is_3d			  = false,
																									 .output_filename = "tex_irradiance_reformat.dds",
																									 .fit_pow2		  = false,
																									 .mip_count		  = 1,
																								 });

				auto temp = read_raw_file(".\\__env_light_temp__\\tex_irradiance_reformat.dds");

				tex_conv_res &= temp.size() > texture::detail::dds_header_size();

				if (tex_conv_res)
				{
					env_light_entry.bake_info.irradiance_texture_buffer_offset = buf.size();

					buf.write_bytes(temp.data() + texture::detail::dds_header_size(), temp.size() - texture::detail::dds_header_size());

					env_light_entry.bake_info.total_size = buf.size();
				}
			}


			if (tex_conv_res is_false)
			{
				std::filesystem::remove_all(tmp_dir);
				return false;
			}

			buf.write_at(0, std::move(env_light_entry));
		}

		c_auto f_header = get_default_file_header<e::kind::env_light>(buf.size(), static_cast<uint8>(std::countr_zero(alignof(entry<e::kind::env_light>::header))));
		write_asset_file(dst.data(), f_header, buf.data());

		std::filesystem::remove_all(tmp_dir);

		return true;
	}

	void
	save(handle h_env_light) noexcept
	{
		if (runtime::is_handle_invalid(h_env_light))
		{
			AGE_ASSERT(false);
			return;
		}

		auto& env_light_entry = h_env_light.get_entry<e::kind::env_light>();
		if (env_light_entry.is_cpu_loaded() is_false)
		{
			AGE_ASSERT(false);
			return;
		}
		c_auto f_header = get_default_file_header<e::kind::env_light>(env_light_entry.get_bake_info().total_size, static_cast<uint8>(std::countr_zero(alignof(entry<e::kind::env_light>::header))));
		write_asset_file(h_env_light.get_path<e::kind::env_light>().data(), f_header, env_light_entry.p_blob);
	}
}	 // namespace age::asset::env_light