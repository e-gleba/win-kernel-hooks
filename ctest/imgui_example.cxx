#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <expected>
#include <imgui.h>
#include <print>

[[nodiscard]] auto init_glfw() -> std::expected<GLFWwindow*, const char*>
{
    if (!glfwInit())
    {
        return std::unexpected("Failed to init GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto* window = glfwCreateWindow(1280, 720, "ImGui Test", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return std::unexpected("Failed to create window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync
    return window;
}

int main()
{
    auto window = init_glfw();
    if (!window)
    {
        std::print("Error: {}\n", window.error());
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.value(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window.value()))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello ImGui");
        ImGui::Text("Welcome to ImGui!");
        if (ImGui::Button("Exit"))
        {
            glfwSetWindowShouldClose(window.value(), GLFW_TRUE);
        }
        ImGui::End();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window.value(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window.value());
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window.value());
    glfwTerminate();

    return EXIT_SUCCESS;
}
