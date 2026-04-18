#pragma once
#include "age.hpp"

namespace age::asset
{
	handle
	load_from_blob(std::string_view file_path, const file_header& header, auto& blob) noexcept
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
	write_to_file(std::string_view file_path, const file_header& header, const auto& asset_data) noexcept
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
	write_to_file(const std::filesystem::path& file_path, const file_header& header, const auto& asset_data) noexcept
	{
		c_auto str = file_path.string();
		return write_to_file(std::string_view{ str }, header, asset_data);
	}

}	 // namespace age::asset