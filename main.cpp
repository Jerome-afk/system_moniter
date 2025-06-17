#include "header.h"

// Global system monitor instance
SystemMonitor g_monitor;
bool g_running = true;

void UpdateThread() {
    while (g_running) {
        g_monitor.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    printf("SDL initialized successfully.\n");
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540,  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    printf("Window created successfully.\n");
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    if (!gl_context) {
        printf("Error creating OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    printf("OpenGL context created successfully.\n");
    
    // Initialize OpenGL
    if (gl3wInit() != 0) {
        printf("Error initializing GLEW\n");
        return -1;
    }
    printf("GLEW initialized successfully.\n");
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    printf("ImGui context created successfully.\n");
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");
    printf("ImGui initialized successfully.\n");
    
    // Start update thread
    std::thread update_thread(UpdateThread);
    
    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = true;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            }
        }
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // Render the UI
        g_monitor.RenderSystemMonitor();

        //Get the size of the main window
        int width, height;
        SDL_GetWindowSize(window, &width, &height);

        if (io.DisplaySize.x != (float)width || io.DisplaySize.y != (float)height) {
            // Update the display size if it has changed
            io.DisplaySize = ImVec2((float)width, (float)height);
        }

        // Set the viewport to match the window size
        io.DisplaySize = ImVec2((float)width, (float)height);

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    g_running = false;
    update_thread.join();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // This is a Windows-specific entry point, but we will use SDL for cross-platform compatibility.
    return main();
}