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
		return p_blob is_nullptr;
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

	const void*
	entry<e::kind::env_light>::get_texture_buffer() const noexcept
	{
		return p_blob + sizeof(header);
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
		return false;
	}
}	 // namespace age::asset::env_light