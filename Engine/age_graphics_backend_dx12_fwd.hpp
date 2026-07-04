#pragma once
#include "age.hpp"

// constants
namespace age::graphics::g
{
	constexpr auto minimum_feature_level = D3D_FEATURE_LEVEL_12_1;

	inline const auto engine_shaders_dir_path				= std::filesystem::path{ "./resources/engine_shaders/dx12/" };
	inline const auto engine_shaders_compiled_blob_dir_path = std::filesystem::path{ "./resources/engine_shaders/dx12/bin/" };
}	 // namespace age::graphics::g

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(descriptor_kind, uint8, cbv, srv, uav, rtv, dsv, sampler, clear_uav);

	AGE_DEFINE_ENUM(queue_kind, uint8, direct, compute, copy);

	AGE_DEFINE_ENUM_WITH_VALUE(
		memory_kind, uint8,
		(gpu_only, D3D12_HEAP_TYPE_DEFAULT),			   // DEFAULT
		(cpu_to_gpu, D3D12_HEAP_TYPE_UPLOAD),			   // UPLOAD
		(gpu_to_cpu, D3D12_HEAP_TYPE_READBACK),			   // READBACK
		(cpu_to_gpu_direct, D3D12_HEAP_TYPE_GPU_UPLOAD)	   // GPU Upload Heaps with Resizable BAR

	);

	AGE_DEFINE_ENUM(engine_shader_kind, uint8,
					hrp_fullscreen_ms,

					hrp_gbuffer_prepass_opaque_as,
					hrp_gbuffer_prepass_opaque_ms,

					hrp_gbuffer_prepass_transparent_as,
					hrp_gbuffer_prepass_transparent_ms,

					hrp_gbuffer_prepass_ps,

					hrp_segment_resolve_cs,

					hrp_ao_resolve_cs,

					hrp_skybox_ps,
					hrp_skybox_ms,

					hrp_light_init_cs,
					hrp_light_sort_prepare_cs,

					hrp_sort_histogram_cs,
					hrp_sort_prefix_cs,
					hrp_gibs_tile_surfel_scatter_cs,
					hrp_gibs_cell_surfel_scatter_cs,
					hrp_sort_scatter_cs,

					hrp_light_zbin_cs,

					hrp_ddgi_update_probe_state_cs,
					hrp_ddgi_reduce_ray_sum_cs,
					hrp_ddgi_prefix_group_cs,
					hrp_ddgi_prefix_group_sum_cs,
					hrp_ddgi_prefix_add_cs,

					hrp_ddgi_probe_trace_cs,
					hrp_ddgi_probe_blend_cs,
					hrp_ddgi_copy_edge_cs,

					hrp_ddgi_render_probes_as,
					hrp_ddgi_render_probes_ms,
					hrp_ddgi_render_probes_ps,

					hrp_gibs_cleanup_cs,
					hrp_gibs_tile_spawn_kill_cs,
					hrp_gibs_cell_spawn_kill_cs,
					hrp_gibs_prepare_cs,
					hrp_gibs_update_surfel_cs,
					hrp_gibs_ray_ideal_count_sum_cs,
					hrp_gibs_ray_count_prefix_cs,
					hrp_gibs_ray_entry_cs,
					hrp_gibs_tile_surfel_count_prefix_cs,
					hrp_gibs_cell_surfel_count_prefix_cs,
					hrp_gibs_ray_trace_cs,
					hrp_gibs_ray_integrate_cs,
					hrp_gibs_build_cdf_cs,
					hrp_gibs_gi_resolve_cs,
					hrp_gibs_gi_upscale_cs,
					hrp_gibs_debug_draw_surfels_ps,

					hrp_opaque_ps,

					hrp_aa_opaque_ray_entry_cs,
					hrp_aa_transparent_ray_entry_cs,
					hrp_aa_indirect_arg_cs,
					hrp_aa_opaque_rt_cs,
					hrp_aa_transparent_rt_cs,
					hrp_aa_resolve_ps,

					hrp_transparent_rt_with_aa_cs,
					hrp_transparent_resolve_ps,
					hrp_transparent_no_aa_ps,

					hrp_raycast_cs,

					hrp_bloom_prefilter_cs,
					hrp_bloom_downsample_cs,
					hrp_bloom_upsample_cs,

					hrp_post_process_ps,

					hrp_selection_outline_mask_as,
					hrp_selection_outline_mask_ms,
					hrp_selection_outline_mask_ps,
					hrp_selection_outline_draw_ps,

					hrp_ui_ms,
					hrp_ui_ps,

					hrp_debug_mesh_as,
					hrp_debug_mesh_ms,
					hrp_debug_mesh_ps,
					hrp_debug_mesh_aot_ps,

					hrp_presentation_ms,
					hrp_presentation_hdr10_ps,
					hrp_presentation_sdr_ps,

					bake_brdf_lut_cs,

					bake_env_light_radiance_cs,
					bake_env_light_irradiance_cs,
					bake_env_light_prefilter_cs,
					bake_env_light_build_marginal_cdf_cs,
					bake_env_light_build_conditional_cdf_cs,
					bake_down_sample_cube_cs


	);
}	 // namespace age::graphics::e

