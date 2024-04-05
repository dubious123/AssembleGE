#pragma once
#include "__common.h"
#include "__meta.h"
#include "__ecs.h"

#define OFFSET(struct_name, field_name) ((size_t) & (((__detail__struct_name*)(nullptr))->field_name))

#define COMPONENT_BEGIN(struct_name)                                           \
	struct struct_name                                                         \
	{                                                                          \
	  public:                                                                  \
		static constinit const uint64 id = STR_HASH(#struct_name);             \
                                                                               \
                                                                               \
	  private:                                                                 \
		typedef struct_name		  __detail_self;                               \
		static inline const char* __detail__struct_name = []() {               \
			/*reflection::register_struct(id, #struct_name);               \*/ \
			reflection::register_struct(#struct_name, id);                     \
			return #struct_name;                                               \
		}();                                                                   \
                                                                               \
	  public:


#define SERIALIZE_FIELD(type, field_name, ...)                                                                                                                          \
	type field_name { __VA_ARGS__ };                                                                                                                                    \
                                                                                                                                                                        \
  private:                                                                                                                                                              \
	static inline bool __detail__field_init_##field_name = []() {                                                                                                       \
		/*reflection::register_field(id, __detail__struct_name, #type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>(), #__VA_ARGS__); \*/ \
		reflection::register_field(__detail__struct_name, #type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>(), #__VA_ARGS__);           \
		return false;                                                                                                                                                   \
	}();                                                                                                                                                                \
                                                                                                                                                                        \
  public:
#define COMPNENT_END() \
	}                  \
	;

// #define SERIALIZE_WORLD(world_name, ...) reflection::world_wrapper<#world_name, __VA_ARGS__>
//
// #define SERIALIZE_SCENE(scene_name, ...) static inline auto scene_name = [] {	   \
//	using t_scene_wrapper = reflection::scene_wrapper<#scene_name,__VA_ARGS__>;	   \
//	t_scene_wrapper();															   \
//	return t_scene_wrapper::scene_type(); }();

#define SCENE_ARGS(world_wrapper_lambda) decltype(world_wrapper_lambda())

#define WORLD_BEGIN(world_name, ...) \
	static inline auto world_name = []() {											\
	using world_wrapper_t = reflection::world_wrapper<#world_name, __VA_ARGS__>;	\
	world_wrapper_t::init_func = [](world_wrapper_t::world_type& w) {				\
	world_wrapper_t::serialize();


#define WORLD_END()           \
	}                         \
	;                         \
	return world_wrapper_t(); \
	}                         \
	;

#define ENTITY_BEGIN(entity_name, ...)        \
	{                                         \
		auto e = w.new_entity<__VA_ARGS__>(); \
		reflection::register_entity(#entity_name, e, (const ecs::world_base&)w);

#define ENTITY_END() }

#define SET_COMPONENT(type, path, ...) w.get_component<type>(e) path = __VA_ARGS__;

#define SERIALIZE_SCENE(scene_name, ...)                                                                       \
	static inline auto& scene_name = []() -> auto& {                                                           \
		using t_scene_wrapper = reflection::scene_wrapper<#scene_name, FOR_EACH_ARG(SCENE_ARGS, __VA_ARGS__)>; \
		t_scene_wrapper();			/*reflction*/                                                              \
		FOR_EACH(CALL, __VA_ARGS__) /*connect init func + world reflection*/                                   \
		/*static auto s = ;*/                                                                                  \
		return t_scene_wrapper::value(); /*scene constructor + world reflection + call world init func*/       \
	}();

template <typename _Tstruct, typename _Tfield, typename _Tfield _Tstruct::*Field>
static size_t field_offset()
{
	return (size_t) & ((_Tstruct*)(0)->*Field);
}

namespace reflection
{
	struct field_info
	{
		const char* name;
		const char* type;
		const char* serialized_value;
		size_t		offset;
		// std::string				child_count;
		// std::vector<field_info> childs;
	};

	struct struct_info
	{
		struct_info();
		struct_info(uint64 idx, const char* name);
		struct_info(uint64 idx, uint64 hash_id, const char* name);
		uint64		idx;
		uint64		hash_id;
		const char* name;
		size_t		field_count;
		field_info* fields;
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
		uint64 struct_idx;
		size_t scene_idx;
		size_t world_idx;
		void*  p_value;
	};

	struct entity_info
	{
		const char*		 name;
		ecs::entity_idx	 idx;
		ecs::archetype_t archetype;
	};

	void register_struct(const char* name, uint64 hash_id);

	void register_field(const char* struct_name, const char* type, const char* name, size_t offset, const char* serialized_value);

	void register_scene(const char* name);

	void register_world(const char* name);

	void register_component_to_world(uint64 struct_hash_id);

	void register_entity(const char* name, ecs::entity_idx idx, const ecs::world_base& w);

	EDITOR_API size_t get_registered_struct_count();
	EDITOR_API size_t get_registered_scene_count();
	EDITOR_API size_t get_registered_world_count();
	EDITOR_API size_t get_registered_entity_count(size_t world_idx);

	EDITOR_API struct_info*	   get_struct_info(uint64 component_id);
	EDITOR_API scene_info*	   get_scene_info(size_t index);
	EDITOR_API world_info*	   get_world_info(size_t index);
	EDITOR_API component_info* get_component_info(size_t index);
	EDITOR_API entity_info*	   get_entity_info(size_t world_idx, size_t entity_idx);
}	 // namespace reflection

namespace reflection
{
	template <meta::string_wrapper str_wrapper, typename... c>
	struct world_wrapper
	{
		using world_type = ecs::world<c...>;

		static inline void (*init_func)(world_type& w);

		static inline void serialize()
		{
			reflection::register_world(str_wrapper.value);
			([] {
				reflection::register_component_to_world(c::id);
			}(),
			 ...);
		}

		world_wrapper()
		{
			// reflection::register_world(str_wrapper.value);
			//([] {
			//	reflection::register_component_to_world(c::id);
			// }(),
			//  ...);
		}
	};

	template <meta::string_wrapper str_wrapper, typename... w_wrapper>
	struct scene_wrapper
	{
		using scene_type = ecs::scene<typename w_wrapper::world_type...>;

		static scene_type& value()
		{
			static auto s	= scene_type();
			auto		idx = 0;
			([&]() {
				auto* p_w = (typename w_wrapper::world_type*)(s.worlds + idx);
				w_wrapper::init_func((typename w_wrapper::world_type&)(*p_w));
				++idx;
			}(),
			 ...);

			return s;
			// static auto v = []() {
			//	auto s	 = scene_type();
			//	auto idx = 0;
			//	([&]() {
			//		// w_wrapper();	// reflection
			//		auto* p_w = (typename w_wrapper::world_type*)(s.worlds + idx);
			//		w_wrapper::init_func((typename w_wrapper::world_type&)(*p_w));
			//		++idx;
			//		// scene_type::worlds auto& world = s.get_world<meta::variadic_index_v<w, w...>>();
			//		// meta::variadic_at_t<meta::variadic_index_v<w, w...>, w...>::init_func(world);
			//	}(),
			//	 ...);

			//	return s;
			//}();

			// return v;
		}

		scene_wrapper()
		{
			reflection::register_scene(str_wrapper.value);
		}
	};
}	 // namespace reflection