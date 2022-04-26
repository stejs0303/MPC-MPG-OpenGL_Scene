#include "imageLoad.h"
#include "OBJ_Loader.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <GL/glut.h>

/**************************************************************************************************
* Autor:				Jan Stejskal
* ID:					xstejs31 / 211272
*
* Název:				Park
*
* Vypracované úlohy:	Modelování objektů -							3b
*						Animace - CHECK									1b
*						Osvětlení - CHECK								1b
*						Volný pohyb v horizontální rovině - CHECK		1b
*						Menu - CHECK									2b
*						Výpis textu - tak napůl							2b
*						Ruční svítilna - CHECK							2b
*						Blender model -	CHECK							2b
*						Létání - CHECK									2b
*						Stoupání, klesání - CHECK						1b
*						Hod předmětu - CHECK							2b
*						Simulace kroku - CHECK							2b
*						Tlačítka -										2b
*						Průhlednost - CHECK								1b
*						Projekční paprsek -								1b
*						Neprůchozí objekt - CHECK						2b
*						Texturování - CHECK								2b
*						Bézierovy pláty -								2b
*
* Očekáváný počet bodù:	24
*
* Ovládací klávesy:		w -- pohyb dopředu	q / PageUp -- pohyb nahoru
*						a -- pohyb doleva	e / PageDown -- pohyb dolu 
*						s -- pohyb dozadu	f -- svítilna
*						d -- pohyb doprava	t -- hodit objekt
* 
* Vlastní nápady:		...
*
* Konfigurace:			Windows SDK 10.0.22000.0, Visual studio v143, C++14, Debug x64, bez ladění
****************************************************************************************************/

#define MENU_RESET 1001
#define MENU_EXIT 1002
#define MENU_TIMER_ON 1004
#define MENU_TIMER_OFF 1005
#define MENU_TIMER_RESET 1006
#define MENU_TEXTURES_ON 1007
#define MENU_TEXTURES_OFF 1008

#define	MENU_MOUSE_SENSITIVITY1 1101
#define MENU_MOUSE_SENSITIVITY5 1105
#define MENU_MOUSE_SENSITIVITY7 1107

#define MENU_MOVEMENT_SPEED1 1201
#define MENU_MOVEMENT_SPEED2 1202
#define MENU_MOVEMENT_SPEED4 1204

#define FLYMODE_ON 1301
#define FLYMODE_OFF 1302

#define TEXTURE_HEIGHT 400
#define TEXTURE_WIDTH 400

#define BOBSPEED 230

#define PI 3.14159265359
#define PIOVER180 0.017453292519943f

struct Player
{
	// Pozice ve světě
	float x, y, z;

	float x_new, y_new, z_new;
	float x_old, y_old, z_old;
	float x_temp, y_temp, z_temp;

	double bob = 0;
	bool down = true;

	// Dochází k otáčení kamery?
	bool changeViewDir = false;

	float viewDirXZ;
	float viewDirXY;

	// Citlivost myši
	int sensitivity = 5;
	int movementSpeed = 2;

	// ovládá zapnutí/vypnutí baterky
	bool flashlight = false;

	Player() {}
	Player(float x, float y, float z) : x(x), y(y), z(z) {}

	void operator()(float x, float y, float z)
	{
		this->x = x, this->y = y, this->z = z;
	}
};

struct Throwable
{
	// Pozice ve světě
	float x, y, z;

	// Směr pohybu
	float direction;

	// Vzdálenost 
	int distance = 0;

	GLfloat rotation = 0;

	// Hozený?
	bool thrown = false;

	Throwable() {}
	Throwable(float x, float y, float z) : x(x), y(y), z(z) {}

	void operator()(float x, float y, float z, float direction)
	{
		this->x = x, this->y = y, this->z = z, this->direction = direction;
	}
};

struct Planet
{
	// Pozice ve světě
	float x, y, z;

	float positionAngle;

	Planet() {}
	Planet(float x, float y, float z) : x(x), y(y), z(z) {}

	void operator()(float x, float y, float z)
	{
		this->x = x, this->y = y, this->z = z;
	}
};

struct Collider
{
	float x, y, z, width, height, depth;

	// -1 a +2, aby se nešlo koukat zkrz texturu
	Collider(float x, float y, float z, float width, float height, float depth) :
		x(x - 1), y(y - 1), z(z - 1), width(width + 2), height(height + 2), depth(depth + 2) {}
};

// nastaveni projekce
float w, h;
float fov = 60.0;
float nearPlane = 0.1f;
float farPlane = 600;

// pozice kamery
Player cam(0, 0, 0);

// strukt objektu
Throwable torus;

// Sun & Moon
Planet suon(0, 150, 0);

// ovládá onTimer func
bool animations = false;

// ovládá létání
bool flymode = false;

// ovládá textury
bool textures = true;

// Náhodný offset pro trávu
GLfloat rand_;

unsigned char texture[TEXTURE_HEIGHT][TEXTURE_WIDTH][4];

enum Textures
{
	ground = 0,
	wall = 1,
	wood = 2,
	sun = 3,
	moon = 4,
	bark = 5,
	leaves = 6
};

GLuint textureType[9];

// pole pro potvrzení zmáčknuté klávesy (w,a,s,d,q,e)
bool keyState[6];

// detekce kolize
std::vector<Collider>* colliderArr = new std::vector<Collider>();

GLUquadric* quadric = gluNewQuadric();

// Načítání blender renderů
bool loadedSign = false;
bool loadedBench = false;
bool loadedTree = false;
bool loadedDeadTree = false;

objl::Loader sign;
objl::Loader bench;
objl::Loader tree;
objl::Loader deadTree;

