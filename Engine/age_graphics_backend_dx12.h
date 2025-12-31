#pragma once

// constants
namespace age::graphics::g
{
#if defined AGE_DEBUG
	constexpr auto dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr auto dxgi_factory_flag = UINT{ 0 };
#endif
	constexpr auto minimum_feature_level = D3D_FEATURE_LEVEL_12_1;
	constexpr auto frame_buffer_count	 = 3;
}	 // namespace age::graphics::g

// cmd_system fwd
namespace age::graphics
{
	template <auto cmd_list_type, auto cmd_list_count>
	struct cmd_system
	{
		ID3D12CommandQueue*			p_cmd_queue = nullptr;
		ID3D12GraphicsCommandList9* cmd_list_pool[g::frame_buffer_count][cmd_list_count]{ nullptr };
		ID3D12CommandAllocator*		cmd_allocator_pool[g::frame_buffer_count][cmd_list_count]{ nullptr };
		ID3D12Fence1*				p_fence		= nullptr;
		HANDLE						fence_event = nullptr;


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
	};
}	 // namespace age::graphics

// swap chain fwd
namespace age::graphics
{
	struct render_surface
	{
		explicit constexpr render_surface(platform::window_handle w_handle) : w_handle{ w_handle } { };

		platform::window_handle w_handle;

		IDXGISwapChain4*								  p_swap_chain = nullptr;
		descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV> rtv_desc_handle_arr[g::frame_buffer_count];
		ID3D12Resource*									  back_buffer_ptr_arr[g::frame_buffer_count];

		UINT present_flags;

		D3D12_VIEWPORT default_viewport;
		D3D12_RECT	   default_scissor_rect;

		// for closing pending (present)
		HANDLE present_waitable_obj;

		// for closing pending (direct cmd list)
		uint64 last_used_cmd_fence_value = 0;

		uint32 back_buffer_idx;

		FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE
		get_rtv() noexcept
		{
			return rtv_desc_handle_arr[back_buffer_idx].h_cpu;
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
	enum class memory_kind : uint8
	{
		gpu_only   = D3D12_HEAP_TYPE_DEFAULT,	  // DEFAULT
		cpu_to_gpu = D3D12_HEAP_TYPE_UPLOAD,	  // UPLOAD
		gpu_to_cpu = D3D12_HEAP_TYPE_READBACK,	  // READBACK
		count
	};

	struct d3d12_resource_desc
	{
		D3D12_RESOURCE_DESC	  d3d12_desc;
		D3D12_CLEAR_VALUE	  clear_value;
		D3D12_RESOURCE_STATES initial_state;
		memory_kind			  heap_memory_kind;
		bool				  has_clear_value;
	};

	struct d3d12_resource
	{
		ID3D12Resource* p_resource;

		void
		release()
		{
			p_resource->Release();
			if constexpr (age::config::debug_mode)
			{
				p_resource = nullptr;
			}
		}
	};

	void
	init() noexcept;

	void
	deinit() noexcept;

	resource_handle
	create_resource(d3d12_resource_desc desc) noexcept;

	void
		release_resource(resource_handle) noexcept;
}	 // namespace age::graphics::resource

// internal globals
namespace age::graphics::g
{
	inline auto frame_buffer_idx	= uint8{ 0 };
	inline auto current_fence_value = uint64{ 0 };

	inline auto* p_dxgi_factory = (IDXGIFactory7*)nullptr;
	inline auto* p_main_adapter = (IDXGIAdapter4*)nullptr;
	inline auto* p_main_device	= (ID3D12Device11*)nullptr;

	inline auto cmd_system_direct  = cmd_system<D3D12_COMMAND_LIST_TYPE_DIRECT, 8>{};
	inline auto cmd_system_compute = cmd_system<D3D12_COMMAND_LIST_TYPE_COMPUTE, 2>{};
	inline auto cmd_system_copy	   = cmd_system<D3D12_COMMAND_LIST_TYPE_COPY, 2>{};

	// todo config capacity
	inline auto rtv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048>{};
	inline auto dsv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 512>{};
	inline auto cbv_srv_uav_desc_pool = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512 * 1024>{};
	inline auto sampler_desc_pool	  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 512>{};

	inline auto render_surface_vec = data_structure::stable_dense_vector<render_surface>{ 2 };

	inline auto resource_vec = data_structure::stable_dense_vector<age::graphics::resource::d3d12_resource>{ 2 };
}	 // namespace age::graphics::g