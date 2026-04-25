#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	namespace detail
	{
		bool
		validate(const file_header& header) noexcept
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
	}	 // namespace detail

	byte_buf
	read_asset_file(std::string_view full_path) noexcept
	{
		auto   ec		 = std::error_code{};
		c_auto file_size = std::filesystem::file_size(full_path, ec);
		if (ec || file_size == 0)
		{
			return {};
		}

		auto file = std::ifstream{ std::filesystem::path{ full_path }, std::ios::in | std::ios::binary };
		if (file.is_open() is_false)
		{
			return {};
		}

		AGE_ASSERT(sizeof(file_header) <= file_size);

		auto header = asset::file_header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(file_header));

		detail::validate(header);
		AGE_ASSERT(header.file_size == file_size);
		AGE_ASSERT(header.header_size <= file_size);

		c_auto blob_size = file_size - header.header_size;

		auto buf = byte_buf::gen_reserved(blob_size);

		file.read(reinterpret_cast<char*>(buf.data()), blob_size);

		buf.move_write_pos(blob_size);

		return buf;
	}

	void
	write_asset_file(const std::filesystem::path& file_path, const file_header& header, const void* p_src) noexcept
	{
		auto file = std::ofstream(file_path, std::ios::out | std::ios::binary | std::ios::trunc);

		AGE_ASSERT(header.file_size > header.header_size);

		file.write(reinterpret_cast<const char*>(&header), header.header_size);
		file.write(reinterpret_cast<const char*>(p_src), header.file_size - header.header_size);

		file.close();
	}
}	 // namespace age::asset

namespace age::asset
{
	void
	deinit() noexcept
	{
		for_each_kind(AGE_LAMBDA(
			<e::kind e_kind>(),
			{
				auto& pool = pool_of<e_kind>();
				for (auto it = pool.begin(); it != pool.end(); ++it)
				{
					auto h = handle::make<e_kind>(it.idx<uint32>());
					destroy_entry<e_kind>(h);
				}
			}

			));

		AGE_ASSERT(g::path_vec.is_empty());

		for (auto& vec : g::registry_map)
		{
			vec.clear();
		}

		for (auto& map : g::registry_path_to_handle_map)
		{
			map.clear();
		}
	}
}	 // namespace age::asset