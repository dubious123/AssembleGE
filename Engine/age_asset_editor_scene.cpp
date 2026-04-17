#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::editor
{
	bool
	validate(const scene_asset_header& header) noexcept
	{
		// todo

		return true;
	}

	const std::byte*
	archetype_data::get_entity_blob_ptr(const scene_data& scene) const noexcept
	{
		return scene.scene_blob.data() + entity_blob_byte_offset;
	}

	std::span<const uint32>
	archetype_data::get_component_data_idx_buffer(const scene_data& scene) const noexcept
	{
		return std::span{ scene.component_data_idx_buffer.data() + component_data_idx_buffer_offset, component_count };
	}

	std::span<const archetype_data>
	entity_storage_data::get_archetype_buffer(const scene_data& scene) const noexcept
	{
		return std::span{ scene.archetype_data_buffer.data() + archetype_data_buffer_offset, archetype_count };
	}

	scene_data::scene_data(std::span<const std::byte> blob, const scene_asset_header& header) noexcept
	{
		version = header.version;
		std::ranges::copy(header.name, name);


		blob = age::dynamic_array(std::from_range, blob);


		component_data_buffer = std::span{ std::start_lifetime_as_array<const component_data>(
											   blob.data() + header.component_data_buffer_byte_offset,
											   header.component_count),
										   header.component_count };

		component_data_idx_buffer = std::span{ std::start_lifetime_as_array<const uint32>(
												   blob.data() + header.component_data_idx_buffer_byte_offset,
												   header.component_count),
											   header.component_count };

		entity_storage_data_buffer = std::span{ std::start_lifetime_as_array<const entity_storage_data>(
													blob.data() + header.entity_storage_buffer_byte_offset,
													header.entity_storage_count),
												header.entity_storage_count };

		archetype_data_buffer = std::span{ std::start_lifetime_as_array<const archetype_data>(
											   blob.data() + header.entity_storage_buffer_byte_offset,
											   header.archetype_count),
										   header.archetype_count };
	}

	scene_data
	load_scene(std::string_view scene_name) noexcept
	{
		auto h_asset = load_from_file(scene_name);

		AGE_ASSERT(runtime::is_handle_valid(h_asset));

		c_auto& header = h_asset->get_asset_header<e::kind::editor_scene>();

		return scene_data(h_asset->blob, header);

		// for (c_auto& storage : res.entity_storage_data_buffer)
		//{
		//	for (c_auto& archetype : storage.get_archetype_buffer(res))
		//	{
		//		for (auto* p_ent_blob : views::loop(archetype.get_entity_blob_ptr(res), archetype.entity_count, archetype.byte_size_per_entity))
		//		{
		//			c_auto& ent_data = std::start_lifetime_as<const entity_data>(p_ent_blob);

		//			for (auto*	p_cmp_blob = p_ent_blob + sizeof(entity_data);
		//				 c_auto cmp_data_idx : archetype.get_component_data_idx_buffer(res))
		//			{
		//				c_auto& cmp_data = res.component_data_buffer[cmp_data_idx];
		//				// handle component_add;

		//				if (cmp_data.version < 1)
		//				{
		//					// handle migrate
		//				}

		//				p_cmp_blob += cmp_data.byte_size;
		//			}
		//		}
		//	}
		//}

		// return res;
	}

	template <asset::e::kind e_kind>
	file_header
	get_file_header(uint64 file_size) noexcept
	{
		if constexpr (e_kind == asset::e::kind::editor_scene)
		{
			return file_header{
				.magic		   = g::asset_header_magic,
				.header_size   = sizeof(file_header),
				.file_size	   = file_size + sizeof(file_header),
				.version_major = config::version_major,
				.version_minor = config::version_minor,
				.asset_kind	   = e::kind::editor_scene,
			};
		}
		else
		{
			AGE_UNREACHABLE();
		}
	}

	void
	save_scene(const char (&scene_name)[config::max_scene_name_len], std::filesystem::path directory_path, ecs::cx_entity_storage auto&&... storage) noexcept
	{
		// std::filesystem::create_directories(directory_path);

		// c_auto asset_path = directory_path / std::format("{}.editor_scene", scene_name);

		// c_auto storage_data_arr = std::array{ editor::get_storage_data(storage)... };


		// auto scene_header = scene_asset_header{
		//	.version			  = 0,
		//	.entity_storage_count = sizeof...(storage),
		//	.component_count	  = 0,
		// };

		// auto component_data_idx_buffer_


		//	std::memcpy(scene_header.name, scene_name, config::max_scene_name_len);

		// auto buffer = age::byte_buf;

		//// read phase
		// auto cmp_visited = std::array<bool, ecs::component_count>{};

		//{
		//	for (c_auto& data : storage_data_arr)
		//	{
		//		for (c_auto cmp_id : data.component_id_vec)
		//		{
		//			if (cmp_visited[cmp_id])
		//			{
		//				continue;
		//			}
		//			cmp_visited[cmp_id] = true;

		//			++scene_header.component_count;
		//		}
		//	}
		//}

		// buffer.write(scene_header);

		// for (auto&& [cmp_id, visited] : cmp_visited | std::views::enumerate)
		//{
		//	if (visited is_false) { continue; }

		//	c_auto cmp_data = editor::get_component_data(cmp_id);

		//	buffer.write(component_data{
		//		.type_id   = cmp_data.type_id,
		//		.version   = cmp_data.version,
		//		.byte_size = cmp_data.byte_size });
		//}

		// for (c_auto& data : storage_data_arr)
		//{
		//	auto storage_data = entity_storage_data{
		//		.entity_count				  = data.entity_count,
		//		.archetype_data_buffer_offset = buffer.size() + sizeof(entity_storage_data),
		//		.archetype_count			  = data.archetype_count
		//	};

		//	std::memcpy(storage_data.name, data.name, sizeof(data.name));

		//	buffer.write(storage_data);

		//	for (c_auto& archetype : data.archetype_data_vec)
		//	{
		//		auto arch_data = archetype_data{
		//			.component_count		 = archetype.component_id_vec.size<uint32>(),
		//			.entity_count			 = archetype.entity_count,
		//			.entity_blob_byte_offset =,
		//			.byte_size_per_entity	 =,

		//		};
		//	}
		//}

		// c_auto f_header = get_file_header<e::kind::editor_scene>(buffer.size());

		// asset::write_to_file(asset_path.string(), f_header, *buffer.data());
	}
}	 // namespace age::asset::editor
