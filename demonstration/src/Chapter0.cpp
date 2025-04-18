///#include <thread>
#include <algorithm>

#include "pandaFramework.h" // Panda3D 1.10.9
//#include "renderBuffer.h"
#include "load_prc_file.h"
//#include "pStatClient.h"
#include "pandaSystem.h"
#include "mouseButton.h"
#include "mouseWatcher.h"
#include "buttonRegistry.h"
#include "orthographicLens.h"
#include "ambientLight.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotlight.h"
#include "shader.h"
#include "nodePathCollection.h"
#include "auto_bind.h"
#include "animControlCollection.h"
#include "cardMaker.h"
#include "fontPool.h"
#include "texturePool.h"
#include "particleSystemManager.h"
#include "physicsManager.h"
#include "spriteParticleRenderer.h"
#include "pointParticleFactory.h"
#include "pointEmitter.h"
#include "physicalNode.h"
#include "forceNode.h"
//#include "linearNoiseForce.h"
#include <chrono>
#include <thread>

#include "linearVectorForce.h"
#include "linearJitterForce.h"
#include "linearCylinderVortexForce.h"
#include "linearEulerIntegrator.h"
#include "audioManager.h"
#include "audioSound.h"
#include "corecrt_math_defines.h"

PT(MouseWatcher) getMouseWatcher(WindowFramework* window)
{
    return DCAST(MouseWatcher, window->get_mouse().node());
}

double toRadians(double d) { return d * M_PI / 180.0; }

LVecBase3f calculateCameraPosition(double radius, double phi, double theta, LVecBase3 lookAt)
{
    double x = radius * sin(toRadians(phi)) * cos(toRadians(theta)) + lookAt[0];
    double y = radius * sin(toRadians(phi)) * sin(toRadians(theta)) + lookAt[1];
    double z = radius * cos(toRadians(phi)) + lookAt[2];
    return LVecBase3f(x, y, z);
}

PT(AudioManager) audioManager = AudioManager::create_AudioManager();

void setTextureToNearestAndClamp(PT(Texture) texture)
{
    texture->set_magfilter(SamplerState::FT_nearest);
    texture->set_minfilter(SamplerState::FT_nearest);
    texture->set_wrap_u(SamplerState::WM_clamp);
    texture->set_wrap_v(SamplerState::WM_clamp);
    texture->set_wrap_w(SamplerState::WM_clamp);
}

int microsecondsSinceEpoch()
{
    return std::chrono::duration_cast
        <std::chrono::microseconds>
        (std::chrono::system_clock::now().time_since_epoch()
        ).count();
}

double microsecondToSecond(int m)
{
    return m / 1000000.0;
}
LVecBase3f calculateCameraLookAt
(double upDownAdjust
 , double leftRightAdjust
 , double phi
 , double theta
 , LVecBase3 lookAt
)
{
    lookAt[0] += upDownAdjust * sin(toRadians(-theta - 90)) * cos(toRadians(phi));
    lookAt[1] += upDownAdjust * cos(toRadians(-theta - 90)) * cos(toRadians(phi));
    lookAt[2] -= -upDownAdjust * sin(toRadians(phi));

    lookAt[0] += leftRightAdjust * sin(toRadians(-theta));
    lookAt[1] += leftRightAdjust * cos(toRadians(-theta));
    return lookAt;
}

PT(AsyncTaskManager) taskManager = AsyncTaskManager::get_global_ptr();

bool isButtonDown(PT(MouseWatcher) mouseWatcher, std::string character)
{
    return mouseWatcher->is_button_down(ButtonRegistry::ptr()->find_button(character));
}

PT(Shader) loadShader
(std::string vert
 , std::string frag
)
{
    return Shader::load
    (Shader::SL_GLSL
     , "../shaders/vertex/" + vert + ".vert"
     , "../shaders/fragment/" + frag + ".frag"
    );
}

struct FramebufferTexture
{
    PT(GraphicsOutput) buffer;
    PT(DisplayRegion) bufferRegion;
    PT(Camera) camera;
    NodePath cameraNP;
    NodePath shaderNP;
};

