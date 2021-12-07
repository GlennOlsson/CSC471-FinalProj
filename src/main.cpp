/*
 * Program 3 base code - includes modifications to shape and initGeom in
 * preparation to load multi shape objects CPE 471 Cal Poly Z. Wood + S. Sueda +
 * I. Dunn
 */

#include <glad/glad.h>
#include <math.h>

#include <iostream>

#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Spline.h"
#include "Texture.h"
#include "WindowManager.h"
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Blocks.h"
#include "ShapeCreeper.h"

using namespace std;
using namespace glm;

#define PI 3.1415f
class Application : public EventCallbacks {
   public:

	WindowManager *windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	// Our shader program for textures
	// std::shared_ptr<Program> texProg;

	// our geometry
	shared_ptr<Shape> sphere;

	shared_ptr<ShapeCreeper> creeper;

	// shared_ptr<Shape> cube;

	// global data for ground plane - direct load constant defined CPU data to
	// GPU (not obj)
	// GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	// int g_GiboLen;
	// ground VAO
	// GLuint GroundVertexArrayID;

	shared_ptr<Texture> creeper_texture;

	// the image to use as a texture (sky)
	shared_ptr<Texture> sky;

	// global data
	vec3 sphere_min;
	vec3 sphere_max;

	// vec3 cube_min;
	// vec3 cube_max;

	// animation data
	float lightTrans = 2;
	float gTrans = -3;

	float camera_phi = -PI / 4.0f;
	float camera_theta = 0;

	vec3 w_diff = vec3(0);
	vec3 u_diff = vec3(0);

	float height = 0;

	// Lookat will be calculated before first render dependent on phi and theta
	vec3 lookat;
	vec3 camera_position = vec3(0, height, 0);
	vec3 camera_up_vector = vec3(0, 1, 0);

	string resourceDir;

	Spline path;

	bool is_entering = false;

	float speed = 0.015f;

	// Every render, add this to current pos etc.
	// When pressing down key to move, update to contain
	// corresponding value. When releasing, set that to 0
	vec2 movement;

	// GLuint cube_texture_buffer;

	Blocks* blocks;

	void calculateDiff() {
		vec3 gaze = lookat - camera_position;

		vec3 w_vec = normalize(gaze);
		vec3 u_vec = cross(camera_up_vector, w_vec);

		// Don't alow character to move around in y-space freely, only in xz
		// plane
		u_vec.y = 0;
		w_vec.y = 0;

		u_diff += movement.x * u_vec;
		w_diff += movement.y * w_vec;
	}

	void keyCallback(GLFWwindow *window, int key, int scancode, int action,
					 int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			lightTrans += 0.25;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS) {
			lightTrans -= 0.25;
		}

		// u ≈ x, w ≈ z
		if (key == GLFW_KEY_W) {
			if (action == GLFW_PRESS)
				movement = vec2(movement.x, speed);
			else if (action == GLFW_RELEASE)
				movement = vec2(movement.x, 0);
		}
		if (key == GLFW_KEY_S) {
			if (action == GLFW_PRESS)
				movement = vec2(movement.x, -speed);
			else if (action == GLFW_RELEASE)
				movement = vec2(movement.x, 0);
		}

		if (key == GLFW_KEY_A) {
			if (action == GLFW_PRESS)
				movement = vec2(speed, movement.y);
			else if (action == GLFW_RELEASE)
				movement = vec2(0, movement.y);
		}
		if (key == GLFW_KEY_D) {
			if (action == GLFW_PRESS)
				movement = vec2(-speed, movement.y);
			else if (action == GLFW_RELEASE)
				movement = vec2(0, movement.y);
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			vec3 lkat = lookat + w_diff + u_diff;
			blocks->addBlock(stone, lkat);
		}

		if (key == GLFW_KEY_G && action == GLFW_PRESS) {
			is_entering = true;
		}

		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
		double posX, posY;

