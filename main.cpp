#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#endif
#include <list>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <iostream>
#include "Vector3.h"

/* Additional Features
 * Improved Camera - The camera has freelook (with constraints) and moves independently from where it's looking.
				   - The camera can look diagonally as well by pressing multiple keys at once.
				   - https://www.opengl.org/discussion_boards/showthread.php/131581-GLUT-two-keys-at-once?p=979717&viewfull=1#post979717
 * Topographic map coloring.
 * Mouse drags change the camera angle.
 * Circle faulting algorithm - Best seen if you reset the terrain first before using it. (the faults only grow upwards)
*/

/* Controls
 * p		 : passiveFaulting : continuously do 1 fault every update
 * <space>	 : do 800 faults
 * l		 : toggle lighting
 * k		 : toggle shaders
 * w		 : toggle between filled polygons, wireframe, and both
 * f		 : toggle between line faulting and circle faulting algorithm
 * r		 : reset the scene and camera position
 * PgUp,PgDn : move the camera forwards and backwards (alt. keys: Home and End)
 * arrows	 : rotate the camera around its own axis
 * mouse	 : rotate the camera on its y-axis only (click and drag)
 * s,x,z,c   : Move first light up,down,backwards,forwards
 * g,b,v,n   : Move second light up,down,backwards,forwards
 * < >		 : Rotate the terrain left and right (alt. keys: comma and period)
 * : ?       : Rotate the terrain on a diagonal axis (alt. keys: semi-colon and forward-slash)
*/

/* Terrain variables */ 
int terrainZ = 50;
int terrainX = 50;
const float maxHeight = 50;
const float minHeight = 0;
const float displacement = 0.5;
float** terrain;
float terrainXZAngle = 0;
float terrainYAngle = 0;
enum PolygonMode { Fill, Wireframe, FilledWire };

/* Camera */
float theta = 300;
Vector3 camPos = { 0, 106.0f, 79.3f };
Vector3 camLook = { 300.f * (float)cos(theta) + camPos.x, -249.f, 300.f * (float)sin(theta) + camPos.z };
Vector3 mover; // moves the camera

/* Mouse controls */
Vector3 mouse; // position
bool lastState = GLUT_UP;
bool currentState = GLUT_UP;

/* Lighting (point lights) */
float light0[4] = { (terrainX / 2.0f), maxHeight + 1.0f, (terrainZ / 2.0f), 1 };
float light1[4] = { -(terrainX / 2.0f), maxHeight + 1.0f, -(terrainZ / 2.0f), 1 };

/* Normals */
Vector3** faceNormals;
Vector3** vertexNormals;

/* States */
PolygonMode polygonMode = Fill;
bool lightingEnabled = false;
bool passiveFaulting = false;
bool needFaceNormals = true;
bool needVertexNormals = false;
bool flatShading = true; // Flat vs Smooth
bool faultedAfterSmooth = false; // When toggling between shading modes, don't calculate vertex normals again unless a fault occurs.
bool keyStates[256] = { false };
bool circleFault = false;

/* Fault terrain algorithms from:
 *  http://www.lighthouse3d.com/opengl/terrain/index.php?fault
 */
