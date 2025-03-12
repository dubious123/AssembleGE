#include "pch.h"
#include "editor.h"
#include "editor_ctx_item.h"
#include "game.h"
#include "editor_models.h"
#include "editor_utilities.h"

namespace
{
	// helper functions
	template <typename t>
	std::string _new_name(std::string base_name, std::vector<t*>&& p_models)
	{
		return [&]() {
			for (auto arg : std::views::iota(0))
			{
				auto new_name = std::format("{}_{}", base_name, arg);
				if (std::ranges::all_of(p_models, [new_name](auto* p_s) { return p_s->name != new_name; }))
				{
					return new_name;
				}
			}

			return std::string();
		}();
	}
}	 // namespace

namespace editor::models::reflection
{
	namespace
	{
		std::unordered_map<editor_id, uint64, editor_id::hash_func>					   _struct_idx_map;
		std::unordered_map<editor_id, std::pair<uint64, uint64>, editor_id::hash_func> _field_idx_map;

		std::vector<em_struct>			   _structs;
		std::vector<std::vector<em_field>> _fields;
	}	 // namespace

	em_struct* find_struct(editor_id id)
	{
		if (_struct_idx_map.contains(id) is_false)
		{
			return nullptr;
		}

		return &_structs[_struct_idx_map[id]];
	}

	em_struct* find_struct(const char* name)
	{
		auto res = std::ranges::find_if(
			_structs, [=](em_struct s) { return s.name == name; });
		return &_structs[std::distance(_structs.begin(), res)];
	}

	editor_id create_struct(uint64 hash_id, game::ecs::struct_idx ecs_idx)
	{
		return create_struct(_new_name("new_struct", reflection::all_structs()), hash_id, ecs_idx);
	}

	editor_id create_struct(std::string name, uint64 hash_id, game::ecs::struct_idx ecs_idx)
	{
		auto  s_id = id::get_new(DataType_Struct);
		auto* p_s  = &_structs.emplace_back();
		_fields.emplace_back();
		_struct_idx_map.insert({ s_id, _structs.size() - 1 });
		p_s->id		 = s_id;
		p_s->name	 = name;
		p_s->hash_id = hash_id;
		p_s->ecs_idx = ecs_idx;

		return s_id;
	}

	void remove_struct(editor_id struct_id)
	{
		if (_struct_idx_map.contains(struct_id) is_false)
		{
			return;
		}

		auto idx						  = _struct_idx_map[struct_id];
		_structs[idx]					  = _structs.back();
		_struct_idx_map[_structs[idx].id] = idx;
		_structs.pop_back();
		_struct_idx_map.erase(struct_id);

		std::ranges::for_each(_fields[idx], [](em_field& f) { remove_field(f.id); });

		_fields[idx] = _fields.back();
		_fields.pop_back();

		std::ranges::for_each(_fields[idx], [=](em_field& f) { _field_idx_map[f.id].first = idx; });

		id::delete_id(struct_id);
	}

	editor_id add_field(editor_id struct_id, e_primitive_type f_type, uint32 offset, game::ecs::field_idx ecs_idx)
	{
		return add_field(struct_id, f_type, _new_name("new_field", reflection::all_fields(struct_id)), offset, ecs_idx);
	}

	editor_id add_field(editor_id struct_id, e_primitive_type f_type, std::string name, uint32 offset, game::ecs::field_idx ecs_idx)
	{
		auto  f_id		 = id::get_new(DataType_Field);
		auto  struct_idx = _struct_idx_map[struct_id];
		auto* p_f		 = &_fields[struct_idx].emplace_back();
		{
			p_f->id		   = f_id;
			p_f->struct_id = struct_id;
			p_f->name	   = name;
			p_f->ecs_idx   = ecs_idx;
			p_f->offset	   = offset;
			p_f->type	   = f_type;
		}

		auto* p_s = reflection::find_struct(struct_id);
		++p_s->field_count;
		p_s->size += reflection::utils::type_size(f_type);

		_field_idx_map.insert({ f_id, { struct_idx, _fields[struct_idx].size() - 1 } });
		return f_id;
	}

	void remove_field(editor_id field_id)
	{
		auto& pair	 = _field_idx_map[field_id];
		auto  s_idx	 = pair.first;
		auto  f_idx	 = pair.second;
		auto& f		 = _fields[f_idx];
		auto& f_vec	 = _fields[s_idx];
		f_vec[f_idx] = f_vec.back();
		f_vec.pop_back();

		_field_idx_map.erase(field_id);
		_field_idx_map[f_vec[f_idx].id].second = f_idx;

		id::delete_id(field_id);
	}

	std::vector<em_struct*> all_structs()
	{
		return std::ranges::to<std::vector>(_structs | std::views::transform([](em_struct& s) { return &s; }));
	}

	std::vector<em_field*> all_fields(editor_id struct_id)
	{
		return std::ranges::to<std::vector>(_fields[_struct_idx_map[struct_id]] | std::views::transform([](em_field& f) { return &f; }));
	}

	em_field* find_field(editor_id id)
	{
		if (_field_idx_map.contains(id) is_false)
		{
			return nullptr;
		}

		auto& pair = _field_idx_map[id];
		return &_fields[pair.first][pair.second];
	}

	std::vector<em_field*> find_fields(std::vector<editor_id> struct_id_vec)
	{
		auto view =
			struct_id_vec
			| std::views::filter([](auto id) { return _field_idx_map.contains(id); })
			| std::views::transform([](auto id) {
				  auto& pair = _field_idx_map[id];
				  return &_fields[pair.first][pair.second];
			  });

		return std::vector<em_field*>(view.begin(), view.end());	// c++ 23 => std::views::to
	}

	void on_project_unloaded()
	{
		_struct_idx_map.clear();
		_field_idx_map.clear();
		_structs.clear();
		_fields.clear();
	}

	void on_project_loaded()
	{
		auto res = true;
		// res		 &= ctx_item::add_context_item("Scene\\Add New World", &editor::models::world::cmd_create);
		// res		 &= ctx_item::add_context_item("World\\Remove World", &editor::models::world::cmd_remove);
		assert(res);
	}
}	 // namespace editor::models::reflection

namespace editor::models
{
	namespace
	{
		constinit auto _rview_entity_components_vec = std::views::transform([](em_entity* p_e) {
														  return std::make_tuple(*p_e,
																				 component::all(p_e->id)
																					 | std::views::reverse
																					 | utilities::deref_view
																					 | std::ranges::to<std::vector>());
													  })
													| std::ranges::to<std::vector>();

		constinit auto _rview_world_entities_vec = std::views::transform([](em_world* p_w) {
													   return std::make_tuple(*p_w,
																			  entity::all(p_w->id)
																				  | std::views::reverse
																				  | _rview_entity_components_vec);
												   })
												 | std::ranges::to<std::vector>();

		constinit auto _rview_scene_worlds_vec = std::views::transform([](em_scene* p_s) {
													 return std::make_tuple(*p_s,
																			world::all(p_s->id)
																				| std::views::reverse
																				| _rview_world_entities_vec);
												 })
											   | std::ranges::to<std::vector>();
	}	 // namespace

	namespace scene
	{
		// todo reserve order after delete, insert => set
		namespace
		{
			// std::vector<em_scene>										_scenes;
			// std::unordered_map<editor_id, uint32, editor_id::hash_func> _idx_map;

			std::map<editor_id, em_scene> _scenes;	  // does not support reordering -> change id?
			editor_id					  _current;
		}											  // namespace