// globální osvětlení
GLfloat SunAmbient[] = { .7f, .7f, .6f, 1 };
GLfloat SunDiffuse[] = { .7f, .7f, .7f, 1.0f };
GLfloat SunSpecular[] = { .7f, .7f, .6f, 1.0f };
GLfloat SunPosition[] = { 0, 150, 0, 0 };
GLfloat SunDirection[] = { 0.0f, -1.0f, 0.0f };

GLfloat MoonAmbient[] = { .3f, .3f, .6f, 1 };
GLfloat MoonDiffuse[] = { .3f, .3f, .4f, 1.0f };
GLfloat MoonSpecular[] = { .1f, .1f, .2f, 1.0f };
GLfloat MoonPosition[] = { 0, -150, 0, 0 };
GLfloat MoonDirection[] = { 0.0f, -1.0f, 0.0f };

GLfloat flashlightAmbient[] = { .3f, .3f, .2f, 1 };
GLfloat flashlightDiffuse[] = { .7f, .7f, .7f, 1.0f };
GLfloat flashlightSpecular[] = { 1, 1, 1, 1.0f };
GLfloat flashlightDirection[] = {0, 0, -1};
// Position a Direction vypocitat

// materiál
GLfloat groundAmbient[] = { .0f, .0f, .0f, 1 };
GLfloat groundDiffuse[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat groundSpecular[] = { .0f, .0f, .0f, 1.0f };
GLfloat groundShininess = 0;

GLfloat wallAmbient[] = { 1, 1, 1, 1.0f };
GLfloat wallDiffuse[] = { 1, 1, 1, 1.0f };
GLfloat wallSpecular[] = { .2f, .2f, .2f, 1.0f };
GLfloat wallShininess = 5;

GLfloat woodAmbient[] = { .4f, .4f, .4f, 1.0f };
GLfloat woodDiffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat woodSpecular[] = { .05f, .05f, .05f, 1.0f };
GLfloat woodShininess = 10;

GLfloat leavesAmbient[] = { .3f, .3f, .3f, 1.0f };
GLfloat leavesDiffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat leavesSpecular[] = { .05f, .05f, .05f, 1.0f };
GLfloat leavesShininess = 10;


bool collision(float x, float y, float z)
{
	for (size_t i = 0; i < colliderArr->size(); i++)
		if ((-x >= colliderArr->at(i).x && -x < colliderArr->at(i).x + colliderArr->at(i).width) &&
			(-z >= colliderArr->at(i).z && -z < colliderArr->at(i).z + colliderArr->at(i).depth) &&
			(-y + 17 >= colliderArr->at(i).y && -y + 17 < colliderArr->at(i).y + colliderArr->at(i).height)) 
			return false;
	return true;
}

void onTimer(int value)
{
	if (animations)
	{
		if (suon.positionAngle >= 360) suon.positionAngle = 0;
		suon.positionAngle += .08;

		glutTimerFunc(15, onTimer, 10);
	}
	if (torus.thrown)
	{
		torus.x += 2 * sin(torus.direction);
		torus.z -= 2 * cos(torus.direction);

		glutTimerFunc(15, onTimer, 10);
	}
	glutPostRedisplay();
}

void menu(int value)
{
	switch (value)
	{
	case MENU_RESET:
		cam(0, 0, 0);
		break;

	case MENU_EXIT:
		exit(1);
		break;

	case MENU_TIMER_ON:
		if (!animations) glutTimerFunc(100, onTimer, value);
		animations = true;
		break;

	case MENU_TIMER_OFF:
		animations = false;
		break;

	case MENU_TIMER_RESET:
		animations = false;
		suon.positionAngle = 0;
		break;

	case MENU_TEXTURES_ON:
		textures = true;
		glEnable(GL_TEXTURE_2D);
		break;

	case MENU_TEXTURES_OFF:
		textures = false;
		glDisable(GL_TEXTURE_2D);
		break;

	case MENU_MOUSE_SENSITIVITY1:
		cam.sensitivity = 1;
		break;

	case MENU_MOUSE_SENSITIVITY5:
		cam.sensitivity = 5;
		break;

	case MENU_MOUSE_SENSITIVITY7:
		cam.sensitivity = 7;
		break;

	case MENU_MOVEMENT_SPEED1:
		cam.movementSpeed = 1;
		break;

	case MENU_MOVEMENT_SPEED2:
		cam.movementSpeed = 2;
		break;

	case MENU_MOVEMENT_SPEED4:
		cam.movementSpeed = 4;
		break;
	
	case FLYMODE_ON:
		flymode = true;
		break;
	
	case FLYMODE_OFF:
		flymode = false;
		break;
	}
}

inline void createMenu(void(*func)(int value))
{
	int idTimer = glutCreateMenu(func);
	glutAddMenuEntry("Spustit", MENU_TIMER_ON);
	glutAddMenuEntry("Zastavit", MENU_TIMER_OFF);
	glutAddMenuEntry("Resetovat", MENU_TIMER_RESET);

	int idTextures = glutCreateMenu(func);
	glutAddMenuEntry("Vypnout", MENU_TEXTURES_OFF);
	glutAddMenuEntry("Zapnout", MENU_TEXTURES_ON);

	int idSensitivity = glutCreateMenu(func);
	glutAddMenuEntry("Vysoka", MENU_MOUSE_SENSITIVITY7);
	glutAddMenuEntry("Stredni", MENU_MOUSE_SENSITIVITY5);
	glutAddMenuEntry("Nizka", MENU_MOUSE_SENSITIVITY1);

	int idMovement = glutCreateMenu(func);
	glutAddMenuEntry("Vysoka", MENU_MOVEMENT_SPEED1);
	glutAddMenuEntry("Stredni", MENU_MOVEMENT_SPEED2);
	glutAddMenuEntry("Nizka", MENU_MOVEMENT_SPEED4);

	int idFlying = glutCreateMenu(func);
	glutAddMenuEntry("Zapnout", FLYMODE_ON);
	glutAddMenuEntry("Vypnout", FLYMODE_OFF);

	glutCreateMenu(func);
	glutPostRedisplay();
	glutAddSubMenu("Animace", idTimer);
	glutAddMenuEntry("Reset pozice ", MENU_RESET);
	glutAddSubMenu("Citlivost mysi", idSensitivity);
	glutAddSubMenu("Rychlost pohybu", idMovement);
	glutAddSubMenu("Létání", idFlying);
	glutAddSubMenu("Textury", idTextures);
	glutAddMenuEntry("Konec", MENU_EXIT);
	glutPostRedisplay();

	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void onReshape(int w_, int h_)
{
	w = w_, h = h_;
	glViewport(0, 0, w_, h_);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, (double)w_ / h_, nearPlane, farPlane);
}

void onKeyDown(unsigned char key, int mx, int my)
{
	if (key < 'a') key += 32;

	switch (key)
	{
	case 'w':
		keyState[0] = true;
		break;

	case 'a':
		keyState[1] = true;
		break;

	case 's':
		keyState[2] = true;
		break;

	case 'd':
		keyState[3] = true;
		break;

	case 'q':
		keyState[4] = true;
		break;

	case 'e':
		keyState[5] = true;
		break;

	case 'f':
		cam.flashlight = cam.flashlight ? false : true;
		if (cam.flashlight) glEnable(GL_LIGHT2);
		else glDisable(GL_LIGHT2);
		break;

	case 't':
		torus(-1 * cam.x, -1 * cam.y, -1 * cam.z, cam.viewDirXZ);
		torus.distance = 0;
		torus.thrown = true;
		glutTimerFunc(15, onTimer, 1);
		break;

	default:
		std::cout << "Not a key\n";
		break;
	}
}

void onKeyUp(unsigned char key, int mx, int my)
{
	if (key < 'a') key += 32;

	switch (key)
	{
	case 'w':
		keyState[0] = false;
		break;

	case 'a':
		keyState[1] = false;
		break;

	case 's':
		keyState[2] = false;
		break;

	case 'd':
		keyState[3] = false;
		break;

	case 'q':
		keyState[4] = false;
		break;

	case 'e':
		keyState[5] = false;
		break;
	}
}

void onSpecialKeyUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_PAGE_UP:
		keyState[4] = false;
		break;
	case GLUT_KEY_PAGE_DOWN:
		keyState[5] = false;
		break;
	}
}

