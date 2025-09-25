#include "MainWindow.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>

#include "Teapot.h"

#include <glm/gtc/matrix_transform.hpp>

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

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lighting no camera", NULL, NULL);
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

	// imGui: create interface
	// ---------------------------------------
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
}

int MainWindow::InitializeGL()
{
	const std::string directory = SHADERS_DIR;
	bool success = true;
	m_phongShader = std::make_unique<ShaderProgram>();
	success &= m_phongShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "phong-shading.vert");
	success &= m_phongShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "phong-shading.frag");
	success &= m_phongShader->link();
	if (!success) {
		std::cerr << "Error when loading Phong shader\n";
		return 4;
	}

	m_gouraudShader = std::make_unique<ShaderProgram>();
	success &= m_gouraudShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "gouraud-shading.vert");
	success &= m_gouraudShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "gouraud-shading.frag");
	success &= m_gouraudShader->link();
	if (!success) {
		std::cerr << "Error when loading Gouraud shader\n";
		return 4;
	}

	// Generate buffers ID
	glCreateVertexArrays(NumVAOs, m_VAOs);
	glCreateBuffers(NumBuffers, m_VBOs);
	
	// Create the geometry and upload on the different buffers
	updateGeometry();

	// Setup VAO
	// Note: On peut reutiliser le VAO pour les deux shaders
	//  vu que les locations sont les memes (specifier en GLSL)
	// - VBO Positions
	int PositionLocation = m_phongShader->attributeLocation("vPosition");
	// -- positions
	glVertexArrayAttribFormat(m_VAOs[Triangles], 
		PositionLocation, // Attribute index 
		3, // Number of components
		GL_FLOAT, // Type 
		GL_FALSE, // Normalize 
		0 // Relative offset (first component)
	);
	glVertexArrayVertexBuffer(m_VAOs[Triangles], 
		PositionLocation, // Binding point 
		m_VBOs[Position], // VBO 
		0, // Offset (when the position starts)
		sizeof(glm::vec3) // Stride
	);
	glEnableVertexArrayAttrib(m_VAOs[Triangles], 
		PositionLocation // Attribute index
	);
	glVertexArrayAttribBinding(m_VAOs[Triangles], 
		PositionLocation, // Attribute index
		PositionLocation  // Binding point
	);
	// - VBO Normales
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[Normal]);
	int NormalLocation = m_phongShader->attributeLocation("vNormal");
	glVertexArrayAttribFormat(m_VAOs[Triangles], 
		NormalLocation, // Attribute index 
		3, // Number of components
		GL_FLOAT, // Type 
		GL_TRUE, // Normalize 
		0 // Relative offset (first component)
	);
	glVertexArrayVertexBuffer(m_VAOs[Triangles], 
		NormalLocation, // Binding point 
		m_VBOs[Normal], // VBO 
		0, // Offset (when the position starts)
		sizeof(glm::vec3) // Stride
	);
	glEnableVertexArrayAttrib(m_VAOs[Triangles], 
		NormalLocation // Attribute index
	);
	glVertexArrayAttribBinding(m_VAOs[Triangles], 
		NormalLocation, // Attribute index
		NormalLocation  // Binding point
	);

	// Other configurations
	// - Test de pronfondeur (pour afficher les triangles dans le bon ordre)
	glEnable(GL_DEPTH_TEST); 
	// - Couleur de fond (apres glClear(...))
	glClearColor(1.0, 1.0, 1.0, 1.0);

	return 0;
}

void MainWindow::RenderImgui()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//imgui 
	{
		
		ImGui::Begin("Plane transformation");
		bool needVerticesUpdate = false;
		needVerticesUpdate |= ImGui::SliderFloat("Rotation X", &m_rotationX, -180.f, 180.f);
		needVerticesUpdate |= ImGui::Checkbox("flatNormal", &m_flatNormals);
		needVerticesUpdate |= ImGui::Checkbox("complexGeometry", &m_complexGeometry);
		if (needVerticesUpdate) {
			updateGeometry();
		}

		if (ImGui::Checkbox("showNormals", &m_showNormals)) {
			m_phongShader->setUniformValue(0, m_showNormals);
		}
		ImGui::Checkbox("Phong shading", &m_phongShading);


		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MainWindow::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (m_phongShading) {
		m_phongShader->bind();
	} else {
		m_gouraudShader->bind();
	}
	
	glBindVertexArray(m_VAOs[Triangles]);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());
}


