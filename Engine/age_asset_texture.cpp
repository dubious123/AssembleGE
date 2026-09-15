#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	bool
	validate_header<e::kind::texture>(const file_header& header) noexcept
	{
		auto res = true;
		{
			c_auto tmp = header.asset_kind == e::kind::texture;
			AGE_ASSERT(tmp);
			res &= tmp;
		}
		{
			c_auto tmp = header.blob_alignment_log2 > 0
					 and header.blob_alignment_log2 == static_cast<uint8>(std::countr_zero(alignof(entry<e::kind::texture>::header)));
			AGE_ASSERT(tmp);
			res &= tmp;
		}

		return res;
	}

	age::array<char, config::max_asset_path_len>&
	entry<e::kind::texture>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::texture>::is_meta_loaded() const noexcept
	{
		return (flags >> 7u) & 1u;
	}

	bool
	entry<e::kind::texture>::is_any_loaded() const noexcept
	{
		return is_cpu_loaded() or is_gpu_loaded();
	}

	bool
	entry<e::kind::texture>::is_cpu_loaded() const noexcept
	{
		return p_blob != nullptr;
	}

	bool
	entry<e::kind::texture>::is_gpu_loaded() const noexcept
	{
		return AGE_IS_INVALID_ID(render_id) is_false;
	}

	const entry<e::kind::texture>::header&
	entry<e::kind::texture>::get_header() const noexcept
	{
		return *reinterpret_cast<const header*>(p_blob);
	}

	const void*
	entry<e::kind::texture>::get_texture_buffer() const noexcept
	{
		return p_blob + sizeof(header);
	}

	bool
	entry<e::kind::texture>::is_cube_map() const noexcept
	{
		return get_header().flags & (1u << 0);
	}

	bool
	entry<e::kind::texture>::is_tex3d() const noexcept
	{
		return get_header().flags & (1u << 1);
	}
}	 // namespace age::asset

namespace age::asset::texture
{
	void
	cpu_unload(handle h_tex) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();
		AGE_ASSERT(entry.is_cpu_loaded());

		using t_entry  = BARE_OF(entry);
		auto allocator = t_entry::allocator_type(alignof(t_entry::header));
		allocator.deallocate(entry.p_blob);
		entry.p_blob = nullptr;

		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	cpu_load(handle h_tex) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (auto file_data = asset::read_asset_file(entry.get_path());
			file_data.is_valid())
		{
			switch (file_data.header.asset_version)
			{
			case 0u:
			{
				struct header_v0
				{
					extent_2d<uint32> extent;
					uint16			  tex_depth_or_array_size;
					uint16			  format;
					uint8			  mip_count;
					uint8			  flags;	// [0] is_cubemap, [1] is_3d,
					uint16			  extra;
				};

				auto&  buf		   = file_data.buf;
				c_auto size_before = buf.size();

				auto& header  = *reinterpret_cast<header_v0*>(buf.data());
				header.format = to_idx(asset::migrate_texture_format(header.format));

				buf.resize(buf.size() + sizeof(float) /*alpha_threshold*/);
				std::memmove(buf.data() + sizeof(header_v0) + sizeof(float),
							 buf.data() + sizeof(header_v0),
							 size_before - sizeof(header_v0));


				// set alpha_cutoff to -1.f (disabled)
				*reinterpret_cast<float*>(buf.data() + sizeof(header_v0)) = -1.f;
				[[fallthrough]];
			}
			case config::texture_asset_version:
			{
				entry.p_blob				  = file_data.buf.release();
				c_auto& header				  = entry.get_header();
				entry.extent				  = header.extent;
				entry.tex_depth_or_array_size = header.tex_depth_or_array_size;
				entry.format				  = header.format;
				entry.mip_count				  = header.mip_count;
				entry.flags					  = header.flags;
				entry.alpha_cutoff			  = header.alpha_cutoff;

				// set meta is loaded
				entry.flags |= uint8(1u << 7u);
				break;
			}
			default:
			{
				AGE_ASSERT(false, "invalid asset version");
				break;
			}
			}

			return;
		}
	}

	handle
	cpu_load(std::string_view tex_name) noexcept
	{
		c_auto h_tex = asset::detail::load_common<e::kind::texture>(tex_name);

		cpu_load(h_tex);

		return h_tex;
	}
}	 // namespace age::asset::texture

