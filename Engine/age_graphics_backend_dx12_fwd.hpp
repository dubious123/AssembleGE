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
	AGE_DEFINE_ENUM(queue_kind, uint8, direct, compute, copy);

	AGE_DEFINE_ENUM_WITH_VALUE(
		memory_kind, uint8,
		(gpu_only, D3D12_HEAP_TYPE_DEFAULT),			   // DEFAULT
		(cpu_to_gpu, D3D12_HEAP_TYPE_UPLOAD),			   // UPLOAD
		(gpu_to_cpu, D3D12_HEAP_TYPE_READBACK),			   // READBACK
		(cpu_to_gpu_direct, D3D12_HEAP_TYPE_GPU_UPLOAD)	   // GPU Upload Heaps with Resizable BAR

	);

	AGE_DEFINE_ENUM(engine_shader_kind, uint8,
					fullscreen_ms,

					forward_plus_depth_ms,

					forward_plus_skybox_ps,
					forward_plus_skybox_ms,

					forward_plus_light_init_cs,
					forward_plus_light_sort_prepare_cs,

					forward_plus_sort_histogram_cs,
					forward_plus_sort_prefix_cs,
					forward_plus_sort_scatter_cs,

					forward_plus_light_zbin_cs,

					forward_plus_ddgi_update_probe_state_cs,
					forward_plus_ddgi_reduce_ray_sum_cs,
					forward_plus_ddgi_probe_trace_cs,
					forward_plus_ddgi_copy_edge_cs,

					forward_plus_ddgi_render_probes_as,
					forward_plus_ddgi_render_probes_ms,
					forward_plus_ddgi_render_probes_ps,

					forward_plus_opaque_as,
					forward_plus_opaque_ms,
					forward_plus_opaque_ps,

					forward_plus_transparent_rt_cs,
					forward_plus_transparent_blend_ps,

					forward_plus_raycast_cs,

					forward_plus_bloom_prefilter_cs,
					forward_plus_bloom_downsample_cs,
					forward_plus_bloom_upsample_cs,

					forward_plus_post_process_ps,

					forward_plus_selection_outline_mask_as,
					forward_plus_selection_outline_mask_ms,
					forward_plus_selection_outline_mask_ps,
					forward_plus_selection_outline_draw_ps,

					forward_plus_ui_ms,
					forward_plus_ui_ps,

					forward_plus_debug_mesh_as,
					forward_plus_debug_mesh_ms,
					forward_plus_debug_mesh_ps,
					forward_plus_debug_mesh_aot_ps,

					forward_plus_presentation_ms,
					forward_plus_presentation_hdr10_ps,
					forward_plus_presentation_sdr_ps,

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
	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	constexpr inline bool is_shader_visible_v =
		heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	struct descriptor_handle;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	requires(is_shader_visible_v<heap_type>)
	struct descriptor_handle<heap_type>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};
		D3D12_GPU_DESCRIPTOR_HANDLE h_gpu = {};

#ifdef AGE_DEBUG
		uint32 idx = 0;
#endif
	};

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	requires(is_shader_visible_v<heap_type> is_false)
	struct descriptor_handle<heap_type>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};

#ifdef AGE_DEBUG
		uint32 idx = 0;
#endif
	};

	using cbv_desc_handle	  = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;
	using srv_desc_handle	  = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;
	using uav_desc_handle	  = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;
	using rtv_desc_handle	  = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>;
	using dsv_desc_handle	  = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>;
	using sampler_desc_handle = descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	struct descriptor_pool
	{
		using t_descriptor_handle = descriptor_handle<heap_type>;

		ID3D12DescriptorHeap*				  p_descriptor_heap = nullptr;
		uint32								  descriptor_size	= 0;
		age::util::idx_pool<uint32, capacity> desc_idx_pool{};

		t_descriptor_handle start_handle = {};

		constexpr descriptor_pool() noexcept = default;

		AGE_DISABLE_COPY_MOVE(descriptor_pool);

		static consteval bool
		is_shader_visible()
		{
			return is_shader_visible_v<heap_type>;
		}

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

	void
	push_descriptor(const auto&) noexcept;
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
		uint64			fence_value;
	};

	struct deferred_release_data_srv
	{
		resource_handle h_resource;
		e::queue_kind	kind;
		uint64			fence_value;
		srv_desc_handle h_srv;
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
		std::array<descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>, global::frame_buffer_count>
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
		get_h_cpu_desc() noexcept
		{
			return rtv_desc_handle_arr[back_buffer_idx].h_cpu;
		}

		FORCE_INLINE descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>
		h_rtv_desc() noexcept
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