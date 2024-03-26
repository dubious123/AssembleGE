#include "pch.h"
#include "editor_view.h"
#include "game_project/game_project.h"

namespace editor::view::hierarchy
{
	auto _open = true;

	const auto cmd_open = editor_command {
		"Open Hierarchy Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		if (_open is_false) return;
		if (editor::game::get_current_p_project()->is_ready is_false) return;

		if (widgets::begin("Hierarchy", &_open))
		{
			static auto selection_mask = 2;
			static char buffer[SCENE_NAME_MAX_LEN];
			static auto rename_scene = false;
			auto		node_clicked = -1;
			auto*		p_project	 = editor::game::get_current_p_project();

			widgets::set_cursor_pos_x((platform::get_window_size().x - platform::get_window_info().default_item_width()) * 0.5f);
			auto p_s_current = editor::models::scene::get_current();

			if (p_s_current == nullptr)
			{
				widgets::end();
				return;
			}

			if (ImGui::BeginCombo("##scene_combo", p_s_current->name.c_str(), ImGuiComboFlags_NoArrowButton))
			{
				auto current_id = models::scene::get_current();
				for (auto& p_scene : models::scene::all())
				{
					auto is_current = p_scene->id == p_s_current->id;
					if (widgets::selectable(p_scene->name, is_current))
					{
						models::scene::cmd_set_current(p_scene->id);
						editor::select_new(p_scene->id);
						// editor::cmd_select_new(p_scene->id);
					}

					if (is_current)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			editor::add_right_click_source(p_s_current->id);

			widgets::separator();


			for (const auto p_w : models::world::all(p_s_current->id))
			{
				auto world_selected			= editor::is_selected(p_w->id);
				auto world_tree_node_opened = widgets::tree_node(p_w->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | (editor::is_selected(p_w->id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));

				editor::add_right_click_source(p_w->id);
				editor::add_left_click_source(p_w->id);

				if (world_tree_node_opened is_false) continue;

				std::ranges::for_each(models::entity::all(p_w->id), [](const auto* p_e) {
					ImGui::TreeNodeEx(p_e->name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (editor::is_selected(p_e->id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));
					editor::add_left_click_source(p_e->id);
				});

				// if (p_w->entities.empty() is_false)
				//{
				//	widgets::separator();
				// }

				// for (const auto& system_id : p_w->systems)
				//{
				//	auto p_system = models::system::find(system_id);
				//	ImGui::TreeNodeEx(p_system->name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (editor::is_selected(system_id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));
				//	editor::selection_source(system_id);
				//	editor::ctx_source(system_id);
				// }

				// if (p_w->systems.empty() is_false)
				//{
				//	widgets::separator();
				// }

				// Widgets::Separator();

				widgets::tree_pop();

				// for (const auto& sub_world_id : p_world->SubWorlds)
				//{
				//	auto p_sub_world = Models::World::Find(sub_world_id);
				//	if (ImGui::TreeNode(p_sub_world->Name.c_str()))
				//	{
				//		ImGui::TreePop();
				//	}
				// }
			}

			// if (node_clicked != -1)
			//{
			//	// Update selection state
			//	// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
			//	if (ImGui::GetIO().KeyCtrl)
			//		selection_mask ^= (1 << node_clicked);	  // CTRL+click to toggle
			//	else										  // if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
			//		selection_mask = (1 << node_clicked);	  // Click to single-select
			// }
		}

		widgets::end();
	}
}	 // namespace editor::view::hierarchy

namespace editor::view::inspector
{
	using namespace editor::models;
	auto _open = true;

	const auto cmd_open = editor_command {
		"Open Inspector Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void _draw_scene(editor_id _)
	{
		auto p_scene = scene::get_current();
		widgets::text(p_scene->name.c_str());
		widgets::text("scene idx : {%d}", p_scene->id);
		// widgets::text("world count : {%d}", p_scene->world_count());
	}

	void _draw_world(editor_id world_id)
	{
		auto p_world = world::find(world_id);
		if (p_world is_nullptr) return;

		auto p_scene = scene::find(p_world->scene_id);

		widgets::text((p_world->name).c_str());

		if (widgets::tree_node("archetype", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::ranges::for_each(p_world->structs, [](editor_id struct_id) {
				auto* p_struct = models::reflection::find_struct(struct_id);
				if (widgets::tree_node(p_struct->name))
				{
					std::ranges::for_each(models::reflection::all_fields(p_struct->id), [](em_field* p_f) {
						widgets::tree_node(std::format("{}({}) : {}", p_f->name, p_f->p_info->type, p_f->p_info->serialized_value).c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
					});
					widgets::tree_pop();
				}
			});
			widgets::tree_pop();
		}


		// for (auto c = 0; c < p_world->p_info->component_count; ++c)
		//{
		//	auto p_c_info = p_world->find_component(p_world->p_info->component_idx + c);
		//	auto p_s_info = reflection::find_struct(p_c_info->struct_id);
		//	widgets::text(p_s_info->name);

		//	for (auto f = 0; f < p_s_info->field_count; ++f)
		//	{
		//		auto p_f_info = (p_s_info->fields + f);
		//		widgets::text(p_f_info->name);
		//		widgets::text(p_f_info->type);
		//		widgets::text(p_f_info->serialized_value);
		//	}
		//}

		// widgets::separator();

		// if (ImGui::CollapsingHeader("Systems"))
		//{
		//	for (auto system_id : p_world->systems)
		//	{
		//		char buffer[100] {};
		//		auto p_system = system::find(system_id);
		//		std::copy_n(p_system->name.begin(), p_system->name.size(), buffer);
		//		if (ImGui::InputText(std::format("##{}", system_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		{
		//			p_system->name = std::string(buffer);
		//		}
		//	}
		// }

		// if (ImGui::CollapsingHeader("Subworlds"))
		//{
		//	for (auto subworld_id : p_world->subworlds)
		//	{
		//		auto p_world = world::get(subworld_id);
		//		ImGui::Text(p_world->name.c_str());
		//	}
		// }

		// if (ImGui::CollapsingHeader("Entities"))
		//{
		//	for (auto& entity_id : p_world->entities)
		//	{
		//		char buffer[100] {};
		//		auto p_entity = entity::get(entity_id);
		//		std::copy_n(p_entity->name.begin(), p_entity->name.size(), buffer);
		//		if (ImGui::InputText(std::format("##{}", entity_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		{
		//			p_entity->name = std::string(buffer);
		//		}
		//	}
		// }
	}

	void _draw_component(editor_id component_id)
	{
		// ImGui::Text(component::find(component_id)->name.c_str());
	}

	void _draw_entity(editor_id entity_id)
	{
		auto p_e = models::entity::find(entity_id);
		widgets::text(std::format("name : {}", p_e->name).c_str());
		widgets::text(std::format("world : {}", models::world::find(p_e->world_id)->name).c_str());
		widgets::text(std::format("archetype : {}", p_e->archetype).c_str());
		// auto p_entity = entity::get(entity_id);

		// ImGui::Text((p_entity->name).c_str());

		// widgets::separator();

		// if (ImGui::CollapsingHeader("Components"))
		//{
		//	for (auto& component_id : p_entity->components)
		//	{
		//		_draw_component(component_id);
		//		// char buffer[100] {};
		//		// auto p_component = Component::Find(component_id);
		//		// std::copy_n(p_component->Name.begin(), p_component->Name.size(), buffer);
		//		// if (ImGui::InputText(std::format("##{}", component_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		//{
		//		//	p_component->Name = std::string(buffer);
		//		// }
		//	}
		// }
	}

	void show()
	{
		if (_open is_false) return;
		if (widgets::begin("Inspector", &_open))
		{
			auto id = get_current_selection();
			switch (id.type())
			{
			case DataType_Scene:
				_draw_scene(id);
				break;
			case DataType_World:
				_draw_world(id);
				break;
			case DataType_Entity:
				_draw_entity(id);
				break;
			default:
				break;
			}
		}

		widgets::end();
	}
}	 // namespace editor::view::inspector

namespace editor::view::reflection
{
	using namespace editor::models;

	auto _open = true;

	const auto cmd_open = editor_command {
		"Open reflection Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		if (_open is_false) return;

		if (widgets::begin("Reflection", &_open))
		{
			std::ranges::for_each(editor::models::reflection::all_structs(), [](em_struct* p_s) {
				if (widgets::tree_node(p_s->name.c_str()))
				{
					std::ranges::for_each(editor::models::reflection::all_fields(p_s->id), [](em_field* p_f) {
						auto selected = false;
						widgets::tree_node(std::format("{} ({}) : {}", p_f->name, p_f->p_info->type, p_f->p_info->serialized_value), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
					});


					widgets::tree_pop();
				}
			});
		}

		widgets::end();
	}
}	 // namespace editor::view::reflection

namespace editor::view
{
	namespace
	{
		void _main_dock_space()
		{
			auto   window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;
			auto   mainViewPort = ImGui::GetMainViewport();
			auto   size			= mainViewPort->Size - ImVec2(0, CAPTION_HIGHT * GEctx->dpi_scale);
			ImVec2 pos			= mainViewPort->Pos + ImVec2(0, CAPTION_HIGHT * GEctx->dpi_scale);
			style::push_var(ImGuiStyleVar_WindowRounding, 0.0f);
			style::push_var(ImGuiStyleVar_WindowBorderSize, 0.0f);
			style::push_var(ImGuiStyleVar_WindowPadding, ImVec2());
			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(mainViewPort->ID);
			if (widgets::begin("MainView", NULL, window_flags))
			{
				ImGui::DockSpace(ImGui::GetID("DockSpace"), size, ImGuiDockNodeFlags_None);	   // | ImGuiDockNodeFlags_NoSplit);
			}

			widgets::end();
			style::pop_var(3);
		}
	}	 // namespace

	void update_dpi_scale()
	{
		project_browser::update_dpi_scale();
	}

	void init()
	{
		logger::init();
	}

	void on_project_loaded()
	{
		auto res  = true;
		res		 &= editor::add_context_item("Main Menu\\Window\\Hierarchy", &hierarchy::cmd_open);
		res		 &= editor::add_context_item("Main Menu\\Window\\Inspector", &inspector::cmd_open);
		res		 &= editor::add_context_item("Main Menu\\Window\\Reflection", &reflection::cmd_open);
		assert(res);
		logger::on_project_loaded();
	}

	void show()
	{
		_main_dock_space();
		project_browser::show();
		if (editor::game::project_opened() is_false) return;

		logger::show();
		hierarchy::show();
		inspector::show();
		reflection::show();
	}
}	 // namespace editor::view