		editor_command cmd_create(
			"New Scene",
			ImGuiKey_N | ImGuiMod_Ctrl,
			[](editor_id _) { return true; },
			[](editor_id _) {
				auto backup_current = scene::_current;
				undoredo::add_and_redo(
					{ "new scene",
					  [](utilities::memory_handle*) {
						  scene::set_current(scene::create(game::ecs::new_scene()));
					  },
					  [=](utilities::memory_handle*) {
						  game::ecs::delete_scene(_scenes.rbegin()->second.ecs_idx, nullptr);
						  scene::remove(_scenes.rbegin()->first);
						  scene::set_current(backup_current);
					  } });
			});

		editor_command cmd_remove(
			"Delete Scene",
			ImGuiKey_Delete,
			[](editor_id _) {
				return count() - get_all_selections().size() > 0 and std::ranges::all_of(get_all_selections(), [](editor_id id) { return find(id) != nullptr; });
			},
			[](editor_id _) {
				auto selections		= editor::get_all_selections();
				auto backup_current = scene::_current;

				const auto em_scenes_backup = selections
											| std::views::transform(scene::find)
											| std::views::reverse
											| _rview_scene_worlds_vec;

				undoredo::add_and_redo(
					{ "delete scene",
					  [=](utilities::memory_handle* p_mem_handle) {
						  p_mem_handle->p_data		  = malloc(sizeof(utilities::memory_handle) * selections.size());
						  p_mem_handle->clean_up_func = [count = selections.size()](auto* p_data) {
							  std::ranges::for_each(std::views::iota((utilities::memory_handle*)p_data) | std::views::take(count),
													[](auto* p_ecs_s_backup) {
														((utilities::memory_handle*)p_ecs_s_backup)->release();
													});
							  free(p_data);
						  };

						  std::ranges::for_each(selections, [p_ecs_s_backup = (utilities::memory_handle*)p_mem_handle->p_data](auto s_id) mutable {
							  new (p_ecs_s_backup) utilities::memory_handle();
							  game::ecs::delete_scene(scene::find(s_id)->ecs_idx, p_ecs_s_backup);
							  scene::remove(s_id);
							  ++p_ecs_s_backup;
						  });
					  },
					  [=](utilities::memory_handle* p_mem_handle) {
						  std::ranges::for_each(
							  em_scenes_backup,
							  [p_ecs_s_backup = (utilities::memory_handle*)p_mem_handle->p_data + em_scenes_backup.size() - 1](auto& tpl) mutable {
								  const auto& [em_s, em_worlds] = tpl;
								  scene::restore(em_s);
								  game::ecs::restore_scene(em_s.ecs_idx, p_ecs_s_backup);

								  for (const auto& [em_w, em_entities] : em_worlds)
								  {
									  world::restore(em_w);

									  for (const auto& [em_e, em_components] : em_entities)
									  {
										  entity::restore(em_e);
										  std::ranges::for_each(em_components, component::restore);
									  }
								  }

								  --p_ecs_s_backup;
							  });

						  p_mem_handle->release();
						  _current = backup_current;
					  } });
			});

		editor_command cmd_set_current(
			"Set Current",
			ImGuiKey_None,
			[](editor_id id) { return id != _current and find(id) != nullptr; },
			[](editor_id id) {
				auto backup_current = _current;

				undoredo::add_and_redo({ std::format("set current scene from {} to {}", backup_current.str(), id.str()),
										 [=](utilities::memory_handle*) { _current = id; },
										 [=](utilities::memory_handle*) { _current = backup_current; } });
			});

		em_scene* find(editor_id id)
		{
			if (_scenes.contains(id))
			{
				return &_scenes[id];
			}
			else
			{
				return nullptr;
			}
		}

		editor_id create(game::ecs::scene_idx ecs_idx)
		{
			return scene::create(_new_name("new_scene", scene::all()), ecs_idx);
		}

		editor_id create(std::string name, game::ecs::scene_idx ecs_idx)
		{
			auto s_id = id::get_new(DataType_Scene);

			auto& s	  = _scenes[s_id];
			s.name	  = name;
			s.id	  = s_id;
			s.ecs_idx = ecs_idx;
			_current  = s_id;

			game::update_proj_data(game::get_pproject()->id, [name](pugi::xml_node proj) {
				if (proj.child("scenes").find_child_by_attribute("name", name.c_str()))
				{
					return;
				}

				auto s_node = proj.child("scenes").append_child("scene");
				s_node.append_attribute("name").set_value(name.c_str());
				s_node.append_attribute("world_count").set_value(0ul);
				s_node.append_child("worlds");
			});

			game::code::edit(
				"/*SCENES*/",
				"SCENE_BEGIN",
				std::format(
					"/*SCENES*/\r\n\r\n"
					"SCENE_BEGIN({})\r\n"
					"SCENE_END()\r\n\r\n"
					"SCENE_BEGIN",
					scene::find(scene::_current)->name));

			return s.id;
		}

		void remove(editor_id s_id)
		{
			std::ranges::for_each(world::all(s_id) | std::views::transform([](auto* p_w) { return p_w->id; }), world::remove);

			auto* p_s = scene::find(s_id);

			game::update_proj_data(s_id, [](pugi::xml_node s_node) {
				s_node.parent().remove_child(s_node);
			});

			game::code::edit(std::format("SCENE_BEGIN({})", p_s->name), "SCENE_END()\r\n\r\n", "");

			_scenes.erase(s_id);
			if (s_id == scene::_current)
			{
				scene::_current = _scenes.rbegin()->first;
			}

			id::delete_id(s_id);
		}

		editor_id restore(const em_scene& em_s)
		{
			_scenes[em_s.id] = em_s;
			id::restore(em_s.id);

			game::update_proj_data(game::get_pproject()->id, [name = em_s.name](pugi::xml_node proj) {
				auto s_node = proj.child("scenes").append_child("scene");
				s_node.append_attribute("name").set_value(name.c_str());
				s_node.append_attribute("world_count").set_value(0ul);
				s_node.append_child("worlds");
			});

			game::code::edit("/*SCENES*/", "SCENE_BEGIN", std::format("/*SCENES*/\r\n\r\nSCENE_BEGIN({})\r\nSCENE_END()\r\n\r\nSCENE_BEGIN", em_s.name));

			return em_s.id;
		}

		size_t count()
		{
			return _scenes.size();
		}

		std::vector<em_scene*> all()
		{
			return std::ranges::to<std::vector>(_scenes | std::views::transform([](auto&& pair) { return &pair.second; }));
		}

		void set_current(editor_id id)
		{
			auto* p_s = scene::find(id);
			if (p_s)
			{
				_current = id;
				editor::select_new(_current);

				game::update_proj_data(game::get_pproject()->id, [name = p_s->name](pugi::xml_node proj_node) {
					proj_node.child("active_scene").attribute("name").set_value(name.c_str());
				});
			}
			else
			{
				assert(false);
			}
		}

		em_scene* get_current()
		{
			auto p_s = find(_current);
			if (p_s is_nullptr)
			{
				return &(_scenes.begin()->second);
			}
			else
			{
				return p_s;
			}
		}

		void on_project_unloaded()
		{
			_scenes.clear();
		}

		void on_project_loaded()
		{
			auto res  = true;
			res		 &= ctx_item::add_context_item("Scene\\Add New Scene", &scene::cmd_create);
			res		 &= ctx_item::add_context_item("Scene\\Remove Scene", &scene::cmd_remove);
			assert(res);
		}
	}	 // namespace scene

