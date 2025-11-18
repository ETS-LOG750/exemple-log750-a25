#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <memory>

#include "ShaderProgram.h"
#include "Camera.h"

class MainWindow
{
public:
	MainWindow();

	// Main functions (initialization, run)
	int Initialisation();
	int RenderLoop();

	// Callback to intersept GLFW calls
	void FramebufferSizeCallback(int width, int height);
	void CursorPositionCallback(double xpos, double ypos);
private:
	// Initialize GLFW callbacks
	void InitializeCallback();
	// Intiialize OpenGL objects (shaders, ...)
	int InitializeGL();

	// Rendering scene (OpenGL)
	void RenderScene(float time);
	// Rendering interface ImGUI
	void RenderImgui();

	bool loadTexture(const std::string& path, 
		unsigned int& textureID, 
		GLint uvMode, // UV/ST warp mode
		GLint minMode,  // Minfication
		GLint magMode); // Magnification

	void generatePoints();


private:
	// settings
	unsigned int m_windowWidth = 900;
	unsigned int m_windowHeight = 720;

	// Grass shader
	std::unique_ptr<ShaderProgram> m_grassShader = nullptr;
	struct {
		GLint mvMatrix = 0; // matrix model-view
 		GLint projMatrix = 1; // projection matrix
		GLint size = 2; // grass size
		GLint time = 3; // time for wind animation
	} m_uGrass;
	float m_grassSize = 0.5f;
	int m_gridSize = 100;
	float m_density = 0.3f;
	bool m_activeWind = false;

	// Camera
	Camera m_camera;
	bool m_imGuiActive = false;

	// VBO/VAO
	enum VAO_IDs { VAO_Points, NumVAOs };
	enum Buffer_IDs { VBO_Points_Pos, VBO_Points_RandomOrientation, NumBuffers };
	GLuint m_VAOs[NumVAOs];
	GLuint m_buffers[NumBuffers];

	// Texture for the grass
	unsigned int TextureId = -1 ;
	// Texture for the wind
	unsigned int WindTextureId = -1 ;

	// GLFW Window
	GLFWwindow* m_window = nullptr;
};