void onSpecialKeyDown(int key, int x, int y)
{
	switch (key)
	{ // Zkopírovat pohyb nahoru a dolu
	case GLUT_KEY_PAGE_UP:
		keyState[4] = true;
		break;
	case GLUT_KEY_PAGE_DOWN:
		keyState[5] = true;
		break;
	}
}

void movement()
{
	float tempx, tempy, tempz;

	if (keyState[0])	// w
	{
		tempx = .1f / cam.movementSpeed * sin(cam.viewDirXZ);
		tempy = flymode ? .1f / cam.movementSpeed * sin(cam.viewDirXY) : 0;
		tempz = .1f / cam.movementSpeed * cos(cam.viewDirXZ);

		if (collision(cam.x - tempx, cam.y + tempy, cam.z + tempz))
		{
			cam.x -= tempx;
			cam.y += tempy;
			cam.z += tempz;
		}
	}
	if (keyState[1])	// a
	{
		tempx = .1f / cam.movementSpeed * cos(cam.viewDirXZ);
		tempz = .1f / cam.movementSpeed * sin(cam.viewDirXZ);

		if (collision(cam.x + tempx, cam.y, cam.z + tempz))
		{
			cam.x += tempx;
			cam.z += tempz;
		}
	}
	if (keyState[2])	// s
	{
		tempx = .1f / cam.movementSpeed * sin(cam.viewDirXZ);
		tempy = flymode ? .1f / cam.movementSpeed * sin(cam.viewDirXY) : 0;
		tempz = .1f / cam.movementSpeed * cos(cam.viewDirXZ);


		if (collision(cam.x + tempx, cam.y - tempy, cam.z - tempz))
		{
			cam.x += tempx;
			cam.y -= tempy;
			cam.z -= tempz;
		}
	}
	if (keyState[3])	// d
	{
		tempx = .1f / cam.movementSpeed * cos(cam.viewDirXZ);
		tempz = .1f / cam.movementSpeed * sin(cam.viewDirXZ);

		if (collision(cam.x - tempx, cam.y, cam.z - tempz))
		{
			cam.x -= tempx;
			cam.z -= tempz;
		}
	}
	if (keyState[4])	// q
	{
		tempy = .1f / cam.movementSpeed;
		if (collision(cam.x, cam.y - tempy, cam.z))
		{
			cam.y -= tempy;
		}
	}
	if (keyState[5])	// e
	{
		tempy = .1f / cam.movementSpeed;
		if (collision(cam.x, cam.y + tempy, cam.z))
		{
			cam.y += cam.y + tempy < 0 ? tempy : 0;
		}
	}

	if ((keyState[0] || keyState[1] || keyState[2] || keyState[3] || cam.bob > 0) && cam.y > -1.0f)
	{
		if (cam.down)
		{
			double temp = cam.bob;
			cam.bob += PI / BOBSPEED;
			cam.y += .5f * (float)(sin(cam.bob) - sin(temp));
			if (cam.bob >= PI) cam.down = false;
		}
		else
		{
			double temp = cam.bob;
			cam.bob -= PI / BOBSPEED;
			cam.y += .5f * (float)(sin(temp) - sin(cam.bob));
			if (cam.bob <= 0) cam.down = true;
		}
	}
	glutPostRedisplay();
}

void onClick(int button, int state, int x, int y)
{
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			cam.changeViewDir = true;
			cam.x_temp = (float)x, cam.y_temp = (float)y;
		}
		else
		{
			cam.changeViewDir = false;
			cam.x_old = cam.x_new;
			cam.y_old = cam.y_new;
		}
	}
}