void faultTerrain(int times)
{
	for (int t = 0; t < times; t++) {
		if (!circleFault) { // line fault
			float v = (float)rand();
			float a = sin(v);
			float b = cos(v);
			float d = sqrt(terrainX*terrainX + terrainZ*terrainZ);
			float c = ((float)rand() / RAND_MAX) * d - d / 2.0f; // random number between -d/2 and d/2

			for (int z = 0; z < terrainZ; z++) {
				for (int x = 0; x < terrainX; x++) {
						if (a*z + b*x - c  > 0) // if on one side of the line equation
							terrain[z][x] += (terrain[z][x] + displacement > maxHeight) ? 0 : displacement;
						else
							terrain[z][x] -= (terrain[z][x] + displacement < minHeight) ? 0 : displacement;
					}
				}
			}
		else { // circle fault
			int randX = rand() % (terrainX + 1);
			int randZ = rand() % (terrainZ + 1);
			int randCircSize = rand() % ((terrainX+terrainZ)/10); // circle diameter
			for (int z = 0; z < terrainZ; z++) {
				for (int x = 0; x < terrainX; x++) {
					float pd = sqrt((randX - x)*(randX - x) + (randZ - z)*(randZ - z)) * 2.0f / randCircSize; // pd = distanceFromCircle*2/size
					if (fabs(pd) <= 1.0f) { // if the vertex is within the circle, displace it upwards
						float diff = (displacement / 2.0f + sin(pd*3.14f)*displacement / 2.0f);
						terrain[z][x] += (terrain[z][x] + diff > maxHeight) ? 0 : diff; // constrain to maxHeight
					}
				}
			}
		}
	}
	
	needFaceNormals = true;
	if (!flatShading) needVertexNormals = true;
	faultedAfterSmooth = true;
}

void resetTerrain(void)
{
	for (int z = 0; z < terrainZ; z++) {
		for (int x = 0; x < terrainX; x++) {
			terrain[z][x] = 0.5; // initial terrain height
		}
	}
	faultTerrain(0);
}

void resetCamera(void)
{
	theta = 300;
	camPos = { 0, 106.0f, 79.3f };
	camLook = { 300.f * (float)cos(theta) + camPos.x, -249.f, 300.f * (float)sin(theta) + camPos.z };
	terrainYAngle = 0;
	terrainXZAngle = 0;
}

Vector3 topographicColoring(Vector3 in)
{
	float scale; // Scaled between each range
	if(in.y < maxHeight * 0.2f){ // green
		scale = in.y / (0.2f * maxHeight);
		return { 0.0f, 0.7f, 0.0f * scale };
	}
	if (in.y < maxHeight * 0.4f){ // yellow
		scale = in.y / (0.4f * maxHeight);
		return { 1.0f * scale, 0.8f * scale, 0.0f };
	}
	if (in.y < maxHeight * 0.6f){ // orange
		scale = in.y / (0.6f * maxHeight);
		return { 1.0f * scale, 0.6f * scale, 0.0f };
	}
	if (in.y < maxHeight * 0.8f){ // red
		scale = in.y / (0.8f * maxHeight);
		return { 1.0f * scale, 0.0f, 0.0f };
	}
	else{ // grey
		scale = in.y / (1.0f * maxHeight);
		return { 0.5f*scale, 0.5f*scale, 0.5f*scale };
	}
}

/* Modifies the face normals array. */
void setFaceNormals(void)
{
	Vector3 v1, v2, v3, v4;
	for (int z = 0; z < terrainZ - 1; z++) {
		for (int x = 0; x < terrainX - 1; x++) {
			// Quad's vertex positions
			Vector3 quad[4] = { { x + 0.f, terrain[z][x], z + 0.f }, { x + 0.f, terrain[ z+ 1][x], z + 1.f },
			{ x + 1.f, terrain[ z+ 1][x + 1], z + 1.f }, { x + 1.f, terrain[z][x + 1], z + 0.f } };
			// Four cross-products, one for each vertex of the quad
			v1 = quad[0].cross(quad[1]);
			v2 = quad[1].cross(quad[2]);
			v3 = quad[2].cross(quad[3]);
			v4 = quad[3].cross(quad[0]);
			// Average the four cross-products then normalize
			Vector3 faceNormal = { (v1.x + v2.x + v3.x + v4.x) / 4.f,
				(v1.y + v2.y + v3.y + v4.y) / 4.f, (v1.z + v2.z + v3.z + v4.z) / 4.f };
			faceNormal = faceNormal.normalize();
			// Save the face normal
			faceNormals[z][x].x = faceNormal.x;
			faceNormals[z][x].y = faceNormal.y;
			faceNormals[z][x].z = faceNormal.z;
		}
	}
	needFaceNormals = false;
}