	namespace world
	{
		namespace
		{
			std::unordered_map<editor_id, std::map<editor_id, em_world>, editor_id::hash_func> _worlds;			 // key : scene_id , value : [key : world_id, value : em_world]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>					   _scene_id_lut;	 // key : world_id, value : scene_id
																												 // _worlds => [scene_idx][world_idx]
																												 // _idx_map => [world_id] [pair<scene_idx, world_idx>]

		}																										 // namespace

		editor_command cmd_create(
			"New World",
			ImGuiKey_None,
			[](editor_id _) { return editor::get_current_selection().type() == DataType_Scene and editor::get_all_selections().size() == 1; },
			[](editor_id _) {
				auto s_id = editor::get_current_selection();

				undoredo::add_and_redo(
					{ "new world",
					  [=](utilities::memory_handle*) {
						  world::create(s_id, _new_name("new_world", world::all(s_id)), game::ecs::new_world(s_id));
					  },
					  [=](utilities::memory_handle*) { 
						auto		  w_id = _worlds[s_id].rbegin()->first;
						game::ecs::delete_world(s_id, w_id);
						world::remove(w_id); } });
			});

		editor_command cmd_remove(
			"Remove World",
			ImGuiKey_Delete,
			[](editor_id _) { return editor::get_all_selections().empty() is_false and std::ranges::all_of(editor::get_all_selections(), [](editor_id id) { return find(id) != nullptr; }); },
			[](editor_id _) {
				auto	   selections		= editor::get_all_selections();
				const auto em_worlds_backup = selections
											| std::views::reverse
											| std::views::transform(world::find)
											| _rview_world_entities_vec;


				undoredo::add_and_redo({ "remove world",
										 [=](utilities::memory_handle* p_mem_handle) {
											 p_mem_handle->p_data		 = malloc(sizeof(utilities::memory_handle) * selections.size());
											 p_mem_handle->clean_up_func = [count = selections.size()](auto* p_data) {
												 std::ranges::for_each(std::views::iota((utilities::memory_handle*)p_data) | std::views::take(count),
																	   [](auto* p_mem_handle) {
																		   ((utilities::memory_handle*)p_mem_handle)->release();
																	   });
												 free(p_data);
											 };

											 std::ranges::for_each(selections, [p_ecs_w_backup = (utilities::memory_handle*)p_mem_handle->p_data](auto w_id) mutable {
												 auto* p_w = world::find(w_id);
												 auto* p_s = scene::find(p_w->scene_id);

												 new (p_ecs_w_backup) utilities::memory_handle();
												 game::ecs::delete_world(p_s->ecs_idx, p_w->ecs_idx, p_ecs_w_backup);
												 ++p_ecs_w_backup;
											 });

											 std::ranges::for_each(selections, world::remove);
										 },
										 [=](utilities::memory_handle* p_mem_handle) {
											 std::ranges::for_each(em_worlds_backup,
																   [p_ecs_w_backup = (utilities::memory_handle*)p_mem_handle->p_data + em_worlds_backup.size() - 1](auto& tpl) mutable {
																	   auto&& [em_w, em_entities] = tpl;
																	   auto* p_scene			  = scene::find(em_w.scene_id);

																	   game::ecs::restore_world(p_scene->ecs_idx, em_w.ecs_idx, p_ecs_w_backup);
																	   world::restore(em_w);

																	   for (auto&& [em_e, em_components] : em_entities)
																	   {
																		   entity::restore(em_e);
																		   std::ranges::for_each(em_components, component::restore);
																	   }

																	   --p_ecs_w_backup;
																   });
											 p_mem_handle->release();
										 }

				});
			});

		editor_command cmd_add_struct(
			"Add Struct",
			ImGuiKey_None,
			[](editor_id struct_id) { return reflection::find_struct(struct_id) != nullptr
										 and std::ranges::all_of(get_all_selections(), [=](const auto& id) {
												 const auto* p_w = world::find(id);
												 return p_w	 is_not_nullptr and std::ranges::contains(p_w->structs, struct_id) is_false and p_w->structs.size() < 64;
											 }); },
			[](editor_id struct_id) {
				auto& w_id_vec = get_all_selections();

				undoredo::add_and_redo(
					{ "add struct",
					  [=](utilities::memory_handle*) { std::ranges::for_each(w_id_vec, [=](const auto& w_id) {
														   game::ecs::world_add_struct(w_id, struct_id);
														   world::add_struct(w_id, struct_id);
													   }); },
					  [=](utilities::memory_handle*) { std::ranges::for_each(w_id_vec | std::views::reverse, [=](const auto& w_id) {
														   game::ecs::world_remove_struct(w_id, struct_id);
														   world::remove_struct(w_id, struct_id);
													   }); } });
			});

		editor_command cmd_remove_struct(
			"Remove Struct",
			ImGuiKey_None,
			// todo check that no entities uses that struct
			[](editor_id struct_id) { return reflection::find_struct(struct_id) is_not_nullptr
										 and std::ranges::all_of(get_all_selections(), [=](const auto& w_id) {
												 auto*		p_w = world::find(w_id);
												 return p_w is_not_nullptr
													and std::ranges::contains(p_w->structs, struct_id)
													and std::ranges::all_of(entity::all(w_id), [=](em_entity* p_e) {
															auto arc = archetype(w_id, struct_id);
															return arc != 0 and ((p_e->archetype & arc) == 0);
														});
											 }); },
			[](editor_id struct_id) {
				auto& world_id_vec = get_all_selections();

				undoredo::add_and_redo(
					{ "remove struct",
					  [=](utilities::memory_handle*) { std::ranges::for_each(world_id_vec, [=](const auto& w_id) {
														   world::remove_struct(w_id, struct_id);
														   game::ecs::world_remove_struct(w_id, struct_id);
													   }); },
					  [=](utilities::memory_handle*) { std::ranges::for_each(world_id_vec | std::views::reverse, [=](const auto& w_id) {
														   world::add_struct(w_id, struct_id);
														   game::ecs::world_add_struct(w_id, struct_id);

														   auto* p_w		 = world::find(w_id);
														   auto* p_s		 = scene::find(p_w->scene_id);
														   auto	 structs_str = std::accumulate(p_w->structs.begin(), p_w->structs.end(), std::string(), [](std::string l, editor_id s_id) { return std::move(l) + ", " + reflection::find_struct(s_id)->name; });
														   game::code::edit(std::format("SCENE_BEGIN({})@@__WORLD_BEGIN({}", p_s->name, p_w->name),
																			"\r\n",
																			std::format("__WORLD_BEGIN({}{})\r\n", p_w->name, structs_str));
													   }); } });
			});

		em_world* find(editor_id w_id)
		{
			return &_worlds[_scene_id_lut[w_id]][w_id];
		}

		editor_id create(editor_id scene_id, game::ecs::world_idx ecs_world_idx)
		{
			return world::create(scene_id, _new_name("new_world", world::all(scene_id)), ecs_world_idx);
		}

