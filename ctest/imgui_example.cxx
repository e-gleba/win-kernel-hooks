#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <expected>
#include <imgui.h>
#include <print>

struct SDLData
{
    SDL_Window*   window;
    SDL_GLContext gl_ctx;
};

[[nodiscard]] auto init_sdl() -> std::expected<SDLData, const char*>
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) != true)
        return std::unexpected(SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window* window =
        SDL_CreateWindow("ImGui SDL3", 1280, 720, SDL_WINDOW_OPENGL);
    if (!window)
        return std::unexpected(SDL_GetError());

    const SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx)
    {
        SDL_DestroyWindow(window);
        return std::unexpected(SDL_GetError());
    }

    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);
    return SDLData{ window, gl_ctx };
}

int main()
{
    auto sdl_data = init_sdl();
    if (!sdl_data)
    {
        std::print("SDL Error: {}\n", sdl_data.error());
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(sdl_data->window, sdl_data->gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello SDL3");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        if (ImGui::Button("Exit"))
            running = false;
        ImGui::End();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(sdl_data->window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(sdl_data->window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(sdl_data->gl_ctx);
    SDL_DestroyWindow(sdl_data->window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
