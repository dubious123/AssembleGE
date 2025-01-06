#pragma once
#include "__common.h"
#include "__meta.h"
#include "__ecs.h"

// todo more generic reflection

#define OFFSET(struct_name, field_name) ((size_t) & (((__detail__struct_name*)(nullptr))->field_name))

#define COMPONENT_BEGIN(struct_name)                                                                 \
	struct struct_name                                                                               \
	{                                                                                                \
	  public:                                                                                        \
		static constinit const uint64 id = STR_HASH(#struct_name);                                   \
                                                                                                     \
                                                                                                     \
	  private:                                                                                       \
		typedef struct_name		  __detail_self;                                                     \
		static inline const char* __detail__struct_name = []() {                                     \
			/*reflection::register_struct(id, #struct_name);               \*/                       \
			reflection::register_struct(#struct_name, id, reflection::malloc_struct<struct_name>()); \
			return #struct_name;                                                                     \
		}();                                                                                         \
                                                                                                     \
	  public:


#define __SERIALIZE_FIELD(type, field_name, ...)                                                                                                                        \
	type field_name { __VA_ARGS__ };                                                                                                                                    \
                                                                                                                                                                        \
  private:                                                                                                                                                              \
	static inline bool __detail__field_init_##field_name = []() {                                                                                                       \
		/*reflection::register_field(id, __detail__struct_name, #type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>(), #__VA_ARGS__); \*/ \
		reflection::register_field(__detail__struct_name, primitive_type_##type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>());         \
		return false;                                                                                                                                                   \
	}();                                                                                                                                                                \
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

template <typename _Tstruct, typename _Tfield, typename _Tfield _Tstruct::*Field>
static size_t field_offset()
{
	return (size_t) & ((_Tstruct*)(0)->*Field);
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

		component_info(size_t size, void* p_value) : size(size), p_value(p_value) {};
	};

	struct entity_info
	{
		const char*		 name;
		ecs::entity_idx	 idx;
		ecs::archetype_t archetype;
	};

	void register_struct(const char* name, uint64 hash_id, void* p_value);

	void register_field(const char* struct_name, e_primitive_type type, const char* name, size_t offset);

	void register_scene(const char* name);

	void register_world(const char* name, const ecs::world_base& w);

	void register_component_to_world(uint64 struct_hash_id);

	void register_entity(const char* name, ecs::entity_idx idx, const ecs::world_base& w);

	EDITOR_API size_t get_registered_struct_count();
	EDITOR_API size_t get_registered_scene_count();
	EDITOR_API size_t get_registered_world_count();
	EDITOR_API size_t get_registered_entity_count(size_t world_idx);

	EDITOR_API struct_info*	  get_struct_info(uint64 component_id);
	EDITOR_API scene_info*	  get_scene_info(size_t index);
	EDITOR_API world_info*	  get_world_info(size_t index);
	EDITOR_API component_info get_component_info(size_t world_idx, size_t entity_idx, size_t component_idx);
	EDITOR_API entity_info*	  get_entity_info(size_t world_idx, size_t entity_idx);
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