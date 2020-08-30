#pragma once

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Application.h"
#include "Artifice/Core/Timestep.h"

#include "Artifice/Graphics/Renderer.h"
#include "Artifice/Graphics/SceneRenderer.h"
#include "Artifice/Graphics/Material.h"


#include "Entity.h"
#include "Component.h"


class Scene
{
private:
    EntityRegistry* m_Registry;

    RenderHandle m_BRDFTexture;
    RenderHandle m_BRDFSampler;

    PrecomputedIBLData m_IBLData;
    bool m_IBLSet = false;

public:
    void Init()
    {
        m_Registry = new EntityRegistry();
        m_Registry->RegisterComponent<MeshComponent>();
        m_Registry->RegisterComponent<TransformComponent>();
    }
    void CleanUp()
    {
        delete m_Registry;
        m_Registry = nullptr;
        if (m_IBLSet)
        {
            SceneRenderer::DestroyEnvironmentMap(m_IBLData);
        }
    }
    void OnUpdate(Timestep ts)
    {
        
    }

    void Render(CommandBuffer* cmd, PerspectiveCamera camera, RenderPassLayout rpl)
    {
        std::vector<EntityHandle> entities =  m_Registry->GetEntitiesWith<MeshComponent, TransformComponent>();

        SceneRenderer::BeginScene(GetEnvironment(), camera, rpl, cmd);
        
        for (Archetype* archetype : m_Registry->GetArchetypesWith<MeshComponent>())
        {
            bool has_transform = archetype->Contains<TransformComponent>();
            MeshComponent* meshes = archetype->GetComponents<MeshComponent>();
            if (has_transform)
            {
                TransformComponent* transforms = archetype->GetComponents<TransformComponent>();
                for (uint32 i = 0; i < archetype->EntityHandles.size(); i++)
                {
                    SceneRenderer::SubmitMesh(meshes[i].Mesh, transforms[i].Transform);
                }
            }
            else
            {
                for (uint32 i = 0; i < archetype->EntityHandles.size(); i++)
                {
                    SceneRenderer::SubmitMesh(meshes[i].Mesh, mat4::identity());
                }
            }
        }

        SceneRenderer::EndScene();
    }

    PrecomputedIBLData GetEnvironment() { return m_IBLData; }

    void SetEnvironment(PrecomputedIBLData environment)
    {
        if (m_IBLSet)
        {
            SceneRenderer::DestroyEnvironmentMap(m_IBLData);
        }
        m_IBLData = environment;
        SetSkybox(environment.PrefilterEnvironmentTexture, environment.PrefilterEnvironmentSampler);
        m_IBLSet = true;
    }
    void SetSkybox(RenderHandle skybox_texture, RenderHandle skybox_sampler)
    {
        SceneRenderer::SetSkybox(skybox_texture, skybox_sampler);
    }

    EntityRegistry* GetRegistry() { return m_Registry; }
};