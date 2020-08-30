#include <Artifice.h>

#include <imgui.h>

#include "SandboxAtmosphere.h"
#include "SandboxDigits.h"
#include "Etna.h"
#include "Sandbox2D.h"
//#include "Sandbox3D.h"
//#include "SandboxPBR.h"
#include "SandboxScene.h"
#include "AtmosphereLayer.h"



class Sandbox : public Application
{
public:
    Sandbox()
    {
        //PushLayer(new Sandbox2D());
        //PushLayer(new SandboxScene());
        //PushLayer(new SandboxDigits());
        PushLayer(new SandboxAtmosphere());
        //PushLayer(new Etna());
        //PushLayer(new AtmosphereLayer());
    }

    ~Sandbox()
    {

    }
};

int main()
{
    AR_PROFILE_BEGIN_SESSION("Startup", "/Users/christophercheng/Desktop/ArtificeProfile-Startup.json");
    Sandbox* app = new Sandbox();
    AR_PROFILE_END_SESSION();
    AR_PROFILE_BEGIN_SESSION("Runtime", "/Users/christophercheng/Desktop/ArtificeProfile-Runtime.json");
    app->Run();
    AR_PROFILE_END_SESSION();
    AR_PROFILE_BEGIN_SESSION("Shutdown", "/Users/christophercheng/Desktop/ArtificeProfile-Shutdown.json");
    delete app;
    AR_PROFILE_END_SESSION();
}
