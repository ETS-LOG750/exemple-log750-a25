#include "MainWindow.h"

#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

MainWindow::MainWindow() :
    m_camera(m_windowWidth, m_windowHeight,
        glm::vec3(2.0, 2.0, 2.0),
        glm::vec3(0.0, 0.0, 0.0))
{
}

int MainWindow::Initialisation()
{
    // OpenGL version (usefull for imGUI and other libraries)
    const char* glsl_version = "#version 460 core";

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Grass", NULL, NULL);
    if (m_window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(m_window);
    InitializeCallback();

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 2;
    }

    // Setup Dear ImGui context
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Other openGL initialization
    // -----------------------------
    return InitializeGL();
}


void MainWindow::InitializeCallback() {
    glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(this));
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        MainWindow* w = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        w->FramebufferSizeCallback(width, height);
        });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        MainWindow* w = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        w->CursorPositionCallback(xpos, ypos);
        });

}

void MainWindow::FramebufferSizeCallback(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, width, height);
    m_camera.viewportEvents(width, height);
}

void MainWindow::CursorPositionCallback(double xpos, double ypos) {
    if (!m_imGuiActive) {
        int state = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);
        m_camera.mouseEvents(glm::vec2(xpos, ypos), state == GLFW_PRESS);
    }
}

int MainWindow::InitializeGL()
{
    // build and compile our shader program
    const std::string directory = SHADERS_DIR;

    // --- Grass shader
    m_grassShader = std::make_unique<ShaderProgram>();
    bool loadSucess = true;
    loadSucess &= m_grassShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "grass.vert");
    loadSucess &= m_grassShader->addShaderFromSource(GL_GEOMETRY_SHADER, directory + "grass.gs");
    loadSucess &= m_grassShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "grass.frag");
    loadSucess &= m_grassShader->link();
    if (!loadSucess) {
        std::cerr << "Error when loading main shader\n";
        return 4;
    }
  
    std::cout << "Load textures ... \n";
    std::string assets_dir = ASSETS_DIR;
    std::string GrassPath = assets_dir + "grass_texture.png";
    if (!loadTexture(&GrassPath[0], TextureId, GL_MIRRORED_REPEAT, GL_LINEAR, GL_LINEAR)) {
        std::cerr << "Error when loading image grass_texture.png\n";
        return 5;
    }
    std::string WindPath = assets_dir + "flowmap.png";
    if (!loadTexture(&WindPath[0], WindTextureId, GL_REPEAT, GL_LINEAR, GL_LINEAR)) {
        std::cerr << "Error when loading image flowmap.png\n";
        return 6;
    }

    std::cout << "Create geometry ... \n";
    glCreateVertexArrays(NumVAOs, m_VAOs);
    glCreateBuffers(NumBuffers, m_buffers); 

    // Generate random points for grass positions
    generatePoints();
    // Load and set buffers/attributes
    // -- Positions (vec3)
    glVertexArrayVertexBuffer(m_VAOs[VAO_Points], 0, m_buffers[VBO_Points_Pos], 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(m_VAOs[VAO_Points], 0);
    glVertexArrayAttribFormat(m_VAOs[VAO_Points], 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_VAOs[VAO_Points], 0, 0);
    // -- Random orientation (float)
    glVertexArrayVertexBuffer(m_VAOs[VAO_Points], 1, m_buffers[VBO_Points_RandomOrientation], 0, sizeof(float));
    glEnableVertexArrayAttrib(m_VAOs[VAO_Points], 1);
    glVertexArrayAttribFormat(m_VAOs[VAO_Points], 1, 1, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_VAOs[VAO_Points], 1, 1);

    // Set uniform (default values) -- otherwise it wont be initialized
    m_grassShader->setFloat(m_uGrass.size, m_grassSize);

    glEnable(GL_DEPTH_TEST);
    std::cout << "Intialization finished\n";

    // Setup projection matrix (a bit hacky)
    FramebufferSizeCallback(m_windowWidth, m_windowHeight);

    return 0;
}