/* Modifies the vertex normals array. */
void setVertexNormals(void)
{
	Vector3 v1, v2, v3, v4;
	for (int z = 1; z < terrainZ - 1; z++) { // Excludes outside perimeter of vertices
		for (int x = 1; x < terrainX - 1; x++) {
			// Get 4 adjacent face normals
			v1 = faceNormals[z-1][x-1];
			v2 = faceNormals[z][x-1];
			v3 = faceNormals[z][x];
			v4 = faceNormals[z-1][x];
			// Average the 4 face normals then normalize
			Vector3 vertexNormal = { (v1.x + v2.x + v3.x + v4.x) / 4.f,
				(v1.y + v2.y + v3.y + v4.y) / 4.f, (v1.z + v2.z + v3.z + v4.z) / 4.f };
			vertexNormal = vertexNormal.normalize();
			// Save the vertex normal
			vertexNormals[z][x].x = vertexNormal.x;
			vertexNormals[z][x].y = vertexNormal.y;
			vertexNormals[z][x].z = vertexNormal.z;
		}
	}
	needVertexNormals = false;
}

/* (0,0)    (x,0)
 *	  ------
 *	  |    |
 *	  |    |
 *	  ------
 * (0,z)    (x,z)
 */
void drawTerrain(void)
{
	for (int z = 0; z < terrainZ - 1; z++) {
		for (int x = 0; x < terrainX - 1; x++) {
			// The quad's four vertex positions
			Vector3 quad[4] = { { x + 0.f, terrain[z][x], z + 0.f }, { x + 0.f, terrain[z + 1][x], z + 1.f },
			{ x + 1.f, terrain[z + 1][x + 1], z + 1.f }, { x + 1.f, terrain[z][x + 1], z + 0.f } };
			
			if (flatShading) glNormal3fv(faceNormals[z][x].v); // Define one face normal for quad face
			glBegin(GL_QUADS);
				for (int q = 0; q < 4; q++) {
					glColor3fv(topographicColoring(quad[q]).v);
					if (!flatShading) glNormal3fv(vertexNormals[z][x].v); // Define vertex normal per vertex
					// the material ambient and diffuse are set to the color due to glColorMaterial [in init()]
					glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20);
					glVertex3fv(quad[q].v);
				}
			glEnd();
			
			// Draw wireframe for combined (filled wireframe) mode
			if (polygonMode == FilledWire) {
				glBegin(GL_LINE_LOOP);
				for (int q = 0; q < 4; q++) {
					glColor3f(0, 0, 1); // wireframe color
					glVertex3fv(quad[q].v);
				}
				glEnd();
			}
		}
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(camPos.x, camPos.y, camPos.z, camLook.x, camLook.y, camLook.z, 0, 1, 0);
	glColor3f(1, 1, 1);

	glPushMatrix();
		glRotatef(terrainXZAngle, 0, 1, 0);
		glRotatef(terrainYAngle, 1, 0, 1);
		glTranslatef(-terrainX / 2.0f, 0, -terrainZ / 2.0f); // center the terrain onto origin
		drawTerrain();
	glPopMatrix();

	glutSwapBuffers();
}