int MainWindow::RenderLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		// Check inputs: Does ESC was pressed?
		if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(m_window, true);

		RenderScene();
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

void MainWindow::FramebufferSizeCallback(int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

glm::vec3 bezierPoint( glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) 
{
    return (1-t)*(1-t)*(1-t)*p0 
        + 3*t*(1-t)*(1-t)*p1
        + 3*t*t*(1-t)*p2
        + t*t*t*p3;
}

glm::vec3 bezierPatch(glm::vec3 p[16], float u, float v) {
	glm::vec3 p0 = bezierPoint(p[0], p[1], p[2], p[3], u);
	glm::vec3 p1 = bezierPoint(p[4], p[5], p[6], p[7], u);
	glm::vec3 p2 = bezierPoint(p[8], p[9], p[10], p[11], u);
	glm::vec3 p3 = bezierPoint(p[12], p[13], p[14], p[15], u);
	return bezierPoint(p0, p1, p2, p3, v);
}

glm::vec3 bezierTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    // Get the derivative of bezierPoint from t
    return 3*(1-t)*(1-t)*(p1 - p0) 
        + 6*t*(1-t)*(p2 - p1)
        + 3*t*t*(p3 - p2);
}

glm::vec3 bezierPatchNormal(glm::vec3 p[16], float u, float v) {
	glm::vec3 p0 = bezierPoint(p[0], p[1], p[2], p[3], u);
	glm::vec3 p1 = bezierPoint(p[4], p[5], p[6], p[7], u);
	glm::vec3 p2 = bezierPoint(p[8], p[9], p[10], p[11], u);
	glm::vec3 p3 = bezierPoint(p[12], p[13], p[14], p[15], u);
	glm::vec3 tangentU = bezierTangent(p0, p1, p2, p3, v);

	glm::vec3 p4 = bezierPoint(p[0], p[4], p[8], p[12], v);
	glm::vec3 p5 = bezierPoint(p[1], p[5], p[9], p[13], v);
	glm::vec3 p6 = bezierPoint(p[2], p[6], p[10], p[14], v);
	glm::vec3 p7 = bezierPoint(p[3], p[7], p[11], p[15], v);
	glm::vec3 tangentV = bezierTangent(p4, p5, p6, p7, u);

	return glm::normalize(glm::cross(tangentU, tangentV));
}

