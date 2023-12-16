#include<iostream>
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<glshaderloader.h>
#include<vmath.h>
#include<vector>
#include"imgui/imgui.h"
#include"imgui/imgui_impl_glfw.h"
#include"imgui/imgui_impl_opengl3.h"

glprogram_dl renderer, marchingSquares, metaball, gridRender, aabbRender;

GLuint texScalarDataField;
GLuint texCellType;
GLuint ssboSpheres;
GLuint vaoDummy;

std::vector<vmath::vec4> sphereArray;
std::vector<vmath::vec2> sphereVelocities;

vmath::vec2 windowCoord;
int selected = -1;
bool isPaused = true;
bool isSelectedBeingDragged = false;
bool isGridPoint = false;
bool isGridPointLinear = false;
bool isLerp = false;
vmath::mat4 mMat;
vmath::mat4 pMat;
vmath::vec2 pressedCoord;
float isolevel = 1.5f;
int gridDimensions = 400;
vmath::vec3 bgcolor = vmath::vec3(0.5f, 0.7f, 0.3f);

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) {
	fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message );
}

void init(void) {
	assert(glewInit() == GLEW_OK);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	sphereArray.push_back({0.05f, 0.0f, 0.0f, 0.1f}),
	sphereArray.push_back({-0.05f, 0.0f, 0.0f, 0.1f});

	vmath::vec2 velocity1 = {1.0f, 0.6f};
	vmath::normalize(velocity1);
	velocity1 *= 0.5f;
	sphereVelocities.push_back(velocity1);
	velocity1 = {0.3f, -0.8f};
	vmath::normalize(velocity1);
	velocity1 *= 0.5f;
	sphereVelocities.push_back(velocity1);
	
	glshader_dl renderVert, renderGeom, renderFrag, gridVert, gridFrag, marchComp, metaballComp, aabbVert, aabbFrag;

	std::cout<<glshaderCreate(&renderVert, GL_VERTEX_SHADER, "shader/render.vert");
	std::cout<<glshaderCreate(&renderGeom, GL_GEOMETRY_SHADER, "shader/render.geom");
	std::cout<<glshaderCreate(&renderFrag, GL_FRAGMENT_SHADER, "shader/render.frag");
	std::cout<<glshaderCreate(&marchComp, GL_COMPUTE_SHADER, "shader/marching_square.comp");
	std::cout<<glshaderCreate(&metaballComp, GL_COMPUTE_SHADER, "shader/metaball.comp");
	std::cout<<glshaderCreate(&gridVert, GL_VERTEX_SHADER, "shader/renderGrid.vert");
	std::cout<<glshaderCreate(&gridFrag, GL_FRAGMENT_SHADER, "shader/renderGrid.frag");
	std::cout<<glshaderCreate(&aabbVert, GL_VERTEX_SHADER, "shader/renderAabb.vert");
	std::cout<<glshaderCreate(&aabbFrag, GL_FRAGMENT_SHADER, "shader/renderAabb.frag");

	std::cout<<glprogramCreate(&renderer, "Render", {renderVert, renderGeom, renderFrag});
	std::cout<<glprogramCreate(&gridRender, "GridRender", {gridVert, gridFrag});
	std::cout<<glprogramCreate(&marchingSquares, "MarchingSquares", {marchComp});
	std::cout<<glprogramCreate(&metaball, "Metaball", {metaballComp});
	std::cout<<glprogramCreate(&aabbRender, "AABBRender", {aabbVert, aabbFrag});

	glCreateTextures(GL_TEXTURE_2D, 1, &texScalarDataField);
	glTextureStorage2D(texScalarDataField, 1, GL_R32F, gridDimensions, gridDimensions);

	glCreateTextures(GL_TEXTURE_2D, 1, &texCellType);
	glTextureStorage2D(texCellType, 1, GL_R8UI, gridDimensions - 1, gridDimensions - 1);

	glCreateBuffers(1, &ssboSpheres);
	glNamedBufferStorage(ssboSpheres, sizeof(vmath::vec4) * sphereArray.size(), sphereArray.data(), GL_DYNAMIC_STORAGE_BIT);

	glCreateVertexArrays(1, &vaoDummy);

	glPointSize(2.0f);
	//glEnable(GL_POINT_SMOOTH);

	glLineWidth(2.0f);
}

void updateSpherePosition(vmath::vec4 &sphere, vmath::vec2 &velocity, double deltaTime) {
	sphere[0] += velocity[0] * deltaTime;
	sphere[1] += velocity[1] * deltaTime;

	if(abs(sphere[0]) > 1.0f) {
		velocity[0] = -velocity[0];
	} else if(abs(sphere[1]) > 1.0f) {
		velocity[1] = -velocity[1];
	}

	sphere[0] = vmath::clamp(vmath::vec1(sphere[0]), vmath::vec1(-1), vmath::vec1(1))[0];
	sphere[1] = vmath::clamp(vmath::vec1(sphere[1]), vmath::vec1(-1), vmath::vec1(1))[0];
}