		if (action == GLFW_PRESS) {
			vec3 lkat = lookat + w_diff + u_diff;
			blocks->removeAt(lookat + w_diff + u_diff);
			// glfwGetCursorPos(window, &posX, &posY);
		}
	}

	void calculateLookat() {
		float lookat_x = cos(camera_phi) * cos(camera_theta);
		float lookat_y = sin(camera_theta);
		float lookat_z = cos(camera_theta) * cos((PI / 2.0f) - camera_phi);

		lookat = vec3(lookat_x, lookat_y, lookat_z);
	}

	void scrollCallback(GLFWwindow *window, double deltaX, double deltaY) {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		float theta = deltaY * PI / height;
		float phi = deltaX * PI / width;

		camera_phi += phi;
		camera_theta += theta;

		float limit_yaw = PI * 4.0f / 9.0f;

		camera_theta = camera_theta < -limit_yaw ? -limit_yaw : camera_theta;
		camera_theta = camera_theta > limit_yaw ? limit_yaw : camera_theta;

		calculateLookat();
	}

	void resizeCallback(GLFWwindow *window, int width, int height) {
		glViewport(0, 0, width, height);
	}

	void setupTree(int x, int y, int z) {
		int stemHeight = 3;
		for(int height = y; height < (y + stemHeight); ++height) {
			blocks->addBlock(wood, x, height, z);
		}

		blocks->addBlock(leave, x + 1,  y + stemHeight, z);
		blocks->addBlock(leave, x - 1,  y + stemHeight, z);
		blocks->addBlock(leave, x,  y + stemHeight, z + 1);
		blocks->addBlock(leave, x,  y + stemHeight, z - 1);

		blocks->addBlock(leave, x + 1,  y + stemHeight + 1, z);
		blocks->addBlock(leave, x - 1,  y + stemHeight + 1, z);
		blocks->addBlock(leave, x,  y + stemHeight + 1, z + 1);
		blocks->addBlock(leave, x,  y + stemHeight + 1, z - 1);

		blocks->addBlock(leave, x,  y + stemHeight + 2, z);

	}

	void setupBlocks() {
		//Create ground
		for(int i = -10; i < 10; ++i) {
			for(int j = -10; j < 10; ++j) {
				blocks->addBlock(grass, i, -1, j);
			}
		}

		setupTree(-2, 0, 0);

		setupTree(-8, 0, 4);

		setupTree(2, 0, 6);

		blocks->addBlock(grass, 3, 0, 3);
		blocks->addBlock(grass, 3, 0, 4);
		blocks->addBlock(grass, 3, 0, 5);

		blocks->addBlock(grass, 4, 0, 3);
		blocks->addBlock(grass, 4, 0, 4);
		blocks->addBlock(grass, 4, 0, 5);

		blocks->addBlock(grass, 5, 0, 3);
		blocks->addBlock(grass, 5, 0, 4);
		blocks->addBlock(grass, 5, 0, 5);


		setupTree(4, 0, 4);

	}

	void init() {
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDir + "/simple_vert.vert",
							 resourceDir + "/simple_frag.frag");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("light_intensity");

		blocks = new Blocks();

		setupBlocks();

		sky = make_shared<Texture>();
		sky->setFilename(resourceDir + "/sky.png");
		sky->init();
		sky->setUnit(1);
		sky->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		creeper_texture = make_shared<Texture>();
		creeper_texture->setFilename(resourceDir + "/creeper.jpg");
		creeper_texture->init();
		creeper_texture->setUnit(1);
		creeper_texture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// Start and end at same point
		path = Spline(camera_position, glm::vec3(20, height, 20),
					  glm::vec3(-20, height, 20), camera_position, 10);
	}

	void initGeom() {
		// EXAMPLE set up to read one shape from one obj file - convert to read
		// several
		// Initialize mesh
		// Load geometry
		// Some obj files contain material information.We'll ignore them for
		// this assignment.
		vector<tinyobj::shape_t> TOshapes;
		vector<tinyobj::material_t> objMaterials;
		string errStr;
		// load in the mesh and make the shape(s)
		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
								   (resourceDir + "/SmoothSphere.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapes[0]);
			sphere->measure();
			sphere->init();
		}
		// Save min and max data of sphere
		sphere_min.x = sphere->min.x;
		sphere_min.y = sphere->min.y;
		sphere_min.z = sphere->min.z;

		sphere_max.x = sphere->max.x;
		sphere_max.y = sphere->max.y;
		sphere_max.z = sphere->max.z;

		vector<tinyobj::shape_t> TOshapes2;
		vector<tinyobj::material_t> objMaterials2;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes2, objMaterials2, errStr,
								   (resourceDir + "/dummy.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			creeper = make_shared<ShapeCreeper>(TOshapes2, blocks->texProg);
		}
	}

	/* helper function to set model trasnforms */
	void SetModel(vec3 trans, float rotY, float rotX, float sc,
				  shared_ptr<Program> curS) {
		mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
		mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
		mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
		mat4 ctm = Trans * RotX * RotY * ScaleS;
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void setModel(std::shared_ptr<Program> prog,
				  std::shared_ptr<MatrixStack> M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,
						   value_ptr(M->topMatrix()));
	}

	void render(float frametime) {
		calculateDiff();

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the matrix stack for Lab 6
		float aspect = width / (float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		if (path.isDone())
			is_entering = false;
		else if (is_entering)
			path.update(frametime);

		vec3 camera_location = path.getPosition();

		vec3 location = camera_location + w_diff + u_diff;

		// cout << "LOC;: " << location[0] << ", " << location[1] << ",  " << location[2] << endl;
		location[1] = this->height;

		vec3 lookat_pt = lookat + w_diff + u_diff;
		lookat_pt[1] += this->height;

		auto view =
			value_ptr(lookAt(location, lookat_pt , camera_up_vector));
		
		vec3 block_under(location[0], location[1] - 1, location[2]);
		if(!blocks->hasBlockAt(vec3(block_under))) {
			this->height -= 9.81f * (1.0f/120.0f);
			if(this->height < -10) {
				this->height = 2;
			}
		}

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// Multiple use
		vec3 light_position = vec3(2.0, 2.0, lightTrans);
		// vec3 light_position = vec3(0);

		// Draw the scene
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE,
						   value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, view);
		glUniform3f(prog->getUniform("lightPos"), light_position.x,
					light_position.y, light_position.z);

		glUniform1f(prog->getUniform("light_intensity"), 1);

		Model->pushMatrix();
		// Draw light source
		Model->pushMatrix();
		{
			// Find middle of sphere. Center at origin, scale, then move to
			// light source
			float midX = sphere_min.x + (sphere_max.x - sphere_min.x) / 2.0f;
			float midY = sphere_min.y + (sphere_max.y - sphere_min.y) / 2.0f;
			float midZ = sphere_min.z + (sphere_max.z - sphere_min.z) / 2.0f;

			Model->translate(light_position);

			Model->scale(vec3(0.1));

			Model->translate(vec3(-midX, -midY, -midZ));

			setModel(prog, Model);

			sphere->draw(prog);
		}
		Model->popMatrix();

		prog->unbind();

		blocks->texProg->bind();

		creeper_texture->bind(blocks->texProg->getUniform("Texture0"));

		creeper->initPlacement(Model);

		// for(auto shape: creeper) {
		// 	shape->draw(blocks->texProg);
		// }

		// creeper[curr_shape]->draw(blocks->texProg);

		blocks->texProg->unbind();

		// Draw all blocks
		blocks->drawBlocks(Model);

		Model->popMatrix();

		//Draw scene for texture
		blocks->texProg->bind();
		
		glUniformMatrix4fv(blocks->texProg->getUniform("P"), 1, GL_FALSE,
						   value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(blocks->texProg->getUniform("V"), 1, GL_FALSE, view);
		glUniformMatrix4fv(blocks->texProg->getUniform("M"), 1, GL_FALSE,
						   value_ptr(Model->topMatrix()));
		blocks->texProg->unbind();

		// Pop matrix stacks.
		Projection->popMatrix();
	}
};

int main(int argc, char *argv[]) {
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2) {
		resourceDir = argv[1];
	}

	Application *application = new Application();

	application->resourceDir = resourceDir;

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init();
	application->initGeom();

	application->calculateLookat();

	auto last_time = chrono::high_resolution_clock::now();

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle())) {
		// Render scene.

		auto now = chrono::high_resolution_clock::now();

		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(now - last_time)
				.count();

		deltaTime *= 0.000001;

		last_time = now;

		application->render(deltaTime);

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