struct FramebufferTextureArguments
{
    PT(WindowFramework) window;
    PT(GraphicsOutput) graphicsOutput;
    PT(GraphicsEngine) graphicsEngine;
    GraphicsOutput::RenderTexturePlane bitplane;
    LVecBase4 rgbaBits;
    LColor clearColor;
    int aux_rgba;
    bool setFloatColor;
    bool setSrgbColor;
    bool setRgbColor;
    bool useScene;
    std::string name;
};

const int BACKGROUND_RENDER_SORT_ORDER = 10;
const int UNSORTED_RENDER_SORT_ORDER = 50;

FramebufferTexture generateFramebufferTexture(FramebufferTextureArguments framebufferTextureArguments);


LVecBase2f makeEnabledVec(int t){
    if (t >= 1) { t = 1; }
    else { t = 0; }
    return LVecBase2f(t, t);
}


struct OutShader
{
    PT(Shader) OutBaseShader;
    PT(Shader) OutGeometryBufferShader0;
};

OutShader GetModifyTexture();


int main(int argc, char* argv[])
{
    LColor backgroundColor[] = {
        LColor(0.392, 0.537, 0.561, 1),
        LColor(0.953, 0.733, 0.525, 1)
    };

    load_prc_file("panda3d-prc-file.prc");

    PT(TextFont) font = FontPool::load_font("fonts/font.ttf");
    std::vector<PT(AudioSound)> sounds =
    {
        audioManager->get_sound("sounds/wheel.ogg", true), audioManager->get_sound("sounds/water.ogg", true)
    };

    PT(Texture) blankTexture = TexturePool::load_texture("images/blank.png");
    PT(Texture) foamPatternTexture = TexturePool::load_texture("images/foam-pattern.png");
    PT(Texture) stillFlowTexture = TexturePool::load_texture("images/still-flow.png");
    PT(Texture) upFlowTexture = TexturePool::load_texture("images/up-flow.png");
    PT(Texture) colorLookupTableTextureN = TexturePool::load_texture("images/lookup-table-neutral.png");
    PT(Texture) colorLookupTableTexture0 = TexturePool::load_texture("images/lookup-table-0.png");
    PT(Texture) colorLookupTableTexture1 = TexturePool::load_texture("images/lookup-table-1.png");
    PT(Texture) smokeTexture = TexturePool::load_texture("images/smoke.png");
    PT(Texture) colorNoiseTexture = TexturePool::load_texture("images/color-noise.png");

    setTextureToNearestAndClamp(colorLookupTableTextureN);
    setTextureToNearestAndClamp(colorLookupTableTexture0);
    setTextureToNearestAndClamp(colorLookupTableTexture1);

    // Open a new window framework
    PandaFramework framework;
    framework.open_framework(argc, argv);

    // Set the window title and open the window
    framework.set_window_title("Panda3D Shader Demo");
    PT(WindowFramework) window = framework.open_window();
    PT(GraphicsWindow) graphicsWindow = window->get_graphics_window();
    PT(GraphicsOutput) graphicsOutput = window->get_graphics_output();
    PT(GraphicsStateGuardian) graphicsStateGuardian = graphicsOutput->get_gsg();
    PT(GraphicsEngine) graphicsEngine = graphicsStateGuardian->get_engine();

    window->enable_keyboard();
    // Here is room for your own code
    PT(DisplayRegion) displayRegion3d = window->get_display_region_3d();
    displayRegion3d->set_clear_color_active(true);
    displayRegion3d->set_clear_depth_active(true);
    displayRegion3d->set_clear_stencil_active(true);
    displayRegion3d->set_clear_color(backgroundColor[1]);
    displayRegion3d->set_clear_depth(1.0f);
    displayRegion3d->set_clear_stencil(0);

    NodePath render = window->get_render();
    NodePath render2d = window->get_render_2d();


    float statusAlpha = 1.0;
    LColor statusColor = LColor(0.9, 0.9, 1.0, statusAlpha);
    LColor statusShadowColor = LColor(0.1, 0.1, 0.3, statusAlpha);
    float statusFadeRate = 2.0;
    std::string statusText = "Ready";

    PT(TextNode) status = new TextNode("status");
    status->set_font(font);
    status->set_text(statusText);
    status->set_text_color(statusColor);
    status->set_shadow(0.0, 0.06);
    status->set_shadow_color(statusShadowColor);
    NodePath statusNP = render2d.attach_new_node(status);
    statusNP.set_scale(0.05);
    statusNP.set_pos(-0.96, 0, -0.95);


    double cameraRotatePhiInitial = 67.5095;
    double cameraRotateThetaInitial = 231.721;
    double cameraRotateRadiusInitial = 1100.83;
    LVecBase3 cameraLookAtInitial = LVecBase3(1.00839, 1.20764, 5.85055);
    float cameraFov = 1.0;
    int cameraNear = 150;
    int cameraFar = 2000;
    LVecBase2f cameraNearFar = LVecBase2f(cameraNear, cameraFar);
    double cameraRotateRadius = cameraRotateRadiusInitial;
    double cameraRotatePhi = cameraRotatePhiInitial;
    double cameraRotateTheta = cameraRotateThetaInitial;
    LVecBase3 cameraLookAt = cameraLookAtInitial;


    double cameraUpDownAdjust = 0;
    double cameraLeftRightAdjust = 0;

    PT(MouseWatcher) mouseWatcher = getMouseWatcher(window);

    bool mouseLeftDown = mouseWatcher->is_button_down(MouseButton::one());
    bool mouseMiddleDown = mouseWatcher->is_button_down(MouseButton::two());
    bool mouseRightDown = mouseWatcher->is_button_down(MouseButton::three());

    LVecBase2f riorInitial = LVecBase2f(1.05, 1.05);
    float riorAdjust = 0.005;
    LVecBase2f rior = riorInitial;

    LVecBase2f mouseFocusPointInitial = LVecBase2f(0.509167, 0.598);
    LVecBase2f mouseFocusPoint = mouseFocusPointInitial;

    LVecBase2f mouseThen = LVecBase2f(0.0, 0.0);
    LVecBase2f mouseNow = mouseThen;
    bool mouseWheelDown = false;
    bool mouseWheelUp = false;

    int then = microsecondsSinceEpoch();
    int loopStartedAt = then;
    int now = then;
    int keyTime = now;

    double delta = microsecondToSecond(now - then);

    then = now;

    double movement = 100 * delta;


    PT(Camera) mainCamera = window->get_camera(0);
    PT(Lens) mainLens = mainCamera->get_lens();
    mainLens->set_fov(cameraFov);
    mainLens->set_near_far(cameraNear, cameraFar);

    NodePath cameraNP = window->get_camera_group();
    cameraNP.set_pos(calculateCameraPosition(cameraRotateRadius, cameraRotatePhi, cameraRotateTheta, cameraLookAt));
    cameraNP.look_at(cameraLookAt);

    bool soundEnabled = true;
    bool soundStarted = false;
    float startSoundAt = 0.5;
    
    auto beforeFrame = [&]() -> void
    {
        WindowProperties windowProperties = graphicsWindow->get_properties();
        if (windowProperties.get_minimized())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        now = microsecondsSinceEpoch();
        double delta = microsecondToSecond(now - then);
        then = now;
        // Avoids a loud audio pop.
        if (!soundStarted && microsecondToSecond(now - loopStartedAt) >= startSoundAt)
        {
            for_each
            (sounds.begin()
             , sounds.end()
             , [](PT(AudioSound) sound)
             {
                 sound->set_loop(true);
                 sound->play();
             }
            );
            soundStarted = true;
        }
        
              double movement = 100 * delta;

        double timeSinceKey = microsecondToSecond(now - keyTime);
        bool keyDebounced = timeSinceKey >= 0.2;

        double cameraUpDownAdjust = 0;
        double cameraLeftRightAdjust = 0;

        bool shiftDown = isButtonDown(mouseWatcher, "shift");
        bool tabDown = isButtonDown(mouseWatcher, "tab");

        bool resetDown = isButtonDown(mouseWatcher, "r");

        bool fogNearDown = isButtonDown(mouseWatcher, "[");
        bool fogFarDown = isButtonDown(mouseWatcher, "]");

        bool equalDown = isButtonDown(mouseWatcher, "=");
        bool minusDown = isButtonDown(mouseWatcher, "-");

        bool deleteDown = isButtonDown(mouseWatcher, "delete");

        bool wDown = isButtonDown(mouseWatcher, "w");
        bool aDown = isButtonDown(mouseWatcher, "a");
        bool dDown = isButtonDown(mouseWatcher, "d");
        bool sDown = isButtonDown(mouseWatcher, "s");
        bool zDown = isButtonDown(mouseWatcher, "z");
        bool xDown = isButtonDown(mouseWatcher, "x");

        bool arrowUpDown = isButtonDown(mouseWatcher, "arrow_up");
        bool arrowDownDown = isButtonDown(mouseWatcher, "arrow_down");
        bool arrowLeftDown = isButtonDown(mouseWatcher, "arrow_left");
        bool arrowRightDown = isButtonDown(mouseWatcher, "arrow_right");

        bool middayDown = isButtonDown(mouseWatcher, "1");
        bool midnightDown = isButtonDown(mouseWatcher, "2");
        bool fresnelDown = isButtonDown(mouseWatcher, "3");
        bool rimLightDown = isButtonDown(mouseWatcher, "4");
        bool particlesDown = isButtonDown(mouseWatcher, "5");
        bool motionBlurDown = isButtonDown(mouseWatcher, "6");
        bool painterlyDown = isButtonDown(mouseWatcher, "7");
        bool celShadingDown = isButtonDown(mouseWatcher, "8");
        bool lookupTableDown = isButtonDown(mouseWatcher, "9");
        bool blinnPhongDown = isButtonDown(mouseWatcher, "0");
        bool ssaoDown = isButtonDown(mouseWatcher, "y");
        bool outlineDown = isButtonDown(mouseWatcher, "u");
        bool bloomDown = isButtonDown(mouseWatcher, "i");
        bool normalMapsDown = isButtonDown(mouseWatcher, "o");
        bool fogDown = isButtonDown(mouseWatcher, "p");
        bool depthOfFieldDown = isButtonDown(mouseWatcher, "h");
        bool posterizeDown = isButtonDown(mouseWatcher, "j");
        bool pixelizeDown = isButtonDown(mouseWatcher, "k");
        bool sharpenDown = isButtonDown(mouseWatcher, "l");
        bool filmGrainDown = isButtonDown(mouseWatcher, "n");
        bool reflectionDown = isButtonDown(mouseWatcher, "m");
        bool refractionDown = isButtonDown(mouseWatcher, ",");
        bool flowMapsDown = isButtonDown(mouseWatcher, ".");
        bool sunlightDown = isButtonDown(mouseWatcher, "/");
        bool chromaticAberrationDown = isButtonDown(mouseWatcher, "\\");

        bool mouseLeftDown = mouseWatcher->is_button_down(MouseButton::one());
        bool mouseMiddleDown = mouseWatcher->is_button_down(MouseButton::two());
        bool mouseRightDown = mouseWatcher->is_button_down(MouseButton::three());

        if (wDown)
        {
            cameraRotatePhi -= movement * 0.5;
        }

        if (sDown)
        {
            cameraRotatePhi += movement * 0.5;
        }

        if (aDown)
        {
            cameraRotateTheta += movement * 0.5;
        }

        if (dDown)
        {
            cameraRotateTheta -= movement * 0.5;
        }

        if (zDown || mouseWheelUp)
        {
            cameraRotateRadius -= movement * 4 + 50 * mouseWheelUp;
            mouseWheelUp = false;
        }

        if (xDown || mouseWheelDown)
        {
            cameraRotateRadius += movement * 4 + 50 * mouseWheelDown;
            mouseWheelDown = false;
        }

        cameraRotatePhi = std::max<double>(cameraRotatePhi, 1);
        cameraRotatePhi = std::min<double>(cameraRotatePhi, 179);
        if (cameraRotatePhi < 0) cameraRotatePhi = 360 - cameraRotateTheta;
        if (cameraRotateTheta > 360) cameraRotateTheta = cameraRotateTheta - 360;
        if (cameraRotateTheta < 0) cameraRotateTheta = 360 - cameraRotateTheta;
        cameraRotateRadius = std::max<double>(cameraRotateRadius, cameraNear + 5);
        cameraRotateRadius = std::min<double>(cameraRotateRadius, cameraFar - 10);

        if (arrowUpDown)
        {
            cameraUpDownAdjust = -2 * delta;
        }
        else if (arrowDownDown)
        {
            cameraUpDownAdjust = 2 * delta;
        }

        if (arrowLeftDown)
        {
            cameraLeftRightAdjust = 2 * delta;
        }
        else if (arrowRightDown)
        {
            cameraLeftRightAdjust = -2 * delta;
        }

        if (mouseWatcher->has_mouse())
        {
            mouseNow[0] = mouseWatcher->get_mouse_x();
            mouseNow[1] = mouseWatcher->get_mouse_y();

            if (mouseLeftDown)
            {
                cameraRotateTheta += (mouseThen[0] - mouseNow[0]) * movement;
                cameraRotatePhi += (mouseNow[1] - mouseThen[1]) * movement;
            }
            else if (mouseRightDown)
            {
                cameraLeftRightAdjust = (mouseThen[0] - mouseNow[0]) * movement;
                cameraUpDownAdjust = (mouseThen[1] - mouseNow[1]) * movement;
            }
            else if (mouseMiddleDown)
            {
                mouseFocusPoint =
                    LVecBase2f
                    ((mouseNow[0] + 1.0) / 2.0
                     , (mouseNow[1] + 1.0) / 2.0
                    );
            }

            if (!mouseLeftDown)
            {
                mouseThen = mouseNow;
            }
        }

        cameraLookAt =
            calculateCameraLookAt
            (cameraUpDownAdjust
             , cameraLeftRightAdjust
             , cameraRotatePhi
             , cameraRotateTheta
             , cameraLookAt
            );

        cameraNP.set_pos
        (calculateCameraPosition
            (cameraRotateRadius
             , cameraRotatePhi
             , cameraRotateTheta
             , cameraLookAt
            )
        );

        cameraNP.look_at(cameraLookAt);
    };

    auto beforeFrameRunner = [](GenericAsyncTask* task, void* arg)-> AsyncTask::DoneStatus
    {
        (*static_cast<decltype(beforeFrame)*>(arg))();
        return AsyncTask::DS_cont;
    };

    
    taskManager->add
    (new GenericAsyncTask("beforeFrame"
                          , beforeFrameRunner
                          , &beforeFrame
        )
    );
    auto setMouseWheelUp =
            [&]()
            {
                mouseWheelUp = true;
            };

    auto setMouseWheelDown =
        [&]()
        {
            mouseWheelDown = true;
        };

    framework.define_key
    ("wheel_up"
     , "Mouse Wheel Up"
     , [](const Event*, void* arg)
     {
         (*static_cast<decltype(setMouseWheelUp)*>(arg))();
     }
     , &setMouseWheelUp
    );

    framework.define_key("wheel_down", "Mouse Wheel Down", [](const Event*, void* arg)
     {
         (*static_cast<decltype(setMouseWheelDown)*>(arg))();
     }, &setMouseWheelDown
    );

    PT(PandaNode) sceneRootPN = new PandaNode("sceneRoot");
    NodePath sceneRootNP = NodePath(sceneRootPN);
    sceneRootNP.reparent_to(render);

    NodePath environmentNP = window->load_model(framework.get_models(), "eggs/mill-scene/mill-scene.bam");
    environmentNP.reparent_to(sceneRootNP);
    NodePath shuttersNP = window->load_model(framework.get_models(), "eggs/mill-scene/shutters.bam");
    shuttersNP.reparent_to(sceneRootNP);
    NodePath weatherVaneNP = window->load_model(framework.get_models(), "eggs/mill-scene/weather-vane.bam");
    weatherVaneNP.reparent_to(sceneRootNP);
    NodePath bannerNP = window->load_model(framework.get_models(), "eggs/mill-scene/banner.bam");

    bannerNP.reparent_to(sceneRootNP);


    OutShader TempOutShader = GetModifyTexture();
    
    PT(Shader) baseShader = TempOutShader.OutBaseShader;//loadShader("base", "base");

    PT(Shader) geometryBufferShader0 = TempOutShader.OutGeometryBufferShader0;//loadShader("base", "geometry-buffer-0");
    
    NodePath mainCameraNP = NodePath("mainCamera");
    mainCameraNP.set_shader(baseShader);
    mainCamera->set_initial_state(mainCameraNP.get_state());

    LVecBase4 rgba8 = (8, 8, 8, 8);
    LVecBase4 rgba16 = (16, 16, 16, 16);
    LVecBase4 rgba32 = (32, 32, 32, 32);
    
    FramebufferTextureArguments framebufferTextureArguments;
    framebufferTextureArguments.window = window;
    framebufferTextureArguments.graphicsOutput = graphicsOutput;
    framebufferTextureArguments.graphicsEngine = graphicsEngine;

    framebufferTextureArguments.bitplane = GraphicsOutput::RTP_color;
    framebufferTextureArguments.rgbaBits = rgba32;
    framebufferTextureArguments.clearColor = LColor(0, 0, 0, 0);
    framebufferTextureArguments.aux_rgba = 1;
    framebufferTextureArguments.setFloatColor = true;
    framebufferTextureArguments.setSrgbColor = false;
    framebufferTextureArguments.setRgbColor = true;
    framebufferTextureArguments.useScene = true;
    framebufferTextureArguments.name = "geometry0";

    
    FramebufferTexture geometryFramebufferTexture0 =generateFramebufferTexture(framebufferTextureArguments);
    PT(GraphicsOutput) geometryBuffer0 = geometryFramebufferTexture0.buffer;
    PT(Camera) geometryCamera0 = geometryFramebufferTexture0.camera;
    NodePath geometryNP0 = geometryFramebufferTexture0.shaderNP;
    geometryBuffer0->add_render_texture(NULL, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_0);
    
    LVecBase2f normalMapsEnabled = makeEnabledVec(1);
    
    geometryBuffer0->set_clear_active(3, true);
    geometryBuffer0->set_clear_value(3, framebufferTextureArguments.clearColor);
    geometryNP0.set_shader(geometryBufferShader0);
    geometryNP0.set_shader_input("normalMapsEnabled", normalMapsEnabled);
    geometryCamera0->set_initial_state(geometryNP0.get_state());
    geometryCamera0->set_camera_mask(BitMask32::bit(1));
    PT(Texture) positionTexture0 = geometryBuffer0->get_texture(0);
    PT(Texture) normalTexture0 = geometryBuffer0->get_texture(1);
    PT(Lens) geometryCameraLens0 = geometryCamera0->get_lens();

    const float TO_RAD = M_PI / 180.0;
    const LVecBase2f PI_SHADER_INPUT = LVecBase2f(M_PI, TO_RAD);

    const float GAMMA = 2.2;
    const float GAMMA_REC = 1.0 / GAMMA;
    const LVecBase2f GAMMA_SHADER_INPUT = LVecBase2f(GAMMA, GAMMA_REC);

    
    FramebufferTexture baseFramebufferTexture =
       generateFramebufferTexture
       (framebufferTextureArguments
       );
    
    NodePath baseNP = baseFramebufferTexture.shaderNP;
    baseNP.set_shader(baseShader);
   
    
    // Do the main loop, equal to run() in python
    framework.main_loop();
    framework.close_framework();
    return (0);
}