vmath::vec2 convertWindowToWorldCoords(const vmath::vec2& coord, const vmath::mat4& pMat, const vmath::mat4& mvMat, float winWidth, float winHeight) {
	vmath::vec2 ndc = vmath::vec2(coord[0] / winWidth, 1.0f - (coord[1] / winHeight)) * 2.0f - 1.0f;
	vmath::mat4 imvpMat = vmath::inverse(pMat * mvMat);
	vmath::vec4 v = vmath::vec4(ndc, 0.0f, 1.0f);
	vmath::vec4 opv = imvpMat * v;
	return vmath::vec2(opv[0], opv[1]);
}

int getSelectedSphere(vmath::vec2 coord) {
	int i = 0;
	for(auto spherePos : sphereArray) {
		float dx = coord[0] - spherePos[0];
		float px = spherePos[3] - abs(dx);
		float dy = coord[1] - spherePos[1];
		float py = spherePos[3] - abs(dy);
		if (px > 0 && py > 0) {
			return i;
		}
		i++;		
	}
	return -1;
}

void addSphere() {
	sphereArray.push_back({0.0f, 0.0f, 0.0f, 0.1f});
	vmath::vec2 v = {static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f, static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f};
	sphereVelocities.push_back(vmath::normalize(v) * 0.5f);
	glDeleteBuffers(1, &ssboSpheres);
	glCreateBuffers(1, &ssboSpheres);
	glNamedBufferStorage(ssboSpheres, sizeof(vmath::vec4) * sphereArray.size(), sphereArray.data(), GL_DYNAMIC_STORAGE_BIT);
	if(selected != -1) {
		selected = sphereArray.size() - 1;
	}
}

void removeSphere(int toDelete) {
	if(toDelete == -1) {
		sphereArray.pop_back();
		sphereVelocities.pop_back();
		return;
	}
	sphereArray.erase(sphereArray.begin() + toDelete);
	sphereVelocities.erase(sphereVelocities.begin() + toDelete);
	glDeleteBuffers(1, &ssboSpheres);
	glCreateBuffers(1, &ssboSpheres);
	glNamedBufferStorage(ssboSpheres, sizeof(vmath::vec4) * sphereArray.size(), sphereArray.data(), GL_DYNAMIC_STORAGE_BIT);
	selected = -1;
}