		editor_id create(editor_id scene_id, std::string name, game::ecs::world_idx ecs_world_idx)
		{
			assert(scene::find(scene_id) != nullptr);

			auto  w_id	  = id::get_new(DataType_World);
			auto* p_scene = scene::find(scene_id);
			auto& w		  = _worlds[scene_id][w_id];
			{
				w.id	   = w_id;
				w.name	   = name;
				w.scene_id = scene_id;

				w.ecs_idx = ecs_world_idx;
			}

			_scene_id_lut[w.id] = scene_id;

			game::update_proj_data(scene_id, [name, world_count = _worlds[scene_id].size()](pugi::xml_node s_node) {
				if (s_node.child("worlds").find_child_by_attribute("name", name.c_str()))
				{
					return;
				}

				auto w_node = s_node.child("worlds").append_child("world");
				w_node.append_attribute("name").set_value(name.c_str());
				w_node.append_child("structs");
				w_node.append_child("entities");

				s_node.attribute("world_count").set_value(world_count);
			});

			{
				auto* p_w = world::find(w_id);
				auto* p_s = scene::find(p_w->scene_id);

				game::code::edit(std::format("SCENE_BEGIN({})", p_s->name),
								 "\r\n",
								 std::format("SCENE_BEGIN({})\r\n"
											 "__WORLD_BEGIN({})\r\n"
											 "__WORLD_END()\r\n",
											 p_s->name, p_w->name));
			}

			return w.id;
		}

		void add_struct(editor_id world_id, editor_id struct_id)
		{
			auto* p_w = world::find(world_id);
			auto* p_s = reflection::find_struct(struct_id);
			if (p_w is_nullptr or p_w->structs.size() == 64 or p_s is_nullptr)
			{
				return;
			}

			p_w->structs.push_back(struct_id);

			game::update_proj_data(world_id, [struct_name = p_s->name](pugi::xml_node w_node) {
				if (w_node.child("structs").find_child_by_attribute("name", struct_name.c_str()))
				{
					return;
				}

				auto s_node = w_node.child("structs").append_child("struct");
				s_node.append_attribute("name").set_value(struct_name.c_str());
			});

			{
				auto* p_w		  = world::find(world_id);
				auto* p_s		  = scene::find(p_w->scene_id);
				auto  structs_str = std::accumulate(p_w->structs.begin(), p_w->structs.end(), std::string(), [](std::string l, editor_id s_id) { return std::move(l) + ", " + reflection::find_struct(s_id)->name; });
				game::code::edit(std::format("SCENE_BEGIN({})@@__WORLD_BEGIN({}", p_s->name, p_w->name),
								 "\r\n",
								 std::format("__WORLD_BEGIN({}{})\r\n", p_w->name, structs_str));
			}
		}

		void remove_struct(editor_id world_id, editor_id struct_id)
		{
			auto* p_w	  = find(world_id);
			auto* p_scene = scene::find(p_w->scene_id);
			auto* p_s	  = reflection::find_struct(struct_id);
			auto  it	  = std::ranges::find(p_w->structs, struct_id);
			auto  nth	  = std::distance(p_w->structs.begin(), it);

			game::update_proj_data(world_id, [struct_name = p_s->name](pugi::xml_node w_node) {
				auto s_node = w_node.child("structs").remove_child(struct_name.c_str());
			});

			p_w->structs.erase(it);

			for (auto* p_e : entity::all(p_w->id))
			{
				p_e->archetype = game::ecs::calc_archetype_remove_component(p_e->archetype, nth);
			}

			{
				auto structs_str = std::accumulate(p_w->structs.begin(), p_w->structs.end(), std::string(), [](std::string l, editor_id s_id) { return std::move(l) + ", " + reflection::find_struct(s_id)->name; });
				game::code::edit(std::format("SCENE_BEGIN({})@@__WORLD_BEGIN({}", p_scene->name, p_w->name),
								 "\r\n",
								 std::format("__WORLD_BEGIN({}{})\r\n", p_w->name, structs_str));
			}
		}

		uint64 archetype(editor_id world_id, editor_id struct_id)
		{
			auto p_w = find(world_id);
			if (p_w is_nullptr)
			{
				return 0;
			}

			auto it = std::ranges::find(p_w->structs, struct_id);

			if (it == p_w->structs.end())
			{
				return 0;
			}

			auto index = it - p_w->structs.begin();
			assert(index < 64);
			return 1ull << index;
		}

		void remove(editor_id world_id)
		{
			if (_scene_id_lut.contains(world_id) is_false)
			{
				return;
			}

			auto  scene_id = _scene_id_lut[world_id];
			auto* p_s	   = scene::find(scene_id);

			std::ranges::for_each(entity::all(world_id) | std::views::transform([](auto* p_e) { return p_e->id; }), entity::remove);

			game::update_proj_data(world_id, [](pugi::xml_node w_node) {
				w_node.parent().remove_child(w_node);
			});

			game::code::edit(std::format("SCENE_BEGIN({})@@"
										 "__WORLD_BEGIN({}",
										 p_s->name, _worlds[scene_id][world_id].name),
							 "__WORLD_END()\r\n",
							 "");

			_worlds[scene_id].erase(world_id);
			_scene_id_lut.erase(world_id);
			id::delete_id(world_id);
		}

		editor_id restore(const em_world& em_w)
		{
			editor::id::restore(em_w.id);
			_scene_id_lut[em_w.id]			= em_w.scene_id;
			_worlds[em_w.scene_id][em_w.id] = em_w;

			auto* p_s = scene::find(em_w.scene_id);

			game::update_proj_data(em_w.scene_id,
								   [name		= em_w.name,
									world_count = _worlds[em_w.scene_id].size(),
									struct_vec	= em_w.structs
											   | std::views::transform([](auto s_id) { return reflection::find_struct(s_id)->name; })
											   | std::ranges::to<std::vector>()](pugi::xml_node s_node) {
									   auto w_node = s_node.child("worlds").append_child("world");
									   s_node.attribute("world_count").set_value(world_count);
									   w_node.append_attribute("name").set_value(name.c_str());
									   w_node.append_child("structs");
									   w_node.append_child("entities");

									   for (auto structs_node = w_node.child("structs"); auto&& struct_name : struct_vec)
									   {
										   structs_node.append_child("struct").append_attribute("name").set_value(struct_name.c_str());
									   }
								   });

			{
				auto structs_str = std::accumulate(em_w.structs.begin(), em_w.structs.end(), std::string(), [](std::string l, editor_id s_id) { return std::move(l) + ", " + reflection::find_struct(s_id)->name; });
				game::code::edit(std::format("SCENE_BEGIN({})", p_s->name),
								 "\r\n",
								 std::format("SCENE_BEGIN({})\r\n"
											 "__WORLD_BEGIN({}{})\r\n"
											 "__WORLD_END()\r\n",
											 p_s->name, em_w.name, structs_str));
			}

			return em_w.id;
		}

		std::vector<em_world*> all(editor_id scene_id)
		{
			assert(scene::find(scene_id) != nullptr);

			return std::ranges::to<std::vector>(_worlds[scene_id]
												| std::views::values
												| std::views::transform([](auto&& w) { return &w; }));
		}

		std::vector<editor_id> get_structs(editor_id world_id, archetype_t archetype)
		{
			auto* p_w = world::find(world_id);
			return std::views::iota(0, std::bit_width(archetype))
				 | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
				 | std::views::transform([p_w](auto nth_struct) { return p_w->structs[nth_struct]; })
				 | std::ranges::to<std::vector>();
		}

		void on_project_unloaded()
		{
			_worlds.clear();
			_scene_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res  = true;
			res		 &= ctx_item::add_context_item("Scene\\Add New World", &world::cmd_create);
			res		 &= ctx_item::add_context_item("World\\Remove World", &world::cmd_remove);

			std::ranges::for_each(reflection::all_structs(), [&](const em_struct* p_s) {
				res &= ctx_item::add_context_item(std::format("World\\Add Struct\\{}", p_s->name), &cmd_add_struct, p_s->id);
				res &= ctx_item::add_context_item(std::format("World\\Remove Struct\\{}", p_s->name), &cmd_remove_struct, p_s->id);
			});
			assert(res);
		}
	}	 // namespace world

