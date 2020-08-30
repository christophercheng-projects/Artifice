#include <Artifice.h>

#include <imgui.h>

#include "EditorLayer.h"



class ArtificeEditor : public Application
{
public:
    ArtificeEditor()
    {
        PushLayer(new EditorLayer());
    }

    ~ArtificeEditor()
    {

    }
};


int main()
{
    ArtificeEditor* app = new ArtificeEditor();
    app->Run();
    delete app;
}
