#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	namespace detail
	{
		constexpr std::align_val_t
		get_alignment(asset::e::kind asset_kind)
		{
			switch (asset_kind)
			{
			case e::kind::font:
			{
				return std::align_val_t{ alignof(font::asset_header) };
			}
			default:
			{
				return std::align_val_t{ alignof(float32) };
			}
			}
		}
	}	 // namespace detail

	bool
	validate(const file_header& header, const std::ifstream& file) noexcept
	{
		static_assert(sizeof(file_header) == 24);

		auto res = true;

		res &= (header.magic == g::asset_header_magic);
		AGE_ASSERT(res);

		res &= (header.header_size >= sizeof(file_header));
		AGE_ASSERT(res);

		res &= (header.file_size >= sizeof(file_header));
		AGE_ASSERT(res);

		res &= (header.version_major == age::config::version_major);
		res &= (header.version_minor == age::config::version_minor);
		AGE_ASSERT(res);

		return res;
	}

	handle
	load_from_file(const std::string_view& file_path) noexcept
	{
		c_auto full_path = std::format("{}{}", file_path, config::asset_extension);
		if (std::filesystem::exists(full_path) is_false)
		{
			return { age::get_invalid_id<t_asset_id>() };
		}

		AGE_ASSERT(std::filesystem::exists(full_path));
		AGE_ASSERT(std::filesystem::file_size(full_path) > 0);

		std::ifstream file{ std::filesystem::path{ full_path }, std::ios::in | std::ios::binary };
		AGE_ASSERT(file.is_open());

		c_auto file_size = std::filesystem::file_size(full_path);
		AGE_ASSERT(sizeof(file_header) <= file_size);

		auto header = asset::file_header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(file_header));

		AGE_ASSERT(header.file_size == file_size);
		c_auto blob_size = file_size - header.header_size;
		c_auto alignment = detail::get_alignment(header.asset_kind);
		auto*  p_blob	 = (std::byte*)::operator new(blob_size, alignment);

		file.read((char*)p_blob, blob_size);

		file.close();

		return asset::handle{
			.id = g::asset_data_vec.emplace_back(asset::data{
				.asset_kind = header.asset_kind,
				.blob		= std::span<std::byte>{ p_blob, blob_size },
				.alignment	= alignment,
				.path		= std::string{ full_path } })
		};
	}

	handle
	load_from_blob(const std::string_view& file_path, const file_header& header, auto& blob) noexcept
	{
		c_auto full_path = std::format("{}{}", file_path, config::asset_extension);
		c_auto blob_size = header.file_size - header.header_size;
		c_auto alignment = detail::get_alignment(header.asset_kind);
		return asset::handle{
			.id = g::asset_data_vec.emplace_back(asset::data{
				.asset_kind = header.asset_kind,
				.blob		= std::span<std::byte>{ &blob, blob_size },
				.alignment	= detail::get_alignment(header.asset_kind),
				.path		= full_path })
		};
	}

	void
	write_to_file(const std::string_view& file_path, const file_header& header, const auto& asset_data) noexcept
	{
		c_auto		  full_path = std::format("{}{}", file_path, config::asset_extension);
		std::ofstream file(std::filesystem::path{ full_path },
						   std::ios::out | std::ios::binary | std::ios::trunc);

		AGE_ASSERT(header.file_size > header.header_size);

		file.write(reinterpret_cast<const char*>(&header), header.header_size);
		file.write(reinterpret_cast<const char*>(&asset_data), header.file_size - header.header_size);

		file.close();
	}

	void
	unload(asset::handle& h_asset) noexcept
	{
		auto& asset_data = g::asset_data_vec[h_asset];
		::operator delete(asset_data.blob.data(), asset_data.alignment);

		if constexpr (age::config::debug_mode)
		{
			asset_data.blob = std::span<std::byte>{ (std::byte*)nullptr, 0 };
		}

		g::asset_data_vec.remove(h_asset);

		h_asset.id = age::get_invalid_id<t_asset_id>();
	}

	void
	deinit() noexcept
	{
		for (auto& asset_data : g::asset_data_vec)
		{
			::operator delete(asset_data.blob.data(), detail::get_alignment(asset_data.asset_kind));
		}

		if constexpr (age::config::debug_mode)
		{
			g::asset_data_vec.debug_validate();
		}

		g::asset_data_vec.clear();
	}
}	 // namespace age::asset