// command
namespace age::graphics
{
	struct queue_context
	{
		ID3D12CommandQueue*		p_queue;
		ID3D12Fence1*			p_fence;
		ID3D12CommandAllocator* p_allocator[global::frame_buffer_count][global::thread_count];
		uint64					frame_fence_value[global::frame_buffer_count];
		uint64					fence_value;

		ID3D12GraphicsCommandList9* p_cmd_list[global::thread_count];

		HANDLE h_fence_event;
	};
}	 // namespace age::graphics

// descriptor_pool
namespace age::graphics
{
	template <e::descriptor_kind e_kind>
	consteval bool
	is_shader_visible()
	{
		if constexpr (e_kind == e::descriptor_kind::cbv
					  or e_kind == e::descriptor_kind::srv
					  or e_kind == e::descriptor_kind::uav
					  or e_kind == e::descriptor_kind::sampler)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	template <e::descriptor_kind>
	struct descriptor_handle;

	template <e::descriptor_kind desc_kind>
	requires(is_shader_visible<desc_kind>() is_true)
	struct descriptor_handle<desc_kind>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE h_gpu;
		AGE_DEBUG_ONLY(uint32 idx);
	};

	template <e::descriptor_kind desc_kind>
	requires(is_shader_visible<desc_kind>() is_false and desc_kind != e::descriptor_kind::clear_uav)
	struct descriptor_handle<desc_kind>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu;
		AGE_DEBUG_ONLY(uint32 idx);
	};

	template <e::descriptor_kind desc_kind>
	requires(desc_kind == e::descriptor_kind::clear_uav)
	struct descriptor_handle<desc_kind>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu;
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu_non_shader_visible;
		D3D12_GPU_DESCRIPTOR_HANDLE h_gpu;
		AGE_DEBUG_ONLY(uint32 idx);
		AGE_DEBUG_ONLY(uint32 idx_non_shader_visible);
	};

	using cbv_desc_handle		= descriptor_handle<e::descriptor_kind::cbv>;
	using srv_desc_handle		= descriptor_handle<e::descriptor_kind::srv>;
	using uav_desc_handle		= descriptor_handle<e::descriptor_kind::uav>;
	using rtv_desc_handle		= descriptor_handle<e::descriptor_kind::rtv>;
	using dsv_desc_handle		= descriptor_handle<e::descriptor_kind::dsv>;
	using sampler_desc_handle	= descriptor_handle<e::descriptor_kind::sampler>;
	using clear_uav_desc_handle = descriptor_handle<e::descriptor_kind::clear_uav>;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	struct descriptor_heap
	{
		ID3D12DescriptorHeap*				  p_heap;
		uint32								  descriptor_size;
		age::util::idx_pool<uint32, capacity> desc_idx_pool{};

		template <bool>
		struct handle;

		template <>
		struct handle<true>
		{
			D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};
			D3D12_GPU_DESCRIPTOR_HANDLE h_gpu = {};
			AGE_DEBUG_ONLY(uint32 idx);
		};

		template <>
		struct handle<false>
		{
			D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};
			AGE_DEBUG_ONLY(uint32 idx);
		};

		handle<shader_visible> h_start;

		void
		init() noexcept;

		void
		deinit() noexcept;

		handle<shader_visible>
		pop() noexcept;

		handle<shader_visible>
		get(uint32 idx) noexcept;

		uint32
		calc_idx(handle<shader_visible> h) const noexcept;

		void
		push(handle<shader_visible> h) noexcept;

		uint32
		count() const noexcept;
	};

	template <e::descriptor_kind e_kind, std::size_t capacity>
	struct descriptor_pool
	{
		using t_descriptor_handle = descriptor_handle<e_kind>;

		ID3D12DescriptorHeap* p_descriptor_heap_arr[std::conditional_t<e_kind == e::descriptor_kind::clear_uav,
																	   std::integral_constant<uint32, 2>,
																	   std::integral_constant<uint32, 1>>::value]{};

		uint32								  descriptor_size = 0;
		age::util::idx_pool<uint32, capacity> desc_idx_pool{};

		t_descriptor_handle start_handle = {};

		constexpr descriptor_pool() noexcept = default;

		AGE_DISABLE_COPY_MOVE(descriptor_pool);

		FORCE_INLINE void
		init() noexcept;

		FORCE_INLINE void
		deinit() noexcept;

		t_descriptor_handle
		pop() noexcept;

		t_descriptor_handle
		get(uint32 idx) noexcept;

		void
		push(t_descriptor_handle h) noexcept;

		uint32
		calc_idx(t_descriptor_handle _) noexcept;

		FORCE_INLINE uint32
		count() noexcept
		{
			return capacity - desc_idx_pool.free_count;
		}
	};

	void
	pop_descriptor(auto& h_descriptor_out) noexcept;

	template <typename t_desc>
	t_desc
	pop_descriptor() noexcept;

	void
	push_descriptor(c_auto& handle) noexcept;

	void
	push_descriptor_deferred(c_auto& h_descriptor, e::queue_kind kind = e::queue_kind::direct) noexcept;

	inline void
	process_deferred_desc_pushes() noexcept;

	uint32
	calc_desc_idx(c_auto& handle) noexcept;


}	 // namespace age::graphics