void onMotion(int x, int y)
{
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	if (cam.changeViewDir)
	{
		cam.x_new = cam.x_old + (x - cam.x_temp) / (8 - cam.sensitivity);		// x-x_temp = x-xx
		cam.y_new = -(cam.y_old + (y - cam.y_temp) / (8 - cam.sensitivity));	// čím vyšší číslo, tím menší přírůstky
		cam.viewDirXZ = cam.x_new * PIOVER180;
		cam.viewDirXY = cam.y_new * PIOVER180;
		glutPostRedisplay();
	}
}

void initTexture()
{
	// Generuje náhodnou texturu "trávy"
	for (int j = 0; j < TEXTURE_HEIGHT; j++)
	{
		unsigned char* pix = texture[j][0];
		for (int i = 0; i < TEXTURE_WIDTH; i++)
		{
			*pix++ = 40;
			*pix++ = 50 + (rand() % 126);
			*pix++ = 0;
			*pix++ = 255;
		}
	}
}

void init()
{
	glFrontFace(GL_CW);					// clockwise fronta
	glPolygonMode(GL_FRONT, GL_FILL);	// fill přední stranu
	glCullFace(GL_BACK);				// nerendruj zadní stranu
	glEnable(GL_CULL_FACE);

	glEnable(GL_LIGHTING);				// osvětlení
	glEnable(GL_NORMALIZE);

	glShadeModel(GL_SMOOTH);

	// Slunko
	glEnable(GL_LIGHT0);				// zdroj 1
	glLightfv(GL_LIGHT0, GL_AMBIENT, SunAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, SunDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, SunSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, SunPosition);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, SunDirection);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 90);

	// Měsíc
	glEnable(GL_LIGHT1);				// zdroj 2
	glLightfv(GL_LIGHT1, GL_AMBIENT, MoonAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, MoonDiffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, MoonSpecular);
	glLightfv(GL_LIGHT1, GL_POSITION, MoonPosition);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, MoonDirection);
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 90);
	// Baterka
	glLightfv(GL_LIGHT2, GL_AMBIENT, flashlightAmbient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, flashlightDiffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, flashlightSpecular);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, flashlightDirection);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 15);

	// Quadric
	gluQuadricDrawStyle(quadric, GLU_FILL);
	gluQuadricOrientation(quadric, GLU_OUTSIDE);
	gluQuadricNormals(quadric, GLU_SMOOTH);
	gluQuadricTexture(quadric, GLU_TRUE);


	loadedSign = sign.LoadFile("obj/cedule.obj");
	loadedBench = bench.LoadFile("obj/lavice.obj");
	loadedTree = tree.LoadFile("obj/strom.obj");

	// !!Přidat textury!!

	// Dynamicky vygenerovaná textura
	initTexture();
	glGenTextures(1, &textureType[ground]);
	glBindTexture(GL_TEXTURE_2D, textureType[ground]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Nastavení země
	glMaterialf(GL_FRONT, GL_SHININESS, groundShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, groundAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, groundDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, groundSpecular);
	
	// Vytvoření textury a uložení do VRAM
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_WIDTH,
		TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);

	// Načtená textura
	setTexture("textury/wall_1024_ivy_05.tga", &textureType[wall], false);
	glBindTexture(GL_TEXTURE_2D, textureType[wall]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Nastavení stěn
	glMaterialf(GL_FRONT, GL_SHININESS, wallShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, wallAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, wallDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, wallSpecular);

	setTexture("textury/drevo.tga", &textureType[wood], false);
	glBindTexture(GL_TEXTURE_2D, textureType[wood]);

	// Nastavení dreva
	glMaterialf(GL_FRONT, GL_SHININESS, woodShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, woodAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, woodDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, woodSpecular);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	setTexture("textury/2k_sun.tga", &textureType[sun], false);
	glBindTexture(GL_TEXTURE_2D, textureType[sun]);

	// Nastavení slunce
	glMaterialf(GL_FRONT, GL_SHININESS, woodShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, woodAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, woodDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, woodSpecular);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	setTexture("textury/2k_moon.tga", &textureType[moon], false);
	glBindTexture(GL_TEXTURE_2D, textureType[moon]);

	// Nastavení měsíce
	glMaterialf(GL_FRONT, GL_SHININESS, woodShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, woodAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, woodDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, woodSpecular);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	setTexture("textury/bark.tga", &textureType[bark], false);
	glBindTexture(GL_TEXTURE_2D, textureType[bark]);

	// Nastavení kůry
	glMaterialf(GL_FRONT, GL_SHININESS, woodShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, woodAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, woodDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, woodSpecular);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	setTexture("textury/leaves.tga", &textureType[leaves], false);
	glBindTexture(GL_TEXTURE_2D, textureType[leaves]);

	// Nastavení listí
	glMaterialf(GL_FRONT, GL_SHININESS, leavesShininess);
	glMaterialfv(GL_FRONT, GL_AMBIENT, leavesAmbient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, leavesDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, leavesSpecular);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_TEXTURE_2D);

	// zed 1
	colliderArr->push_back(Collider(95, 15, -99.8f, 5, 30, 50));
	// zed 2
	colliderArr->push_back(Collider(-0.2f, 15, -100, 100, 30, 5));
	// cedule
	colliderArr->push_back(Collider(-26, 6, -21, 2, 30, 12));
	// lavice 1
	colliderArr->push_back(Collider(79, 2, -75, 10, 17, 22));
	// lavice 2
	colliderArr->push_back(Collider(53, 2, -90, 22, 17, 11));
	// Stromy
	colliderArr->push_back(Collider(80, 5, 20, 10, 20, 10));
	colliderArr->push_back(Collider(-5, 5, 47, 10, 20, 10));
	colliderArr->push_back(Collider(12, 5, 77, 10, 20, 10));
	colliderArr->push_back(Collider(51, 5, 69, 10, 20, 10));

	srand((unsigned int)std::time(0));
	rand_ = rand() % 3;
}

