#pragma once
#include "age.hpp"

// using
namespace age::graphics
{
	using t_cmd_list = ID3D12GraphicsCommandList9;
}

// constants
namespace age::graphics::g
{
#if defined AGE_DEBUG
	constexpr auto dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr auto dxgi_factory_flag = UINT{ 0 };
#endif
	constexpr auto thread_count = 8;

	constexpr auto minimum_feature_level = D3D_FEATURE_LEVEL_12_1;
	constexpr auto frame_buffer_count	 = 3;

	const auto engine_shaders_dir_path				 = std::filesystem::path{ "./resources/engine_shaders/dx12/" };
	const auto engine_shaders_compiled_blob_dir_path = std::filesystem::path{ "./resources/engine_shaders/dx12/bin/" };
}	 // namespace age::graphics::g

// util
namespace age::graphics
{
	bool
	is_tearing_allowed() noexcept;

	constexpr std::string_view
	to_string(D3D12_RESOURCE_DIMENSION dim) noexcept;

	constexpr std::string_view
		to_string(DXGI_FORMAT) noexcept;
}	 // namespace age::graphics

// cmd_system fwd
namespace age::graphics
{
	template <auto cmd_list_type, auto cmd_list_count>
	struct cmd_system
	{
		ID3D12CommandQueue*		p_cmd_queue = nullptr;
		t_cmd_list*				cmd_list_pool[g::frame_buffer_count][cmd_list_count]{ nullptr };
		ID3D12CommandAllocator* cmd_allocator_pool[g::frame_buffer_count][cmd_list_count]{ nullptr };
		ID3D12Fence1*			p_fence		= nullptr;
		HANDLE					fence_event = nullptr;

		constexpr cmd_system() = default;

		AGE_DISABLE_COPY_MOVE(cmd_system)

		FORCE_INLINE void
		init() noexcept;

		FORCE_INLINE void
		deinit() noexcept;

		FORCE_INLINE void
		wait() noexcept;

		FORCE_INLINE void
		flush() noexcept;

		FORCE_INLINE void
		begin_frame() noexcept;

		FORCE_INLINE void
		end_frame() noexcept;
	};
}	 // namespace age::graphics

// descriptor_pool fwd
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
	};

	void
	pop_descriptor(auto& h_descriptor_out) noexcept;

	void
	push_descriptor(const auto&) noexcept;
}	 // namespace age::graphics

// render_surface, swap chain fwd
namespace age::graphics
{
	struct render_surface
	{
		std::array<descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>, g::frame_buffer_count>
			rtv_desc_handle_arr;

		std::array<ID3D12Resource*, g::frame_buffer_count>
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
		uint64 last_used_cmd_fence_value = 0;

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

// resource
namespace age::graphics::resource
{
	namespace e
	{
		enum class memory_kind : uint8
		{
			gpu_only		  = D3D12_HEAP_TYPE_DEFAULT,	   // DEFAULT
			cpu_to_gpu		  = D3D12_HEAP_TYPE_UPLOAD,		   // UPLOAD
			gpu_to_cpu		  = D3D12_HEAP_TYPE_READBACK,	   // READBACK
			cpu_to_gpu_direct = D3D12_HEAP_TYPE_GPU_UPLOAD,	   // GPU Upload Heaps with Resizable BAR
			count
		};

	}

	struct resource_create_desc
	{
		D3D12_RESOURCE_DESC	  d3d12_resource_desc;
		D3D12_CLEAR_VALUE	  clear_value;
		D3D12_RESOURCE_STATES initial_state;
		e::memory_kind		  heap_memory_kind;
		bool				  has_clear_value;
	};

	struct d3d12_resource
	{
		ID3D12Resource* p_resource = nullptr;

		uint32 map_count = {};
	};

	struct resource_mapping
	{
		std::byte*		ptr;
		resource_handle h_resource;
	};

	using t_mapping_handle_id = uint32;

	struct mapping_handle
	{
		t_mapping_handle_id id;

		FORCE_INLINE resource_mapping*
		operator->() noexcept;
	};

	void
	init() noexcept;

	void
	deinit() noexcept;

	resource_handle
	create_committed(const resource_create_desc& desc) noexcept;

	resource_handle
	create_placed(const resource_create_desc& desc, ID3D12Heap& heap, uint64 offset) noexcept;

	resource::mapping_handle
	create_buffer_committed(uint32					 buffer_byte_size,
							const void*				 p_data = nullptr,
							resource::e::memory_kind kind	= resource::e::memory_kind::cpu_to_gpu_direct,
							D3D12_RESOURCE_STATES	 state	= D3D12_RESOURCE_STATE_COMMON,
							D3D12_RESOURCE_FLAGS	 flags	= D3D12_RESOURCE_FLAG_NONE) noexcept;

	mapping_handle
	create_buffer_placed(uint32				   buffer_byte_size,
						 ID3D12Heap&		   heap,
						 uint64				   offset,
						 const void*		   p_data = nullptr,
						 e::memory_kind		   kind	  = resource::e::memory_kind::cpu_to_gpu_direct,
						 D3D12_RESOURCE_STATES state  = D3D12_RESOURCE_STATE_COMMON,
						 D3D12_RESOURCE_FLAGS  flags  = D3D12_RESOURCE_FLAG_NONE) noexcept;

	void
	release_resource(resource_handle _) noexcept;

	mapping_handle
	map_all(resource_handle _) noexcept;

	void
	unmap(mapping_handle _) noexcept;

	void
	unmap_and_release(mapping_handle _) noexcept;

