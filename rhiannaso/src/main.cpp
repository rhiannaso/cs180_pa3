/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	//our geometry
	shared_ptr<Shape> sphere;
	shared_ptr<Shape> theBunny;
    vector<shared_ptr<Shape>> carMesh;
    vector<shared_ptr<Shape>> houseMesh;
    vector<shared_ptr<Shape>> santaMesh;
    vector<shared_ptr<Shape>> sleighMesh;
    shared_ptr<Shape> floorMesh;
    shared_ptr<Shape> lampMesh;
    shared_ptr<Shape> treeMesh;

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	//ground VAO
	GLuint GroundVertexArrayID;

	//the image to use as a texture (ground)
	shared_ptr<Texture> texture0;

	//global data (larger program should be encapsulated)
	vec3 gMin;
	float gRot = -0.3;
	float gCamH = -4;
	//animation data
	float lightTrans = -2.0;
	float gTrans = -3;
	float sTheta = 0;
	float eTheta = 0;
	float wTheta = 0;
    float driveTheta = 0;

    // color toggling
    int houseColor = 0;
    int carColor = 2;
    int lampColor = 4;
    int treeColor = 3;
    int decColor = 1;

    vec3 santaMin;
    vec3 santaMax;
    vec3 sleighMin;
    vec3 sleighMax;
    vec3 houseMin;
    vec3 houseMax;
    vec3 carMin;
    vec3 carMax;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		//update global camera rotate
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			gRot += 0.2;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			gRot -= 0.2;
		}
		//update camera height
		if (key == GLFW_KEY_S && action == GLFW_PRESS){
			gCamH  += 0.25;
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS){
			gCamH  -= 0.25;
		}
        if (key == GLFW_KEY_M && action == GLFW_PRESS){
			houseColor += 1;
            carColor += 1;
            lampColor += 1;
            treeColor += 1;
            decColor += 1;
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans -= 1;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans += 1;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
        prog->addUniform("MatDif");
        prog->addUniform("MatSpec");
        prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
        prog->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		//read in a load the texture
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/asphalt3.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	}

    void findMin(float x, float y, float z, string mesh) {
        if (mesh == "santa") {
            if (x < santaMin.x)
                santaMin.x = x;
            if (y < santaMin.y)
                santaMin.y = y;
            if (z < santaMin.z)
                santaMin.z = z;
        } else if (mesh == "sleigh") {
            if (x < sleighMin.x)
                sleighMin.x = x;
            if (y < sleighMin.y)
                sleighMin.y = y;
            if (z < sleighMin.z)
                sleighMin.z = z;
        } else if (mesh == "house") {
            if (x < houseMin.x)
                houseMin.x = x;
            if (y < houseMin.y)
                houseMin.y = y;
            if (z < houseMin.z)
                houseMin.z = z;
        } else {
            if (x < carMin.x)
                carMin.x = x;
            if (y < carMin.y)
                carMin.y = y;
            if (z < carMin.z)
                carMin.z = z;
        }
    }

    void findMax(float x, float y, float z, string mesh) {
        if (mesh == "santa") {
            if (x > santaMax.x)
                santaMax.x = x;
            if (y > santaMax.y)
                santaMax.y = y;
            if (z > santaMax.z)
                santaMax.z = z;
        } else if (mesh == "sleigh") {
            if (x > sleighMax.x)
                sleighMax.x = x;
            if (y > sleighMax.y)
                sleighMax.y = y;
            if (z > sleighMax.z)
                sleighMax.z = z;
        } else if (mesh == "house") {
            if (x > houseMax.x)
                houseMax.x = x;
            if (y > houseMax.y)
                houseMax.y = y;
            if (z > houseMax.z)
                houseMax.z = z;
        } else {
            if (x > carMax.x)
                carMax.x = x;
            if (y > carMax.y)
                carMax.y = y;
            if (z > carMax.z)
                carMax.z = z;
        }
    }

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shape(s)
        bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/streetlamp.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
            lampMesh = make_shared<Shape>(false);
            lampMesh->createShape(TOshapes[0]);
            lampMesh->measure();
            lampMesh->init();
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sphereNoNormals.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
            sphere = make_shared<Shape>(false);
            sphere->createShape(TOshapes[0]);
            sphere->measure();
            sphere->computeNormals();
            sphere->init();
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/xmas.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
            treeMesh = make_shared<Shape>(false);
            treeMesh->createShape(TOshapes[0]);
            treeMesh->measure();
            treeMesh->init();
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sleigh.obj").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>(false);
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                findMin(tmp->min.x, tmp->min.y, tmp->min.z, "sleigh");
                findMax(tmp->max.x, tmp->max.y, tmp->max.z, "sleigh");

                sleighMesh.push_back(tmp);
            }
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/santa.obj").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>(false);
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                findMin(tmp->min.x, tmp->min.y, tmp->min.z, "santa");
                findMax(tmp->max.x, tmp->max.y, tmp->max.z, "santa");

                santaMesh.push_back(tmp);
            }
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/smallHouse.obj").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>(false);
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                findMin(tmp->min.x, tmp->min.y, tmp->min.z, "house");
                findMax(tmp->max.x, tmp->max.y, tmp->max.z, "house");
                
                houseMesh.push_back(tmp);
            }
		}


        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/car.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>(false);
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                findMin(tmp->min.x, tmp->min.y, tmp->min.z, "car");
                findMax(tmp->max.x, tmp->max.y, tmp->max.z, "car");

                carMesh.push_back(tmp);
            }
		}

		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 40;
		float g_groundY = 0;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 
        };

        unsigned short idx[] = {0, 1, 2, 0, 2, 3};

        //generate the ground VAO
        glGenVertexArrays(1, &GroundVertexArrayID);
        glBindVertexArray(GroundVertexArrayID);

        g_GiboLen = 6;
        glGenBuffers(1, &GrndBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

        glGenBuffers(1, &GrndNorBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

        glGenBuffers(1, &GrndTexBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

        glGenBuffers(1, &GIndxBuffObj);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    }

    //code to draw the ground plane
    void drawGround(shared_ptr<Program> curS) {
        curS->bind();
        glBindVertexArray(GroundVertexArrayID);
        texture0->bind(curS->getUniform("Texture0"));
        //draw the ground plane 
        SetModel(vec3(0, 0, 0), 0, 0, 1, curS);
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

    float findCenter(float min, float max) {
        float dist = (max-min)/2;
        return max-dist;
    }

    void drawTrees(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();
            float zVals[10] = {30, 15, 0, -15, -30, 30, 15, 0, -15, -30};
            float xVals[10] = {15, 15, 15, 15, 15, -15, -15,-15, -15, -15};

            for (int j=0; j < 10; j++) {
                Model->pushMatrix();
                    Model->translate(vec3(xVals[j], 0, zVals[j]));
                    Model->scale(vec3(0.5, 0.5, 0.5));
                    setModel(prog, Model);
                    treeMesh->draw(prog);
                Model->popMatrix();
            }
       Model->popMatrix();
    }

    void drawSpheres(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();
            float zVals[10] = {30, 15, 0, -15, -30, 30, 15, 0, -15, -30};
            float xVals[10] = {15, 15, 15, 15, 15, -15, -15,-15, -15, -15};

            for (int j=0; j < 10; j++) {
                Model->pushMatrix();
                    Model->translate(vec3(xVals[j], 14.5, zVals[j]));
                    Model->scale(vec3(0.8, 0.8, 0.8));
                    setModel(prog, Model);
                    sphere->draw(prog);
                Model->popMatrix();
            }
       Model->popMatrix();
    }

    void drawLamps(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();

            float zVals[4] = {22.5, 7.5, -7.5, -22.5};

            for (int l=0; l < 4; l++) {
                Model->pushMatrix();
                    Model->translate(vec3(15, 0, zVals[l]));
                    Model->scale(vec3(3, 3, 3));
                    setModel(prog, Model);
                    lampMesh->draw(prog);
                Model->popMatrix();
            }

            for (int r=0; r < 4; r++) {
                Model->pushMatrix();
                    Model->translate(vec3(-15, 0, zVals[r]));
                    Model->rotate(3.14159, vec3(0, 1, 0));
                    Model->scale(vec3(3, 3, 3));
                    setModel(prog, Model);
                    lampMesh->draw(prog);
                Model->popMatrix();
            }
        Model->popMatrix();
    }

    void drawHouse(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();
            float zCenter = findCenter(houseMin.z, houseMax.z);
            Model->loadIdentity();
            Model->translate(vec3(0, 0, -zCenter));

            Model->pushMatrix();
                Model->translate(vec3(0, 0, -3));
                Model->rotate(1.5, vec3(0, 1, 0));
                Model->scale(vec3(0.07, 0.07, 0.07));

                setModel(prog, Model);
                for (int i=0; i < houseMesh.size(); i++) {
                    houseMesh[i]->draw(prog);
                }
            Model->popMatrix();
        Model->popMatrix();
    }

    void drawCar(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();
            driveTheta = 2*sin(glfwGetTime());

            Model->translate(vec3(-5, 0.25, 4));
            Model->translate(vec3(0, 0, driveTheta));
            Model->scale(vec3(0.4, 0.4, 0.4));

            setModel(prog, Model);
            for (int i=0; i < carMesh.size(); i++) {
                if (i == 4) {
                    SetMaterial(prog, carColor%5);
                } else {
                    SetMaterial(prog, 4);
                }
                carMesh[i]->draw(prog);
            }
        Model->popMatrix();
    }

    void drawDecorations(shared_ptr<MatrixStack> Model) {
        Model->pushMatrix();
            float center = findCenter(santaMin.z, santaMax.z);
            Model->translate(vec3(0, 0, -center));

            Model->translate(vec3(-7, 0, -15));
            Model->scale(vec3(0.0125, 0.0125, 0.0125));

            setModel(prog, Model);
            for (int i=0; i < santaMesh.size(); i++) {
                if (i == 4 || i == 6 || i == 8 || i == 10 || i == 12 || i == 17 || i == 24) {
                    SetMaterial(prog, 2);
                } else if (i == 1 || i == 3 || i == 9 || i == 11 || i == 13 || i == 25 || i == 18 || i == 26) {
                    SetMaterial(prog, 5);
                } else if (i == 0 || i == 2 || i == 5 || i == 7) {
                    SetMaterial(prog, 7);
                } else {
                    SetMaterial(prog, 6);
                }
                santaMesh[i]->draw(prog);
            }
        Model->popMatrix();

        Model->pushMatrix();
            center = findCenter(sleighMin.z, sleighMax.z);
            Model->translate(vec3(0, 0, -center));

            Model->translate(vec3(1, 9, -19));
            Model->rotate(1.5, vec3(0, 1, 0));
            Model->scale(vec3(1.25, 1.25, 1.25));

            setModel(prog, Model);
            SetMaterial(prog, decColor%5);
            for (int i=0; i < sleighMesh.size(); i++) {
                sleighMesh[i]->draw(prog);
            }
        Model->popMatrix();
    }

     //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

    	switch (i) {
    		case 0: // house
    			// glUniform3f(curS->getUniform("MatAmb"), 0.01, 0.01, 0.01);
    			// glUniform3f(curS->getUniform("MatDif"), 0.45, 0.3, 0.05);
    			// glUniform3f(curS->getUniform("MatSpec"), 0.2, 0.15, 0.02);
    			// glUniform1f(curS->getUniform("MatShine"), 10.0);
                glUniform3f(curS->getUniform("MatAmb"), 0.25, 0.20725, 0.20725);
    			glUniform3f(curS->getUniform("MatDif"), 0.38, 0.34, 0.28);
    			glUniform3f(curS->getUniform("MatSpec"), 0.296648, 0.296648, 0.296648);
    			glUniform1f(curS->getUniform("MatShine"), 11.264);
    		break;
    		case 1: // sleigh (gold)
                glUniform3f(curS->getUniform("MatAmb"), 0.2472, 0.1995, 0.0745);
    			glUniform3f(curS->getUniform("MatDif"), 0.75164, 0.60648, 0.22648);
    			glUniform3f(curS->getUniform("MatSpec"), 0.628281, 0.555802, 0.366065);
    			glUniform1f(curS->getUniform("MatShine"), 51.2);
    		break;
    		case 2: // car body (ruby)
                glUniform3f(curS->getUniform("MatAmb"), 0.1745, 0.01175, 0.01175);
    			glUniform3f(curS->getUniform("MatDif"), 0.61424, 0.04136, 0.04136);
    			glUniform3f(curS->getUniform("MatSpec"), 0.727811, 0.626959, 0.626959);
    			glUniform1f(curS->getUniform("MatShine"), 76.8);
    		break;
            case 3: //plant
                glUniform3f(curS->getUniform("MatAmb"), 0.01, 0.1, 0.01);
    			glUniform3f(curS->getUniform("MatDif"), 0.2, 0.3, 0.2);
    			glUniform3f(curS->getUniform("MatSpec"), 0.05, 0.075, 0.05);
    			glUniform1f(curS->getUniform("MatShine"), 10.0);
            break;
            case 4: // lamp (chrome)
    			glUniform3f(curS->getUniform("MatAmb"), 0.25, 0.25, 0.25);
    			glUniform3f(curS->getUniform("MatDif"), 0.4, 0.4, 0.4);
    			glUniform3f(curS->getUniform("MatSpec"), 0.77, 0.77, 0.77);
    			glUniform1f(curS->getUniform("MatShine"), 76.8);
            break;
            case 5: // white rubber
    			glUniform3f(curS->getUniform("MatAmb"), 0.05, 0.05, 0.05);
    			glUniform3f(curS->getUniform("MatDif"), 0.5, 0.5, 0.5);
    			glUniform3f(curS->getUniform("MatSpec"), 0.7, 0.7, 0.7);
    			glUniform1f(curS->getUniform("MatShine"), 10.8);
            break;
            case 6: // perl
    			glUniform3f(curS->getUniform("MatAmb"), 0.25, 0.2, 0.2);
    			glUniform3f(curS->getUniform("MatDif"), 1.0, 0.829, 0.829);
    			glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.3, 0.3);
    			glUniform1f(curS->getUniform("MatShine"), 11.8);
            break;
            case 7: // black plastic
    			glUniform3f(curS->getUniform("MatAmb"), 0.0, 0.0, 0.0);
    			glUniform3f(curS->getUniform("MatDif"), 0.01, 0.01, 0.01);
    			glUniform3f(curS->getUniform("MatSpec"), 0.5, 0.5, 0.5);
    			glUniform1f(curS->getUniform("MatShine"), 32.8);
            break;
  		}
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

	void render() {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// View is global translation along negative z for now
		View->pushMatrix();
		View->loadIdentity();
		//camera up and down
		View->translate(vec3(0, gCamH, -20));
		//global rotate (the whole scene )
		View->rotate(gRot, vec3(0, 1, 0));

		// Draw the scene
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(prog->getUniform("lightPos"), lightTrans, 15.0, 20.0);
        Model->pushMatrix();
            Model->loadIdentity();
            Model->translate(vec3(0, 0, 0));

            // Draw house
            SetMaterial(prog, houseColor%5);
            drawHouse(Model);

            // Draw santa and sleigh
            drawDecorations(Model);

            // Draw car
            drawCar(Model);

            // Draw trees
            SetMaterial(prog, treeColor%5);
            drawTrees(Model);

            // Draw "stars"
            SetMaterial(prog, decColor%5);
            drawSpheres(Model);

            // Draw streetlamps
            SetMaterial(prog, lampColor%5);
            drawLamps(Model);

        Model->popMatrix();

		prog->unbind();

		//switch shaders to the texture mapping shader and draw the ground
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
				
		drawGround(texProg);

		texProg->unbind();
		
		//animation update example
		sTheta = sin(glfwGetTime());
        eTheta = (sin(glfwGetTime()) + 1)/1.5;
        wTheta = sin(4*glfwGetTime())/2;

		// Pop matrix stacks.
		Projection->popMatrix();
		View->popMatrix();

	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
