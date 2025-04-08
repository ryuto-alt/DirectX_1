#include "Scene.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"

Scene::Scene()
    : m_name("")
    , m_isLoaded(false)
    , m_isPaused(false)
    , m_isActive(false)
    , m_sceneManager(nullptr)
{
}

Scene::~Scene()
{
    Shutdown();
}

bool Scene::Initialize()
{
    return true;
}

void Scene::Shutdown()
{
    Unload();
}

bool Scene::Load()
{
    m_isLoaded = true;
    return true;
}

void Scene::Unload()
{
    m_isLoaded = false;
}

void Scene::Update(float deltaTime)
{
    // シーンが一時停止中なら更新しない
    if (m_isPaused) {
        return;
    }
}

void Scene::Render()
{
}

void Scene::Pause()
{
    m_isPaused = true;
}

void Scene::Resume()
{
    m_isPaused = false;
}

void Scene::OnActivated()
{
    m_isActive = true;
}

void Scene::OnDeactivated()
{
    m_isActive = false;
}

const std::string& Scene::GetName() const
{
    return m_name;
}

void Scene::SetName(const std::string& name)
{
    m_name = name;
}

bool Scene::IsLoaded() const
{
    return m_isLoaded;
}

bool Scene::IsPaused() const
{
    return m_isPaused;
}

bool Scene::IsActive() const
{
    return m_isActive;
}

void Scene::SetSceneManager(SceneManager* sceneManager)
{
    m_sceneManager = sceneManager;
}

Engine* Scene::GetEngine() const
{
    return &Engine::GetInstance();
}

SceneManager* Scene::GetSceneManager() const
{
    return m_sceneManager;
}

ResourceManager* Scene::GetResourceManager() const
{
    return GetEngine()->GetResourceManager();
}

InputManager* Scene::GetInputManager() const
{
    return GetEngine()->GetInputManager();
}

SpriteRenderer* Scene::GetSpriteRenderer() const
{
    return GetEngine()->GetSpriteRenderer();
}

ModelRenderer* Scene::GetModelRenderer() const
{
    return GetEngine()->GetModelRenderer();
}

PhysicsSystem* Scene::GetPhysicsSystem() const
{
    return GetEngine()->GetPhysicsSystem();
}

AudioSystem* Scene::GetAudioSystem() const
{
    return GetEngine()->GetAudioSystem();
}