	FORCE_INLINE void
	create_view(const ID3D12Resource&, const auto& h_desc, const auto& view_desc) noexcept;

	FORCE_INLINE void
	create_view(const graphics::resource_handle& h_resource, const auto& h_desc, const auto& view_desc) noexcept;
}	 // namespace age::graphics::resource

// resource_barrier
namespace age::graphics
{
	struct resource_barrier
	{
		static constexpr uint32						  max_count		= 32;
		uint32										  barrier_count = 0;
		std::array<D3D12_RESOURCE_BARRIER, max_count> barrier_arr	= {};

		FORCE_INLINE void
		add(const D3D12_RESOURCE_BARRIER&) noexcept;

		FORCE_INLINE void
		add_transition(ID3D12Resource&,
					   D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after,
					   D3D12_RESOURCE_BARRIER_FLAGS = D3D12_RESOURCE_BARRIER_FLAG_NONE,
					   uint32 subresource			= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) noexcept;

		FORCE_INLINE void
		add_uav(ID3D12Resource&, D3D12_RESOURCE_BARRIER_FLAGS = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept;

		FORCE_INLINE void
		add_aliasing(ID3D12Resource& p_resource_before, ID3D12Resource& p_resource_after,
					 D3D12_RESOURCE_BARRIER_FLAGS = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept;

		FORCE_INLINE void
		apply_and_reset(t_cmd_list&) noexcept;

		FORCE_INLINE void
		apply(t_cmd_list&) noexcept;

		FORCE_INLINE void
		reset() noexcept;
	};
}	 // namespace age::graphics

// root signature
namespace age::graphics::root_signature
{
	using t_root_signature_id = uint32;

	struct handle
	{
		t_root_signature_id id;
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

	inline void destroy(handle) noexcept;

	inline void
	init() noexcept;

	inline void
	deinit() noexcept;
}	 // namespace age::graphics::root_signature

namespace age::graphics::shader::e
{
	AGE_DEFINE_ENUM(engine_shader_kind, uint8,

					test_vs,
					test_ps,

					fullscreen_triangle_vs,
					fill_color_ps,

					fx_present_ps,

					forward_plus_depth_ms,

					forward_plus_opaque_as,
					forward_plus_opaque_ms,
					forward_plus_opaque_ps,
					forward_plus_presentation_ms,
					forward_plus_presentation_hdr10_ps,
					forward_plus_presentation_sdr_ps);
}

// shader
namespace age::graphics::shader
{
	using t_shader_id = uint32;

	struct shader_handle
	{
		t_shader_id id;
	};

	struct shader_blob
	{
		const void* p_blob;
		std::size_t size;
	};

	void
	init() noexcept;

	void
	deinit() noexcept;

	void
	compile_shader(
		std::wstring_view	  hlsl_path,
		std::wstring_view	  entry_point,
		std::wstring_view	  target,
		std::filesystem::path save_path) noexcept;

	shader_handle
	load_shader(std::filesystem::path shader_path) noexcept;

	D3D12_SHADER_BYTECODE
	get_d3d12_bytecode(shader_handle) noexcept;

	void
		unload_shader(shader_handle) noexcept;
}	 // namespace age::graphics::shader

// internal globals
namespace age::graphics::g
{
	inline auto frame_buffer_idx	= uint8{ 0 };	 // [0, 1 ... ,frame_buffer_count - 1]
	inline auto current_fence_value = uint64{ 0 };

	inline auto* p_dxgi_factory = (IDXGIFactory7*)nullptr;
	inline auto* p_main_adapter = (IDXGIAdapter4*)nullptr;
	inline auto* p_main_device	= (ID3D12Device11*)nullptr;

	//---[ shader ]------------------------------------------------------------
	inline auto* p_dxc_compiler		   = (IDxcCompiler3*)nullptr;
	inline auto* p_dxc_utils		   = (IDxcUtils*)nullptr;
	inline auto* p_dxc_include_handler = (IDxcIncludeHandler*)nullptr;
	//------------------------------------------------------------------------------

	inline auto cmd_system_direct  = cmd_system<D3D12_COMMAND_LIST_TYPE_DIRECT, graphics::g::thread_count>{};
	inline auto cmd_system_compute = cmd_system<D3D12_COMMAND_LIST_TYPE_COMPUTE, graphics::g::thread_count>{};
	inline auto cmd_system_copy	   = cmd_system<D3D12_COMMAND_LIST_TYPE_COPY, graphics::g::thread_count>{};

	// todo config capacity
	inline auto rtv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048>{};
	inline auto dsv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 512>{};
	inline auto cbv_srv_uav_desc_pool = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512 * 1024>{};
	inline auto sampler_desc_pool	  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 512>{};

	inline auto render_surface_vec = data_structure::stable_dense_vector<render_surface>{ 2 };

	inline auto root_signature_ptr_vec = data_structure::stable_dense_vector<ID3D12RootSignature*>{ 2 };

	inline auto pso_ptr_vec = data_structure::stable_dense_vector<ID3D12PipelineState*>{ 2 };

	inline auto shader_blob_vec = data_structure::stable_dense_vector<shader::shader_blob>{ 16 };

	//---[ resource ]--------------------------------------------------------------
	inline auto resource_vec = data_structure::stable_dense_vector<age::graphics::resource::d3d12_resource>{ 2 };

	inline auto resource_mapping_vec = data_structure::stable_dense_vector<resource::resource_mapping>{ 2 };
	//------------------------------------------------------------------------------

	//---[ stage ]------------------------------------------------------------

	//------------------------------------------------------------------------------
}	 // namespace age::graphics::g