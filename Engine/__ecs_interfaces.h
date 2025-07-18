#pragma once

template <typename t_game>
struct interface_game
{
	t_game& game;

	// t_game game;

	// interface_game(t_game& game) : game(game) { }

	// interface_game(t_game&& game) : game(std::move(game)) { }

	inline void
	init()
	{
		game.init();
	}

	inline auto
	get_current_scene_idx()
	{
		return game.current_scene_idx;
	}

	inline auto
	get_running()
	{
		return game.running;
	}

	inline void
	deinit()
	{
		game.deinit();
	}

	template <typename scene_t>
	inline decltype(auto)
	get_scene()
	{
		return game.get_scene<scene_t>();
	}
};

template <typename t>
struct interface_init
{
	t& obj;

	inline void
	init()
	{
		obj.init();
	}
};

template <typename t_scene>
struct interface_scene
{
	t_scene& scene;

	inline void
	init()
	{
		scene.init();
	}

	template <typename t_world>
	decltype(auto)
	get_world()
	{
		return scene.get_world<t_world>();
	}
};

template <typename t_world>
struct i_world
{
	t_world& world;

	void inline init()
	{
	}
};

template <typename t_entity_storage>
struct i_entity_storage
{
	using t_ent_storage		 = std::remove_reference_t<t_entity_storage>;
	using t_ent_id			 = typename t_ent_storage::t_ent_id;
	using t_archetype_traits = typename t_ent_storage::t_archetype_traits;
	using t_archetype		 = typename t_ent_storage::t_archetype;
	using t_storage_cmp_idx	 = typename t_ent_storage::t_storage_cmp_idx;
	using t_local_cmp_idx	 = typename t_ent_storage::t_local_cmp_idx;
	using t_entity_group_idx = typename t_ent_storage::t_entity_group_idx;
	using t_entity_group	 = typename t_ent_storage::t_entity_group;
	using t_local_entity_idx = typename t_ent_storage::t_local_entity_idx;

	t_entity_storage& entity_storage;

	FORCE_INLINE
	std::size_t
	entity_count() const noexcept
	{
		return entity_storage.entity_count();
	}

	FORCE_INLINE bool
	is_valid(t_ent_id id) const
	{
		return entity_storage.is_valid(id);
	}

	template <typename... t_cmp, typename... t_arg>
	FORCE_INLINE t_ent_id
	new_entity(t_arg&&... arg)
	{
		return entity_storage.new_entity(FWD(arg)...);
	}

	FORCE_INLINE void
	remove_entity(const t_ent_id id)
	{
		return entity_storage.remove_entity(id);
	}

	template <typename... t_cmp, typename... t_arg>
	FORCE_INLINE void
	add_component(const t_ent_id id, t_arg&&... arg)
	{
		return entity_storage.add_component<t_cmp...>(id, FWD(arg)...);
	}

	template <typename... t_cmp>
	FORCE_INLINE void
	remove_component(const t_ent_id id)
	{
		return entity_storage.remove_component<t_cmp...>(id);
	}

	template <typename... t_cmp>
	FORCE_INLINE decltype(auto)
	get_component(const t_ent_id id)
	{
		return entity_storage.get_component<t_cmp...>(id);
	}

	template <typename... t_cmp>
	FORCE_INLINE bool
	has_component(const t_ent_id id)
	{
		return entity_storage.has_component<t_cmp...>(id);
	}

	template <typename t_query, typename t_sys>
	FORCE_INLINE decltype(auto)
	foreach_group(t_query&& group_query, t_sys&& sys)
	{
		return entity_storage.foreach_group(FWD(group_query), FWD(sys));
	}

	template <typename t_query, typename t_sys>
	FORCE_INLINE decltype(auto)
	foreach_entity(t_query&& ent_query, t_sys&& sys)
	{
		return entity_storage.foreach_entity(FWD(ent_query), FWD(sys));
	}

	FORCE_INLINE void
	init()
	{
		entity_storage.init();
	}

	FORCE_INLINE void
	deinit()
	{
		entity_storage.deinit();
	}
};

template <typename t_entity_group>
struct i_entity_group
{
	using t_ent_group			= std::remove_reference_t<t_entity_group>;
	using t_ent_id				= typename t_ent_group::t_ent_id;
	using t_ent_group_idx		= typename t_ent_group::t_ent_group_idx;
	using t_local_entity_idx	= typename t_ent_group::t_local_entity_idx;
	using t_storage_cmp_idx		= typename t_ent_group::t_storage_cmp_idx;
	using t_archetype			= typename t_ent_group::t_archetype;
	using t_entity_count		= typename t_ent_group::t_entity_count;
	using t_capacity			= typename t_ent_group::t_capacity;
	using t_local_cmp_idx		= typename t_ent_group::t_local_cmp_idx;
	using t_component_count		= typename t_ent_group::t_component_count;
	using t_component_size		= typename t_ent_group::t_component_size;
	using t_component_offset	= typename t_ent_group::t_component_offset;
	using t_cmp_offset_arr_base = typename t_ent_group::t_cmp_offset_arr_base;
	using t_cmp_size_arr_base	= typename t_ent_group::t_cmp_size_arr_base;
	using t_entity_id_arr_base	= typename t_ent_group::t_entity_id_arr_base;

	t_entity_group& entity_group;

	FORCE_INLINE t_ent_group_idx&
	entity_group_idx()
	{
		return entity_group.entity_group_idx();
	}

	FORCE_INLINE t_entity_count&
	entity_count()
	{
		return entity_group.entity_count();
	}

	FORCE_INLINE t_capacity&
	capacity()
	{
		return entity_group.capacity();
	}

	FORCE_INLINE t_component_count&
	component_count()
	{
		return entity_group.component_count();
	}

	FORCE_INLINE t_archetype&
	local_archetype()
	{
		return entity_group.local_archetype();
	}

	FORCE_INLINE t_cmp_size_arr_base&
	component_size_arr_base()
	{
		return entity_group.component_size_arr_base();
	}

	FORCE_INLINE t_cmp_offset_arr_base&
	component_offset_arr_base()
	{
		return entity_group.component_offset_arr_base();
	}

	FORCE_INLINE t_entity_id_arr_base&
	entity_id_arr_base()
	{
		return entity_group.entity_id_arr_base();
	}

	FORCE_INLINE t_ent_id&
	ent_id(t_local_entity_idx ent_idx)
	{
		return entity_group.ent_id(ent_idx);
	}

	template <ecs::component_type... t_cmp>
	FORCE_INLINE decltype(auto)
	get_component(const t_local_entity_idx local_ent_idx)
	{
		return entity_group.get_component<t_cmp...>(local_ent_idx);
	}

	template <typename t_sys>
	FORCE_INLINE decltype(auto)
	foreach_entity(t_sys&& sys)
	{
		return entity_group.foreach_entity(FWD(sys));
	}

	FORCE_INLINE bool
	is_full()
	{
		return entity_group.is_full();
	}

	FORCE_INLINE bool
	is_empty()
	{
		return entity_group.is_empty();
	}
};