void MainWindow::updateGeometry()
{
	// Clear CPU memory
	m_vertices.clear();
	m_normals.clear();

	// Matrix to rotate the surface
	glm::mat4 latitude(1);
	latitude = glm::rotate(latitude, glm::radians(m_rotationX), glm::vec3(1, 0, 0));
	// Ignorer la transposition 
	glm::mat3 m = glm::transpose(glm::mat3(latitude));
	
	if(m_complexGeometry) {
		// Compute center of the teapot
		glm::vec3 center(0);
		for (unsigned int i = 0; i < 306; i++) {
			center += teapotVertices[i+1];
		}
		center /= 306.f;

		// Utilisation des patchs de bezier
		for (unsigned int npatch = 0; npatch < 16; npatch++) {
			glm::vec3 pos[16];
			for (unsigned int i = 0; i < 16; i++) {
				int index = teapotPatches[npatch][i];
				pos[i] = (teapotVertices[index] - center) * 0.45f;
			}
			// Pour chaque patch, on genere les triangles
			// en forme de grille
			// On utilise les coordonnees u et v pour parcourir
			// la surface parametrique
			for (unsigned int i = 0; i < COUNT; i++) {
				for(unsigned int j = 0; j < COUNT; j++) {
					// Calcul des positions
					float u1 = (float)i / (COUNT - 1);
					float v1 = (float)j / (COUNT - 1);
					float u2 = (float)(i + 1) / (COUNT - 1);
					float v2 = (float)(j + 1) / (COUNT - 1);
					glm::vec3 p1bot = bezierPatch(pos, u1, v1);
					glm::vec3 p2bot = bezierPatch(pos, u2, v1);
					glm::vec3 p1top = bezierPatch(pos, u1, v2);
					glm::vec3 p2top = bezierPatch(pos, u2, v2);

					// Triangle 1
					m_vertices.push_back(m * p1bot);
					m_vertices.push_back(m * p1top);
					m_vertices.push_back(m * p2bot);
					// Triangle 2
					m_vertices.push_back(m * p2bot);
					m_vertices.push_back(m * p1top);
					m_vertices.push_back(m * p2top);

					// Calcul des normales
					// TODO: Voir pourquoi incoherence entre les normales
					if (m_flatNormals) {
						// Calcul des normals avec le produit en croix
						// sur un des triangle
						glm::vec3 e1 = p1top - p1bot;
						glm::vec3 e2 = p2bot - p1bot;
						glm::vec3 n = glm::normalize(glm::cross(e1, e2));
						m_normals.push_back(- m * n);
						m_normals.push_back(- m * n);
						m_normals.push_back(- m * n);
						
						// Triangle 2
						e1 = p1top - p2bot;
						e2 = p2top - p2bot;
						n = glm::normalize(glm::cross(e1, e2));
						m_normals.push_back(- m * n);
						m_normals.push_back(- m * n);
						m_normals.push_back(- m * n);
					}
					else {
						// Utilisation de la derivee de la surface
						// pour calculer les normales
						glm::vec3 n1bot = bezierPatchNormal(pos, u1, v1);
						glm::vec3 n2bot = bezierPatchNormal(pos, u2, v1);
						glm::vec3 n1top = bezierPatchNormal(pos, u1, v2);
						glm::vec3 n2top = bezierPatchNormal(pos, u2, v2);
						// Triangle 1
						m_normals.push_back(- m * n1bot);
						m_normals.push_back(- m * n1top);
						m_normals.push_back(- m * n2bot);
						// Triangle 2
						m_normals.push_back(- m * n2bot);
						m_normals.push_back(- m * n1top);
						m_normals.push_back(- m * n2top);
					}
				}
			}
		}
	} else {
		// Base positions
		const float PI = 3.14159265358979323846f;
		const float offset = PI * 2.f / (float)COUNT;
		for (unsigned int i = 0; i < COUNT; i++) {
			// Calcul des positions
			glm::vec3 p1bot = glm::vec3(
				0.7 * std::cos(offset * i), 
				-0.7, 
				0.7 *  std::sin(offset * i)
			);
			glm::vec3 p1top = glm::vec3(
				0.7 * std::cos(offset * i),
				0.7,
				0.7 * std::sin(offset * i)
			);
			glm::vec3 p2bot = glm::vec3(
				0.7 *  std::cos(offset * (i + 1)), 
				-0.7, 
				0.7 *  std::sin(offset * (i + 1))
			);
			glm::vec3 p2top = glm::vec3(
				0.7 * std::cos(offset * (i + 1)),
				0.7,
				0.7 * std::sin(offset * (i + 1))
			);

			// Triangle 1
			m_vertices.push_back(m * p1top);
			m_vertices.push_back(m * p1bot);
			m_vertices.push_back(m * p2bot);
			// Triangle 2
			m_vertices.push_back(m * p2top);
			m_vertices.push_back(m * p1top);
			m_vertices.push_back(m * p2bot);

			// Calcul des normales
			if (m_flatNormals) {
				// Calcul des normals avec le produit en croix
				// sur un des triangle
				glm::vec3 e1 = p1top - p1bot;
				glm::vec3 e2 = p2bot - p1bot;
				glm::vec3 n = glm::normalize(glm::cross(e1, e2));
				// les normales sont les memes pour les deux triangles
				// On les ajoute 6 fois
				for (int j = 0; j < 6; j++) {
					m_normals.push_back(m * n);
				}
			}
			else {
				// Calcul des normales a chaque sommets
				// Note: p1top et p1bot ont les meme normales
				glm::vec3 n1 = glm::normalize(p1bot - glm::vec3(0, -0.7, 0));
				glm::vec3 n2 = glm::normalize(p2bot - glm::vec3(0, -0.7, 0));
				// Triangle 1
				m_normals.push_back(m * n1);
				m_normals.push_back(m * n1);
				m_normals.push_back(m * n2);
				// Triangle 2
				m_normals.push_back(m * n2);
				m_normals.push_back(m * n1);
				m_normals.push_back(m * n2);
			}
		}
	}
	
	// Add data on the GPU (position)
	glNamedBufferData(m_VBOs[Position], m_vertices.size() * sizeof(glm::vec3), m_vertices.data(), GL_STATIC_DRAW);
	// Add data on the GPU (normals)
	glNamedBufferData(m_VBOs[Normal], m_normals.size() * sizeof(glm::vec3), m_normals.data(), GL_STATIC_DRAW);
}