void drawImGuiComp() {
	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::ColorPicker3("Background Color", &bgcolor[0], ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoPicker);
	ImGui::BeginDisabled(selected == -1);
	ImGui::SliderFloat("Radius", &sphereArray[selected][3], 0.1f, 0.8f);
	ImGui::EndDisabled();
	ImGui::BeginDisabled(sphereArray.size() >= 7);
	if(ImGui::Button("Add")) {
		addSphere();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(sphereArray.size() <= 1);
	if(ImGui::Button("Remove")) {
		removeSphere(selected);
	}
	ImGui::EndDisabled();
	if(ImGui::SliderInt("Grid Dimensions", &gridDimensions, 20, 600)) {
		glDeleteTextures(1, &texScalarDataField);
		glCreateTextures(GL_TEXTURE_2D, 1, &texScalarDataField);
		glTextureStorage2D(texScalarDataField, 1, GL_R32F, gridDimensions, gridDimensions);
		glDeleteTextures(1, &texCellType);
		glCreateTextures(GL_TEXTURE_2D, 1, &texCellType);
		glTextureStorage2D(texCellType, 1, GL_R8UI, gridDimensions - 1, gridDimensions - 1);
	}
	ImGui::SliderFloat("Isolevel", &isolevel, 0.1f, 4.0f);
	ImGui::Checkbox("Use Lerp", &isLerp);
	ImGui::Checkbox("Show Grid Points", &isGridPoint);
	ImGui::BeginDisabled(!isGridPoint);
	ImGui::Checkbox("Use Linear Coloring for Grid Points", &isGridPointLinear);
	ImGui::EndDisabled();
	if(ImGui::Button("Play / Pause")) {
		isPaused = !isPaused;
	}
	ImGui::Text("Press Esc to Exit");
	ImGui::End();
}

void render(GLFWwindow* window, double currentTime) {
	int width, height, hover;

	static double lastTime = 0.0;

	double deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	if(!isPaused) {
		for(int i = 0; i < sphereArray.size(); i++) {
			if(!(selected == i && isSelectedBeingDragged)) {
				updateSpherePosition(sphereArray[i], sphereVelocities[i], deltaTime);
			}
		}
	}
	
	glfwGetWindowSize(window, &width, &height);
	pMat = vmath::ortho(-1.0f * static_cast<float>(width) / static_cast<float>(height), 1.0f * static_cast<float>(width) / static_cast<float>(height), -1.0f, 1.0f, -1.0f, 1.0f);
	mMat = vmath::translate(0.5f, 0.0f, 0.0f) * vmath::scale(0.9f);
	hover = getSelectedSphere(convertWindowToWorldCoords(windowCoord, pMat, mMat, width, height));

	vmath::mat4 mvpMat = pMat * mMat;
	
	glClearBufferfv(GL_COLOR, 0, vmath::vec4(bgcolor, 1.0f));

	glViewport(0, 0, width, height);

	glUseProgram(metaball.programObject);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboSpheres);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vmath::vec4) * sphereArray.size(), sphereArray.data());
	glBindImageTexture(0, texScalarDataField, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
	glUniform1i(0, sphereArray.size());

	glDispatchCompute(gridDimensions, gridDimensions, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(marchingSquares.programObject);
	glBindImageTexture(0, texScalarDataField, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, texCellType, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
	glUniform1f(0, isolevel);

	glDispatchCompute(gridDimensions - 1, gridDimensions - 1, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindVertexArray(vaoDummy);

	//Grid Points
	if(isGridPoint) {
		glUseProgram(gridRender.programObject);
		glUniform2i(0, gridDimensions, gridDimensions);
		glUniformMatrix4fv(1, 1, GL_FALSE, mvpMat);
		glUniform1i(2, isGridPointLinear);
		glUniform1f(3, isolevel);
		glDrawArrays(GL_POINTS, 0, gridDimensions * gridDimensions);
	}

	//Lines
	glUseProgram(renderer.programObject);
	glUniform2i(0, gridDimensions - 1, gridDimensions - 1);
	glUniformMatrix4fv(1, 1, GL_FALSE, mvpMat);
	glUniform1f(2, isolevel);
	glUniform1i(3, isLerp);
	glBindImageTexture(0, texScalarDataField, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, texCellType, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
	glDrawArrays(GL_POINTS, 0, (gridDimensions - 1) * (gridDimensions - 1));

	//Aabb
	if(hover != -1) {
		glUseProgram(aabbRender.programObject);
		glUniform4fv(0, 1, sphereArray[hover]);
		glUniformMatrix4fv(1, 1, GL_FALSE, mvpMat);
		glUniform3f(2, 1.0f, 0.0f, 0.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
	if(selected != -1) {
		glUseProgram(aabbRender.programObject);
		glUniform4fv(0, 1, sphereArray[selected]);
		glUniformMatrix4fv(1, 1, GL_FALSE, mvpMat);
		glUniform3f(2, 0.0f, 1.0f, 0.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}

	//Border
	glUseProgram(aabbRender.programObject);
	glUniform4f(0, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniformMatrix4fv(1, 1, GL_FALSE, mvpMat);
	glUniform3f(2, 1.0f, 1.0f, 1.0f);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void uninit(void) {

}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch(key) {
	case 'F': case 'f':
		break;
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, 1);
		break;
	}
}

void mousemove(GLFWwindow* window, double xpos, double ypos) {
	isSelectedBeingDragged = false;
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
		windowCoord = vmath::vec2(xpos, ypos);
		return;
	}
	if(selected != -1 && !ImGui::GetIO().WantCaptureMouse) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		vmath::vec2 coords = convertWindowToWorldCoords(vmath::vec2(xpos, ypos), pMat, mMat, w, h);
		vmath::vec2 difference = coords - pressedCoord;
		sphereArray[selected][0] += difference[0];
		sphereArray[selected][1] += difference[1];
		pressedCoord = coords;
		isSelectedBeingDragged = true;
	}
}

void mouseclick(GLFWwindow* window, int button, int action, int mod) {
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
		int w, h;
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetWindowSize(window, &w, &h);
		selected = getSelectedSphere(convertWindowToWorldCoords(vmath::vec2(xpos, ypos), pMat, mMat, w, h));
		pressedCoord = convertWindowToWorldCoords(vmath::vec2(xpos, ypos), pMat, mMat, w, h);
	}
	isSelectedBeingDragged = false;
}

void resize(GLFWwindow* window, int width, int height) {
}

int main(void) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1, 1, "Metaballs", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, keyboard);
	glfwSetCursorPosCallback(window, mousemove);
	glfwSetMouseButtonCallback(window, mouseclick);
	glfwSetWindowSizeCallback(window, resize);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.FontGlobalScale = 1.2;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460 core");
	init();
	const GLFWvidmode *vidmode;
	vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowSize(window, vidmode->width, vidmode->height);
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();	
		drawImGuiComp();
		render(window, glfwGetTime());
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}
	uninit();
	glfwDestroyWindow(window);
	glfwTerminate();
}
