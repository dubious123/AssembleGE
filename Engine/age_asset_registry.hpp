#pragma once
#include "age.hpp"

namespace age::asset
{
	struct mesh_baked_entry
	{
		const char* path;
		handle		h_asset;		 // invalid if cpu unloaded
		uint8		status_flags;	 // [gpu_loaded : 1][reserved : 7]
		uint8_3		_;

									 // struct mesh_data
		//{

		// age::graphics::rt::blas_handle h_blas;

		// 1. remove blas handles
		// 2. move mesh data from renderer to asset registry

		uint32 blas_offset;
		uint32 blas_size;

		// asset_path_type path;
		uint32 mesh_offset;
		uint32 mesh_byte_size;
		uint32 meshlet_count;
		uint32 rt_index_buffer_elem_offset;
		// rt::blas_handle h_blas;

		//};
	};

	struct texture_entry
	{
		const char* path;
		handle		h_asset;
		// todo
	};

	struct material_entry
	{
		const char* path;
		handle		h_asset;
		// todo
	};

	struct registry
	{
		using t_asset_path = std::array<char, config::max_asset_path_len>;

		struct entry_handle
		{
			uint32 id = age::get_invalid_id<uint32>();	  // [kind : 8][idx : 24]

			e::kind
			get_kind() const noexcept;

			uint32
			get_idx() const noexcept;

			bool
			is_valid() const noexcept;

			static entry_handle
			make(e::kind, uint32 idx) noexcept;
		};

		std::filesystem::path file_path;

		age::sparse_vector<mesh_baked_entry> mesh_baked_entry_vec;
		age::sparse_vector<texture_entry>	 texture_entry_vec;
		age::sparse_vector<material_entry>	 material_entry_vec;

		std::array<age::unordered_map<t_asset_path, uint32>, e::kind_size> path_to_id_map;

		void
		load(std::filesystem::path file_path) noexcept;

		void
		save() noexcept;

		// registry_entry&
		// get_entry(entry_handle _) noexcept;

		// const registry_entry&
		// get_entry(entry_handle _) const noexcept;

		entry_handle
		get_handle(e::kind, const t_asset_path&) const noexcept;

		entry_handle
		register_asset(e::kind, const t_asset_path&) noexcept;

		void
		unregister_asset(entry_handle&) noexcept;

		bool
		cpu_loaded(entry_handle _) noexcept;

		bool
		ready_to_use(entry_handle _) noexcept;

		age::unordered_map<t_asset_path, uint32>&
		get_path_map(e::kind _) noexcept;
	};
}	 // namespace age::asset