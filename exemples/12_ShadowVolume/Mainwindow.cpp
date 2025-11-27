#include "MainWindow.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <map>
#include <vector>
#include <array>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#ifndef M_PI
#define M_PI (3.14159)
#endif

MainWindow::MainWindow() :
	m_eye(glm::vec3(-2, 4, 8)),
	m_at(glm::vec3(0, 0, -1)),
	m_up(glm::vec3(0, 1, 0)),
	m_lightPosition(0, 0, 0),
	m_lightRadius(4.0),
	m_lightAngle(0.0),
	m_lightHeight(4.0)
{
	UpdateLightPosition(0);
}

void MainWindow::FramebufferSizeCallback(int width, int height) {
	m_proj = glm::perspective(45.0f, float(width) / height, 0.01f, 100.0f);
}

int MainWindow::Initialisation()
{
	// OpenGL version (usefull for imGUI and other libraries)
	const char* glsl_version = "#version 430 core";

	// glfw: initialize and configure
    // ------------------------------
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_STENCIL_BITS, 8); // We need stencil buffer (in case not default)

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shadow Volume", NULL, NULL);
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // Needed for 3rd pass

	// Create VAOs and VBOs
	glGenVertexArrays(NumVAOs, m_VAOs);
	glGenBuffers(NumVBOs, VBOs);

	// build and compile our shader program
	const std::string directory = SHADERS_DIR;

	m_mainShader = std::make_unique<ShaderProgram>();
	bool mainShaderSuccess = true;
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "triangles.vert");
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "triangles.frag");
	mainShaderSuccess &= m_mainShader->link();
	if (!mainShaderSuccess) {
		std::cerr << "Error when loading main shader\n";
		return 4;
	}

	m_volumeShader = std::make_unique<ShaderProgram>();
	bool shadowMapShaderSuccess = true;
	shadowMapShaderSuccess &= m_volumeShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "volume.vert");
	shadowMapShaderSuccess &= m_volumeShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "volume.frag");
	shadowMapShaderSuccess &= m_volumeShader->addShaderFromSource(GL_GEOMETRY_SHADER, directory + "volume.geom");
	shadowMapShaderSuccess &= m_volumeShader->link();
	if (!shadowMapShaderSuccess) {
		std::cerr << "Error when loading shadow map shader\n";
		return 4;
	}

	// Check if locations for positions matches
	// so we can use a single VAO
	int locP1 = m_mainShader->attributeLocation("vPosition");
	int locP2 = m_volumeShader->attributeLocation("vPosition");
	bool posLocEqual = (locP1 == locP2);
	if (!posLocEqual) {
		std::cerr << "Location of vPosition between shaders mismatch" << std::endl;
		std::cerr << "Main shader           : " << locP1 << std::endl;
		std::cerr << "Shadow map gen. shader: " << locP2 << std::endl;
		return 10;
	}

	// Initialize the geometry
	int GeometryCubeReturn = InitGeometryCube();
	if (GeometryCubeReturn != 0)
	{
		return GeometryCubeReturn;
	}

	// Initialize camera... etc
	FramebufferSizeCallback(SCR_WIDTH, SCR_HEIGHT);

	return 0;
}

