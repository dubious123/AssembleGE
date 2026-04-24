#pragma once
#include "age.hpp"

namespace age::asset::registry
{
	void
	load(const char* root_dir) noexcept;

	void
	save() noexcept;

	void
	register_asset(asset::handle _) noexcept;

	void
	register_asset(e::kind, const char* path) noexcept;

	void
	unregister_asset(asset::handle _) noexcept;

	void
	unregister_asset(e::kind, const char* path) noexcept;

	template <e::kind e_kind>
	asset::handle
	find(const char* path) noexcept
	{
		auto& map = g::registry_path_to_handle_map[to_idx(e_kind)];
		if (auto it = map.find(util::to_fixed_str<config::max_asset_path_len>(path)); it != map.end())
		{
			return it->second;
		}

		return handle{ age::get_invalid_id<t_asset_id>() };
	}

	std::span<const asset::handle>
	by_kind(e::kind k) noexcept;
}	 // namespace age::asset::registry

//
// namespace age::asset::registry::g
//{
//
// }	 // namespace age::asset::registry::g
//
// namespace age::asset::registry
//{
//	using t_entry_handle_id = uint32;
//
//	struct entry_handle
//	{
//		t_entry_handle_id id = age::get_invalid_id<t_entry_handle_id>();	// [kind : 8][idx : 24]
//
//		e::kind
//		get_kind() const noexcept;
//
//		uint32
//		get_idx() const noexcept;
//
//		bool
//		is_valid() const noexcept;
//
//		static entry_handle
//		make(e::kind, uint32 idx) noexcept;
//
//		template <e::kind>
//		auto&
//		get_entry() noexcept;
//	};
//
//	void
//	init(const char* path) noexcept;
//
//	void
//	save_registry() noexcept;
//
//	void
//	copy_registry(const char* path) noexcept;
//
//	void
//	deinit() noexcept;
//
//	template <e::kind>
//	entry_handle
//	register_from_file(const char* path) noexcept;
//
//	template <e::kind>
//	entry_handle
//	register_from_memory(auto&&, const char* path = nullptr) noexcept;
//
//	template <e::kind>
//	void
//	unregister(entry_handle&) noexcept;
//
//	template <e::kind>
//	void
//	load_from_file(entry_handle _) noexcept;
//
//	template <e::kind>
//	void
//	release_from_cpu(entry_handle _) noexcept;
//
//	template <e::kind>
//	void
//	load_to_renderer(entry_handle, auto& renderer) noexcept;
//
//	template <e::kind>
//	void
//	release_from_renderer(entry_handle, auto& renderer) noexcept;
//
//	template <e::kind>
//	void
//	save_asset(entry_handle _) noexcept;
//
//	template <e::kind>
//	void
//	copy_asset(entry_handle _, const char* path) noexcept;
//
//	template <e::kind>
//	auto&
//	get_entry(entry_handle _) noexcept;
//
//	template <e::kind>
//	auto&
//	get_cold(entry_handle _) noexcept;
//
//	template <e::kind>
//	entry_handle
//	find_entry(const char* path) noexcept;
//
//	template <e::kind>
//	auto&
//	get_path_to_handle_map() noexcept;
//
//	template <e::kind>
//	bool
//	is_cpu_loaded(entry_handle _) noexcept;
//
//	template <e::kind>
//	bool
//	is_gpu_loaded(entry_handle _) noexcept;
//
//	void
//	save_all_cpu_loaded_asset() noexcept;
//
//	void
//	release_all_cpu_loaded_asset() noexcept;
//
//	void
//	release_all_renderer_loaded_asset(auto& renderer) noexcept;
//
//	struct mesh_baked_cold
//	{
//		const std::array<char, config::max_asset_path_len> path;
//
//		handle	h_asset;		 // invalid if cpu unloaded
//		uint8	status_flags;	 // [gpu_loaded : 1][reserved : 7]
//		uint8_3 _;
//
//		entry_handle h_entry;
//	};
//
//	struct mesh_baked_entry
//	{
//		uint32 cold_idx;
//
//		uint32					  offset;
//		uint32					  byte_size;
//		uint32					  meshlet_count;
//		uint32					  rt_index_buffer_elem_offset;
//		graphics::resource_handle h_blas;
//	};
//
//	struct texture_entry
//	{
//		const char* path;
//		handle		h_asset;
//		// todo
//	};
//
//	struct material_entry
//	{
//		const char* path;
//		handle		h_asset;
//		// todo
//	};
//
//	using t_asset_path = std::array<char, config::max_asset_path_len>;
//
//	age::vector<mesh_baked_cold>				   cold_vec;
//	age::sparse_vector<mesh_baked_entry>		   entry_vec;
//	age::unordered_map<t_asset_path, entry_handle> path_to_handle_map;
//
//	struct mesh_baked_registry
//	{
//		using t_asset_path = std::array<char, config::max_asset_path_len>;
//
//		age::vector<mesh_baked_cold>				   cold_vec;
//		age::sparse_vector<mesh_baked_entry>		   entry_vec;
//		age::unordered_map<t_asset_path, entry_handle> path_to_handle_map;
//
//		entry_handle
//		register_mesh_baked_from_file(const char* path) noexcept;
//
//		entry_handle
//		register_from_memory(mesh_baked&&) noexcept;
//
//		void
//		load_from_file(entry_handle _) noexcept;
//
//		void
//		release_from_cpu(entry_handle _) noexcept;
//
//		void
//		load_to_gpu(entry_handle, auto& renderer) noexcept;
//
//		void
//		release_from_renderer(entry_handle, auto& renderer) noexcept;
//
//		void
//		unregister_mesh_baked(entry_handle&) noexcept;
//
//		mesh_baked_entry&
//		get_entry(entry_handle _) noexcept;
//
//		const mesh_baked_entry&
//		get_entry(entry_handle _) const noexcept;
//
//		mesh_baked_cold&
//		get_cold(entry_handle _) noexcept;
//
//		const mesh_baked_cold&
//		get_cold(entry_handle _) const noexcept;
//
//		entry_handle
//		find_mesh_baked(const char* path) const noexcept;
//
//		const auto&
//		get_path_to_handle_map() const noexcept;
//
//		bool
//		is_cpu_loaded(entry_handle _) const noexcept;
//
//		bool
//		is_gpu_loaded(entry_handle _) const noexcept;
//	};
// }	 // namespace age::asset::registry