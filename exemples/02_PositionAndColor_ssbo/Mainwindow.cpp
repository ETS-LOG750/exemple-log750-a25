#include "MainWindow.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

using namespace std;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

MainWindow::MainWindow()
{
}


int MainWindow::Initialisation()
{
	// OpenGL version (usefull for imGUI and other libraries)
	const char* glsl_version = "#version 460 core";

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();

	// Request OpenGL 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);  // For debug callback 
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Position and colors - SSBO", NULL, NULL);
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

	// Configure the error handling
	int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		std::cout << "Debug context created\n";
		// initialize debug output 
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, NULL, GL_TRUE);
	}

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
	// For other callbacks:
	// https://www.glfw.org/docs/3.3/input_guide.html
}

int MainWindow::InitializeGL()
{	
	// Shader configuration
	const std::string directory = SHADERS_DIR;
	bool mainShaderSuccess = true;
	m_mainShader = std::make_unique<ShaderProgram>();
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "triangles.vert");
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "triangles.frag");
	mainShaderSuccess &= m_mainShader->link();
	if (!mainShaderSuccess) {
		std::cerr << "Error when loading main shader\n";
		return 4;
	}

	// Position of 3 triangles
	struct Vertex {
		glm::vec3 position;
		glm::vec4 color;
	};
	std::vector<Vertex> vertices = {
		{glm::vec3(-0.9f,  0.1f, 0.0f), glm::vec4(0, 1, 1, 1)}, 
		{glm::vec3(0.0f,  0.9f, 0.0f), glm::vec4(1, 0, 1, 1)},
		{glm::vec3(0.9f,  0.1f, 0.0f), glm::vec4(1, 1, 0, 1)},
		
	};

	// Create VAO and VBO
	glCreateVertexArrays(NumVAOs, m_VAOs); // Create empty VAO
	glCreateBuffers(NumBuffers, m_buffers);

	// Allocations
	glNamedBufferData(m_buffers[MainBuffer], 
		long(sizeof(Vertex) * vertices.size()), // Taille des données à transférer
		vertices.data(),  // Données à transférer
		GL_STATIC_DRAW // Utilisation des données (modification fréquente ou non)
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[MainBuffer]);
	
	// Background color
	glClearColor(0.5f, 0.5f, 0.5f, 1.0);

	return 0;
}

void MainWindow::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT);
	// Note: Need a VAO to render (even if we are not using it)
	glBindVertexArray(m_VAOs[Triangles]);
	m_mainShader->bind();

	// First triangle, vertex color
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glFlush();
}

int MainWindow::RenderLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		// Check inputs: Does ESC was pressed?
		if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(m_window, true);

		RenderScene();

		// Show rendering and get events
		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}

	// Cleanup
	glfwDestroyWindow(m_window);
	glfwTerminate();

	return 0;
}

void MainWindow::FramebufferSizeCallback(int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
