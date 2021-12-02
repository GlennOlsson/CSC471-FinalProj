
#include "Shape.h"

#include <cassert>
#include <iostream>
#include <unordered_map>

#include "GLSL.h"
#include "Program.h"

using namespace std;
using namespace glm;

// copy the data from the shape to this object
void Shape::createShape(tinyobj::shape_t &shape) {
	posBuf = shape.mesh.positions;
	norBuf = shape.mesh.normals;
	texBuf = shape.mesh.texcoords;
	eleBuf = shape.mesh.indices;
}

void Shape::measure() {
	float minX, minY, minZ;
	float maxX, maxY, maxZ;

	minX = minY = minZ = std::numeric_limits<float>::max();
	maxX = maxY = maxZ = -std::numeric_limits<float>::max();

	// Go through all vertices to determine min and max of each dimension
	for (size_t v = 0; v < posBuf.size() / 3; v++) {
		if (posBuf[3 * v + 0] < minX) minX = posBuf[3 * v + 0];
		if (posBuf[3 * v + 0] > maxX) maxX = posBuf[3 * v + 0];

		if (posBuf[3 * v + 1] < minY) minY = posBuf[3 * v + 1];
		if (posBuf[3 * v + 1] > maxY) maxY = posBuf[3 * v + 1];

		if (posBuf[3 * v + 2] < minZ) minZ = posBuf[3 * v + 2];
		if (posBuf[3 * v + 2] > maxZ) maxZ = posBuf[3 * v + 2];
	}

	min.x = minX;
	min.y = minY;
	min.z = minZ;
	max.x = maxX;
	max.y = maxY;
	max.z = maxZ;
}

void Shape::calculateNormals() {
	//Each index is that vertex summed normals
	vector<vec3> vertex_normals(posBuf.size());

	// eleBuf is list of all indexes of faces, so each tripple is the indexes
	// of the verticies in the triangle
	for (size_t i = 0; 3 * i + 2 < eleBuf.size(); i++) {
		//eleBuf[i] gives the vertex id, i.e. number between 0 and n, n is
		// amount of verticies
		unsigned int v0_i = eleBuf[3 * i];
		unsigned int v1_i = eleBuf[3 * i + 1];
		unsigned int v2_i = eleBuf[3 * i + 2];

		// vec3 v0 = vec3(posBuf[v0_i], posBuf[v0_i + 1], posBuf[v0_i + 2]);
		// vec3 v1 = vec3(posBuf[v1_i], posBuf[v1_i + 1], posBuf[v1_i + 2]);
		// vec3 v2 = vec3(posBuf[v2_i], posBuf[v2_i + 1], posBuf[v2_i + 2]);

		// Don't need 3rd vector, but need the index to add to its normal sum
		vec3 v0 =
			vec3(posBuf[3 * v0_i], posBuf[3 * v0_i + 1], posBuf[3 * v0_i + 2]);
		vec3 v1 =
			vec3(posBuf[3 * v1_i], posBuf[3 * v1_i + 1], posBuf[3 * v1_i + 2]);
		// vec3 v2 =
			// vec3(posBuf[3 * v2_i], posBuf[3 * v2_i + 1], posBuf[3 * v2_i + 2]);

		vec3 normal = cross(v1, v0);

		vertex_normals[v0_i] += normal;
		vertex_normals[v1_i] += normal;
		vertex_normals[v2_i] += normal;
	}

	assert(norBuf.size() == 0);

	// Normalize all vectors
	for (int vertex_index = 0; vertex_index < vertex_normals.size();
		 vertex_index++) {
		vec3 normal = normalize(vertex_normals[vertex_index]);
		norBuf.push_back(normal.x);
		norBuf.push_back(normal.y);
		norBuf.push_back(normal.z);
	}
}

void Shape::init() {
	// Initialize the vertex array object
	CHECKED_GL_CALL(glGenVertexArrays(1, &vaoID));
	CHECKED_GL_CALL(glBindVertexArray(vaoID));

	// Send the position array to the GPU
	CHECKED_GL_CALL(glGenBuffers(1, &posBufID));
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
	CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float),
								 &posBuf[0], GL_STATIC_DRAW));

	// Send the normal array to the GPU
	if (norBuf.empty()) this->calculateNormals();

	CHECKED_GL_CALL(glGenBuffers(1, &norBufID));
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
	CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float),
								 &norBuf[0], GL_STATIC_DRAW));

	// Send the texture array to the GPU
	if (texBuf.empty()) {
		texBufID = 0;
	} else {
		CHECKED_GL_CALL(glGenBuffers(1, &texBufID));
		CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
		CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER,
									 texBuf.size() * sizeof(float), &texBuf[0],
									 GL_STATIC_DRAW));
	}

	// Send the element array to the GPU
	CHECKED_GL_CALL(glGenBuffers(1, &eleBufID));
	CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));
	CHECKED_GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
								 eleBuf.size() * sizeof(unsigned int),
								 &eleBuf[0], GL_STATIC_DRAW));

	// Unbind the arrays
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
	CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Shape::setTexBuf(std::vector<float> texBuf) {
	this->texBuf = texBuf;

	CHECKED_GL_CALL(glGenBuffers(1, &texBufID));
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
	CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER,
									texBuf.size() * sizeof(float), &texBuf[0],
									GL_STATIC_DRAW));
}

void Shape::draw(const shared_ptr<Program> prog) const {
	int h_pos, h_nor, h_tex;
	h_pos = h_nor = h_tex = -1;

	CHECKED_GL_CALL(glBindVertexArray(vaoID));

	// Bind position buffer
	h_pos = prog->getAttribute("vertPos");
	GLSL::enableVertexAttribArray(h_pos);
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
	CHECKED_GL_CALL(glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0,
										  (const void *)0));

	// Bind normal buffer
	h_nor = prog->getAttribute("vertNor");
	if (h_nor != -1 && norBufID != 0) {
		GLSL::enableVertexAttribArray(h_nor);
		CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
		CHECKED_GL_CALL(glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0,
											  (const void *)0));
	}

	if (texBufID != 0) {
		// Bind texcoords buffer
		h_tex = prog->getAttribute("vertTex");

		if (h_tex != -1 && texBufID != 0) {
			GLSL::enableVertexAttribArray(h_tex);
			CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
			CHECKED_GL_CALL(glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE,
												  0, (const void *)0));
		}
	}

	// Bind element buffer
	CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));

	// Draw
	CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, (int)eleBuf.size(),
								   GL_UNSIGNED_INT, (const void *)0));

	// Disable and unbind
	if (h_tex != -1) {
		GLSL::disableVertexAttribArray(h_tex);
	}
	if (h_nor != -1) {
		GLSL::disableVertexAttribArray(h_nor);
	}
	GLSL::disableVertexAttribArray(h_pos);
	CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
	CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