void drawGround(GLfloat x, GLfloat z)
{
	glPushMatrix();

	glTranslatef(0, -15, 0);

	// Výběr textury pro aplikaci
	// Pokud jsou vypnutý textury, aplikuj zelenou
	if (textures)
	{
		glBindTexture(GL_TEXTURE_2D, textureType[ground]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glColor3f(0, .5, 0);
	}

	glBegin(GL_QUADS);
	{
		// Pozice v textuøe + vertex
		glNormal3f(0, 1, 0);
		glTexCoord2f(1.0, 1.0); glVertex3f(x / 2, 0, z / 2);
		glTexCoord2f(0.0, 1.0); glVertex3f(-x / 2, 0, z / 2);
		glTexCoord2f(0.0, 0.0); glVertex3f(-x / 2, 0, -z / 2);
		glTexCoord2f(1.0, 0.0); glVertex3f(x / 2, 0, -z / 2);
	}
	glEnd();

	if (!textures) glEnable(GL_LIGHTING);
	glPopMatrix();
}

void drawSmallStone(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation)
{
	glPushMatrix();

	GLboolean lighting;
	glGetBooleanv(GL_LIGHTING, &lighting);
	if (lighting) glDisable(GL_LIGHTING);

	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	glBegin(GL_QUADS);



	glEnd();

	if (lighting) glEnable(GL_LIGHTING);

	glPopMatrix();
}

void drawBigStone(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation)
{
	glPushMatrix();

	GLboolean lighting;
	glGetBooleanv(GL_LIGHTING, &lighting);
	if (lighting) glDisable(GL_LIGHTING);

	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	glBegin(GL_QUADS);



	glEnd();

	if (lighting) glEnable(GL_LIGHTING);

	glPopMatrix();
}

void drawGrassPatch(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation)
{
	glPushMatrix();

	GLboolean cullface;
	glGetBooleanv(GL_CULL_FACE, &cullface);
	if (cullface) glDisable(GL_CULL_FACE);

	glBindTexture(GL_TEXTURE_2D, textureType[ground]);

	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	glColor3f(0.1f, .8f, 0.1f);

	glBegin(GL_QUADS);

	glNormal3f(0, 0, 1);
	glVertex3f(0 + rand_, 0, 0);
	glVertex3f(0 + rand_, 2, 0);
	glVertex3f(0.5f + rand_, 2, 0);
	glVertex3f(0.5f + rand_, 0, 0);

	glTexCoord2f(0.2f, 0.4f);
	glNormal3f(0, 0, 1);
	glVertex3f(4, 0, 2 + rand_);
	glVertex3f(4, 3, 2 + rand_);
	glVertex3f(4.5f, 3, 2 + rand_);
	glVertex3f(4.5f, 0, 2 + rand_);
	
	glNormal3f(0, 0, 1);
	glVertex3f(-3, 0, 5 - rand_);
	glVertex3f(-3, 2, 5 - rand_);
	glVertex3f(-2.5f, 2, 5 - rand_);
	glVertex3f(-2.5f, 0, 5 - rand_);

	glNormal3f(0, 0, 1);
	glVertex3f(-7, 0, -1);
	glVertex3f(-7, 2, -1);
	glVertex3f(-6.5f, 2, -1);
	glVertex3f(-6.5f, 0, -1);

	glTexCoord2f(0.6f, 0.7f);
	glNormal3f(0, 0, 1);
	glVertex3f(9 - rand_, 0, -6);
	glVertex3f(9 - rand_, 3, -6);
	glVertex3f(9.5f - rand_, 3, -6);
	glVertex3f(9.5f - rand_, 0, -6);

	glNormal3f(0, 0, 1);
	glVertex3f(-4 + rand_, 0, -6);
	glVertex3f(-4 + rand_, 2, -6);
	glVertex3f(-3.5f + rand_, 2, -6);
	glVertex3f(-3.5f + rand_, 0, -6);

	glNormal3f(0, 0, 1);
	glVertex3f(3, 0, 6 - rand_);
	glVertex3f(3, 2, 6 - rand_);
	glVertex3f(3.5f, 2, 6 - rand_);
	glVertex3f(3.5f, 0, 6 - rand_);

	glNormal3f(0, 0, 1);
	glVertex3f(1, 0, -2);
	glVertex3f(1, 3, -2);
	glVertex3f(1.5f, 3, -2);
	glVertex3f(1.5f, 0, -2);
	
	glTexCoord2f(0.45f, 0.4f);
	glNormal3f(0, 0, 1);
	glVertex3f(2.5f, 0, -4);
	glVertex3f(2.5f, 1, -4);
	glVertex3f(3, 1, -4);
	glVertex3f(3, 0, -4);

	glNormal3f(0, 0, 1);
	glVertex3f(-2, 0, 3);
	glVertex3f(-2, 1, 3);
	glVertex3f(-1.5f, 1, 3);
	glVertex3f(-1.5f, 0, 3);

	glNormal3f(0, 0, 1);
	glVertex3f(7.5f, 0, -1);
	glVertex3f(7.5f, 1, -1);
	glVertex3f(8, 1, -1);
	glVertex3f(8, 0, -1);

	glTexCoord2f(0.1f, 0.154f);
	glNormal3f(0, 0, 1);
	glVertex3f(-4.5f - rand_, 0, -3);
	glVertex3f(-4.5f - rand_, 1, -3);
	glVertex3f(-4 - rand_, 1, -3);
	glVertex3f(-4 - rand_, 0, -3);

	glNormal3f(0, 0, 1);
	glVertex3f(-1 , 0, 1);
	glVertex3f(-1 , 1, 1);
	glVertex3f(-1.5f, 1, 1);
	glVertex3f(-1.5f, 0, 1);

	glEnd();

	glRotatef(90, 0, 1, 0);
	glTranslatef(3, 0, 1);

	glBegin(GL_QUADS);

	glTexCoord2f(0.22f, 0.9f);
	glNormal3f(-1, 0, 0);
	glVertex3f(0 + rand_, 0, 0);
	glVertex3f(0 + rand_, 2, 0);
	glVertex3f(0.5f + rand_, 2, 0);
	glVertex3f(0.5f + rand_, 0, 0);

	glNormal3f(-1, 0, 0);
	glVertex3f(4, 0, 2 + rand_);
	glVertex3f(4, 3, 2 + rand_);
	glVertex3f(4.5f, 3, 2 + rand_);
	glVertex3f(4.5f, 0, 2 + rand_);

	glTexCoord2f(0.723f, 0.8f);
	glNormal3f(-1, 0, 0);
	glVertex3f(-3, 0, 5 - rand_);
	glVertex3f(-3, 2, 5 - rand_);
	glVertex3f(-2.5f, 2, 5 - rand_);
	glVertex3f(-2.5f, 0, 5 - rand_);

	glNormal3f(-1, 0, 0);
	glVertex3f(-7, 0, -1);
	glVertex3f(-7, 2, -1);
	glVertex3f(-6.5f, 2, -1);
	glVertex3f(-6.5f, 0, -1);

	glNormal3f(-1, 0, 0);
	glVertex3f(9 - rand_, 0, -6);
	glVertex3f(9 - rand_, 3, -6);
	glVertex3f(9.5f - rand_, 3, -6);
	glVertex3f(9.5f - rand_, 0, -6);
	
	glTexCoord2f(0.13f, 0.74f);
	glNormal3f(-1, 0, 0);
	glVertex3f(-4 + rand_, 0, -6);
	glVertex3f(-4 + rand_, 2, -6);
	glVertex3f(-3.5f + rand_, 2, -6);
	glVertex3f(-3.5f + rand_, 0, -6);

	glNormal3f(-1, 0, 0);
	glVertex3f(3, 0, 6 - rand_);
	glVertex3f(3, 2, 6 - rand_);
	glVertex3f(3.5f, 2, 6 - rand_);
	glVertex3f(3.5f, 0, 6 - rand_);

	glNormal3f(-1, 0, 0);
	glVertex3f(1, 0, -2);
	glVertex3f(1, 3, -2);
	glVertex3f(1.5f, 3, -2);
	glVertex3f(1.5f, 0, -2);

	glTexCoord2f(0.87f, 0.45f);
	glNormal3f(-1, 0, 0);
	glVertex3f(2.5f, 0, -4);
	glVertex3f(2.5f, 1, -4);
	glVertex3f(3, 1, -4);
	glVertex3f(3, 0, -4);

	glNormal3f(-1, 0, 0);
	glVertex3f(-2, 0, 3);
	glVertex3f(-2, 1, 3);
	glVertex3f(-1.5f, 1, 3);
	glVertex3f(-1.5f, 0, 3);

	glTexCoord2f(0.33f, 0.3f);
	glNormal3f(-1, 0, 0);
	glVertex3f(7.5f, 0, -1);
	glVertex3f(7.5f, 1, -1);
	glVertex3f(8, 1, -1);
	glVertex3f(8, 0, -1);

	glNormal3f(-1, 0, 0);
	glVertex3f(-4.5f - rand_, 0, -3);
	glVertex3f(-4.5f - rand_, 1, -3);
	glVertex3f(-4 - rand_, 1, -3);
	glVertex3f(-4 - rand_, 0, -3);

	glNormal3f(-1, 0, 0);
	glVertex3f(-1, 0, 1);
	glVertex3f(-1, 1, 1);
	glVertex3f(-1.5f, 1, 1);
	glVertex3f(-1.5f, 0, 1);

	glEnd();

	if (cullface) glEnable(GL_CULL_FACE);

	glPopMatrix();
}

void drawWall(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height, GLfloat depth)
{
	glPushMatrix();

	if (textures)
	{
		glColor3f(1, 1, 1);
		glBindTexture(GL_TEXTURE_2D, textureType[wall]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glColor3f(0.6f, 0.137f, 0.04f);
	}

	glTranslatef(x, y, z);

	// Přední stěna
	glBegin(GL_QUADS);
	glNormal3f(0, 0, -1);
	glTexCoord2f(1.0, 1.0); glVertex3f(0, 0, 0);
	glTexCoord2f(1.0, 0.0); glVertex3f(0, -height, 0);
	glTexCoord2f(0.0, 0.0); glVertex3f(width, -height, 0);
	glTexCoord2f(0.0, 1.0); glVertex3f(width, 0, 0);
	glEnd();

	// Zadní stěna
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0.0, 1.0); glVertex3f(width, 0, depth);
	glTexCoord2f(0.0, 0.0); glVertex3f(width, -height, depth);
	glTexCoord2f(1.0, 0.0); glVertex3f(0, -height, depth);
	glTexCoord2f(1.0, 1.0); glVertex3f(0, 0, depth);
	glEnd();

	// Levá stěna
	glBegin(GL_QUADS);
	glNormal3f(-1, 0, 0);
	glTexCoord2f(1.0, 1.0); glVertex3f(0, 0, depth);
	glTexCoord2f(1.0, 0.0); glVertex3f(0, -height, depth);
	glTexCoord2f(0.0, 0.0); glVertex3f(0, -height, 0);
	glTexCoord2f(0.0, 1.0); glVertex3f(0, 0, 0);
	glEnd();

	// Pravá stěna
	glBegin(GL_QUADS);
	glNormal3f(1, 0, 0);
	glTexCoord2f(0.0, 1.0); glVertex3f(width, 0, 0);
	glTexCoord2f(0.0, 0.0); glVertex3f(width, -height, 0);
	glTexCoord2f(1.0, 0.0); glVertex3f(width, -height, depth);
	glTexCoord2f(1.0, 1.0); glVertex3f(width, 0, depth);
	glEnd();

	// Vršek
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glTexCoord2f(1.0, 1.0); glVertex3f(width, 0, 0);
	glTexCoord2f(1.0, 0.9f); glVertex3f(width, 0, depth);
	glTexCoord2f(0.9f, 0.9f); glVertex3f(0, 0, depth);
	glTexCoord2f(0.9f, 1.0); glVertex3f(0, 0, 0);
	glEnd();

	// Spodek
	glBegin(GL_QUADS);
	glNormal3f(0, -1, 0);
	glTexCoord2f(0.1f, 0.0); glVertex3f(0, -height, 0);
	glTexCoord2f(0.1f, 0.1f); glVertex3f(0, -height, depth);
	glTexCoord2f(0.0, 0.1f); glVertex3f(width, -height, depth);
	glTexCoord2f(0.0, 0.0); glVertex3f(width, -height, 0);
	glEnd();

	if (!textures) glEnable(GL_LIGHTING);
	glPopMatrix();
}

void drawObjSign(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation)
{
	glPushMatrix();

	GLboolean cullface;
	GLboolean lighting;
	glGetBooleanv(GL_CULL_FACE, &cullface);
	glGetBooleanv(GL_LIGHTING, &lighting);

	if (cullface) glDisable(GL_CULL_FACE);

	glBindTexture(GL_TEXTURE_2D, textureType[wood]);

	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	for (size_t i = 0; i < sign.LoadedMeshes.size(); i++)
	{
		objl::Mesh mesh = sign.LoadedMeshes[i];

		if (mesh.MeshName == "Krychle.002") 
		{
			if (lighting) glDisable(GL_LIGHTING);
			if (textures) glDisable(GL_TEXTURE_2D);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glColor4f(.8f, .8f, .8f, 0.5f);
		}
		else
		{	// Opravit hnědou barvu
			if (textures) glEnable(GL_TEXTURE_2D);
			if (lighting) glEnable(GL_LIGHTING);
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);

			glColor3b(54, 24, 17);
		}
		
		glBegin(GL_QUADS);
		for (size_t j = 0; j < mesh.Vertices.size(); j++)
		{
			glTexCoord2f(mesh.Vertices[j].TextureCoordinate.Y, mesh.Vertices[j].TextureCoordinate.X);
			glNormal3f(mesh.Vertices[j].Normal.X, mesh.Vertices[j].Normal.Y, mesh.Vertices[j].Normal.Z);
			glVertex3f(mesh.Vertices[j].Position.X, mesh.Vertices[j].Position.Y, mesh.Vertices[j].Position.Z);
		}
		glEnd();
	}
	
	if (lighting) glEnable(GL_LIGHTING);
	if (cullface) glEnable(GL_CULL_FACE);

	glPopMatrix();
}

