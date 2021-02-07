#include "cocoa/scenes/Scene.h"

#include "cocoa/file/OutputArchive.h"
#include "cocoa/file/IFile.h"
#include "cocoa/util/Settings.h"
#include "cocoa/core/Entity.h"
#include "cocoa/components/Transform.h"
#include "cocoa/systems/ScriptSystem.h"
#include "cocoa/scenes/SceneInitializer.h"
#include "cocoa/core/AssetManager.h"
#include "cocoa/renderer/DebugDraw.h"

#include <nlohmann/json.hpp>

namespace Cocoa
{
	Scene::Scene(SceneInitializer* sceneInitializer)
		: m_PickingTexture(3840, 2160)
	{
		m_SceneInitializer = sceneInitializer;

		m_Camera = nullptr;
		m_IsPlaying = false;

		m_Registry = entt::registry();
	}

	Entity Scene::CreateEntity()
	{
		entt::entity e = m_Registry.create();
		Entity entity = Entity(e, this);
		entity.AddComponent<TransformData>();
		return entity;
	}

	void Scene::Init()
	{
		LoadDefaultAssets();

		glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0);
		m_Camera = new Camera(cameraPos);

		Input::SetScene(this);
		Entity::SetScene(this);

		RenderSystem::Init(this);
		Physics2DSystem::Init({ 0, -10.0f });