void MainWindow::RenderGeometry(ShaderProgram& prog, bool useColor, bool adjency) {
	glm::mat4 lookAt = glm::lookAt(m_eye, m_at, m_up);
	
	// Draw RED cube
	{
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1), m_cubePosition);   // translate up by 1.0
		glm::mat4 modelViewMatrix = lookAt * modelMatrix;
		glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat4(modelViewMatrix));
		if (useColor)
			prog.setVec4(4, glm::vec4(1.0, 0.0, 0.0, 1.0));
		prog.setMat4(0, modelViewMatrix);
		prog.setMat3(2, normalMatrix);

		if (adjency) {
			glBindVertexArray(m_VAOs[CubeVAOAdjancy]);
			glDrawElements(GL_TRIANGLES_ADJACENCY, 6 * NumTriCube, GL_UNSIGNED_INT, 0);
		}
		else {
			glBindVertexArray(m_VAOs[CubeVAO]);
			glDrawElements(GL_TRIANGLES, 3 * NumTriCube, GL_UNSIGNED_INT, 0);
		}
	}

	// Draw white cube
	{
		glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(10, 0.1, 10));   // translate up by 1.0
		glm::mat4 modelViewMatrix = lookAt * modelMatrix;
		glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat4(modelViewMatrix));
		if (useColor)
			prog.setVec4(4, glm::vec4(0.1, 0.1, 0.1, 1.0));
		prog.setMat4(0, modelViewMatrix);
		prog.setMat3(2, normalMatrix);

		if (adjency) {
			glBindVertexArray(m_VAOs[CubeVAOAdjancy]);
			glDrawElements(GL_TRIANGLES_ADJACENCY, 6 * NumTriCube, GL_UNSIGNED_INT, 0);
		}
		else {
			glBindVertexArray(m_VAOs[CubeVAO]);
			glDrawElements(GL_TRIANGLES, 3 * NumTriCube, GL_UNSIGNED_INT, 0);
	}
	}
}

void MainWindow::RenderScene()
{
	glm::mat4 lookAt = glm::lookAt(m_eye, m_at, m_up);

	// Matrices and lighting informations
	// These informations are not necessary at this stage (but needed at stage 3)
	m_mainShader->bind();
	m_mainShader->setMat4(0, lookAt);
	m_mainShader->setMat4(1, m_proj);
	m_mainShader->setVec3(3, lookAt * glm::vec4(m_lightPosition, 1.0));

	// Pass 1: Depth only
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawBuffer(GL_NONE);
    glEnable(GL_CULL_FACE); // Optional
    glCullFace(GL_BACK);
    RenderGeometry(*m_mainShader, true, false);

    // Pass 2: Stencil - render shadow volumes
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);  
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP); 
    glDisable(GL_CULL_FACE);  // Optional: Must render both faces to detect edges
    m_volumeShader->bind();
    m_volumeShader->setMat4(0, lookAt);
    m_volumeShader->setMat4(5, m_proj);
    m_volumeShader->setVec3(4, glm::vec3(lookAt * glm::vec4(m_lightPosition, 1.0)));
    RenderGeometry(*m_volumeShader, false, true);

    // Pass 3: Final render where stencil == 0
    glEnable(GL_CULL_FACE); // Optional
    glDrawBuffer(GL_BACK);
    glStencilFunc(GL_EQUAL, 0x0, 0xFF);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilOpSeparate(GL_FRONT , GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    m_mainShader->bind();
    RenderGeometry(*m_mainShader, true, false);

}

