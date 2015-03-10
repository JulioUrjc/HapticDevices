// Practica 4 Julio Martín
// DHTV Máster en Informática Gráfica, Juegos y Realidad Virtual

#include "stdafx.h"
#include <iostream>
using namespace std;
#include "chai3d.h"

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W = 600;
const int WINDOW_SIZE_H = 600;

// Grafo de escena
cWorld* world;
cCamera* camera;
cLight* light;

//Dispositivo haptico
cHapticDeviceHandler* handler;
cGenericHapticDevice* hapticDevice;

//Objeto a tocar y puntero
cMesh* object;
cGeneric3dofPointer* tool;

// status of the main simulation haptics loop
bool simulationRunning = false;
// has exited haptics simulation thread
bool simulationFinished = false;

// width and height of the current window display
int displayW = 800;
int displayH = 640;


void updateGraphics(void){
	camera->renderView(displayW, displayH);
	glutSwapBuffers();
	if (simulationRunning)
		glutPostRedisplay();
}

void updateHaptics(void){
	cVector3d velocity;
	velocity.zero();
	double timestep = 0.0001;
	double mass = 1.0;
	double friction = 0.0001;
	double pushGain = 1.0;

	while (simulationRunning)
	{
		// compute global reference frames for each object
		world->computeGlobalPositions(true);
		// 1. Read device and update position of tool
		tool->updatePose();
		// 2. Compute interaction forces
		tool->computeInteractionForces();
		// 3. Send forces to device
		tool->applyForces();
		// 4. Collision response
		// 4.1. check if the tool is touching something
		if (tool->m_proxyPointForceModel->getNumContacts() > 0){
			// 4.2. get last force computed on tool
			cVector3d force = cNegate(tool->m_lastComputedGlobalForce);
			force = pushGain * force;
			// 4.3. compute acceleration
			cVector3d acceleration = (1.0 / mass)*force;
			// 4.4. integrate velocity
			velocity = velocity + timestep*acceleration;
		}
		// 4.5. Apply friction
		if (velocity.length() > CHAI_SMALL)
			velocity = (1 - friction)*velocity;
		else
			velocity.zero();
		// 4.6. Integrate position
		cVector3d objectPos = object->getGlobalPos();
		objectPos = objectPos + timestep*velocity;
		object->setPos(objectPos);
	}
	// exit haptics thread
	simulationFinished = true;
}

void close(void)
{
	simulationRunning = false;
	while (!simulationFinished) { cSleepMs(100); }
	tool->stop();
}

void keySelect(unsigned char key, int x, int y)
{
	// escape key
	if ((key == 27) || (key == 'x')){
		close();
		exit(0);
	}
}

void resizeWindow(int w, int h)
{
	// update the size of the viewport
	displayW = w;
	displayH = h;
	glViewport(0, 0, displayW, displayH);
}

int main(int argc, char* argv[]){

	//-----------------------------------------------------------------------
	// HAPTIC DEVICES / TOOLS
	//-----------------------------------------------------------------------
	handler = new cHapticDeviceHandler();
	handler->getDevice(hapticDevice, 0);
	world = new cWorld();
	// create a 3D tool and add it to the world
	tool = new cGeneric3dofPointer(world);
	world->addChild(tool);
	// connect the haptic device to the tool
	tool->setHapticDevice(hapticDevice);
	// initialize tool by connecting to haptic device
	int exito = tool->start();

	tool->setWorkspaceRadius(1.0);
	tool->setRadius(0.01);
	tool->m_deviceSphere->setShowEnabled(false);
	tool->m_proxyPointForceModel->setProxyRadius(0.01);
	tool->m_proxyPointForceModel->m_collisionSettings.m_checkBothSidesOfTriangles = false;
	tool->m_proxyPointForceModel->m_useDynamicProxy = true;

	// Stiffness of material
	double workspaceScaleFactor = tool->getWorkspaceScaleFactor();
	cHapticDeviceInfo info;
	info = hapticDevice->getSpecifications();
	double maxStiffness = info.m_maxForceStiffness / workspaceScaleFactor;
	//-----------------------------------------------------------------------
	// OBJECT CREATION
	//-----------------------------------------------------------------------
	
	object = new cMesh(world);
	world->addChild(object);
	//Modelos a cargar
	//object->loadFromFile("./bunny.obj");
	//object->loadFromFile("./duck-green.obj");
	object->loadFromFile("./face.3ds");

	object->computeBoundaryBox(true);
	double size = 5 * cSub(object->getBoundaryMax(), object->getBoundaryMin()).length(); //Multiplico por 3 porque sino sale muy grande
	object->scale(tool->getWorkspaceRadius() / size);

	//Textura para el modelo duck-green
	/*cTexture2D* texture = new cTexture2D();
	texture->loadFromFile("./duckCM.bmp");
	object->setTexture(texture, true);
	object->setUseTexture(true);
	object->m_material.m_ambient.set(1.0, 1.0, 1.0);
	object->m_material.m_diffuse.set(1.0, 1.0, 1.0);
	object->m_material.m_specular.set(1.0, 1.0, 1.0);*/

	object->createAABBCollisionDetector(1.01 * tool->m_proxyPointForceModel->getProxyRadius(), true, false);

	object->setStiffness(1.0*maxStiffness, true);
	object->setFriction(0.1, 0.2, true);

	///////////////////////////////////////////////////////////////////////////////
	//-----------------------------------------------------------------------
	// 3D - SCENEGRAPH
	//-----------------------------------------------------------------------

	// set the background color of the environment //world->setBackgroundColor(0.0, 0.0, 0.0);
	// create a camera and insert it into the virtual world
	camera = new cCamera(world);
	world->addChild(camera);

	// position and oriente the camera
	camera->set(cVector3d(0.5, 0.0, 0.0),    // camera position (eye)
		cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
		cVector3d(0.0, 0.0, 1.0));   // direction of the "up" vector

	// set the near and far clipping planes of the camera
	camera->setClippingPlanes(0.01, 10.0);

	// create a light source and attach it to the camera
	light = new cLight(world);
	camera->addChild(light);                   // attach light to camera
	light->setEnabled(true);                   // enable light source
	light->setPos(cVector3d(2.0, 0.5, 1.0));  // position the light source
	light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam

	// initialize GLUT OPEN GL - WINDOW DISPLAY
	glutInit(&argc, argv);

	// retrieve the resolution
	int screenW = glutGet(GLUT_SCREEN_WIDTH);
	int screenH = glutGet(GLUT_SCREEN_HEIGHT);
	int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
	int windowPosY = (screenH - WINDOW_SIZE_H) / 2;

	// initialize the OpenGL GLUT window
	glutInitWindowPosition(windowPosX, windowPosY);
	glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(updateGraphics);
	glutKeyboardFunc(keySelect);
	glutReshapeFunc(resizeWindow);
	glutSetWindowTitle("DHTV: Ejercicio 4");
	///////////////////////////////////////////////////////////////////////////////

	//Comienza la Simulacion
	simulationRunning = true;

	cThread* hapticsThread = new cThread();
	hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);

	glutMainLoop();

	// close everything
	close();
	return 0;
}