void drawObjBench(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation)
{
	glPushMatrix();

	GLboolean cullface;
	glGetBooleanv(GL_CULL_FACE, &cullface);

	if (cullface) glDisable(GL_CULL_FACE);
	if (textures) glDisable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, textureType[wood]);
	
	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	for (size_t i = 0; i < bench.LoadedMeshes.size(); i++)
	{
		objl::Mesh mesh = bench.LoadedMeshes[i];
		
		if (strstr(mesh.MeshName.c_str(), "Drevo"))
		{
			if (textures)
			{
				glEnable(GL_TEXTURE_2D);
				glColor3b(72, 64, 51);
			}
			else glColor3b(54, 24, 17);
		}
		else 
		{	
			if (textures) glDisable(GL_TEXTURE_2D);
			glColor3f(.3f, .3f, .3f);
		}

		glBegin(GL_TRIANGLES);
		for (size_t j = 0; j < mesh.Vertices.size(); j++)
		{
			glTexCoord2f(mesh.Vertices[j].TextureCoordinate.Y, mesh.Vertices[j].TextureCoordinate.X);
			glNormal3f(mesh.Vertices[j].Normal.X, mesh.Vertices[j].Normal.Y, mesh.Vertices[j].Normal.Z);
			glVertex3f(mesh.Vertices[j].Position.X, mesh.Vertices[j].Position.Y, mesh.Vertices[j].Position.Z);
		}
		glEnd();
	}

	if (textures) glEnable(GL_TEXTURE_2D);
	if (cullface) glEnable(GL_CULL_FACE);

	glPopMatrix();
}

