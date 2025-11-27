#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <memory>

#include "ShaderProgram.h"

class MainWindow
{
public:
	MainWindow();

	// Main functions (initialization, run)
	int Initialisation();
	int RenderLoop();

	// Callback to intersept GLFW calls
	void FramebufferSizeCallback(int width, int height);

private:
	// Initialize GLFW callbacks
	void InitializeCallback();
	// Intiialize OpenGL objects (shaders, ...)
	int InitializeGL();

	// Rendering scene (OpenGL)
	void RenderScene();
	// Rendering interface ImGUI
	void RenderImgui();
	// Rendering Geometry
	void RenderGeometry(ShaderProgram& prog, bool useColor, bool adjency);

	// Geometry
	int InitGeometryCube();
	
	// Animation light position
	void UpdateLightPosition(float delta_time);

private:
	// settings
	const unsigned int SCR_WIDTH = 1200;
	const unsigned int SCR_HEIGHT = 800;

	// Camera settings 
	glm::vec3 m_eye, m_at, m_up;
	glm::mat4 m_proj;

	// Main shader
	std::unique_ptr<ShaderProgram> m_mainShader = nullptr;;

	// Shadow map shader
	std::unique_ptr<ShaderProgram> m_volumeShader = nullptr;

	// Light position
	// - For animation
	bool m_lightAnimation = true;
	double m_lightRadius;
	double m_lightAngle;
	double m_lightHeight;
	// - Light position 3d
	glm::vec3 m_lightPosition;


	// Face cube
	static const int NumFacesCube = 6;
	static const int NumTriCube = NumFacesCube * 2;
	static const int NumVerticesCube = 4 * NumFacesCube;
	static const int NumVerticesFloor = 4;

	enum VAO_IDs { CubeVAO, CubeVAOAdjancy, NumVAOs };
	enum VBO_IDs { CubeVBO, CubeEBO, CubeEBOAdj, NumVBOs };

	GLuint m_VAOs[NumVAOs];
	GLuint VBOs[NumVBOs];

	glm::vec4 m_color;
	glm::vec3 m_cubePosition = glm::vec3(0.0, 1.0, 0.0);

	// GLFW Window
	GLFWwindow* m_window = nullptr;
};