void MainWindow::generatePoints()
{
    std::vector<glm::vec3> points;
    std::vector<float> randomOrientations;
    points.reserve(m_gridSize * m_gridSize);
    float halfSize = (m_gridSize - 1) * 0.5f;
    for (int i = 0; i < m_gridSize; ++i) {
        for (int j = 0; j < m_gridSize; ++j) {

            // Shuffle the point  a bit
            float shuffleX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
            float shuffleZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
            points.emplace_back(
                ((float(i) - halfSize) / halfSize * m_gridSize * 0.5f + shuffleX) * m_density,
                0.0f,
                ((float(j) - halfSize) / halfSize * m_gridSize * 0.5f + shuffleZ) * m_density
            );
            // Random orientation
            float randomAngle = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f; // 2*PI
            randomOrientations.push_back(randomAngle);
        }
    }

    glNamedBufferData(m_buffers[VBO_Points_Pos],
        sizeof(glm::vec3) * points.size(),
        points.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(m_buffers[VBO_Points_RandomOrientation],
        sizeof(float) * randomOrientations.size(),
        randomOrientations.data(),
        GL_STATIC_DRAW);
}

void MainWindow::RenderImgui()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //imgui 
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Grass");

        if(ImGui::InputFloat("Grass size", &m_grassSize)) {
            m_grassShader->setFloat(m_uGrass.size, m_grassSize);
        }
        if(ImGui::InputInt("Grid size", &m_gridSize) || ImGui::InputFloat("Density", &m_density)) {
            if (m_gridSize < 1) m_gridSize = 1;
            generatePoints();
        }
        ImGui::Checkbox("Active Wind", &m_activeWind);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void MainWindow::RenderScene(float time)
{
    // render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_grassShader->bind();
    
    glm::mat4 modelMatrix = glm::mat4(1.0);
    glm::mat4 viewMatrix = m_camera.viewMatrix();
    m_grassShader->setMat4(m_uGrass.mvMatrix, viewMatrix);
    m_grassShader->setMat4(m_uGrass.projMatrix, m_camera.projectionMatrix());
    if(m_activeWind)
        m_grassShader->setFloat(m_uGrass.time, time);
    else
        m_grassShader->setFloat(m_uGrass.time, 0.0f);
    glBindTextureUnit(0, TextureId);
    glBindTextureUnit(1, WindTextureId);

    glBindVertexArray(m_VAOs[VAO_Points]);
    glDrawArrays(GL_POINTS, 0, m_gridSize * m_gridSize);
    glBindVertexArray(0);
}


int MainWindow::RenderLoop()
{
    float time = glfwGetTime();
    while (!glfwWindowShouldClose(m_window))
    {
        // Compute delta time between two frames
        float new_time = glfwGetTime();
        const float delta_time = new_time - time;
        time = new_time;

        // Check inputs: Does ESC was pressed?
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, true);
        if (!m_imGuiActive) {
            m_camera.keybordEvents(m_window, delta_time);
        }

        RenderScene(time);
        RenderImgui();

        // Show rendering and get events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();

    return 0;
}

bool MainWindow::loadTexture(const std::string& path, unsigned int& textureID, GLint uvMode, GLint minMode, GLint magMode)
{
    // OpenGL 4.6 -- need to specify the texture type
	glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

	// Ask the library to flip the image horizontally
	// This is necessary as TexImage2D assume "The first element corresponds to the lower left corner of the texture image"
	// whereas stb_image load the image such "the first pixel pointed to is top-left-most in the image"
	stbi_set_flip_vertically_on_load(true);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);
	if (data)
	{
		glTextureStorage2D(textureID, 1, GL_RGBA8, width, height);
		glTextureSubImage2D(textureID, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		if (minMode == GL_LINEAR_MIPMAP_LINEAR) {
			glGenerateTextureMipmap(textureID);
		}

		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, uvMode);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, uvMode);
		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, minMode);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, magMode);

		stbi_image_free(data);

		std::cout << "Texture loaded at path: " << path << std::endl;
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		return false;
	}

	return true;
}