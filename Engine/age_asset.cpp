#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	namespace detail
	{
		constexpr std::align_val_t
		get_alignment(age::asset::type asset_type)
		{
			switch (asset_type)
			{
			default:
			{
				return std::align_val_t{ alignof(float32) };
			}
			}
		}
	}	 // namespace detail

	handle
	load_from_file(const std::string_view& file_path) noexcept
	{
		AGE_ASSERT(std::filesystem::exists(file_path));
		AGE_ASSERT(std::filesystem::file_size(file_path) > 0);

		auto size	= std::filesystem::file_size(file_path);
		auto p_blob = ::operator new(size, detail::get_alignment());

		std::ifstream file{ shader_path, std::ios::in | std::ios::binary };

		file.read((char*)p_blob, size);

		file.close();

		return { .id = g::shader_blob_vec.emplace_back(shader_blob{ .p_blob = p_blob, .size = size }) };
	}

	void
	deinit() noexcept
	{
		for (auto& asset_data : g::asset_data_vec)
		{
			::operator delete(asset_data.blob.data(), detail::get_alignment(asset_data.type));
		}

		if constexpr (age::config::debug_mode)
		{
			g::asset_data_vec.debug_validate();
		}

		g::asset_data_vec.clear();
	}
}	 // namespace age::asset