/* Moves the camera and its view depending on keys currently pressed. Allows for multiple key presses at once. */
void updateCamera(void) {
	mover = Vector3(camLook.x - camPos.x, camLook.y - camPos.y, camLook.z - camPos.z).normalize();

	if (keyStates[GLUT_KEY_PAGE_UP] || keyStates[GLUT_KEY_HOME]) {
		camPos = camPos.add(mover.scale(+2));
	}
	else if (keyStates[GLUT_KEY_PAGE_DOWN] || keyStates[GLUT_KEY_END]) {
		camPos = camPos.add(mover.scale(-2));
	}
	if (keyStates[GLUT_KEY_UP]) {
		if (camLook.y <= 300){
			camLook.y += 10;
		}
	}
	else if (keyStates[GLUT_KEY_DOWN]) {
		if (camLook.y >= -300){
			camLook.y -= 10;
		}
	}
	if (keyStates[GLUT_KEY_LEFT]) {
		theta -= 0.1;
		camLook.x = 300 * cos(theta) + camPos.x;
		camLook.z = 300 * sin(theta) + camPos.z;
	}
	else if (keyStates[GLUT_KEY_RIGHT]) {
		theta += 0.1;
		camLook.x = 300 * cos(theta) + camPos.x;
		camLook.z = 300 * sin(theta) + camPos.z;
	}
}

/* timer function. */
void update(int value)
{
	// Camera
	updateCamera();

	// Normals
	if (needFaceNormals)
		setFaceNormals();
	if (needVertexNormals)
		setVertexNormals();

    // Lights
	glLightfv(GL_LIGHT0, GL_POSITION, light0);
	glLightfv(GL_LIGHT1, GL_POSITION, light1);

	// Passively do the fault algorithm
	if (passiveFaulting) faultTerrain(1);

	glutPostRedisplay();
	glutTimerFunc(16, update, 0);
}

void toggleLighting(bool lit) {
	if (lit)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
	lightingEnabled = lit;
}

void toggleShading(bool flat) {
	if (flat) {
		glShadeModel(GL_FLAT);
	}
	else {
		glShadeModel(GL_SMOOTH);
		if (faultedAfterSmooth) {
			needFaceNormals = true;
			needVertexNormals = true;
			faultedAfterSmooth = false;
		}
	}
	flatShading = flat;
}

void activeMouse(int button, int state, int x, int y)
{
	currentState = state;
	if (lastState == GLUT_UP) {
		if (currentState == GLUT_DOWN) { // save the start position of a mouse drag
			mouse.x = x;
			mouse.y = y;
		}
	}
	lastState = state;
}

void passiveMouse(int x, int y)
{
	if (lastState == GLUT_DOWN) {
		if (currentState == GLUT_DOWN) { // dragging the mouse
			camLook.y += mouse.y - y;
			mouse.y = y;
			mouse.x = x;
		}
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {

	// Toggle Lighting
	case 'l':
		if (lightingEnabled)
			toggleLighting(false);
		else
			toggleLighting(true);
		break;

	// Toggle Shading mode
	case 'k': 
		if (flatShading)
			toggleShading(false);
		else
			toggleShading(true);
		break;
	
	// Toggle wireframe
	case 'w': 
		switch (polygonMode) {
		case(Fill):
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			polygonMode = Wireframe;
			break;
		case(Wireframe):
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			polygonMode = FilledWire;
			break;
		case(FilledWire):
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			polygonMode = Fill;
		}
		break;

	// Reset terrain
	case 'r':
		resetTerrain();
		resetCamera();
		break;

	// Fault
	case ' ':
		faultTerrain(800);
		break;
	case 'p': // continuous faulting
		passiveFaulting = !passiveFaulting;
		break;
	case 'f': // Toggle between line or circle faults
		circleFault = !circleFault;
		break;

	// Rotate terrain
	case '<':
	case ',':
		terrainXZAngle += (terrainXZAngle == 350) ? -350 : 10;
		break;
	case '>':
	case '.':
		terrainXZAngle += (terrainXZAngle == 0) ? +350 : -10;
		break;
	case ':':
	case ';':
		terrainYAngle += (terrainYAngle == 350) ? -350 : 10;
		break;
	case '?':
	case '/':
		terrainYAngle += (terrainYAngle == 0) ? +350 : -10;
		break;

	// Move Light 0
	case 'z':
		light0[0] -= 5;
		light0[2] -= 5;
		break;
	case 'c':
		light0[0] += 5;
		light0[2] += 5;
		break;
	case 's':
		light0[1] += 5;
		break;
	case 'x':
		light0[1] -= 5;
		break;

	// Move Light 1
	case 'v':
		light1[0] -= 5;
		light1[2] -= 5;
		break;
	case 'n':
		light1[0] += 5;
		light1[3] += 5;
		break;
	case 'g':
		light1[1] += 5;
		break;
	case 'b':
		light1[1] -= 5;
		break;

	case 'q': // Quit
		exit(0);
		break;

	}
}

/* special key released */
void specialUp(int key, int x, int y)
{
	keyStates[key] = false;
}

/* special key pressed */
void special(int key, int x, int y)
{
	keyStates[key] = true;
}

void init(void)
{
	// Background Color
	glClearColor(0, 0, 0, 0);

	// Culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Shading
	toggleShading(flatShading);

	// Lighting
	toggleLighting(lightingEnabled);
	float pureWhite[4] = { 1, 1, 1, 1 };
	float light0ambient[4] = { 0.804f, 1.f, 0.98f, 1.f };
	float light0diffuse[4] = { 0.804f, 1.f, 0.98f, 1.f };
	float light1ambient[4] = { 0.8f, 0.2f, 0.4f, 1.f }; 
	float light1diffuse[4] = { 0.8f, 1.f, 0.4f, 1.f };
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, pureWhite);
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, pureWhite);

	// Meterials
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ; // use glColor for material
}