FramebufferTexture generateFramebufferTexture(FramebufferTextureArguments framebufferTextureArguments)
{
    PT(WindowFramework) window = framebufferTextureArguments.window;
    PT(GraphicsOutput) graphicsOutput = framebufferTextureArguments.graphicsOutput;
    PT(GraphicsEngine) graphicsEngine = framebufferTextureArguments.graphicsEngine;
    LVecBase4 rgbaBits = framebufferTextureArguments.rgbaBits;
    GraphicsOutput::RenderTexturePlane bitplane = framebufferTextureArguments.bitplane;
    int aux_rgba = framebufferTextureArguments.aux_rgba;
    bool setFloatColor = framebufferTextureArguments.setFloatColor;
    bool setSrgbColor = framebufferTextureArguments.setSrgbColor;
    bool setRgbColor = framebufferTextureArguments.setRgbColor;
    bool useScene = framebufferTextureArguments.useScene;
    std::string name = framebufferTextureArguments.name;
    LColor clearColor = framebufferTextureArguments.clearColor;

    FrameBufferProperties fbp = FrameBufferProperties::get_default();
    fbp.set_back_buffers(0);
    fbp.set_rgba_bits
    (rgbaBits[0]
     , rgbaBits[1]
     , rgbaBits[2]
     , rgbaBits[3]
    );
    fbp.set_aux_rgba(aux_rgba);
    fbp.set_float_color(setFloatColor);
    fbp.set_srgb_color(setSrgbColor);
    fbp.set_rgb_color(setRgbColor);

    PT(GraphicsOutput) buffer =
        graphicsEngine
        ->make_output
        (graphicsOutput->get_pipe()
         , name + "Buffer"
         , BACKGROUND_RENDER_SORT_ORDER - 1
         , fbp
         , WindowProperties::size(0, 0),
         GraphicsPipe::BF_refuse_window
         | GraphicsPipe::BF_resizeable
         | GraphicsPipe::BF_can_bind_every
         | GraphicsPipe::BF_rtt_cumulative
         | GraphicsPipe::BF_size_track_host
         , graphicsOutput->get_gsg()
         , graphicsOutput->get_host()
        );
    buffer->add_render_texture
    (NULL
     , GraphicsOutput::RTM_bind_or_copy
     , bitplane
    );
    buffer->set_clear_color(clearColor);

    NodePath cameraNP = NodePath("");
    PT(Camera) camera = NULL;

    if (useScene)
    {
        cameraNP = window->make_camera();
        camera = DCAST(Camera, cameraNP.node());
        camera->set_lens(window->get_camera(0)->get_lens());
    }
    else
    {
        camera = new Camera(name + "Camera");
        PT(OrthographicLens) lens = new OrthographicLens();
        lens->set_film_size(2, 2);
        lens->set_film_offset(0, 0);
        lens->set_near_far(-1, 1);
        camera->set_lens(lens);
        cameraNP = NodePath(camera);
    }

    PT(DisplayRegion) bufferRegion =
        buffer->make_display_region(0, 1, 0, 1);
    bufferRegion->set_camera(cameraNP);

    NodePath shaderNP = NodePath(name + "Shader");

    if (!useScene)
    {
        NodePath renderNP = NodePath(name + "Render");
        renderNP.set_depth_test(false);
        renderNP.set_depth_write(false);
        cameraNP.reparent_to(renderNP);
        CardMaker card = CardMaker(name);
        card.set_frame_fullscreen_quad();
        card.set_has_uvs(true);
        NodePath cardNP = NodePath(card.generate());
        cardNP.reparent_to(renderNP);
        cardNP.set_pos(0, 0, 0);
        cardNP.set_hpr(0, 0, 0);
        cameraNP.look_at(cardNP);
    }

    FramebufferTexture result;
    result.buffer = buffer;
    result.bufferRegion = bufferRegion;
    result.camera = camera;
    result.cameraNP = cameraNP;
    result.shaderNP = shaderNP;
    return result;
}

OutShader GetModifyTexture()
{
    PT(Shader) baseShader = loadShader("base", "base_color");
    PT(Shader) geometryBufferShader0 = loadShader("base", "geometry-buffer-0");
    OutShader out_shader;
    out_shader.OutBaseShader = baseShader;
    out_shader.OutGeometryBufferShader0 = geometryBufferShader0;
    return out_shader;
}

