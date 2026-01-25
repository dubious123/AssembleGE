#pragma once
#include "age.hpp"	  // intellisense

// using
namespace age::graphics
{
	using t_igraphics_cmd_list = ID3D12GraphicsCommandList9;
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

	constexpr struct
	{
		const D3D12_RASTERIZER_DESC no_cull{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_NONE,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		const D3D12_RASTERIZER_DESC backface_cull{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_BACK,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		const D3D12_RASTERIZER_DESC frontface_cull{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_FRONT,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		const D3D12_RASTERIZER_DESC wireframe{
			D3D12_FILL_MODE_WIREFRAME,					  // FillMode
			D3D12_CULL_MODE_NONE,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};
	} rasterizer_desc;

	constexpr struct
	{
		const D3D12_DEPTH_STENCIL_DESC1 disabled{
			0,									 // DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,		 // DepthWriteMask
			D3D12_COMPARISON_FUNC_LESS_EQUAL,	 // DepthFunc
			0,									 // StencilEnable
			0,									 // StencilReadMask
			0,									 // StencilWriteMask
			{},									 // FrontFace
			{},									 // BackFace
			0									 // DepthBoundsTestEnable
		};
	} depth_stencil_desc1;
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
		t_igraphics_cmd_list*	cmd_list_pool[g::frame_buffer_count][cmd_list_count]{ nullptr };
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
			calc_idx(t_descriptor_handle) noexcept;
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
			rtv_desc_handle_arr{};

		std::array<ID3D12Resource*, g::frame_buffer_count>
			back_buffer_ptr_arr{};

		IDXGISwapChain4* p_swap_chain = nullptr;

		platform::window_handle h_window{};

		DXGI_FORMAT rtv_format{};

		UINT present_flags = 0;
		bool should_render = true;

		D3D12_VIEWPORT default_viewport{};
		D3D12_RECT	   default_scissor_rect{};

		// for closing pending (present)
		HANDLE present_waitable_obj = nullptr;

		// for closing pending (direct cmd list)
		uint64 last_used_cmd_fence_value = 0;

		uint32 back_buffer_idx = 0;

		FORCE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE
		get_rtv() noexcept
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
	create_resource(const d3d12_resource_desc& desc) noexcept;

	void
		release_resource(resource_handle) noexcept;

	FORCE_INLINE void
	create_view(const ID3D12Resource&, const auto& h_desc, const auto& view_desc) noexcept;
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
		apply_and_reset(t_igraphics_cmd_list&) noexcept;

		FORCE_INLINE void
		apply(t_igraphics_cmd_list&) noexcept;

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

	handle
	create(auto&&... root_parameter) noexcept;

	inline void
		destroy(handle) noexcept;

	inline void
	init() noexcept;

	inline void
	deinit() noexcept;
}	 // namespace age::graphics::root_signature

// pipeline
namespace age::graphics
{
	using t_pso_id = uint32;

	struct pso_handle
	{
		t_pso_id id;
	};

	template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pss_type_v, typename t_subobj>
	struct alignas(void*) pss
	{
		// using type = typename pss<pss_type, t_subobj>;

		const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pss_type = pss_type_v;
		t_subobj								  subobj{};
	};

	template <typename... t_pss>
	struct alignas(void*) pss_stream
	{
		static constexpr std::size_t size_in_bytes = (age::util::align_up(sizeof(t_pss), alignof(void*)) + ...);
		alignas(void*)
			std::byte storage[size_in_bytes]{ std::byte{ 0 } };

		constexpr pss_stream(t_pss&&... arg) noexcept
		{
			auto offset = std::size_t{ 0 };

			(([&] {
				 std::memcpy(&storage[offset], &arg, sizeof(arg));
				 offset += age::util::align_up(sizeof(t_pss), alignof(void*));
			 }()),
			 ...);

			AGE_ASSERT(offset == size_in_bytes);
		}
	};

	template <typename... t_pss>
	pss_stream(t_pss&&...) -> pss_stream<t_pss...>;

	using pss_root_signature		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
	using pss_vs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;
	using pss_ps					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
	using pss_ds					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE>;
	using pss_hs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE>;
	using pss_gs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE>;
	using pss_cs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE>;
	using pss_stream_output			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC>;
	using pss_blend					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
	using pss_sample_mask			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT>;
	using pss_rasterizer			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
	using pss_depth_stencil			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
	using pss_input_layout			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>;
	using pss_ib_strip_cut_value	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>;
	using pss_primitive_topology	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>;
	using pss_render_target_formats = pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
	using pss_depth_stencil_format	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
	using pss_sample_desc			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;
	using pss_node_mask				= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, D3D12_NODE_MASK>;
	using pss_cached_pso			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE>;
	using pss_flags					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS, D3D12_PIPELINE_STATE_FLAGS>;
	using pss_depth_stencil1		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1>;
	using pss_view_instancing		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC>;
	using pss_as					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>;
	using pss_ms					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>;
	using pss_depth_stencil2		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2>;
	using pss_rasterizer1			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1, D3D12_RASTERIZER_DESC1>;
	using pss_rasterizer2			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2>;

	pso_handle
	create_pso(void* pss_stream, uint32 size_in_bytes) noexcept;

	void
		destroy_pso(pso_handle) noexcept;
}	 // namespace age::graphics

// shader
namespace age::graphics::shader
{
	using t_shader_id = uint32;

	struct shader_handle
	{
		t_shader_id id;
	};

	enum class shader_type : uint32
	{
		vertex = 0,
		hull,
		domain,
		geometry,
		pixel,
		compute,
		amplification,
		mesh,

		count
	};

	enum class engine_shader : uint32
	{
		test_vs = 0,
		test_ps,

		fullscreen_triangle_vs,
		fill_color_ps,

		fx_present_ps,

		count
	};

	constexpr auto engine_shaders = std::array<std::wstring_view, std::to_underlying(engine_shader::count)>{
		L"test_vs",
		L"test_ps",
		L"fullscreen_triangle_vs",
		L"fill_color_ps",
		L"fx_present_ps"
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

// stage
namespace age::graphics::stage
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	struct my_stage
	{
		AGE_DEFINE_LOCAL_RESOURCE_VIEW(
			main_buffer_view,
			AGE_RESOURCE_VIEW_VALIDATE(
				AGE_VALIDATE_DIMENSION(D3D12_RESOURCE_DIMENSION_TEXTURE2D)),
			AGE_DESC_HANDLE_MEMBER_RTV(
				h_rtv_desc,
				D3D12_RENDER_TARGET_VIEW_DESC{
					.Format		   = DXGI_FORMAT_R16G16B16A16_FLOAT,
					.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
					.Texture2D	   = { .MipSlice = 0, .PlaneSlice = 0 } }),

			AGE_DESC_HANDLE_MEMBER_SRV(
				h_srv_desc,
				D3D12_SHADER_RESOURCE_VIEW_DESC{
					.Format					 = DXGI_FORMAT_R16G16B16A16_FLOAT,
					.ViewDimension			 = D3D12_SRV_DIMENSION_TEXTURE2D,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2D				 = { .MostDetailedMip	  = 0,
												 .MipLevels			  = 1,
												 .PlaneSlice		  = 0,
												 .ResourceMinLODClamp = 0.f } }))

		AGE_DEFINE_LOCAL_RESOURCE_VIEW(
			depth_buffer_view,
			AGE_RESOURCE_VIEW_VALIDATE(
				AGE_VALIDATE_DIMENSION(D3D12_RESOURCE_DIMENSION_TEXTURE2D)),
			AGE_DESC_HANDLE_MEMBER_DSV(
				h_dsv_desc,
				D3D12_DEPTH_STENCIL_VIEW_DESC{
					.Format		   = DXGI_FORMAT_D32_FLOAT,
					.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
					.Flags		   = D3D12_DSV_FLAG_NONE,
					.Texture2D	   = { .MipSlice = 0 } }))

		AGE_RESOURCE_FLOW_PHASE(
			phase_geo,
			AGE_RESOURCE_BARRIER_ARR(
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(main_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						.StateAfter	 = D3D12_RESOURCE_STATE_RENDER_TARGET,
					},
				},
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(depth_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_DEPTH_READ,
						.StateAfter	 = D3D12_RESOURCE_STATE_DEPTH_WRITE,
					},
				}),
			AGE_RENDER_PASS_RT_DESC_ARR({
				.cpuDescriptor	 = main_buffer_view.h_rtv_desc.h_cpu,
				.BeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = {
						.ClearValue = {
							.Format = main_buffer_view.h_rtv_desc_view_desc().Format,
							.Color	= { 0.5f, 0.5f, 0.5f, 1.0f },
						},
					},
				},
				.EndingAccess = { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE },
			}),
			AGE_RENDER_PASS_DS_DESC({
				.cpuDescriptor		  = depth_buffer_view.h_dsv_desc.h_cpu,
				.DepthBeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = {
						.ClearValue = {
							.Format		  = depth_buffer_view.h_dsv_desc_view_desc().Format,
							.DepthStencil = { .Depth = 1.f },
						},
					},
				},
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD },
				.StencilEndingAccess{ .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS },
			}))