	namespace entity
	{
		namespace
		{
			std::unordered_map<editor_id, std::map<editor_id, em_entity>, editor_id::hash_func> _entities;		  // key : world_id, value : map [  entity_id, em_entity ]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>						_world_id_lut;	  // key : entity id, value : world_id
		}																										  // namespace

		editor_command cmd_create_empty(
			"Create Emtpy Entity",
			ImGuiKey_None,
			[](editor_id _) { return std::ranges::all_of(
								  editor::get_all_selections(),
								  [](const auto id) { return id.type() == DataType_World and world::find(id) != nullptr; }); },
			[](editor_id _) {
				auto selections = editor::get_all_selections();

				undoredo::add_and_redo(
					{ "create empty entity",
					  [=](utilities::memory_handle*) {
						  std::ranges::for_each(
							  selections,
							  [](auto w_id) {
								  entity::create(w_id, 0, game::ecs::new_entity(w_id));
							  });
					  },
					  [=](utilities::memory_handle*) {
						  std::ranges::for_each(
							  selections
								  | std::views::transform([](auto world_id) { return _entities[world_id].rbegin()->first; })
								  | std::views::reverse,
							  [](auto e_id) {
								  game::ecs::delete_entity(e_id);
								  entity::remove(e_id);
							  });
					  } });
			});

		editor_command cmd_remove(
			"Remove Entities",
			ImGuiKey_Delete,
			[](editor_id _) { return std::ranges::all_of(
								  editor::get_all_selections(),
								  [](const auto id) { return id.type() == DataType_Entity and entity::find(id) != nullptr; }); },
			[](editor_id _) {
				auto&	   selections		  = editor::get_all_selections();
				const auto em_entities_backup = selections
											  | std::views::reverse
											  | std::views::transform(entity::find)
											  | _rview_entity_components_vec;

				auto cmd = editor::undoredo::undo_redo_cmd {
					"remove entities",
					[=](utilities::memory_handle* mem_handle) {
						auto view = selections
								  | std::views::transform([](auto e_id) {
									 auto* p_e = entity::find(e_id);
									 auto* p_w = world::find(p_e->world_id);
									 auto* p_s = scene::find(p_w->scene_id);
									 return std::tuple { p_s, p_w, p_e, game::ecs::get_archetype_size(p_s->ecs_idx, p_w->ecs_idx, p_e->archetype) }; });

						auto mem_size = std::ranges::fold_left(view | std::views::transform([](auto&& tpl) { return std::get<3>(tpl); }), 0, std::plus {});

						mem_handle->p_data		  = malloc(mem_size);
						mem_handle->clean_up_func = [](auto* p_mem) { free(p_mem); };

						std::ranges::for_each(view, [p_memory = (uint8*)mem_handle->p_data + mem_size](auto&& tpl) mutable {
							auto& [p_s, p_w, p_e, archetype_size]  = tpl;
							p_memory							  -= archetype_size;
							game::ecs::copy_archetype_memory(p_memory, p_s->ecs_idx, p_w->ecs_idx, p_e->ecs_idx);
						});

						for (auto e_id : selections)
						{
							editor::game::ecs::delete_entity(e_id);
							models::entity::remove(e_id);
						}
					},
					[=](utilities::memory_handle* mem_handle) mutable {
						std::ranges::for_each(em_entities_backup,
											  [p_archetype_mem = (uint8*)mem_handle->p_data](auto&& tpl) mutable {
												  auto& [em_e, components] = tpl;
												  auto* p_world			   = world::find(em_e.world_id);
												  auto* p_scene			   = scene::find(p_world->scene_id);

												  auto ecs_idx = game::ecs::new_entity(p_scene->ecs_idx, p_world->ecs_idx, em_e.archetype);
												  game::ecs::restore_archetype_memory(p_scene->ecs_idx, p_world->ecs_idx, em_e.ecs_idx, p_archetype_mem);
												  assert(ecs_idx == em_e.ecs_idx);

												  entity::restore(em_e);

												  std::ranges::for_each(components, component::restore);

												  p_archetype_mem += game::ecs::get_archetype_size(p_scene->ecs_idx, p_world->ecs_idx, em_e.archetype);
											  });

						mem_handle->release();
					}
				};

				undoredo::add_and_redo(std::move(cmd));
			});

		editor_command cmd_add_component(
			"Add Component",
			ImGuiKey_None,
			[](editor_id struct_id) { return std::ranges::all_of(
										  editor::get_all_selections() | std::views::transform(entity::find),
										  [=](auto* p_e) { return world::archetype(p_e->world_id, struct_id) != 0 and component::find(p_e->id, struct_id) is_nullptr; }); },
			[](editor_id struct_id) {
				auto& selections = editor::get_all_selections();

				undoredo::add_and_redo(
					{ "add component",
					  [=](utilities::memory_handle*) {
						  std::ranges::for_each(selections, [=](auto e_id) { entity::add_component(e_id, struct_id); });
					  },
					  [=](utilities::memory_handle*) {
						  std::ranges::for_each(selections | std::views::reverse, [=](auto e_id) { entity::remove_component(e_id, struct_id); });
					  } });
			});

		editor_command cmd_remove_component(
			"Remove Component",
			ImGuiKey_None,
			[](editor_id struct_id) { return std::ranges::all_of(
										  editor::get_all_selections() | std::views::transform(entity::find),
										  [=](auto* p_e) { return world::archetype(p_e->world_id, struct_id) != 0 and component::find(p_e->id, struct_id) is_not_nullptr; }); },
			[](editor_id struct_id) {
				auto& selections = editor::get_all_selections();
				auto  backup_vec = std::ranges::to<std::vector>(selections
																| std::views::transform([=](auto e_id) { return *component::find(e_id, struct_id); }));

				undoredo::add_and_redo({ "remove component",
										 [=](utilities::memory_handle* p_backup) {
											 std::ranges::for_each(
												 selections,
												 [=](auto e_id) {
													 auto* p_struct			 = reflection::find_struct(struct_id);
													 p_backup->p_data		 = malloc(p_struct->size * selections.size());
													 p_backup->clean_up_func = [](auto* p_data) { free(p_data); };

													 for (auto* p_dest = (uint8*)p_backup->p_data; auto _ : std::views::iota(0) | std::views::take(selections.size()))
													 {
														 auto* p_src = component::get_memory(component::find(e_id, p_struct->id)->id);
														 (uint8*)memcpy(p_dest, p_src, p_struct->size);
														 p_dest += p_struct->size;
													 }

													 entity::remove_component(e_id, struct_id);
												 });
										 },
										 [=](utilities::memory_handle* p_backup) {
											 auto* p_struct = reflection::find_struct(struct_id);

											 std::ranges::for_each(
												 backup_vec | std::views::reverse,
												 [p_src = (uint8*)p_backup->p_data + (p_struct->size * (backup_vec.size() - 1)), p_struct](auto& em_c) mutable {
													 auto* p_e	   = entity::find(em_c.entity_id);
													 auto* p_w	   = world::find(p_e->world_id);
													 auto* p_scene = scene::find(p_w->scene_id);

													 game::ecs::add_component(p_scene->ecs_idx, p_w->ecs_idx, p_e->ecs_idx, p_struct->ecs_idx);

													 p_e->archetype = game::ecs::get_archetype(p_scene->ecs_idx, p_w->ecs_idx, p_e->ecs_idx);

													 game::update_proj_data(p_e->id, [archetype = p_e->archetype](pugi::xml_node e_node) {
														 e_node.attribute("archetype").set_value(archetype);
													 });

													 memcpy(game::ecs::get_component_memory(p_scene->ecs_idx, p_w->ecs_idx, p_e->ecs_idx, p_struct->ecs_idx), p_src, p_struct->size);

													 component::restore(em_c);

													 p_src -= p_struct->size;
												 });

											 p_backup->release();
										 } });
			});

