#include "MainWindow.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "OBJLoader.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

MainWindow::MainWindow() :
	m_at(glm::vec3(0, 0,-1)),
	m_up(glm::vec3(0, 1, 0)),
	m_light_position(glm::vec3(0.0, 0.0, 8.0))
{
	updateCameraEye();
}

void MainWindow::FramebufferSizeCallback(int width, int height) {
	m_proj = glm::perspective(45.0f, float(width) / height, 0.01f, 100.0f);
	glViewport(0, 0, width, height);
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
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simple FBO", NULL, NULL);
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
}

// Helper function to configure VBO
inline void configureVBO(int location, int vaoID, int vboID, int nbComp, GLsizei stride) {
	glVertexArrayVertexBuffer(vaoID, location, vboID, 0, stride);
	glVertexArrayAttribFormat(vaoID, location, nbComp, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vaoID, location, location);
	glEnableVertexArrayAttrib(vaoID, location);
}


int MainWindow::InitializeGL()
{
	// Enable the depth test
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader program
	const std::string directory = SHADERS_DIR;
	m_mainShader = std::make_unique<ShaderProgram>();
	bool mainShaderSuccess = true;
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "basicShader.vert");
	mainShaderSuccess &= m_mainShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "basicShader.frag");
	mainShaderSuccess &= m_mainShader->link();
	if (!mainShaderSuccess) {
		std::cerr << "Error when loading main shader\n";
		return 4;
	}

	// 
	m_filterShader = std::make_unique<ShaderProgram>();
	bool filterShaderSuccess = true;
	filterShaderSuccess &= m_filterShader->addShaderFromSource(GL_VERTEX_SHADER, directory + "filter.vert");
	filterShaderSuccess &= m_filterShader->addShaderFromSource(GL_FRAGMENT_SHADER, directory + "filter.frag");
	filterShaderSuccess &= m_filterShader->link();
	if (!filterShaderSuccess) {
		std::cerr << "Error when loading filter shader\n";
		return 4;
	}
	m_filterShader->setInt(m_filterUniforms.iChannel0, 0); // Set unit texture 0

	// Load the 3D model from the obj file
	loadObjFile();
	// Create simple plane
	glCreateVertexArrays(NumVAOs, m_VAOs);
	glCreateBuffers(NumBuffers, m_buffers);
	// -- VBO
	const int NumVertices = 4;
	glm::vec3 Vertices[NumVertices] = {
		  glm::vec3(-1, -1, 0),
		  glm::vec3(1, -1, 0),
		  glm::vec3(-1,  1, 0),
		  glm::vec3(1,  1, 0)
	};
	glm::vec2 Uvs[NumVertices] = {
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 1)
	};

	glNamedBufferData(m_buffers[Position], sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	glNamedBufferData(m_buffers[UV], sizeof(Uvs), Uvs, GL_STATIC_DRAW);
	
	// -- VAO
	int locPos = m_filterShader->attributeLocation("vPosition");
	configureVBO(locPos, m_VAOs[Triangles], m_buffers[Position], 3, sizeof(glm::vec3));
	int locUV = m_filterShader->attributeLocation("vUV");
	configureVBO(locUV, m_VAOs[Triangles], m_buffers[UV], 2, sizeof(glm::vec2));

	// Create FBO
	glCreateFramebuffers(1, &m_fboID);

	// Create texture (simple to store the rendering)
	glCreateTextures(GL_TEXTURE_2D, 1, &m_texID);
	glTextureStorage2D(m_texID, 1, GL_RGB8, SCR_WIDTH, SCR_HEIGHT);
	// Configure the texture
	glTextureParameteri(m_texID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_texID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(m_texID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_texID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Create texture (simple to store the positions)
	glCreateTextures(GL_TEXTURE_2D, 1, &m_texIDPos);
	glTextureStorage2D(m_texIDPos, 1, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT);
	// Configure the texture
	glTextureParameteri(m_texIDPos, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_texIDPos, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(m_texIDPos, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_texIDPos, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Create render buffer (similar to a texture, but only to store temporary data that we will not access it)
	unsigned int rboID;
	glCreateRenderbuffers(1, &rboID);
	glNamedRenderbufferStorage(rboID, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	
	// Attach the texture to the FBO
	glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT0, m_texID, 0);
	glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT1, m_texIDPos, 0);
	glNamedFramebufferRenderbuffer(m_fboID, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboID);

	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glNamedFramebufferDrawBuffers(m_fboID, 2, drawBuffers);
	GLenum status = glCheckNamedFramebufferStatus(m_fboID, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		// Cas d'erreur
		std::cout << "error FBO!\n";
		return 5;
	}


	FramebufferSizeCallback(SCR_WIDTH, SCR_HEIGHT);

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
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Simple FBO");
		ImGui::Checkbox("Active FBO", &m_activeFBO);
		ImGui::Checkbox("Position tex", &m_usePositionTexture);
		ImGui::Checkbox("Kuwahara filter", &m_useFilter);
		if(ImGui::SliderInt("Kernel size", &m_kernelSize, 1, 20)) {
			m_filterShader->setInt(2, m_kernelSize);
		}

		ImGui::Text("Camera settings");
		bool updateCamera = ImGui::SliderFloat("Longitude", &m_longitude, -180.f, 180.f);
		updateCamera |= ImGui::SliderFloat("Latitude", &m_latitude, -89.f, 89.f);
		updateCamera |= ImGui::SliderFloat("Distance", &m_distance, 2.f, 14.0f);
		if (updateCamera) {
			updateCameraEye();
		}

		ImGui::Separator();
		ImGui::Text("Lighting information");
		ImGui::InputFloat3("Position", &m_light_position.x);
		if (ImGui::Button("Copy camera position")) {
			m_light_position = m_eye;
		}

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MainWindow::RenderScene()
{
	if (m_activeFBO) {
		// If true, we will redirect the rendering inside the texture
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	// Clear the frame buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind our vertex/fragment shaders
	m_mainShader->bind();

	// Get projection and camera transformations
	glm::mat4 viewMatrix = glm::lookAt(m_eye, m_at, m_up);
	glm::mat4 modelViewMatrix = glm::scale(glm::translate(viewMatrix, glm::vec3(0, -0.5, 0)), glm::vec3(1.0));

	m_mainShader->setMat4(m_mainUniforms.mvMatrix, modelViewMatrix);
	m_mainShader->setMat4(m_mainUniforms.projMatrix, m_proj);
	m_mainShader->setMat3(m_mainUniforms.normalMatrix, glm::inverseTranspose(glm::mat3(modelViewMatrix)));
	m_mainShader->setVec3(m_mainUniforms.light_position, viewMatrix * glm::vec4(m_light_position, 1.0));
	m_mainShader->setVec3(m_mainUniforms.light_position2, viewMatrix * glm::vec4(glm::vec3(-m_light_position.x, m_light_position.y, m_light_position.z), 1.0));
	m_mainShader->setVec3(m_mainUniforms.light_position2, viewMatrix * glm::vec4(glm::vec3(m_light_position.x, -m_light_position.y, -m_light_position.z), 1.0));


	// Draw the meshes
	for(const MeshGL& m : m_meshesGL)
	{
		// Set its material properties
		m_mainShader->setVec3(m_mainUniforms.Kd, m.diffuse);
		m_mainShader->setVec3(m_mainUniforms.Ks, m.specular);
		m_mainShader->setFloat(m_mainUniforms.Kn, m.specularExponent);

		// Draw the mesh
		glBindVertexArray(m.vao);
		glDrawArrays(GL_TRIANGLES, 0, m.numVertices);
	}

	// Second pass (only if the FBO is activated)
	if (m_activeFBO) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Active the filter shader
		m_filterShader->bind();
		m_filterShader->setBool(m_filterUniforms.useFilter, m_useFilter);
		m_filterShader->setInt(2, m_kernelSize); // Set the number of iterations
		m_filterShader->setVec2(3, glm::vec2(SCR_WIDTH, SCR_HEIGHT)); // Set the size of the texture
		// Active the texture filled by the FBO
		glActiveTexture(GL_TEXTURE0);
		if (m_usePositionTexture) {
			glBindTexture(GL_TEXTURE_2D, m_texIDPos);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, m_texID);
		}
		// Rendering
		glBindVertexArray(m_VAOs[Triangles]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
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

	// Clean memory
	// Delete vaos and vbos
	for (const MeshGL& m : m_meshesGL)
	{
		// Set material properties

		// Draw the mesh
		glDeleteVertexArrays(1, &m.vao);
		glDeleteBuffers(1, &m.vboPosition);
		glDeleteBuffers(1, &m.vboNormal);
	}
	m_meshesGL.clear();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_window);
	glfwTerminate();

	return 0;
}

void MainWindow::updateCameraEye()
{
	m_eye = glm::vec3(0, 0, m_distance);
	glm::mat4 longitude(1), latitude(1);
	latitude= glm::rotate(latitude, glm::radians(m_latitude), glm::vec3(1, 0, 0));
	longitude= glm::rotate(longitude, glm::radians(m_longitude), glm::vec3(0, 1, 0));
	m_eye = longitude * latitude * glm::vec4(m_eye,1);
}

void MainWindow::loadObjFile()
{
	std::string assets_dir = ASSETS_DIR;
	std::string ObjPath = assets_dir + "bunny.obj";
	// Load the obj file
	OBJLoader::Loader loader(ObjPath);

	// Create a GL object for each mesh extracted from the OBJ file
	// Note that if the 3D object have several different material
	// This will create multiple Mesh objects (one for each different material)
	const std::vector<OBJLoader::Mesh>& meshes = loader.getMeshes();
	const std::vector<OBJLoader::Material>& materials = loader.getMaterials();
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		if (meshes[i].vertices.size() == 0)
			continue;

		MeshGL meshGL;
		meshGL.numVertices = meshes[i].vertices.size();

		// Set material properties of the mesh
		const float* Kd = materials[meshes[i].materialID].Kd;
		const float* Ks = materials[meshes[i].materialID].Ks;

		meshGL.diffuse = glm::vec3(Kd[0], Kd[1], Kd[2]);
		meshGL.specular = glm::vec3(Ks[0], Ks[1], Ks[2]);
		meshGL.specularExponent = materials[meshes[i].materialID].Kn;

		// Create its VAO and VBO object
		glCreateVertexArrays(1, &meshGL.vao);
		glCreateBuffers(1, &meshGL.vboPosition);
		glCreateBuffers(1, &meshGL.vboNormal);

		// Make unique vector to store the vertices and normal
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		for (const OBJLoader::Vertex& v : meshes[i].vertices)
		{
			vertices.push_back(glm::vec3(v.position[0], v.position[1], v.position[2]));
			normals.push_back(glm::vec3(v.normal[0], v.normal[1], v.normal[2]));
		}

		// Load the data on the GPU
		glNamedBufferData(meshGL.vboPosition, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glNamedBufferData(meshGL.vboNormal, sizeof(glm::vec3) * normals.size(), normals.data(), GL_STATIC_DRAW);
		// Configure the VAO
		glUseProgram(m_mainShader->programId());
		int PositionLoc = m_mainShader->attributeLocation("vPosition");
		configureVBO(PositionLoc, meshGL.vao, meshGL.vboPosition, 3, sizeof(glm::vec3));
		int NormalLoc = m_mainShader->attributeLocation("vNormal");
		configureVBO(NormalLoc, meshGL.vao, meshGL.vboNormal, 3, sizeof(glm::vec3));

		// Add it to the list
		m_meshesGL.push_back(meshGL);
	}
}