void drawObjTree(GLfloat x, GLfloat y, GLfloat z, GLfloat rotation, GLfloat scale)
{
	glPushMatrix();

	GLboolean cullface;
	glGetBooleanv(GL_CULL_FACE, &cullface);
	if (cullface) glDisable(GL_CULL_FACE);

	glTranslatef(x, y, z);
	glRotatef(rotation, 0, 1, 0);

	for (size_t i = 0; i < tree.LoadedMeshes.size(); i++)
	{
		objl::Mesh mesh = tree.LoadedMeshes[i];

		if (strstr(mesh.MeshName.c_str(), "Kmen"))
			glBindTexture(GL_TEXTURE_2D, textureType[bark]);
		else		
			glBindTexture(GL_TEXTURE_2D, textureType[leaves]);

		glBegin(GL_TRIANGLES);
		for (size_t j = 0; j < mesh.Vertices.size(); j++)
		{
			glTexCoord2f(mesh.Vertices[j].TextureCoordinate.Y, mesh.Vertices[j].TextureCoordinate.X);
			glNormal3f(mesh.Vertices[j].Normal.X, mesh.Vertices[j].Normal.Y, mesh.Vertices[j].Normal.Z);
			glVertex3f(mesh.Vertices[j].Position.X/scale, mesh.Vertices[j].Position.Y/scale, mesh.Vertices[j].Position.Z/scale);
		}
		glEnd();
	}

	if (cullface) glEnable(GL_CULL_FACE);

	glPopMatrix();
}