		em_entity* find(editor_id entity_id)
		{
			if (_world_id_lut.contains(entity_id))
			{
				auto world_id = _world_id_lut[entity_id];
				assert(world::find(world_id));

				return &_entities[world_id][entity_id];
			}
			else
			{
				return nullptr;
			}
		}

		editor_id create(editor_id world_id, archetype_t archetype, game::ecs::entity_idx ecs_idx)
		{
			return create(world_id, _new_name("new_entity", entity::all(world_id)), archetype, ecs_idx);
		}

		editor_id create(editor_id world_id, std::string name, archetype_t archetype, game::ecs::entity_idx ecs_idx)
		{
			assert(world::find(world_id));

			auto  entity_id = id::get_new(DataType_Entity);
			auto* p_world	= world::find(world_id);
			auto* p_scene	= scene::find(p_world->scene_id);
			auto& e			= _entities[world_id][entity_id];
			{
				e.id		= entity_id;
				e.world_id	= world_id;
				e.archetype = archetype;
				e.name		= name;
				e.ecs_idx	= ecs_idx;
			}

			_world_id_lut[entity_id] = world_id;

			game::update_proj_data(world_id, [name, archetype](pugi::xml_node w_node) {
				if (w_node.child("entities").find_child_by_attribute("name", name.c_str()))
				{
					return;
				}

				auto e_node = w_node.child("entities").append_child("entity");

				e_node.append_attribute("name").set_value(name.c_str());
				e_node.append_attribute("archetype").set_value(archetype);
				e_node.append_child("components");
			});

			game::code::edit(
				std::format(
					"SCENE_BEGIN({})@@"
					"__WORLD_BEGIN({}@@"
					"__WORLD_END()",
					p_scene->name, p_world->name),
				"\r\n",
				std::format(
					"____ENTITY_BEGIN({})\r\n"
					"____ENTITY_END()\r\n"
					"__WORLD_END()\r\n",
					name));

			std::ranges::for_each(
				std::views::iota(0, std::bit_width(archetype))
					| std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
					| std::views::transform([p_world](auto nth_component) { return p_world->structs[nth_component]; }),
				[entity_id](auto struct_id) {
					component::create(entity_id, struct_id);
				});
			return entity_id;
		}

		editor_id restore(const em_entity& em_e)
		{
			assert(world::find(em_e.world_id));
			id::restore(em_e.id);

			_world_id_lut[em_e.id]			  = em_e.world_id;
			_entities[em_e.world_id][em_e.id] = em_e;

			game::update_proj_data(em_e.world_id, [name = em_e.name, archetype = em_e.archetype](pugi::xml_node w_node) {
				auto e_node = w_node.child("entities").append_child("entity");

				e_node.append_attribute("name").set_value(name.c_str());
				e_node.append_attribute("archetype").set_value(archetype);
				e_node.append_child("components");
			});

			auto* p_w = world::find(em_e.world_id);
			auto* p_s = scene::find(p_w->scene_id);

			auto components		= world::get_structs(p_w->id, em_e.archetype);
			auto components_str = std::accumulate(components.begin(), components.end(), std::string(), [](std::string l, editor_id struct_id) { return std::move(l) + ", " + reflection::find_struct(struct_id)->name; });

			game::code::edit(
				std::format(
					"SCENE_BEGIN({})@@"
					"__WORLD_BEGIN({}@@"
					"__WORLD_END()",
					p_s->name, p_w->name),
				"\r\n",
				std::format(
					"____ENTITY_BEGIN({}{})\r\n"
					"____ENTITY_END()\r\n"
					"__WORLD_END()\r\n",
					em_e.name, components_str));

			return em_e.id;
		}

		void remove(editor_id entity_id)
		{
			if (_world_id_lut.contains(entity_id) is_false)
			{
				return;
			}
			auto  world_id = _world_id_lut[entity_id];
			auto* p_e	   = &_entities[world_id][entity_id];
			auto* p_w	   = world::find(world_id);
			auto* p_s	   = scene::find(p_w->scene_id);

			std::ranges::for_each(component::all(entity_id) | std::views::transform([](auto* p_c) { return p_c->id; }),
								  component::remove);

			game::update_proj_data(entity_id, [](pugi::xml_node e_node) {
				e_node.parent().remove_child(e_node);
			});

			game::code::edit(
				std::format(
					"SCENE_BEGIN({})@@"
					"__WORLD_BEGIN({}@@"
					"____ENTITY_BEGIN({}",
					p_s->name, p_w->name, p_e->name),
				"____ENTITY_END()\r\n",
				"");

			_world_id_lut.erase(entity_id);
			_entities[world_id].erase(entity_id);
			id::delete_id(entity_id);
		}

