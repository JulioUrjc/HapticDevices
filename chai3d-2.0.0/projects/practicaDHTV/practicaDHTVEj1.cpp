// Practica 1 Julio Martín
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

//Cursor
cShapeSphere* cursor;

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
	while (simulationRunning)
	{
		// 1. Read position of haptic device
		cVector3d newPosition;
		hapticDevice->getPosition(newPosition);
		// Update position of cursor
		cursor->setPos(newPosition);
		// 2. Compute a reaction force
		cVector3d newForce(0, 0, 0);
		// 3. Send computed force to haptic device
		hapticDevice->setForce(newForce);
	}
	// exit haptics thread
	simulationFinished = true;
}

void close(void)
{
	simulationRunning = false;
	while (!simulationFinished) { cSleepMs(100); }
	hapticDevice->close();
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

	//Adquirir Puntero
	handler = new cHapticDeviceHandler();
	handler->getDevice(hapticDevice, 0);

	//Abrir conexion
	hapticDevice->open();
	hapticDevice->initialize();

	//Creamos el mundo y la esfera
	world = new cWorld();
	cursor = new cShapeSphere(0.09);
	world->addChild(cursor);

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
	glutSetWindowTitle("DHTV: Ejercicio 1");
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