		m_SceneInitializer->Init(this);
	}

	void Scene::Start()
	{		
		ScriptSystem::Start();
		m_SceneInitializer->Start(this);
	}

	void Scene::Update(float dt)
	{
		Physics2DSystem::Update(this, dt);
		ScriptSystem::Update(this, dt);
	}

	void Scene::EditorUpdate(float dt)
	{
		ScriptSystem::EditorUpdate(this, dt);
	}

	void Scene::Render()
	{
		glDisable(GL_BLEND);
		m_PickingTexture.EnableWriting();

		glViewport(0, 0, 3840, 2160);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO: Very temporary ugly, horrible fix, fix this ASAP
		m_PickingShader = AssetManager::GetShader(Settings::General::s_EngineAssetsPath + "shaders/Picking.glsl");
		RenderSystem::BindShader(m_PickingShader);
		RenderSystem::Render(this);

		m_PickingTexture.DisableWriting();
		glEnable(GL_BLEND);

		glBindFramebuffer(GL_FRAMEBUFFER, RenderSystem::GetMainFramebuffer().GetId());

		glViewport(0, 0, 3840, 2160);
		glClearColor(0.45f, 0.55f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		RenderSystem::BindShader(Handle<Shader>());
		//RenderSystem::UploadUniform1ui("uActiveEntityID", InspectorWindow::GetActiveEntity().GetID() + 1);

		DebugDraw::DrawBottomBatches();
		RenderSystem::Render(this);
		DebugDraw::DrawTopBatches();
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		entt::entity newEntEntity = m_Registry.create();
		Entity newEntity = Entity(newEntEntity, this);
		if (entity.HasComponent<TransformData>())
		{
			newEntity.AddComponent<TransformData>(entity.GetComponent<TransformData>());
		}

		if (entity.HasComponent<SpriteRenderer>())
		{
			newEntity.AddComponent<SpriteRenderer>(entity.GetComponent<SpriteRenderer>());
		}

		if (entity.HasComponent<Rigidbody2D>())
		{
			newEntity.AddComponent<Rigidbody2D>(entity.GetComponent<Rigidbody2D>());
		}

		if (entity.HasComponent<Box2D>())
		{
			newEntity.AddComponent<Box2D>(entity.GetComponent<Box2D>());
		}

		return newEntity;
	}

	Entity Scene::GetEntity(uint32 id)
	{
		entt::entity entity = entt::null;
		if (id < std::numeric_limits<uint32>::max())
		{
			entity = entt::entity(id);
		}
		return Entity(entity, this);
	}

	void Scene::Play()
	{
		m_IsPlaying = true;
		auto view = m_Registry.view<TransformData>();
		for (auto entity : view)
		{
			Physics2DSystem::AddEntity(Entity(entity));
		}
	}

	void Scene::Stop()
	{
		m_IsPlaying = false;
	}

	void Scene::Destroy()
	{
		AssetManager::Clear();
		auto view = m_Registry.view<TransformData>();
		m_Registry.destroy(view.begin(), view.end());

		RenderSystem::Destroy();
		Physics2DSystem::Destroy(this);
		ScriptSystem::FreeScriptLibrary();

		delete m_Camera;
	}

	void Scene::OnEvent(const Event& e)
	{
		// TODO: If you have systems that require events, place them in here
	}

	void Scene::Save(const CPath& filename)
	{
		Log::Info("Saving scene for %s", filename.Filepath());
		m_SaveDataJson = {
			{"Components", {}},
			{"Project", Settings::General::s_CurrentProject.Filepath()},
			{"Assets", AssetManager::Serialize()}
		};

		OutputArchive output(m_SaveDataJson);
		entt::snapshot{ m_Registry }
			.entities(output)
			.component<TransformData, Rigidbody2D, Box2D, SpriteRenderer, FontRenderer, AABB>(output);

		ScriptSystem::SaveScripts(m_SaveDataJson);

		IFile::WriteFile(m_SaveDataJson.dump(4).c_str(), filename);
	}

	void Scene::Reset()
	{
		Destroy();
		Init();
	}

	void Scene::LoadDefaultAssets()
	{
		Texture gizmoSpec;
		gizmoSpec.MagFilter = FilterMode::Linear;
		gizmoSpec.MinFilter = FilterMode::Linear;
		gizmoSpec.WrapS = WrapMode::Repeat;
		gizmoSpec.WrapT = WrapMode::Repeat;
		gizmoSpec.IsDefault = true;
		auto asset = AssetManager::LoadTextureFromFile(gizmoSpec, Settings::General::s_EngineAssetsPath + "images/gizmos.png");
	}

	static Entity FindOrCreateEntity(int id, Scene* scene, entt::registry& registry)
	{
		Entity entity;
		if (registry.valid(entt::entity(id)))
		{
			entity = Entity(entt::entity(id), scene);
		}
		else
		{
			entity = Entity(registry.create(entt::entity(id)), scene);
		}

		return entity;
	}

	void Scene::Load(const CPath& filename)
	{
		Reset();
		Log::Info("Loading scene %s", filename.Filepath());

		Settings::General::s_CurrentScene = filename;
		File* file = IFile::OpenFile(filename);
		if (file->m_Data.size() <= 0)
		{
			return;
		}

		Start();

		json j = json::parse(file->m_Data);

		if (j.contains("Assets"))
		{
			AssetManager::LoadTexturesFrom(j["Assets"]);
			AssetManager::LoadFontsFrom(j["Assets"]);
		}

		int size = !j.contains("Components") ? 0 : j["Components"].size();
		for (int i=0; i < size; i++)
		{
			json::iterator it = j["Components"][i].begin();
			json component = j["Components"][i];
			if (it.key() == "SpriteRenderer")
			{
				Entity entity = FindOrCreateEntity(component["SpriteRenderer"]["Entity"], this, m_Registry);
				RenderSystem::DeserializeSpriteRenderer(component, entity);
			}
			else if (it.key() == "FontRenderer")
			{
				Entity entity = FindOrCreateEntity(component["FontRenderer"]["Entity"], this, m_Registry);
				RenderSystem::DeserializeFontRenderer(component, entity);
			}
			else if (it.key() == "Transform")
			{
				Entity entity = FindOrCreateEntity(component["Transform"]["Entity"], this, m_Registry);
				Transform::Deserialize(component, entity);
			}
			else if (it.key() == "Rigidbody2D")
			{
				Entity entity = FindOrCreateEntity(component["Rigidbody2D"]["Entity"], this, m_Registry);
				Physics2DSystem::DeserializeRigidbody2D(component, entity);
			}
			else if (it.key() == "Box2D")
			{
				Entity entity = FindOrCreateEntity(component["Box2D"]["Entity"], this, m_Registry);
				Physics2DSystem::DeserializeBox2D(component, entity);
			}
			else if (it.key() == "AABB")
			{
				Entity entity = FindOrCreateEntity(component["AABB"]["Entity"], this, m_Registry);
				Physics2DSystem::DeserializeAABB(component, entity);
			}
			else
			{
				Entity entity = FindOrCreateEntity(component.front()["Entity"], this, m_Registry);
				ScriptSystem::Deserialize(component, entity);
			}
		}

		IFile::CloseFile(file);
	}

	void Scene::LoadScriptsOnly(const CPath& filename)
	{
		File* file = IFile::OpenFile(filename);
		if (file->m_Data.size() <= 0)
		{
			return;
		}

		Log::Info("Loading scripts only for %s", filename.Filepath());
		json j = json::parse(file->m_Data);
		int size = !j.contains("Components") ? 0 : j["Components"].size();
		for (int i = 0; i < size; i++)
		{
			json::iterator it = j["Components"][i].begin();
			json component = j["Components"][i];
			if (it.key() != "SpriteRenderer" && it.key() != "Transform" && it.key() != "Rigidbody2D" && it.key() != "Box2D" && it.key() != "AABB")
			{
				Entity entity = FindOrCreateEntity(component.front()["Entity"], this, m_Registry);
				ScriptSystem::Deserialize(component, entity);
			}
		}

		IFile::CloseFile(file);
	}
}