		editor_id add_component(editor_id entity_id, editor_id struct_id)
		{
			auto* p_entity = entity::find(entity_id);
			auto* p_world  = world::find(p_entity->world_id);
			auto* p_scene  = scene::find(p_world->scene_id);
			auto* p_struct = reflection::find_struct(struct_id);

			game::ecs::add_component(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx, p_struct->ecs_idx);

			p_entity->archetype = game::ecs::get_archetype(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx);

			game::update_proj_data(entity_id, [archetype = p_entity->archetype](pugi::xml_node e_node) {
				e_node.attribute("archetype").set_value(archetype);
			});

			{
				auto components		= world::get_structs(p_entity->world_id, p_entity->archetype);
				auto components_str = std::accumulate(components.begin(), components.end(), std::string(),
													  [](std::string l, editor_id struct_id) { return std::move(l) + ", " + reflection::find_struct(struct_id)->name; });

				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}",
						p_scene->name, p_world->name, p_entity->name),
					"\r\n",
					std::format("____ENTITY_BEGIN({}{})\r\n", p_entity->name, components_str));
			}

			return component::create(entity_id, struct_id);
		}

		void remove_component(editor_id entity_id, editor_id struct_id)
		{
			auto* p_entity = entity::find(entity_id);
			auto* p_world  = world::find(p_entity->world_id);
			auto* p_scene  = scene::find(p_world->scene_id);
			auto* p_struct = reflection::find_struct(struct_id);

			game::ecs::remove_component(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx, p_struct->ecs_idx);

			p_entity->archetype = game::ecs::get_archetype(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx);

			game::update_proj_data(entity_id, [archetype = p_entity->archetype](pugi::xml_node e_node) {
				e_node.attribute("archetype").set_value(archetype);
			});

			{
				auto components		= world::get_structs(p_entity->world_id, p_entity->archetype);
				auto components_str = std::accumulate(components.begin(), components.end(), std::string(),
													  [](std::string l, editor_id struct_id) { return std::move(l) + ", " + reflection::find_struct(struct_id)->name; });

				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}",
						p_scene->name, p_world->name, p_entity->name),
					"\r\n",
					std::format("____ENTITY_BEGIN({}{})\r\n", p_entity->name, components_str));
			}

			component::remove(component::find(entity_id, struct_id)->id);
		}

		std::vector<em_entity*> all(editor_id world_id)
		{
			assert(world::find(world_id) != nullptr);

			return std::ranges::to<std::vector>(_entities[world_id] | std::views::transform([](auto&& pair) { return &pair.second; }));
		}

		size_t count(editor_id world_id)
		{
			return _entities[world_id].size();
		}

		void on_project_unloaded()
		{
			_entities.clear();
			_world_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res = true;
			// res		 &= ctx_item::add_context_item("Entity\\Add New Entity", &entity::cmd_create);
			// res		 &= ctx_item::add_context_item("Entity\\Remove Entity", &entity::cmd_remove);
			res &= ctx_item::add_context_item("World\\Entity\\Create Empty", &entity::cmd_create_empty);
			res &= ctx_item::add_context_item("Entity\\Remove Entity", &entity::cmd_remove);

			std::ranges::for_each(reflection::_structs, [&res](auto&& s) {
				res &= ctx_item::add_context_item(std::format("Entity\\Add Component\\{}", s.name), &cmd_add_component, s.id);
				res &= ctx_item::add_context_item(std::format("Entity\\Remove Component\\{}", s.name), &cmd_remove_component, s.id);
			});


			assert(res);
		}
	}	 // namespace entity

	namespace component
	{
		namespace
		{
			std::unordered_map<editor_id, std::vector<em_component>, editor_id::hash_func> _components;		  // key : endity_id, value : [key : component_id, value : em_component]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>				   _entity_id_lut;	  // key: component_id, value: entity_id
		}																									  // namespace

		em_component* find(editor_id component_id)
		{
			if (_entity_id_lut.contains(component_id))
			{
				auto e_id = _entity_id_lut[component_id];
				auto res  = std::ranges::find_if(_components[e_id], [=](auto&& c) { return c.id == component_id; });
				if (res != _components[e_id].end())
				{
					return &(*res);
				}
			}

			return nullptr;
		}

		em_component* find(editor_id entity_id, editor_id struct_id)
		{
			if (_components.contains(entity_id))
			{
				auto res = std::ranges::find_if(_components[entity_id], [=](auto&& c) { return c.struct_id == struct_id; });
				if (res != _components[entity_id].end())
				{
					return &(*res);
				}
			}

			return nullptr;
		}

		editor_id create(editor_id entity_id, editor_id struct_id)
		{
			auto* p_entity = entity::find(entity_id);
			auto* p_struct = reflection::find_struct(struct_id);
			assert(p_entity and p_struct);

			auto&& em_c = em_component();
			{
				em_c.id		   = id::get_new(DataType_Component);
				em_c.entity_id = entity_id;
				em_c.struct_id = struct_id;
			}

			_entity_id_lut[em_c.id] = entity_id;

			_components[entity_id].push_back(std::move(em_c));

			//_components[entity_id].insert(std::ranges::upper_bound(_components[entity_id], struct_id, std::ranges::less {}, &em_component::struct_id), std::move(em_c));
			auto field_value_vec = reflection::utils::deserialize(struct_id, component::get_memory(em_c.id));
			game::update_proj_data(entity_id, [s_name = p_struct->name, field_value_vec](pugi::xml_node e_node) {
				if (e_node.child("components").find_child_by_attribute("name", s_name.c_str()))
				{
					return;
				}

				auto c_node = e_node.child("components").append_child("component");
				c_node.append_attribute("name").set_value(s_name.c_str());
				c_node.append_child("fields");

				for (auto fields_node = c_node.child("fields"); auto&& f_value : field_value_vec)
				{
					fields_node.append_child("field").set_value(f_value.c_str());
				}
			});

			{
				auto* p_world = world::find(p_entity->world_id);
				auto* p_scene = scene::find(p_world->scene_id);
				auto  fields  = reflection::all_fields(struct_id);

				auto fields_str = std::accumulate(fields.begin(), fields.end(), std::string(), [p_struct, f_value_it = field_value_vec.begin()](auto str, em_field* p_f) mutable {
					auto&& ret = str + std::format("\r\n______SET_COMPONENT({}, .{}, {{ {} }})", p_struct->name, p_f->name, *f_value_it);
					f_value_it = std::next(f_value_it);
					return std::move(ret);
				});

				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}@@"
						"\r\n",
						p_scene->name, p_world->name, p_entity->name),
					"",
					std::format("{}", fields_str));
			}

			return em_c.id;
		}

		void remove(editor_id component_id)
		{
			auto e_id = _entity_id_lut[component_id];

			game::update_proj_data(component_id, [](pugi::xml_node c_node) {
				c_node.parent().remove_child(c_node);
			});

			{
				auto* p_e	   = entity::find(e_id);
				auto* p_w	   = world::find(p_e->world_id);
				auto* p_scene  = scene::find(p_w->scene_id);
				auto  p_c	   = component::find(component_id);
				auto* p_struct = reflection::find_struct(p_c->struct_id);

				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}@@"
						"______SET_COMPONENT({}",
						p_scene->name, p_w->name, p_e->name, p_struct->name),
					std::format(")\r\n"),
					"", true, "__WORLD_END()");
			}

			_components[e_id].erase(std::ranges::find_if(_components[e_id], [=](auto&& c) { return c.id == component_id; }));
			_entity_id_lut.erase(component_id);

			id::delete_id(component_id);
		}

		void restore(const em_component& em_c)
		{
			id::restore(em_c.id);
			_entity_id_lut[em_c.id] = em_c.entity_id;
			//_components[em_c.entity_id].insert(std::ranges::upper_bound(_components[em_c.entity_id], em_c.struct_id, std::ranges::less {}, &em_component::struct_id), em_c);
			_components[em_c.entity_id].push_back(std::move(em_c));
			auto field_value_vec = reflection::utils::deserialize(em_c.struct_id, component::get_memory(em_c.id));
			game::update_proj_data(em_c.entity_id, [s_name = reflection::find_struct(em_c.struct_id)->name, field_value_vec](pugi::xml_node e_node) {
				auto c_node = e_node.child("components").append_child("component");
				c_node.append_attribute("name").set_value(s_name.c_str());
				c_node.append_child("fields");

				for (auto fields_node = c_node.child("fields"); auto&& f_value : field_value_vec)
				{
					fields_node.append_child("field").append_attribute("value").set_value(f_value.c_str());
				}
			});

			{
				auto* p_entity = entity::find(em_c.entity_id);
				auto* p_world  = world::find(p_entity->world_id);
				auto* p_scene  = scene::find(p_world->scene_id);
				auto* p_struct = reflection::find_struct(em_c.struct_id);
				auto  fields   = reflection::all_fields(em_c.struct_id);

				auto fields_str = std::accumulate(fields.begin(), fields.end(), std::string(), [p_struct, f_value_it = field_value_vec.begin()](auto str, em_field* p_f) mutable {
					auto&& ret = str + std::format("\r\n______SET_COMPONENT({}, .{}, {{ {} }})", p_struct->name, p_f->name, *f_value_it);
					f_value_it = std::next(f_value_it);
					return std::move(ret);
				});

				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}@@"
						"\r\n",
						p_scene->name, p_world->name, p_entity->name),
					"",
					std::format("{}", fields_str));
			}
		}

		void update(editor_id c_id)
		{
			auto* p_c = find(c_id);
			game::update_proj_data(c_id, [f_vec = reflection::utils::deserialize(p_c->struct_id, component::get_memory(p_c->id))](pugi::xml_node c_node) {
				for (auto field_node = c_node.child("fields").first_child(); auto&& f_value : f_vec)
				{
					field_node.append_attribute("value").set_value(f_value.c_str());
					field_node = field_node.next_sibling();
				}
			});
		}

		void update_field(editor_id c_id, editor_id f_id)
		{
			auto* p_c = find(c_id);

			auto view = reflection::all_fields(p_c->struct_id) | std::views::enumerate;

			auto [f_idx, p_f] = *std::ranges::find_if(view, [f_id](std::tuple<uint64, em_field*> tpl) { return std::get<1>(tpl)->id == f_id; });
			auto f_value_str  = reflection::utils::deserialize(p_f->type, (uint8*)component::get_memory(p_c->id) + p_f->offset);
			game::update_proj_data(c_id, [f_idx, f_value_str](pugi::xml_node c_node) {
				auto field_node = c_node.child("fields").first_child();

				std::ranges::for_each(std::views::iota(0, f_idx), [&field_node](auto _) { field_node = field_node.next_sibling(); });

				field_node.attribute("value").set_value(f_value_str.c_str());
			});

			{
				auto* p_e	   = entity::find(p_c->entity_id);
				auto* p_w	   = world::find(p_e->world_id);
				auto* p_s	   = scene::find(p_w->scene_id);
				auto* p_struct = reflection::find_struct(p_c->struct_id);
				game::code::edit(
					std::format(
						"SCENE_BEGIN({})@@"
						"__WORLD_BEGIN({}@@"
						"____ENTITY_BEGIN({}@@"
						"______SET_COMPONENT({}, .{}@@"
						",",
						p_s->name, p_w->name, p_e->name, p_struct->name, p_f->name),
					"\r\n",
					std::format(", .{}, {{ {} }})\r\n", p_f->name, f_value_str));
			}
		}

		std::vector<em_component*> all(editor_id entity_id)
		{
			assert(entity::find(entity_id) is_not_nullptr);

			return std::ranges::to<std::vector>(_components[entity_id] | std::views::transform([](em_component& s) { return &s; }));
		}

		size_t count(editor_id entity_id)
		{
			assert(_components.contains(entity_id));
			return _components[entity_id].size();
		}

		void* get_memory(editor_id c_id)
		{
			auto* p_component = component::find(c_id);
			auto* p_struct	  = reflection::find_struct(p_component->struct_id);

			auto* p_entity = entity::find(p_component->entity_id);
			auto* p_world  = world::find(p_entity->world_id);
			auto* p_scene  = scene::find(p_world->scene_id);

			return game::ecs::get_component_memory(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx, p_struct->ecs_idx);
		}

		void on_project_unloaded()
		{
			_components.clear();
			_entity_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res = true;
			// res		 &= ctx_item::add_context_item("Component\\Remove Component", &cmd_remove_component);

			assert(res);
		}

		// namespace
	}	 // namespace component

	namespace system
	{
		namespace
		{
			std::unordered_map<editor_id, em_system, editor_id::hash_func> _systems;
		}

		em_system* find(editor_id s_id)
		{
			if (_systems.contains(s_id))
			{
				return &_systems[s_id];
			}

			return nullptr;
		}

		editor_id create()
		{
			return create(_new_name("new_system", system::all()));
		}

		editor_id create(std::string name)
		{
			auto s_id			= editor::id::get_new(DataType_System);
			_systems[s_id].name = name;
			return s_id;
		}

		void add_interface(editor_id sys_id, std::string func_name, std::string arg_type)
		{
			auto* p_sys = system::find(sys_id);
			p_sys->interfaces.emplace_back(func_name, arg_type);
		}

		void add_update_param(editor_id sys_id, editor_id struct_id)
		{
			auto* p_sys = system::find(sys_id);
			p_sys->update_params.emplace_back(struct_id);
		}

		std::vector<em_system*> all()
		{
			return _systems
				 | std::views::values
				 | std::views::transform([](auto& val) { return &val; })
				 | std::ranges::to<std::vector>();
		}

		size_t count()
		{
			return _systems.size();
		}


	}	 // namespace system

	namespace text
	{
		namespace
		{
			std::unordered_map<editor_id, const char*, editor_id::hash_func> _text_map;
		}

		editor_id create(const char* p_text)
		{
			auto id = id::get_new(DataType_Editor_Text);
			_text_map.insert({ id, p_text });
			return id;
		}

		void remove(editor_id id)
		{
			_text_map.erase(id);
		}

		const char* find(editor_id id)
		{
			return _text_map[id];
		}
	}	 // namespace text

	editor_command cmd_rename_selection(
		"Rename",
		ImGuiKey_None,
		// todo check that no entities uses that struct
		[](editor_id text_id) { return text_id.type() == DataType_Editor_Text and find(get_current_selection()); },
		[](editor_id text_id) {
			//  todo
			//  change editor command signature to take void*
			//  or create editor object with corresponding datatype and editor_id (ex. editor::models::text::create(const char* )
			//  const char* new_text = editor::models::text::find(text_id);

			// todo maybe another way to to do this
			auto id			= editor::get_current_selection();
			auto backup_str = *get_name(id);
			auto text_str	= std::string(text::find(text_id));

			undoredo::add_and_redo({ std::format("rename {} from {} to {}", id.value, *get_name(id), text_str),
									 [=](utilities::memory_handle*) { *get_name(id) = backup_str; },
									 [=](utilities::memory_handle*) { *get_name(id) = text_str; } });

			text::remove(text_id);
		});

	void on_project_unloaded()
	{
		reflection::on_project_unloaded();
		scene::on_project_unloaded();
		world::on_project_unloaded();
		entity::on_project_unloaded();
		component::on_project_unloaded();

		// component::on_project_unloaded();
	}

	void* find(editor_id id)
	{
		switch (id.type())
		{
		case DataType_Entity:
			return entity::find(id);
		case DataType_Project:
			return nullptr;
		case DataType_Scene:
			return scene::find(id);
		case DataType_World:
			return world::find(id);
		case DataType_SubWorld:
			return nullptr;
		case DataType_Component:
			return component::find(id);
		case DataType_System:
			return nullptr;
		case DataType_Struct:
			return reflection::find_struct(id);
		case DataType_Field:
			return reflection::find_field(id);
		case DataType_Editor_Text:
			return (void*)text::find(id);
		case DataType_Editor_Command:
		case DataType_Editor_UndoRedo:
		case DataType_Count:
		case DataType_InValid:
			return nullptr;
		}

		return nullptr;
	}

	std::string* get_name(editor_id id)
	{
		switch (id.type())
		{
		case DataType_Entity:
		{
			auto* ptr = entity::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Project:
			return nullptr;
		case DataType_Scene:
		{
			auto* ptr = scene::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_World:
		{
			auto* ptr = world::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_SubWorld:
			return nullptr;
		case DataType_Component:
		case DataType_System:
			return nullptr;
		case DataType_Struct:
		{
			auto* ptr = reflection::find_struct(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Field:
		{
			auto* ptr = reflection::find_field(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Editor_Text:
		case DataType_Editor_Command:
		case DataType_Editor_UndoRedo:
		case DataType_Count:
		case DataType_InValid:
			return nullptr;
		}

		return nullptr;
	}

	bool change_exists()
	{
		return true;
	}

	void on_project_loaded()
	{
		reflection::on_project_loaded();
		scene::on_project_loaded();
		world::on_project_loaded();
		entity::on_project_loaded();
		component::on_project_loaded();
	}
}	 // namespace editor::models