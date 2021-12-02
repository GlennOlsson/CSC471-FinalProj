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

using namespace std;
using namespace glm;

#define PI 3.1415f
class Application : public EventCallbacks {
   public:
	enum Material { leaves, wood, dirt, stone };

	WindowManager *windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	// Our shader program for textures
	std::shared_ptr<Program> texProg;

	// our geometry
	shared_ptr<Shape> sphere;

	shared_ptr<Shape> theBunny;
	shared_ptr<Shape> bunnyNoNorm;

	shared_ptr<Shape> cube;

	// global data for ground plane - direct load constant defined CPU data to
	// GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	// ground VAO
	GLuint GroundVertexArrayID;

	// the image to use as a texture (ground)
	shared_ptr<Texture> texture0;

	// the image to use as a texture (sky)
	shared_ptr<Texture> sky;

	// global data
	vec3 sphere_min;
	vec3 sphere_max;

	vec3 cube_min;
	vec3 cube_max;

	float gCamH = 0;
	// animation data
	float lightTrans = 2;
	float gTrans = -3;

	float camera_phi = -PI / 4.0f;
	float camera_theta = 0;

	vec3 w_diff = vec3(0);
	vec3 u_diff = vec3(0);

	// Lookat will be calculated before first render dependent on phi and theta
	vec3 lookat;
	vec3 camera_position = vec3(0, gCamH, 0);
	vec3 camera_up_vector = vec3(0, 1, 0);

	// Used to toggle the material with M
	int current_material = 0;

	string resourceDir;

	Spline path;

	bool is_entering = false;

	float speed = 0.025f;

	// Every render, add this to current pos etc.
	// When pressing down key to move, update to contain
	// corresponding value. When releasing, set that to 0
	vec2 movement;

	GLuint cube_texture_buffer;

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

		if (key == GLFW_KEY_G && action == GLFW_PRESS) {
			is_entering = true;
		}