namespace age::asset::texture::detail
{

	struct dds_legacy_header
	{
		uint32 size;
		uint32 flags;
		uint32 height;
		uint32 width;
		uint32 pitch_or_linear_size;
		uint32 depth;
		uint32 mip_map_count;
		uint32 reserved1[11];

		struct
		{
			uint32 size;
			uint32 flags;
			uint32 four_cc;
			uint32 rgb_bit_count;
			uint32 r_bit_mask;
			uint32 g_bit_mask;
			uint32 b_bit_mask;
			uint32 a_bit_mask;
		} pixel_format;

		uint32 caps;
		uint32 caps2;
		uint32 caps3;
		uint32 caps4;
		uint32 reserved2;
	};

	static_assert(sizeof(dds_legacy_header) == 124);

	struct dds_dx10_header
	{
		uint32 dxgi_format;
		uint32 resource_dimension;
		uint32 misc_flag;
		uint32 array_size;
		uint32 misc_flags2;
	};

	static_assert(sizeof(dds_dx10_header) == 20);

	graphics::e::texture_format
	dxgi_to_engine_format(uint32 dxgi) noexcept
	{
		return (dxgi <= 99u or dxgi == 115 or (dxgi >= 189u and dxgi <= 191u))
				 ? static_cast<graphics::e::texture_format>(dxgi)
				 : graphics::e::texture_format::unknown;
	}

	consteval uint32
	dds_header_size() noexcept
	{
		return static_cast<uint32>(sizeof(uint32) + sizeof(dds_legacy_header) + sizeof(dds_dx10_header));
	}

	static_assert(dds_header_size() == 148);
}	 // namespace age::asset::texture::detail

namespace age::asset::texture
{
	bool
	bake(std::span<const char* const> src,
		 std::string_view			  dst,
		 texture_bake_option		  opt) noexcept
	{
		AGE_ASSERT(opt.output_filename is_nullptr);
		opt.output_filename = "tmp.dds";

		constexpr const char* tmp_dir = "__texture_temp__";

		if (external::texconv::bake_texture(src, tmp_dir, opt) is_false)
		{
			std::filesystem::remove_all(tmp_dir);
			return false;
		}

		c_auto dds_path = std::filesystem::path{ tmp_dir } / opt.output_filename;

		auto buf = asset::read_raw_file(dds_path.string());

		if (buf.size() < detail::dds_header_size())				 // sizeof(magic) + sizeof(dds_legacy_header) + sizeof(dds_dx10_header)
		{
			std::filesystem::remove_all(tmp_dir);
			return false;
		}
		c_auto magic_test = buf.read<uint32>() == 0x20534444;	 // 'DDS '
		AGE_ASSERT(magic_test);

		auto dds_legacy = buf.read<detail::dds_legacy_header>();
		auto dds_dx10	= buf.read<detail::dds_dx10_header>();

		auto h			= entry<e::kind::texture>::header{};
		h.extent.width	= dds_legacy.width;
		h.extent.height = dds_legacy.height;

		h.format	= detail::dxgi_to_engine_format(dds_dx10.dxgi_format);
		h.mip_count = static_cast<uint8>(dds_legacy.mip_map_count == 0 ? 1 : dds_legacy.mip_map_count);

		c_auto is_cubemap = (dds_dx10.misc_flag & 4) != 0;
		c_auto is_3d	  = dds_dx10.resource_dimension == 4;

		h.flags = 0;
		if (is_cubemap) { h.flags |= 0x1; }
		if (is_3d) { h.flags |= 0x2; }

		if (is_3d)
		{
			h.tex_depth_or_array_size = static_cast<uint16>(dds_legacy.depth);
		}
		else
		{
			h.tex_depth_or_array_size = static_cast<uint16>(dds_dx10.array_size);
		}

		h.extra = 0;

		auto new_buf = byte_buf::gen_reserved(sizeof(h) + buf.size() - detail::dds_header_size());
		new_buf.write(h);
		new_buf.write_bytes(buf.data() + detail::dds_header_size(), buf.size() - detail::dds_header_size());

		c_auto f_header = get_default_file_header<e::kind::texture>(new_buf.size(), static_cast<uint8>(std::countr_zero(alignof(decltype(h)))));

		write_asset_file(dst, f_header, new_buf.data());

		std::filesystem::remove_all(tmp_dir);
		return true;
	}

