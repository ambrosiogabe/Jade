#include "editorWindows/MenuBar.h"
#include "core/ImGuiLayer.h"
#include "core/CocoaEditorApplication.h"
#include "editorWindows/ProjectWizard.h"
#include "gui/ImGuiExtended.h"
#include "util/Settings.h"

#include "cocoa/core/Entity.h"
#include "cocoa/file/IFileDialog.h"
#include "cocoa/components/components.h"
#include "cocoa/components/Transform.h"
#include "cocoa/util/Settings.h"
#include "cocoa/core/Application.h"
#include "cocoa/file/IFile.h"

namespace Cocoa
{
	namespace MenuBarUtil
	{
		// Internal variables
		static bool m_CreatingProject = false;
		static glm::vec2 m_DefaultPopupSize = { 900, 600 };

		// Forward declarations
		static void SettingsWindow();
		static void StylesWindow();
		static bool CPathVectorGetter(void* data, int n, const char** out_text);

		static void SettingsWindow()
		{
			ImGui::SetNextWindowSize(m_DefaultPopupSize, ImGuiCond_Once);
			ImGui::Begin("Settings", &Settings::Editor::ShowSettingsWindow);
			CImGui::UndoableDragInt2("Grid Size: ", Settings::Editor::GridSize);
			ImGui::Checkbox("Draw Grid: ", &Settings::Editor::DrawGrid);
			ImGui::End();
		}

		static void StylesWindow()
		{
			ImGui::SetNextWindowSize(m_DefaultPopupSize, ImGuiCond_Once);
			ImGui::Begin("Styles", &Settings::Editor::ShowStyleSelect);
			std::vector<CPath> styles = IFile::GetFilesInDir(Settings::General::s_StylesDirectory);
			if (ImGui::ListBox("Styles", &Settings::Editor::SelectedStyle, CPathVectorGetter, (void*)&styles, (int)styles.size()))
			{
				ImGuiLayer::LoadStyle(styles[Settings::Editor::SelectedStyle]);
			}
			ImGui::End();
		}

		static bool CPathVectorGetter(void* data, int n, const char** out_text)
		{
			const std::vector<CPath>* v = (std::vector<CPath>*)data;
			*out_text = v->at(n).Filename();
			return true;
		}


		void ImGui(MenuBar& menuBar)
		{
			if (Settings::Editor::ShowSettingsWindow)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
				SettingsWindow();
				ImGui::PopStyleVar();
			}

			if (Settings::Editor::ShowStyleSelect)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
				StylesWindow();
				ImGui::PopStyleVar();
			}

			if (m_CreatingProject)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
				ProjectWizard::CreateProjectImGui(m_CreatingProject);
				ImGui::PopStyleVar();
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarButtonBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarButtonBgHover));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarButtonBgActive));
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (CImGui::MenuButton("New Project"))
					{
						m_CreatingProject = true;
					}

					if (CImGui::MenuButton("Open Project"))
					{
						FileDialogResult result{};
						if (IFileDialog::GetOpenFileName(".", result, { {"Jade Scenes *.jade", "*.jprj"}, {"All Files", "*.*"} }))
						{
							EditorLayer::LoadProject(CPath(result.filepath));
						}
					}

					if (CImGui::MenuButton("Save Scene As"))
					{
						FileDialogResult result{};
						if (IFileDialog::GetSaveFileName(".", result, { {"Jade Scenes *.jade", "*.jade"}, {"All Files", "*.*"} }, ".jade"))
						{
							Settings::General::s_CurrentScene = result.filepath;
							menuBar.ScenePtr->Save(result.filepath);
						}
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Jade"))
				{
					if (CImGui::MenuButton("Settings"))
					{
						Settings::Editor::ShowSettingsWindow = true;
					}

					if (CImGui::MenuButton("Styles"))
					{
						Settings::Editor::ShowStyleSelect = true;
					}

					if (CImGui::MenuButton("Show Demo Window"))
					{
						Settings::Editor::ShowDemoWindow = true;
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Game Objects"))
				{
					if (CImGui::MenuButton("Add Sprite Object"))
					{
						Entity entity = menuBar.ScenePtr->CreateEntity();
						entity.AddComponent<SpriteRenderer>();
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			ImGui::PopStyleColor(3);
		}
	}
}