		// Change material
		if (key == GLFW_KEY_M && action == GLFW_PRESS) {
			current_material++;
			current_material %= 4;
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
			glfwGetCursorPos(window, &posX, &posY);
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

	void setFloorTexture() {
		string file = "minecraft.jpg";

		texture0 = make_shared<Texture>();
		texture0->setFilename(resourceDir + "/" + file);
		texture0->init();
		texture0->setUnit(0);
		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
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
		// prog->addAttribute("diffuse_coef");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDir + "/tex_vert.vert",
								resourceDir + "/tex_frag0.frag");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		sky = make_shared<Texture>();
		sky->setFilename(resourceDir + "/sky.png");
		sky->init();
		sky->setUnit(1);
		sky->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		setFloorTexture();

		// Start and end at same point
		path = Spline(camera_position, glm::vec3(20, gCamH, 20),
					  glm::vec3(-20, gCamH, 20), camera_position, 10);
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

		vector<tinyobj::shape_t> TOshapesB;
		vector<tinyobj::material_t> objMaterialsB;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr,
							  (resourceDir + "/cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			cube = make_shared<Shape>();
			cube->createShape(TOshapesB[0]);
			cube->measure();
			cube->init();

			float blocks = 16;

			float block_size = 1.0/blocks;

			//two per vertex, in order
			vector<float> cube_texture_coords = {
				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1,

				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1,

				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1,

				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1,

				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1,

				// 0, 0,
				// 1, 0,
				// 0, 1,
				// 1, 1

				// - NEG Z
				0.5, 0.5,
				0.5f + block_size, 0.5,
				0.5, 0.5f + block_size,
				0.5f + block_size, 0.5f + block_size,

				// TOP (?), POS Y
				0.5f + block_size, 0.5f + block_size,
				0.5f + 2*block_size, 0.5f + block_size,
				0.5f + block_size, 0.5f + 2*block_size,
				0.5f + 2*block_size, 0.5f + 2*block_size,

				// POS Z
				0.5f + 2*block_size, 0.5f + 2*block_size,
				0.5f + 3*block_size, 0.5f + 2*block_size,
				0.5f + 2*block_size, 0.5f + 3*block_size,
				0.5f + 3*block_size, 0.5f + 3*block_size,


				// BOTTOM, NEG Y
				0.5f + 3*block_size, 0.5f + 3*block_size,
				0.5f + 4*block_size, 0.5f + 3*block_size,
				0.5f + 3*block_size, 0.5f + 4*block_size,
				0.5f + 4*block_size, 0.5f + 4*block_size,

				// POS X
				0.5f + 4*block_size, 0.5f + 4*block_size,
				0.5f + 5*block_size, 0.5f + 4*block_size,
				0.5f + 4*block_size, 0.5f + 5*block_size,
				0.5f + 5*block_size, 0.5f + 5*block_size,

				// 0, 0,
				// block_size, 0,
				// 0, block_size,
				// block_size, block_size,

				// NEG X
				0.5f + 5*block_size, 0.5f + 5*block_size,
				0.5f + 6*block_size, 0.5f + 5*block_size,
				0.5f + 5*block_size, 0.5f + 6*block_size,
				0.5f + 6*block_size, 0.5f + 6*block_size,
				
				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,
				// // 1, 0,
				// // 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,


				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,


				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 0, 1,
				// 0, 0,
				// 1, 0,
				// 1, 1,

				// 1, 1,
				// 1, 0,
				// 0, 0,
				// 0, 1,
			};

			cube->setTexBuf(cube_texture_coords);
		}

		cube_min.x = cube->min.x;
		cube_min.y = cube->min.y;
		cube_min.z = cube->min.z;

		cube_max.x = cube->max.x;
		cube_max.y = cube->max.y;
		cube_max.z = cube->max.z;

		// code to load in the ground plane (CPU defined data passed to GPU)

		// directly pass quad for the ground to the GPU

		float g_groundSize = 20;
		float g_groundY = 0;

		// A x-z plane at y = g_groundY of dimension [-g_groundSize,
		// g_groundSize]^2
		float GrndPos[] = {-g_groundSize, g_groundY, -g_groundSize,
						   -g_groundSize, g_groundY, g_groundSize,
						   g_groundSize,  g_groundY, g_groundSize,
						   g_groundSize,  g_groundY, -g_groundSize};

		float GrndNorm[] = {0, 1, 0, 
							0, 1, 0, 
							0, 1, 0,
							0, 1, 0, 
							0, 1, 0, 
							0, 1, 0
		};

		// Texture coords: 0-1, ratio of whole image
		static GLfloat GrndTex[] = {0, 0,  // back
									0, 1, 
									1, 1, 
									1, 0};

		unsigned short idx[] = {0, 1, 2,  //First face
								0, 2, 3}; //Second face

		// generate the ground VAO
		glGenVertexArrays(1, &GroundVertexArrayID);

		g_GiboLen = 6;
		glGenBuffers(1, &GrndBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

		glGenBuffers(1, &GrndNorBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm,
					 GL_STATIC_DRAW);
		
		glGenBuffers(1, &GrndTexBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

		glGenBuffers(1, &GIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	}

	// code to draw the ground plane
	void drawGround(shared_ptr<Program> curS) {
		curS->bind();
		glBindVertexArray(GroundVertexArrayID);
		texture0->bind(curS->getUniform("Texture0"));
		// draw the ground plane
		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// draw!
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		curS->unbind();
	}

	// helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, Material m) {
		float r;
		float g;
		float b;

		float shine;

		// Give r, g, b a value between 0 and 255
		switch (m) {
			case wood:
				r = 106;
				g = 86;
				b = 56;

				shine = 50;
				break;
			case leaves:
				r = 7;
				g = 52;
				b = 0;

				shine = 4;
				break;
			case dirt:
				r = 121;
				g = 85;
				b = 58;

				shine = 1;
				break;
			case stone:
				r = 116;
				g = 116;
				b = 116;

				shine = 10;
				break;
		}

		// Divide by 255 to get number between 0 and 1
		r /= 255.0;
		g /= 255.0;
		b /= 255.0;

		// Assign reasonable rgb values
		glUniform3f(curS->getUniform("MatDif"), r, g, b);
		glUniform3f(curS->getUniform("MatSpec"), 0.5 * r, 0.5 * g, 0.5 * b);
		glUniform3f(curS->getUniform("MatAmb"), 0.3 * r, 0.3 * g, 0.3 * b);
		glUniform1f(curS->getUniform("MatShine"), shine);
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

	void drawBlock(float x, float y, float z, shared_ptr<MatrixStack> Model,
				   Material m) {
		// Cube is one unit in length, from -0.5 to 0.5 all axises
		texProg->bind();
		texture0->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();

		Model->translate(vec3(x - 0.5, y - 0.5, z - 0.5));

		setModel(texProg, Model);

		cube->draw(texProg);

		Model->popMatrix();

		texProg->unbind();
	}

	void drawTree(int x, int z, shared_ptr<MatrixStack> Model) {
		// Draw tree
		// Top leaf
		drawBlock(x, 4, z, Model, Material(current_material));

		// Middle leaves
		drawBlock(x - 1, 3, z, Model, leaves);
		drawBlock(x, 3, z, Model, leaves);
		drawBlock(x + 1, 3, z, Model, leaves);
		drawBlock(x, 3, z + 1, Model, leaves);
		drawBlock(x, 3, z - 1, Model, leaves);

		// Bottom leaves
		drawBlock(x - 1, 2, z, Model, leaves);
		drawBlock(x, 2, z, Model, leaves);
		drawBlock(x + 1, 2, z, Model, leaves);
		drawBlock(x, 2, z + 1, Model, leaves);
		drawBlock(x, 2, z - 1, Model, leaves);

		drawBlock(x + 1, 2, z - 1, Model, leaves);
		drawBlock(x - 1, 2, z - 1, Model, leaves);
		drawBlock(x + 1, 2, z + 1, Model, leaves);
		drawBlock(x - 1, 2, z + 1, Model, leaves);

		// Stem
		drawBlock(x, 1, z, Model, wood);
		drawBlock(x, 0, z, Model, wood);
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

		auto view =
			value_ptr(lookAt(camera_location + w_diff + u_diff,
							 lookat + w_diff + u_diff, camera_up_vector));

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// Multiple use
		vec3 light_position = vec3(2.0, 2.0, lightTrans);

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

		drawTree(1, -8, Model);
		drawTree(10, 7, Model);
		drawTree(-4, 8, Model);
		drawTree(-19, -8, Model);
		drawTree(6, -3, Model);

		// Dirt formation
		drawBlock(6, 0, -10, Model, dirt);
		drawBlock(5, 0, -10, Model, dirt);
		drawBlock(4, 0, -10, Model, dirt);

		drawBlock(6, 0, -11, Model, dirt);
		drawBlock(5, 0, -11, Model, dirt);
		drawBlock(4, 0, -11, Model, dirt);

		drawBlock(6, 1, -11, Model, dirt);
		drawBlock(5, 1, -11, Model, dirt);
		drawBlock(4, 1, -11, Model, dirt);

		// Create rock formationusing quadratic formula

		// minX where y >= 0, i.e. where quadratic equation interesects y plane
		int minX = -2;
		int maxX = 10;
		int midX = minX + (maxX - minX);

		int maxY = 10;

		float k = maxY / pow(minX - midX, 2);

		for (int x = minX; x <= maxX; x++) {
			// f(x) = -k * (x - midX)^2 + maxY
			// Solve knowing f(minX) = 0

			int f_of_x = -k * pow(x - minX, 2) + maxY;

			for (int y = 0; y <= f_of_x; y++) {
				drawBlock(x, y, -13, Model, stone);
			}
		}

		Model->popMatrix();

		// prog->unbind();

		// switch shaders to the texture mapping shader and draw the ground
		texProg->bind();

		// Draw dome (sky)
		Model->pushMatrix();
		Model->scale(vec3(40, 40, 40));
		glUniform1i(texProg->getUniform("flip"), -1);
		sky->bind(texProg->getUniform("Texture0"));
		setModel(texProg, Model);
		sphere->draw(texProg);
		Model->popMatrix();

		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE,
						   value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, view);
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE,
						   value_ptr(Model->topMatrix()));

		drawGround(texProg);

		texProg->unbind();

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