// resource
namespace age::graphics
{
	struct resource_create_desc
	{
		D3D12_RESOURCE_DESC1 d3d12_resource_desc;
		D3D12_CLEAR_VALUE	 clear_value;
		D3D12_BARRIER_LAYOUT initial_layout;
		e::memory_kind		 heap_memory_kind;
		bool				 has_clear_value;
	};

	struct d3d12_resource
	{
		ID3D12Resource*		 p_resource = nullptr;
		resource_create_desc desc;

		uint32 map_count = {};

		FORCE_INLINE std::size_t
		buffer_size() const noexcept;

		FORCE_INLINE D3D12_GPU_VIRTUAL_ADDRESS
		get_va() const noexcept;

		void
		set_name(const wchar_t* name) noexcept;

		std::wstring
		get_name() const noexcept;
	};

	struct resource_mapping
	{
		std::byte*		ptr;
		resource_handle h_resource;

		FORCE_INLINE void
		upload(const void* p_src, std::size_t size, std::size_t offset = 0u) noexcept;

		FORCE_INLINE void
		readback(void* p_dst, std::size_t size, std::size_t offset = 0u) noexcept;

		FORCE_INLINE std::byte*
		get_ptr() noexcept;
	};

	using t_mapping_handle_id = uint32;

	struct mapping_handle
	{
		t_mapping_handle_id id = age::get_invalid_id<t_mapping_handle_id>();

