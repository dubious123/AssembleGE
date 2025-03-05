#pragma once
#include "__common.h"
#include "__meta.h"
#include "__ecs.h"

// todo more generic reflection
#define OFFSET(struct_name, field_name) ((size_t)&(((__detail__struct_name*)(nullptr))->field_name))

#define COMPONENT_BEGIN(struct_name)                                                                 \
	struct struct_name                                                                               \
	{                                                                                                \
	  public:                                                                                        \
		static constinit const uint64 id = STR_HASH(#struct_name);                                   \
                                                                                                     \
	  private:                                                                                       \
		typedef struct_name		  __detail_self;                                                     \
		static inline const char* __detail__p_struct_name = []() {                                   \
			reflection::register_struct(#struct_name, id, reflection::malloc_struct<struct_name>()); \
			return #struct_name;                                                                     \
		}();                                                                                         \
                                                                                                     \
	  public:


#define __SERIALIZE_FIELD(type, field_name, ...)                                                                                         \
	type field_name { __VA_ARGS__ };                                                                                                     \
                                                                                                                                         \
  private:                                                                                                                               \
	static inline const char __detail__##field_name = []() {                                                                             \
		reflection::register_field(primitive_type_##type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>()); \
		return 0;                                                                                                                        \
	}();                                                                                                                                 \
                                                                                                                                         \
  public:
#define COMPNENT_END() \
	}                  \
	;

#define REGISTER_COMPONENT_TO_WORLD(component) reflection::register_component_to_world(component::id);

#define SCENE_BEGIN(scene_name) \
	static inline auto& scene_name = []() -> auto& {			\
return reflection::scene_wrapper<#scene_name>::init( 0

#define __WORLD_BEGIN(world_name, ...) \
	, []() {																							\
using w_wrapper	   = reflection::world_wrapper<#world_name __VA_OPT__(,) __VA_ARGS__>;					\
w_wrapper::init_func = [](ecs::world_base& world) -> void {												\
	using world_t = ecs::world<__VA_ARGS__>;															\
	reflection::register_world(#world_name, world);													\
__VA_OPT__( FOR_EACH(REGISTER_COMPONENT_TO_WORLD, __VA_ARGS__))

#define ____ENTITY_BEGIN(entity_name, ...)                         \
	{                                                              \
		auto entity = ((world_t&)world).new_entity<__VA_ARGS__>(); \
		reflection::register_entity(#entity_name, entity, world);

#define ______SET_COMPONENT(type, path, ...) ((world_t&)world).get_component<type>(entity) path = __VA_ARGS__;

#define ____ENTITY_END() }

#define __WORLD_END()   \
	}                   \
	;                   \
                        \
	return w_wrapper(); \
	}                   \
	()

#define SCENE_END() \
	);              \
	}               \
	();

#define SYS_BEGIN(system_name)                                     \
	struct system_name                                             \
	{                                                              \
	  private:                                                     \
		typedef system_name		  __detail_self;                   \
		static inline const char* __detail__sys_name = []() { reflection::register_system_begin(#system_name); return #system_name; }(); \
                                                                   \
	  public:                                                      \
		system_name(const system_name& other)			 = delete; \
		system_name& operator=(const system_name& other) = delete; \
		system_name(system_name&& other)				 = delete; \
		system_name& operator=(system_name&& other)		 = delete;

#define SYS_END()                                          \
  private:                                                 \
	static inline bool __detail_register_sys = []() { \
reflection::register_system<__detail_self>(); \
return true; }(); \
	}                                                      \
	;

template <typename _Tstruct, typename _Tfield, typename _Tfield _Tstruct::* Field>
static size_t field_offset()
{
	return (size_t)&((_Tstruct*)(0)->*Field);
}

namespace reflection
{
	struct field_info
	{
		const char*		 name;
		e_primitive_type type;
		void*			 p_value;
		size_t			 offset;
		// std::string				child_count;
		// std::vector<field_info> childs;
	};

	struct struct_info
	{
		// struct_info();
		// struct_info(uint64 idx, const char* name);
		struct_info(uint64 idx, uint64 hash_id, const char* name, void* p_value);
		struct_info(const struct_info& other)			 = delete;
		struct_info& operator=(const struct_info& other) = delete;
		struct_info(struct_info&& other) noexcept;
		struct_info& operator=(struct_info&& other) noexcept;
		~struct_info();

		uint64		idx;
		uint64		hash_id;
		const char* name;
		size_t		field_count;
		field_info* fields;

		void* p_default_value;
	};

	struct scene_info
	{
		const char* name;
		size_t		world_count;
		size_t		world_idx;
	};

	struct world_info
	{
		const char* name;
		size_t		scene_idx;
		uint64		struct_count;
		uint64		struct_idx_vec[64];

		world_info(const char* name, size_t s_idx, uint64 s_count);
	};

	struct component_info
	{
		size_t size;
		void*  p_value;

		component_info(size_t size, void* p_value) : size(size), p_value(p_value) { };
	};

	struct entity_info
	{
		const char*		 name;
		ecs::entity_idx	 idx;
		ecs::archetype_t archetype;
	};

	struct system_info
	{
		const char* name;
		int32		interfaces[8]		  = { 0 };
		uint32		update_argument_count = 0;
		uint64*		p_arguments			  = nullptr;
	};

	void register_struct(const char* name, uint64 hash_id, void* p_value);

	void register_field(e_primitive_type type, const char* name, size_t offset);

	void register_scene(const char* name);

	void register_world(const char* name, const ecs::world_base& w);

	void register_component_to_world(uint64 struct_hash_id);

	void register_entity(const char* name, ecs::entity_idx idx, const ecs::world_base& w);

	void register_system_begin(const char* name);

	void register_system_function(int type, int param_type);

	void register_system_update(uint32 count, uint64* (*alloc_func)());

	template <typename... args>
	void register_system_update()
	{
		register_system_update(
			sizeof...(args),
			[]() {
				// no need to free
				auto* p_res = (uint64*)malloc(sizeof(uint64) * (sizeof...(args)));
				auto  idx	= 0;
				([&idx, p_res]() {
					p_res[idx] = std::remove_cvref_t<args>::id;
					++idx;
				}(),
				 ...);


				return p_res;
			});
	}

	template <typename c, typename... args>
	void register_update_function(void (c::*func)(args...))
	{
		register_system_function(2, 0);
		register_system_update<args...>();
	}

	template <typename c, typename world_t, typename... args>
	void register_update_function(void (c::*func)(world_t, args...))
	{
		register_system_function(2, 1);
		register_system_update<args...>();
	}

	template <typename c, typename... args>
	void register_update_function(void (c::*func)(ecs::entity_idx, args...))
	{
		register_system_function(2, 2);
		register_system_update<args...>();
	}

	template <typename c, typename world_t, typename... args>
	void register_update_function(void (c::*func)(world_t, ecs::entity_idx, args...))
	{
		register_system_function(2, 3);
		register_system_update<args...>();
	}

	template <typename c, typename... args>
	void register_update_function(void (c::*func)(args...) const)
	{
		register_system_function(2, 0);
		register_system_update<args...>();
	}

	template <typename c, typename world_t, typename... args>
	void register_update_function(void (c::*func)(world_t, args...) const)
	{
		register_system_function(2, 1);
		register_system_update<args...>();
	}

	template <typename c, typename... args>
	void register_update_function(void (c::*func)(ecs::entity_idx, args...) const)
	{
		register_system_function(2, 2);
		register_system_update<args...>();
	}

	template <typename c, typename world_t, typename... args>
	void register_update_function(void (c::*func)(world_t, ecs::entity_idx, args...) const)
	{
		register_system_function(2, 3);
		register_system_update<args...>();
	}

	template <typename system_t>
	void register_system()
	{
		using namespace ecs;
		using world_t = world_base;
		if constexpr (has_on_system_begin<system_t>)
		{
			register_system_function(0, 0);
		}
		else if constexpr (has_on_system_begin_w<system_t, world_t>)
		{
			register_system_function(0, 1);
		}

		if constexpr (has_on_thread_begin<system_t>)
		{
			register_system_function(1, 0);
		}
		else if constexpr (has_on_thread_begin_w<system_t, world_t>)
		{
			register_system_function(1, 1);
		}

		if constexpr (has_update<system_t>)
		{
			register_update_function(&system_t::update);
		}
		else if constexpr (has_update_w<system_t, world_t>)
		{
			register_update_function(&system_t::template update<world_t>);
		}

		if constexpr (has_on_thread_end<system_t>)
		{
			register_system_function(3, 0);
		}
		else if constexpr (has_on_thread_end_w<system_t, world_t>)
		{
			register_system_function(3, 1);
		}

		if constexpr (has_on_system_end<system_t>)
		{
			register_system_function(4, 0);
		}
		else if constexpr (has_on_system_end_w<system_t, world_t>)
		{
			register_system_function(4, 1);
		}
	};

	EDITOR_API size_t get_registered_struct_count();
	EDITOR_API size_t get_registered_scene_count();
	EDITOR_API size_t get_registered_world_count();
	EDITOR_API size_t get_registered_entity_count(size_t world_idx);
	EDITOR_API size_t get_registered_system_count();

	EDITOR_API struct_info*	  get_struct_info(uint64 component_id);
	EDITOR_API scene_info*	  get_scene_info(size_t index);
	EDITOR_API world_info*	  get_world_info(size_t index);
	EDITOR_API component_info get_component_info(size_t world_idx, size_t entity_idx, size_t component_idx);
	EDITOR_API entity_info*	  get_entity_info(size_t world_idx, size_t entity_idx);
	EDITOR_API system_info*	  get_system_info(size_t system_idx);
}	 // namespace reflection

namespace reflection
{
	template <typename t>
	void* malloc_struct()
	{
		void* ptr = malloc(sizeof(t));
		new (ptr) t();
		return ptr;
	}

	template <meta::string_wrapper str_wrapper, typename... c>
	struct world_wrapper
	{
		using world_type = ecs::world<c...>;
		static inline void (*init_func)(ecs::world_base& world_base);
	};

	template <meta::string_wrapper str_wrapper>
	struct scene_wrapper
	{
		template <typename... w_wrapper>
		static auto& init(int place_holder, w_wrapper&&...)
		{
			static auto s = ecs::scene<typename w_wrapper::world_type...>();
			reflection::register_scene(str_wrapper.value);

			auto idx = 0;
			([&]() {
				w_wrapper::init_func(*(s.worlds + idx));
				++idx;
			}(),
			 ...);

			return s;
		}
	};
}	 // namespace reflection