void daycycle()
{
	glPushMatrix();

	glTranslatef(0, -15, 0);
	glRotatef(-suon.positionAngle, 0, 0, 1);

	glLightfv(GL_LIGHT0, GL_POSITION, SunPosition);
	glLightfv(GL_LIGHT1, GL_POSITION, MoonPosition);

	glPushMatrix();
	glRotatef(suon.positionAngle, 0, 0, 1);
	
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, MoonDirection);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, SunDirection);
	glPopMatrix();

	glPushMatrix();

	glBindTexture(GL_TEXTURE_2D, textureType[sun]);

	// Slunce
	glTranslatef(suon.x, suon.y, suon.z);
	glColor3f(1.0, 1.0, 0.0);
	gluSphere(quadric, 10.0, 20, 50);

	glPopMatrix();
	glPushMatrix();

	glBindTexture(GL_TEXTURE_2D, textureType[moon]);

	// Měsíc
	glTranslatef(-suon.x, -suon.y, -suon.z);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(quadric, 5.0, 20, 25);

	glPopMatrix();
	glPopMatrix();
}

bool throwTorus()
{
	GLboolean isLighting;
	glGetBooleanv(GL_LIGHTING, &isLighting);
	if (isLighting) glDisable(GL_LIGHTING);

	glPushMatrix();

	glColor3f(.2f, .7f, .1f);
	glTranslatef(torus.x, 0, torus.z);
	glRotatef(torus.rotation++, 0, 1, 1);
	glutSolidTorus(1, 2, 8, 20);
	glutPostRedisplay();

	glPopMatrix();

	if (isLighting) glEnable(GL_LIGHTING);
	return true;
}

void flashlight()
{
	glPushMatrix();
	GLfloat pos[] = { cam.x, cam.y, cam.z };
	glLightfv(GL_LIGHT2, GL_POSITION, pos);
	glPopMatrix();
}

void informations()
{
	glPushMatrix();

	glColor3f(0, 0, 0);
	std::string text = "x: " + std::to_string(std::ceil(cam.x * 100) / 100) + " sklon: " + std::to_string(std::ceil(cam.y_new * 100) / 100);
	std::string text2 = "y: " + std::to_string(std::ceil(cam.y * 100) / 100) + " svitilna: " + (cam.flashlight ? "true" : "false");
	std::string text3 = "z: " + std::to_string(std::ceil(cam.z * 100) / 100) + " letani: " + (flymode ? "true" : "false");

	glRasterPos3f(0, 70, -100);
	for (int i = 0; i < text.length(); i++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, text[i]);

	glRasterPos3f(0, 66, -100);
	for (int i = 0; i < text2.length(); i++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, text2[i]);

	glRasterPos3f(0, 62, -100);
	for (int i = 0; i < text3.length(); i++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, text3[i]);

	glPopMatrix();
}

void onRedraw()
{
	// Parametry pro mazání roviny
	glClearColor(.8f, .8f, .8f, 0.0f);
	glClearDepth(1);

	// mazání roviny
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// inicializace MODELVIEW
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// provedení transformace sceny (rotace/pohyb)
	// omezení na +- 90° ve vertikálním směru
	glRotatef(cam.y_new <= -90 ? cam.y_new = -89.9f : cam.y_new > 90 ? cam.y_new = 89.9f : cam.y_new, 1, 0, 0);
	glRotatef(cam.x_new, 0, 1, 0);
	glTranslatef(cam.x, cam.y, cam.z);
	

	// !!VLASTNÍ!!
	drawGround(TEXTURE_HEIGHT / 2, TEXTURE_WIDTH / 2);

	daycycle();

	drawWall(95, 15, -99.8f, 5, 30, 50);
	drawWall(-0.2f, 15, -100, 100, 30, 5);

	drawGrassPatch(87, -15, -87, 0);
	drawGrassPatch(87, -15, -70, 90);
	drawGrassPatch(87, -15, -60, 0);
	drawGrassPatch(87, -15, -52, 90);
	drawGrassPatch(70, -15, -87, 90);
	drawGrassPatch(60, -15, -87, 0);
	drawGrassPatch(52, -15, -87, 180);

	drawGrassPatch(60, -15, -52, 0);
	drawGrassPatch(55, -15, -47, 90);
	drawGrassPatch(51, -15, -60, 0);

	if (loadedBench) drawObjBench(85, -11, -65, 90);
	if (loadedBench) drawObjBench(63, -11, -85, 180);

	if (loadedTree) drawObjTree(56, -15, 74, 0, 1);
	if (loadedTree) drawObjTree(17, -15, 82, 180, 1.4f);
	if (loadedTree) drawObjTree(0, -15, 52, 90, 1);
	if (loadedTree) drawObjTree(85, -15, 25, 135, 1.5f);

	if (loadedSign) drawObjSign(-25, -4, -15, 90);

	if (torus.thrown) (torus.distance++ < farPlane * 8) ? throwTorus() : torus.thrown = false;

	if (cam.flashlight) flashlight();

	movement();

	informations();

	glFlush();
	glutSwapBuffers();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutInitWindowSize(1280, 720);
	glutInitWindowPosition(50, 50);

	glutCreateWindow("xstejs31 - MPG projekt");

	glutDisplayFunc(onRedraw);
	glutReshapeFunc(onReshape);
	glutMouseFunc(onClick);
	glutMotionFunc(onMotion);
	glutKeyboardFunc(onKeyDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialFunc(onSpecialKeyDown);
	glutSpecialUpFunc(onSpecialKeyUp);
	createMenu(menu);

	glutIgnoreKeyRepeat(1);
	init();

	glutMainLoop();

	gluDeleteQuadric(quadric);
	delete colliderArr;
	return 0;
}