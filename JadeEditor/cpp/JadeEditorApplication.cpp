#include "Jade.h"

#include "gui/ImGuiHeader.h"
#include "gui/ImGuiLayer.h"
#include "editorWindows/InspectorWindow.h"
#include "scripting/ScriptCompiler.h"
#include "util/Settings.h"
#include "LevelEditorScene.h"
#include "jadeEditorApplication.h"

#include "jade/file/IFile.h"
#include "jade/util/Settings.h"
#include "jade/systems/RenderSystem.h"
#include "jade/scripting/ScriptRuntime.h"

#include <glad/glad.h>
#include <nlohmann/json.hpp>

namespace Jade
{
	// ===================================================================================
	// Editor Layer
	// ===================================================================================
	EditorLayer::EditorLayer(Scene* scene)
		: Layer(scene), m_PickingTexture(3840, 2160)
	{
		m_PickingShader = std::make_shared<Shader>(JPath(Settings::General::s_EngineAssetsPath + "shaders/Picking.glsl"));
		m_DefaultShader = std::make_shared<Shader>(Settings::General::s_EngineAssetsPath + "shaders/SpriteRenderer.glsl");
		m_OutlineShader = std::make_shared<Shader>(Settings::General::s_EngineAssetsPath + "shaders/SingleColor.glsl");
	}

	bool EditorLayer::CreateProject(const JPath& projectPath, const char* filename)
	{
		Settings::General::s_CurrentProject = projectPath + (std::string(filename) + ".jprj");
		Settings::General::s_CurrentScene = projectPath + "scenes" + "NewScene.jade";

		json saveData = {
			{"ProjectPath", Settings::General::s_CurrentProject.Filepath()},
			{"CurrentScene", Settings::General::s_CurrentScene.Filepath()},
			{"WorkingDirectory", Settings::General::s_CurrentProject.GetDirectory(-1) }
		};

		IFile::WriteFile(saveData.dump(4).c_str(), Settings::General::s_CurrentProject);
		IFile::CreateDirIfNotExists(projectPath + "assets");
		IFile::CreateDirIfNotExists(projectPath + "scripts");
		IFile::CreateDirIfNotExists(projectPath + "scenes");

		JadeEditor* application = (JadeEditor*)Application::Get();
		application->GetEditorLayer()->m_Scene->Reset();
		application->GetEditorLayer()->m_Scene->Save(Settings::General::s_CurrentScene);
		SaveEditorData();

		return true;
	}

	void EditorLayer::SaveEditorData()
	{
		if (static_cast<JadeEditor*>(Application::Get())->GetEditorLayer()->m_ProjectLoaded)
		{
			ImGui::SaveIniSettingsToDisk(Settings::General::s_ImGuiConfigPath.Filepath());
		}

		json saveData = {
			{"ProjectPath", Settings::General::s_CurrentProject.Filepath()},
			{"EditorStyle", Settings::General::s_EditorStyleData.Filepath()},
			{"ImGuiConfig", Settings::General::s_ImGuiConfigPath.Filepath()}
		};

		IFile::WriteFile(saveData.dump(4).c_str(), Settings::General::s_EditorSaveData);
	}

	bool EditorLayer::LoadEditorData(const JPath& path)
	{
		File* editorData = IFile::OpenFile(path);
		if (editorData->m_Data.size() > 0)
		{
			json j = json::parse(editorData->m_Data);
			if (!j["EditorStyle"].is_null())
			{
				Settings::General::s_EditorStyleData = JPath(j["EditorStyle"]);
			}

			if (!j["ImGuiConfigPath"].is_null())
			{
				Settings::General::s_ImGuiConfigPath = JPath(j["ImGuiConfig"]);
			}

			if (!j["ProjectPath"].is_null())
			{
				LoadProject(JPath(j["ProjectPath"]));
			}
		}
		IFile::CloseFile(editorData);

		return true;
	}

	void EditorLayer::SaveProject()
	{
		json saveData = {
			{"ProjectPath", Settings::General::s_CurrentProject.Filepath()},
			{"CurrentScene", Settings::General::s_CurrentScene.Filepath()},
			{"WorkingDirectory", Settings::General::s_CurrentProject.GetDirectory(-1) }
		};

		IFile::WriteFile(saveData.dump(4).c_str(), Settings::General::s_CurrentProject);

		SaveEditorData();
	}

