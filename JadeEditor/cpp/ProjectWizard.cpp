#pragma once
#include "jade/core/JWindow.h"
#include "jade/core/Application.h"
#include "jade/core/ImGuiExtended.h"
#include "jade/file/IFile.h"
#include "jade/file/IFileDialog.h"
#include "FontAwesome.h"
#include "JadeEditorApplication.h"

namespace Jade
{
	ProjectWizard::ProjectWizard()
	{
		m_IdealSize = Application::Get()->GetWindow()->GetMonitorSize() / 2.0f;
		m_JadeLogo = new Texture(JPath("assets/jadeLogo.png"));
		m_JadeLogo->Load();
		m_TexturePos.x = (m_IdealSize.x / 2.0f) - (m_JadeLogo->GetWidth() / 2.0f);
		m_TexturePos.y = m_IdealSize.y / 10.0f;
		m_VersionPos = glm::vec2(-1.0f, -1.0f);
		m_ButtonSize = glm::vec2(m_IdealSize.x / 3.0f, m_IdealSize.y / 18.0f);
		m_CreateProjButtonPos = (m_IdealSize / 2.0f) - (m_ButtonSize / 2.0f);
		m_OpenProjectButtonPos = m_CreateProjButtonPos + glm::vec2(0.0f, m_ButtonSize.y) + glm::vec2(0.0f, m_Padding.y);
	}

	ProjectWizard::~ProjectWizard()
	{
		delete m_JadeLogo;
	}

	void ProjectWizard::Update(float dt)
	{

	}

	void ProjectWizard::ImGui()
	{
		static bool open = true;
		if (m_VersionPos.x < 0 && m_VersionPos.y < 0)
		{
			ImVec2 textSize = ImGui::CalcTextSize("Version 1.0 Alpha");
			m_VersionPos = (m_IdealSize / 2.0f) - glm::vec2(textSize.x / 2.0f, textSize.y / 2.0f);
			m_VersionPos.y = m_TexturePos.y + m_JadeLogo->GetHeight() + textSize.y / 2.0f;
		}

		Application::Get()->GetWindow()->SetSize(m_IdealSize);
		glm::vec2 winPos = Application::Get()->GetWindow()->GetWindowPos();
		ImGui::SetNextWindowPos(ImVec2(winPos.x, winPos.y));
		ImGui::SetNextWindowSize(ImVec2(m_IdealSize.x, m_IdealSize.y), ImGuiCond_Once);
		ImGui::Begin("Create or Open Project", &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		ImGui::SetCursorPos(ImVec2(m_TexturePos.x, m_TexturePos.y));
		ImGui::Image(reinterpret_cast<void*>(m_JadeLogo->GetId()), ImVec2(m_JadeLogo->GetWidth(), m_JadeLogo->GetHeight()));
		ImGui::SetCursorPos(ImVec2(m_VersionPos.x, m_VersionPos.y));
		ImGui::Text("Version 1.0 Alpha");

		ImGui::SetCursorPos(ImVec2(m_CreateProjButtonPos.x, m_CreateProjButtonPos.y));
		ImGui::JButton(ICON_FA_PLUS " Create Project", m_ButtonSize);

		ImGui::SetCursorPos(ImVec2(m_OpenProjectButtonPos.x, m_OpenProjectButtonPos.y));
		if (ImGui::JButton(ICON_FA_FOLDER_OPEN " Open Project", m_ButtonSize))
		{
			FileDialogResult res;
			if (IFileDialog::GetOpenFileName("", res, { {"Jade Project", "*.jprj"} }))
			{
				if (!EditorLayer::LoadProject(JPath(res.filepath)))
				{
					Log::Warning("Unable to load project: %s", res.filepath.c_str());
				}
			}
		}

		ImGui::End();
	}

}