void initalizeArrays(void)
{
	terrain = new float*[terrainZ];
	faceNormals = new Vector3*[terrainZ];
	vertexNormals = new Vector3*[terrainZ];
	for (int i = 0; i < terrainZ; i++) {
		terrain[i] = new float[terrainX];
		faceNormals[i] = new Vector3[terrainX];
		vertexNormals[i] = new Vector3[terrainX];
	}
	resetTerrain();
}

int getUserInput(void)
{
	// Illegal inputs default to the values in brackets
	int faultTimes;
	std::cout << "Enter a terrain length [50]: ";
	std::cin >> terrainZ;
	if (terrainZ < 50) terrainZ = 50;
	std::cout << "Enter a terrain width [50]: ";
	std::cin >> terrainX;
	if (terrainX < 50) terrainX = 50;
	std::cout << "How many inital faults? [0]: ";
	std::cin >> faultTimes;
	if (faultTimes < 0) return 0;
	return faultTimes;
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)w / (float)h, 1, 500);
}

int main(int argc, char* argv[])
{
	printf("\
Controls \n\
* <space> : do 800 faults \n\
* p	  : toggle passiveFaulting : continuously do 1 fault every update \n\
* l	  : toggle lighting \n\
* k	  : toggle shaders \n\
* w	  : toggle between filled polygons, wireframe, and both \n\
* f	  : toggle between line faulting and circle faulting algorithm \n\
* r	  : reset the scene and camera position \n\
* PgUp, PgDn : move the camera forwards and backwards(alt.keys: Home and End) \n\
* arrowskeys : rotate the camera around its own axis \n\
* mouse   : rotate the camera on its y - axis only(click and drag) \n\
* s,x,z,c : Move first light up, down, backwards, forwards \n\
* g,b,v,n : Move second light up, down, backwards, forwards \n\
* <, >    : Rotate the terrain left and right(alt.keys: comma and period) \n\
* :, ?    : Rotate the terrain on a diagonal axis(alt.keys: semicolon and slash) \n");
	srand(time(NULL));
	int faultTimes = getUserInput();
	initalizeArrays();
	std::cout << "Faulting...";
	faultTerrain(faultTimes);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	glutInitWindowSize(800, 800);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Fault Terrain Modeling");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutSpecialUpFunc(specialUp);
	glutMouseFunc(activeMouse);
	glutMotionFunc(passiveMouse);
	glutTimerFunc(16, update, 0);
	init();

	glutMainLoop();
	return(0);
}