		FORCE_INLINE resource_mapping*
		operator->() noexcept;

		FORCE_INLINE const resource_mapping*
		operator->() const noexcept;
	};

	struct deferred_release_data
	{
		resource_handle h_resource;
		e::queue_kind	kind;
		uint8_3			_;
		uint64			fence_value;
	};

	struct deferred_release_data_srv
	{
		resource_handle h_resource;
		e::queue_kind	kind;
		uint8_3			_;
		uint64			fence_value;
		srv_desc_handle h_srv;
	};

	template <typename t_desc>
	struct deferred_desc_push_data
	{
		using descriptor_type = t_desc;

		uint64 fence_value;
		t_desc h_desc;

		e::queue_kind queue_kind;
		uint8_3		  _;
	};
}	 // namespace age::graphics

// root_signature
namespace age::graphics::root_signature
{
	using t_root_signature_id = uint32;

	struct handle
	{
		t_root_signature_id id = get_invalid_id<t_root_signature_id>();

		FORCE_INLINE decltype(auto)
		operator->() noexcept;

		FORCE_INLINE decltype(auto)
		ptr() const noexcept;
	};

	struct constants;

	template <typename t_reg>
	struct descriptor;

	template <typename t_reg>
	struct descriptor_range;

	template <typename... t_desc_range>
	struct descriptor_table;

	template <typename... t_arg>
	descriptor_table(D3D12_SHADER_VISIBILITY, t_arg&&...) -> descriptor_table<t_arg...>;
}	 // namespace age::graphics::root_signature

// command_signature
namespace age::graphics::command_signature
{
	using t_command_signature_id = uint32;

	struct handle
	{
		t_command_signature_id id = get_invalid_id<t_command_signature_id>();

		FORCE_INLINE decltype(auto)
		ptr() const noexcept;
	};
}	 // namespace age::graphics::command_signature

// shader
namespace age::graphics::shader
{
	using t_shader_id = uint32;

	struct shader_handle
	{
		t_shader_id id = get_invalid_id<t_shader_id>();
	};

	struct shader_blob
	{
		const void* p_blob;
		std::size_t size;
	};
}	 // namespace age::graphics::shader

// render_surface, swap chain
namespace age::graphics
{
	struct render_surface
	{
		std::array<rtv_desc_handle, global::frame_buffer_count>
			rtv_desc_handle_arr;

		std::array<ID3D12Resource*, global::frame_buffer_count>
			back_buffer_ptr_arr;

		IDXGISwapChain4* p_swap_chain = nullptr;

		platform::window_handle h_window;

		DXGI_FORMAT rtv_format;

		UINT present_flags = 0;
		bool should_render = true;

		D3D12_VIEWPORT default_viewport;
		D3D12_RECT	   default_scissor_rect;

		// for closing pending (present)
		HANDLE present_waitable_obj = nullptr;

		// for closing pending (direct cmd list)
		uint64 present_fence_value = 0;

		uint32 back_buffer_idx = 0;

		FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE
		get_h_cpu_desc() const noexcept
		{
			return rtv_desc_handle_arr[back_buffer_idx].h_cpu;
		}

		FORCE_INLINE rtv_desc_handle
		h_rtv_desc() const noexcept
		{
			return rtv_desc_handle_arr[back_buffer_idx];
		}

		FORCE_INLINE ID3D12Resource&
		get_back_buffer() noexcept
		{
			return *(back_buffer_ptr_arr[back_buffer_idx]);
		}

		void
		init(age::platform::window_handle w_handle) noexcept;

		void
		resize() noexcept;

		void
		present() noexcept;

		void
		deinit() noexcept;

	  private:
		void
		rebuild_from_swapchain() noexcept;
	};
}	 // namespace age::graphics