	bool
	bake_dds_texture_2d(std::span<std::byte>		src,
						std::string_view			dst,
						extent_2d<uint32>			extent,
						graphics::e::texture_format format,
						uint32						mip_count,
						uint32						array_or_depth_count,
						bool						is_cube,
						bool						is_3d) noexcept
	{
		constexpr uint32 DDSD_CAPS		  = 0x1;
		constexpr uint32 DDSD_HEIGHT	  = 0x2;
		constexpr uint32 DDSD_WIDTH		  = 0x4;
		constexpr uint32 DDSD_PIXELFORMAT = 0x1000;
		constexpr uint32 DDSD_MIPMAPCOUNT = 0x20000;
		constexpr uint32 DDSD_PITCH		  = 0x8;
		constexpr uint32 DDSD_LINEARSIZE  = 0x80000;
		constexpr uint32 DDSD_DEPTH		  = 0x800000;

		constexpr uint32 DDPF_FOURCC = 0x4;
		constexpr uint32 FOURCC_DX10 = 0x30315844;

		constexpr uint32 DDSCAPS_TEXTURE = 0x1000;
		constexpr uint32 DDSCAPS_MIPMAP	 = 0x400000;
		constexpr uint32 DDSCAPS_COMPLEX = 0x8;

		constexpr uint32 DDSCAPS2_CUBEMAP			= 0x200;
		constexpr uint32 DDSCAPS2_CUBEMAP_ALL_FACES = 0xFC00;
		constexpr uint32 DDSCAPS2_VOLUME			= 0x200000;

		constexpr uint32 D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3;
		constexpr uint32 D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4;
		constexpr uint32 D3D10_RESOURCE_MISC_TEXTURECUBE	= 0x4;

		c_auto dxgi_format = graphics::dx12_format(format);
		c_auto texel_size  = graphics::format_size(format);


		auto flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
		if (mip_count > 1) { flags |= DDSD_MIPMAPCOUNT; }
		if (is_3d) { flags |= DDSD_DEPTH; }

		auto caps = DDSCAPS_TEXTURE;
		if (mip_count > 1) { caps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX; }
		if (is_cube) { caps |= DDSCAPS_COMPLEX; }

		auto caps2 = 0u;
		if (is_cube) { caps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES; }
		if (is_3d) { caps2 |= DDSCAPS2_VOLUME; }

		c_auto depth = is_3d ? array_or_depth_count : 0;

		c_auto resource_dim = is_3d ? D3D10_RESOURCE_DIMENSION_TEXTURE3D
									: D3D10_RESOURCE_DIMENSION_TEXTURE2D;

		c_auto misc_flag = is_cube ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;

		c_auto array_size = is_3d ? 1 : array_or_depth_count;

		auto buf = byte_buf::gen_reserved(detail::dds_header_size() + src.size_bytes());

		buf.write(0x20534444,
				  detail::dds_legacy_header{
					  .size					= 124,
					  .flags				= flags,
					  .height				= extent.height,
					  .width				= extent.width,
					  .pitch_or_linear_size = extent.width * texel_size,
					  .depth				= depth,
					  .mip_map_count		= mip_count,
					  .reserved1			= {},
					  .pixel_format			= {
						  .size			 = 32,
						  .flags		 = DDPF_FOURCC,
						  .four_cc		 = FOURCC_DX10,
						  .rgb_bit_count = 0,
						  .r_bit_mask	 = 0,
						  .g_bit_mask	 = 0,
						  .b_bit_mask	 = 0,
						  .a_bit_mask	 = 0,
					  },
					  .caps		 = caps,
					  .caps2	 = caps2,
					  .caps3	 = 0,
					  .caps4	 = 0,
					  .reserved2 = 0,
				  },
				  detail::dds_dx10_header{
					  .dxgi_format		  = static_cast<uint32>(dxgi_format),
					  .resource_dimension = resource_dim,
					  .misc_flag		  = misc_flag,
					  .array_size		  = array_size,
					  .misc_flags2		  = 0,
				  });
		buf.write_bytes(src.data(), src.size_bytes());

		return write_raw_file(dst, buf);
	}
}	 // namespace age::asset::texture