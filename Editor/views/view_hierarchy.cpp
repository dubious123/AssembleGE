#include "../editor_view.h"
#include "imgui/imgui_internal.h"
#include "../game_project/game_project.h"

namespace Editor::View::Hierarchy
{
	namespace
	{
		constexpr auto _main_menu_path = "Window\\Hierarchy";
		static auto	   _open		   = true;
	}	 // namespace

	void Open()
	{
		_open ^= true;
	}

	void Show()
	{
		static auto selection_mask = 2;
		auto		node_clicked   = -1;
		if (_open is_false) return;

		if (ImGui::Begin("Hierarchy", &_open))
		{
			static char buffer[SCENE_NAME_MAX_LEN];
			ImGui::InputText("###scene input", buffer, SCENE_NAME_MAX_LEN);
			if (ImGui::Button("Add Scene"))
			{
				Editor::GameProject::Add_Scene(std::string(buffer));
			}

			static auto combo_idx	  = 0;
			auto		p_project	  = Editor::GameProject::Get_Current_PProject();
			auto		combo_preview = p_project->Scenes.empty() ? "" : p_project->Scenes[combo_idx].Name.c_str();

			if (ImGui::BeginCombo("##scene_combo", combo_preview, ImGuiComboFlags_NoArrowButton))
			{
				auto i = 0;
				for (const auto& scene : p_project->Scenes)
				{
					auto	   str		   = scene.Name;
					const auto is_selected = (combo_idx == i);
					if (ImGui::Selectable(str.c_str(), is_selected))
						combo_idx = i;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();

					++i;
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("World Main"))
			{
				// ImGui::TreeNodeEx("Entity Player", ImGuiTreeNodeFlags_Leaf | base_flags);

				if (ImGui::TreeNode("Entity Car 1"))
				{
					const auto base_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					for (int i = 0; i < 4; ++i)
					{

						const bool is_selected = (selection_mask & (1 << i)) != 0;
						auto	   flag		   = is_selected ? base_flags | ImGuiTreeNodeFlags_Selected : base_flags;
						ImGui::TreeNodeEx((void*)(intptr_t)i, flag, "Entity Wheal_{%d}", i);
						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
							node_clicked = i;
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (node_clicked != -1)
			{
				// Update selection state
				// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
				if (ImGui::GetIO().KeyCtrl)
					selection_mask ^= (1 << node_clicked);	  // CTRL+click to toggle
				else										  // if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
					selection_mask = (1 << node_clicked);	  // Click to single-select
			}
		}

		ImGui::End();
	}
}	 // namespace Editor::View::Hierarchy