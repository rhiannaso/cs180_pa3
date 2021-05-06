#include "Shape.h"
#include <iostream>
#include <assert.h>

#include "GLSL.h"
#include "Program.h"

using namespace std;

Shape::Shape(bool textured) :
	eleBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0), 
   vaoID(0)
{
	min = glm::vec3(0);
	max = glm::vec3(0);
	texOff = !textured;
}

Shape::~Shape()
{
}

/* copy the data from the shape to this object */
void Shape::createShape(tinyobj::shape_t & shape)
{
		posBuf = shape.mesh.positions;
		norBuf = shape.mesh.normals;
		texBuf = shape.mesh.texcoords;
		eleBuf = shape.mesh.indices;
}

void Shape::measure() {
  float minX, minY, minZ;
   float maxX, maxY, maxZ;

   minX = minY = minZ = 1.1754E+38F;
   maxX = maxY = maxZ = -1.1754E+38F;

   //Go through all vertices to determine min and max of each dimension
   for (size_t v = 0; v < posBuf.size() / 3; v++) {
		if(posBuf[3*v+0] < minX) minX = posBuf[3*v+0];
		if(posBuf[3*v+0] > maxX) maxX = posBuf[3*v+0];

		if(posBuf[3*v+1] < minY) minY = posBuf[3*v+1];
		if(posBuf[3*v+1] > maxY) maxY = posBuf[3*v+1];

		if(posBuf[3*v+2] < minZ) minZ = posBuf[3*v+2];
		if(posBuf[3*v+2] > maxZ) maxZ = posBuf[3*v+2];
	}

	min.x = minX;
	min.y = minY;
	min.z = minZ;
   max.x = maxX;
   max.y = maxY;
   max.z = maxZ;
}

void Shape::storeNormal(glm::vec3 n, int index)
{
    norBuf[3*index] += n.x;
    norBuf[3*index+1] += n.y;
    norBuf[3*index+2] += n.z;
}

void Shape::computeNormals()
{
    vector<glm::vec3> parsedT; // Hold grouped vertices
    for (int i=0; i < posBuf.size(); i += 3) {
        parsedT.push_back(glm::vec3(posBuf[i], posBuf[i+1], posBuf[i+2]));
        norBuf.push_back(0);
        norBuf.push_back(0);
        norBuf.push_back(0);
    } 
    // Iterate over each face
    for (int i=0; i < eleBuf.size(); i += 3) {
        glm::vec3 v0 = parsedT[eleBuf[i]];
        glm::vec3 v1 = parsedT[eleBuf[i+1]];
        glm::vec3 v2 = parsedT[eleBuf[i+2]];
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 n = glm::cross(e1, e2);
        storeNormal(n, eleBuf[i]);
        storeNormal(n, eleBuf[i+1]);
        storeNormal(n, eleBuf[i+2]);
    }
    // Iterate over each vertex and normalize
    for (size_t v = 0; v < posBuf.size() / 3; v++) {
        glm::vec3 n = glm::vec3(norBuf[3*v], norBuf[3*v+1], norBuf[3*v+2]);
        glm::vec3 normalized = glm::normalize(n);
        norBuf[3*v] = normalized.x;
        norBuf[3*v+1] = normalized.y;
        norBuf[3*v+2] = normalized.z;
    }
}

void Shape::init()
{
   // Initialize the vertex array object
   glGenVertexArrays(1, &vaoID);
   glBindVertexArray(vaoID);

	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	if(norBuf.empty()) {
		norBufID = 0;
	} else {
		glGenBuffers(1, &norBufID);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	}
	
	// Send the texture array to the GPU - for now no textures
	if(texBuf.empty() || texOff) {
		texBufID = 0;
	} else {
		glGenBuffers(1, &texBufID);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	}
	
	// Send the element array to the GPU
	glGenBuffers(1, &eleBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size()*sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	assert(glGetError() == GL_NO_ERROR);
}

//always untextured for intro labs until texture mapping
void Shape::draw(const shared_ptr<Program> prog) const
{
	int h_pos, h_nor, h_tex;
	h_pos = h_nor = h_tex = -1;

   glBindVertexArray(vaoID);
	// Bind position buffer
	h_pos = prog->getAttribute("vertPos");
	GLSL::enableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Bind normal buffer
	h_nor = prog->getAttribute("vertNor");
	if(h_nor != -1 && norBufID != 0) {
		GLSL::enableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}

	if (texBufID != 0) {	
		// Bind texcoords buffer
		h_tex = prog->getAttribute("vertTex");
		if(h_tex != -1 && texBufID != 0) {
			GLSL::enableVertexAttribArray(h_tex);
			glBindBuffer(GL_ARRAY_BUFFER, texBufID);
			glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
		}
	}
	
	// Bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	
	// Draw
	glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	// Disable and unbind
	if(h_tex != -1) {
		GLSL::disableVertexAttribArray(h_tex);
	}
	if(h_nor != -1) {
		GLSL::disableVertexAttribArray(h_nor);
	}
	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