void MainWindow::RenderImgui()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//imgui 
	{
		ImGui::Begin("Shadow mapping");
		

		ImGui::Separator();
		ImGui::Text("Camera");
		ImGui::Checkbox("Animate", &m_lightAnimation);
		ImGui::InputFloat3("Position Light", &m_lightPosition[0]);


		ImGui::Separator();
		ImGui::Text("Cube");
		ImGui::InputFloat3("Position Cube", &m_cubePosition[0]);

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


int MainWindow::RenderLoop()
{
	float time = float(glfwGetTime());
	while (!glfwWindowShouldClose(m_window))
	{
		float new_time = float(glfwGetTime());
		const float delta_time = new_time - time;
		time = new_time;

		UpdateLightPosition(delta_time);

		// Check inputs: Does ESC was pressed?
		if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(m_window, true);

		RenderScene();
		RenderImgui();

		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}

	glfwDestroyWindow(m_window);
	glfwTerminate();

	return 0;
}

bool compare(glm::vec3 a, glm::vec3 b) {
	if (a.x == b.x) {
		if (a.y == b.y) {
			return a.z < b.z;
		}
		return a.y < b.y;
	}
	return a.x < b.x;
}

// Order free edge
struct Edge {
	glm::vec3 p;
	glm::vec3 e;
	Edge(glm::vec3 p1, glm::vec3 p2) {
		if (p1.x != p2.x) {
			if (p1.x > p2.x)
				std::swap(p1, p2);
			p = p1;
			e = p2 - p1;
			return;
		}
		if (p1.y != p2.y) {
			if (p1.y > p2.y)
				std::swap(p1, p2);
			p = p1;
			e = p2 - p1;
			return;
		}
		if (p1.z > p2.z)
			std::swap(p1, p2);
		p = p1;
		e = p2 - p1;
	}
};
struct CompEdge {
	bool operator()(const Edge& a, const Edge& b) const {
		if (a.p == b.p) {
			return compare(a.e, b.e);
		}
		return compare(a.p, b.p);
	}
};

int MainWindow::InitGeometryCube()
{
	// Create cube vertices and faces
	glm::vec3 VerticesCube[NumVerticesCube] = {
	{ -0.5, -0.5,  0.5 },	// Front face
	{  0.5, -0.5,  0.5 },
	{  0.5,  0.5,  0.5 },
	{ -0.5,  0.5,  0.5 },
	{ -0.5, -0.5, -0.5 },	// Left face
	{ -0.5, -0.5,  0.5 },
	{ -0.5,  0.5,  0.5 },
	{ -0.5,  0.5, -0.5 },
	{  0.5, -0.5,  0.5 },	// Right face
	{  0.5, -0.5, -0.5 },
	{  0.5,  0.5, -0.5 },
	{  0.5,  0.5,  0.5 },
	{  0.5, -0.5, -0.5 },	// Back face
	{ -0.5, -0.5, -0.5 },
	{ -0.5,  0.5, -0.5 },
	{  0.5,  0.5, -0.5 },
	{ -0.5,  0.5,  0.5 },	// Top face
	{  0.5,  0.5,  0.5 },
	{  0.5,  0.5, -0.5 },
	{ -0.5,  0.5, -0.5 },
	{ -0.5, -0.5, -0.5 },	// Bottom face
	{  0.5, -0.5, -0.5 },
	{  0.5, -0.5,  0.5 },
	{ -0.5, -0.5,  0.5 }
	};
	glm::vec3 NormalsCube[NumVerticesCube] = {
	{  0.0,  0.0,  1.0 },	// Front face
	{  0.0,  0.0,  1.0 },
	{  0.0,  0.0,  1.0 },
	{  0.0,  0.0,  1.0 },
	{ -1.0,  0.0,  0.0 },	// Left face
	{ -1.0,  0.0,  0.0 },
	{ -1.0,  0.0,  0.0 },
	{ -1.0,  0.0,  0.0 },
	{  1.0,  0.0,  0.0 },	// Right face
	{  1.0,  0.0,  0.0 },
	{  1.0,  0.0,  0.0 },
	{  1.0,  0.0,  0.0 },
	{  0.0,  0.0, -1.0 },	// Back face
	{  0.0,  0.0, -1.0 },
	{  0.0,  0.0, -1.0 },
	{  0.0,  0.0, -1.0 },
	{  0.0,  1.0,  0.0 },	// Top face
	{  0.0,  1.0,  0.0 },
	{  0.0,  1.0,  0.0 },
	{  0.0,  1.0,  0.0 },
	{  0.0, -1.0,  0.0 },	// Bottom face
	{  0.0, -1.0,  0.0 },
	{  0.0, -1.0,  0.0 },
	{  0.0, -1.0,  0.0 }
	};

	glm::ivec3 IndicesCube[NumTriCube] = {
	{ 0, 2, 3 },	// Front face
	{ 0, 1, 2 },
	{ 4, 6, 7 },	// Left face
	{ 4, 5, 6 },
	{ 8, 10, 11 },	// Right face
	{ 8, 9, 10 },
	{ 12, 14, 15 },	// Back face
	{ 12, 13, 14 },
	{ 16, 18, 19 },	// Top face
	{ 16, 17, 18 },
	{ 20, 22, 23 },	// Bottom face
	{ 20, 21, 22 }
	};

	// Build the map of triangles and shared edges
	std::map<Edge, std::vector<int>, CompEdge> mapEdgeTriangles;
	for (int i = 0; i < NumTriCube; i++) {
		for (int k = 0; k < 3; k++) {
			auto a = VerticesCube[IndicesCube[i][k]];
			auto b = VerticesCube[IndicesCube[i][(k + 1) % 3]];
			auto entry = mapEdgeTriangles.find(Edge(a, b));
			if (entry != mapEdgeTriangles.end()) {
				entry->second.push_back(i);
			}
			else {
				mapEdgeTriangles.insert({ Edge(a, b), {i} });
			}
		}
	}

	std::cout << "Adjancy map: \n";
	for (auto e : mapEdgeTriangles) {
		for (auto t : e.second) {
			std::cout << t << " ";
		}
		std::cout << "\n";
	}

	std::vector<std::array<int, 6>> indicesAdj;
	for (int i = 0; i < NumTriCube; i++) {
		int otherIndices[3] = { -1, -1, -1 };
		for (int k = 0; k < 3; k++) {
			auto a = VerticesCube[IndicesCube[i][k]];
			auto b = VerticesCube[IndicesCube[i][(k + 1) % 3]];
			auto entry = mapEdgeTriangles.find(Edge(a, b));
			int idTri;
			if (entry != mapEdgeTriangles.end()) {
				// Should have only 2 trianles
				idTri = entry->second[0];
				if (idTri == i) {
					idTri = entry->second[1];
				}
			}
			else {
				// Unexcepted!
				std::cerr << "Missing adgency?\n";
				continue;
			}

			// Look at the other index with position
			for (int l = 0; l < 3; l++) {
				glm::vec3 posOther = VerticesCube[IndicesCube[idTri][l]];
				glm::vec3 posA = VerticesCube[IndicesCube[i][k]];
				glm::vec3 posB = VerticesCube[IndicesCube[i][(k + 1) % 3]];
				if (posOther != posA && posOther != posB) {
					otherIndices[k] = IndicesCube[idTri][l];
					break;
				}
			}
		}
		// Construct final indices
		indicesAdj.push_back(
			{
				IndicesCube[i][0],
				otherIndices[0],
				IndicesCube[i][1],
				otherIndices[1],
				IndicesCube[i][2],
				otherIndices[2]
			}
		);
	}

	// Set VAO
	glBindVertexArray(m_VAOs[CubeVAO]);

	// Fill vertex VBO
	GLsizeiptr OffsetVertices = 0;
	GLsizeiptr OffsetNormals = sizeof(VerticesCube);
	GLsizeiptr DataSize = sizeof(VerticesCube) + sizeof(NormalsCube);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[CubeVBO]);
	glBufferData(GL_ARRAY_BUFFER, DataSize, nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, OffsetVertices, sizeof(VerticesCube), VerticesCube);
	glBufferSubData(GL_ARRAY_BUFFER, OffsetNormals, sizeof(NormalsCube), NormalsCube);

	// Setup shader variables
	int locPos = m_mainShader->attributeLocation("vPosition");
	glVertexAttribPointer(locPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(OffsetVertices));
	glEnableVertexAttribArray(locPos);
	int locNormal = m_mainShader->attributeLocation("vNormal");
	glVertexAttribPointer(locNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(OffsetNormals));
	glEnableVertexAttribArray(locNormal);

	// Fill in indices VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOs[CubeEBO]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndicesCube), IndicesCube, GL_STATIC_DRAW);

	glBindVertexArray(m_VAOs[CubeVAOAdjancy]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[CubeVBO]);
	glVertexAttribPointer(locPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(OffsetVertices));
	glEnableVertexAttribArray(locPos);
	glVertexAttribPointer(locNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(OffsetNormals));
	glEnableVertexAttribArray(locNormal);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOs[CubeEBOAdj]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(int)*indicesAdj.size(), indicesAdj.data(), GL_STATIC_DRAW);

	return 0;
}

void MainWindow::UpdateLightPosition(float delta_time)
{
	if (m_lightAnimation) {
		m_lightAngle += delta_time;
		m_lightPosition.x = float(m_lightRadius * cos(m_lightAngle));
		m_lightPosition.z = float(-m_lightRadius * sin(m_lightAngle));
		m_lightPosition.y = float(m_lightHeight);
	}
}