		AGE_RESOURCE_FLOW_PHASE(
			phase_fx,
			AGE_RESOURCE_BARRIER_ARR(
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(main_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
						.StateAfter	 = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					},
				},
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(depth_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE,
						.StateAfter	 = D3D12_RESOURCE_STATE_DEPTH_READ,
					},
				}),
			AGE_RENDER_PASS_RT_DESC_ARR(), AGE_RENDER_PASS_DS_DESC())

		ID3D12RootSignature* p_root_sig = nullptr;
		ID3D12PipelineState* p_pso		= nullptr;


		void
		init() noexcept;

		void
		bind(graphics::root_signature::handle,
			 graphics::pso_handle,
			 graphics::resource_handle h_main_buffer,
			 graphics::resource_handle h_depth_buffer) noexcept;

		void
		execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept;

		void
		deinit() noexcept;
	};

	struct my_pipeline
	{
		my_stage stage{};

		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		resource_handle h_main_buffer{};
		resource_handle h_depth_buffer{};

		void
		init() noexcept
		{
			stage.init();
			this->create_buffers();
		}

		void
		execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept;

		void
		deinit() noexcept
		{
			stage.deinit();
		}

	  private:
		void
		create_buffers() noexcept
		{
			h_main_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
				  .clear_value{
					  .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .Color  = { 0.5f, 0.5f, 0.5f, 1.0f } },
				  .initial_state	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });

			h_depth_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_D32_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL },
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 1.f, .Stencil = 0 } },
				  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });
		}

		void
		resize(const age::extent_2d<uint16>& new_extent) noexcept
		{
			extent = new_extent;

			resource::release_resource(h_main_buffer);
			resource::release_resource(h_depth_buffer);

			this->create_buffers();
		}
	};
}	 // namespace age::graphics::stage

// internal globals
namespace age::graphics::g
{
	inline auto frame_buffer_idx	= uint8{ 0 };
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
	//------------------------------------------------------------------------------

	//---[ stage ]------------------------------------------------------------
	inline auto h_geometry_stage_default_pso	  = pso_handle{};
	inline auto h_geometry_stage_default_root_sig = root_signature::handle{};

	inline auto h_fx_present_stage_default_pso		= pso_handle{};
	inline auto h_fx_present_stage_default_root_sig = root_signature::handle{};

	inline auto test_pipeline = stage::my_pipeline{};
	//------------------------------------------------------------------------------
}	 // namespace age::graphics::g