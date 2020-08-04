#pragma once
#include "externalLibs.h"

#include "MenuBar.h"
#include "AssetWindow.h"
#include "jade/core/Layer.h"

namespace Jade
{
    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer()
        {
            m_Window = nullptr;
        }

        void Setup(void* window);

        virtual void OnAttach() override;
        virtual void OnEvent(Event& e) override;
        void BeginFrame();
        void EndFrame();

    private:
        void SetupDockspace();
        void RenderGameViewport();

    private:
        glm::vec2 m_GameviewPos = glm::vec2();
        glm::vec2 m_GameviewSize = glm::vec2();
        glm::vec2 m_GameviewMousePos = glm::vec2();
        bool m_BlockEvents = false;

        void* m_Window;
        MenuBar m_MenuBar{};
        AssetWindow m_AssetWindow;
    };
}