	bool EditorLayer::LoadProject(const JPath& path)
	{
		Settings::General::s_CurrentProject = path;
		File* projectData = IFile::OpenFile(Settings::General::s_CurrentProject);
		if (projectData->m_Data.size() > 0)
		{
			json j = json::parse(projectData->m_Data);
			if (!j["CurrentScene"].is_null())
			{
				Settings::General::s_CurrentScene = JPath(j["CurrentScene"]);
				Settings::General::s_WorkingDirectory = JPath(j["WorkingDirectory"]);

				JadeEditor* application = (JadeEditor*)Application::Get();
				application->GetEditorLayer()->m_Scene->Load(Settings::General::s_CurrentScene);

				SaveEditorData();
				std::string winTitle = std::string(Settings::General::s_CurrentProject.Filename()) + " -- " + std::string(Settings::General::s_CurrentScene.Filename());
				Application::Get()->GetWindow()->SetTitle(winTitle.c_str());

				static_cast<JadeEditor*>(Application::Get())->SetProjectLoaded();
				return true;
			}
		}

		return false;
	}

	void EditorLayer::OnAttach()
	{
		// Set the assets path as CWD (which should be where the exe is currently located)
		JPath exePath = IFile::GetExecutableDirectory().GetDirectory(-1);
		Settings::General::s_EngineAssetsPath = exePath + Settings::General::s_EngineAssetsPath;
		Settings::General::s_ImGuiConfigPath = Settings::General::s_EngineAssetsPath + Settings::General::s_ImGuiConfigPath;
		Settings::General::s_EngineExecutableDirectory = exePath;

		// Create application store data if it does not exist
		IFile::CreateDirIfNotExists(IFile::GetSpecialAppFolder() + "JadeEngine");

		Settings::General::s_EditorSaveData = IFile::GetSpecialAppFolder() + "JadeEngine" + Settings::General::s_EditorSaveData;
		Settings::General::s_EditorStyleData = IFile::GetSpecialAppFolder() + "JadeEngine" + Settings::General::s_EditorStyleData;
		Settings::EditorVariables::s_CodeEditorExe = IFile::GetSpecialAppFolder() + ".." + "Local" + "Programs" + "Microsoft VS Code" + Settings::EditorVariables::s_CodeEditorExe;

		LoadEditorData(Settings::General::s_EditorSaveData);
	}

	void EditorLayer::OnUpdate(float dt)
	{
		if (JadeEditor::IsProjectLoaded())
		{
			DebugDraw::BeginFrame();
			m_Scene->Update(dt);
		}
	}

	void EditorLayer::OnRender()
	{
		if (JadeEditor::IsProjectLoaded())
		{
			glDisable(GL_BLEND);
			m_PickingTexture.EnableWriting();

			glViewport(0, 0, 3840, 2160);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			RenderSystem::BindShader(m_PickingShader);
			m_Scene->Render();

			m_PickingTexture.DisableWriting();
			glEnable(GL_BLEND);

			glBindFramebuffer(GL_FRAMEBUFFER, Application::Get()->GetFramebuffer()->GetId());

			glViewport(0, 0, 3840, 2160);
			glClearColor(0.45f, 0.55f, 0.6f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			RenderSystem::BindShader(m_DefaultShader);
			RenderSystem::UploadUniform1ui("uActiveEntityID", InspectorWindow::GetActiveEntity().GetID() + 1);
			DebugDraw::DrawBottomBatches();
			m_Scene->Render();
			DebugDraw::DrawTopBatches();
		}
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (JadeEditor::IsProjectLoaded())
		{
			const auto& systems = m_Scene->GetSystems();

			for (const auto& system : systems)
			{
				system->OnEvent(e);
			}
		}
	}

	// ===================================================================================
	// Editor Application
	// ===================================================================================
	JadeEditor::JadeEditor()
		: Application()
	{
	}

	void JadeEditor::Init()
	{
		Log::Info("Initializing GLAD functions in exe.");
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			Log::Error("Unable to initialize GLAD in exe.");
		}

		// Engine initialization
		Jade::AssetManager::Init(0);
		Jade::IFileDialog::Init();
		Jade::IFile::Init();
		Jade::ProjectWizard::Init();
		Jade::Physics2D::Init();
		Jade::Input::Init();
		ChangeScene(new LevelEditorScene());

		Jade::ScriptRuntime::Init();
		Jade::ScriptCompiler::Init(m_CurrentScene->GetRuntime());

		m_ImGuiLayer = new ImGuiLayer(m_CurrentScene);
		m_EditorLayer = new EditorLayer(m_CurrentScene);
		PushOverlay(m_ImGuiLayer);
		PushLayer(m_EditorLayer);
	}

	void JadeEditor::Shutdown()
	{
		// Engine shutdown sequence
		Jade::ScriptRuntime::Destroy();
		Jade::IFileDialog::Destroy();
		Jade::IFile::Destroy();
	}

	void JadeEditor::BeginFrame()
	{
		m_ImGuiLayer->BeginFrame();
	}

	void JadeEditor::EndFrame()
	{
		m_ImGuiLayer->EndFrame();
	}

	// ===================================================================================
	// Create application entry point
	// ===================================================================================
	Application* CreateApplication()
	{
		JadeEditor* editor = new JadeEditor();
		return editor;
	}
}