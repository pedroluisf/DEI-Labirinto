#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glut.h>
#include <GL/glaux.h>

#include "mathlib.h"
#include "studio.h"
#include "mdlviewer.h"

#pragma comment (lib, "glaux.lib")    /* link with Win32 GLAUX lib usada para ler bitmaps */

// função para ler jpegs do ficheiro readjpeg.c
extern "C" int read_JPEG_file(char *, char **, int *, int *, int *);

#ifndef M_PI
		#define M_PI 3.1415926535897932384626433832795
#endif

#define RAD(x)          (M_PI*(x)/180)
#define GRAUS(x)        (180*(x)/M_PI)

#define	GAP					              25

#define MAZE_HEIGHT			          18
#define MAZE_WIDTH			          18

#define	OBJECTO_ALTURA		      0.365
#define OBJECTO_VELOCIDADE	      0.5
#define OBJECTO_ROTACAO		        5
#define OBJECTO_RAIO		      0.12
#define SCALE_HOMER               0.005
#define EYE_ROTACAO			        1

#define NOME_TEXTURA_CUBOS        "Textura.bmp"
#define NOME_TEXTURA_CHAO         "Chao.jpg"
#define NUM_TEXTURAS              2
#define ID_TEXTURA_CUBOS          0
#define ID_TEXTURA_CHAO           1

#define	CHAO_DIMENSAO		      10

#define NUM_JANELAS               2
#define JANELA_TOP                0
#define JANELA_NAVIGATE           1
#define NUM_BONUS				  10

#define BONUS_TEAPOT_SIZE		  0.18
#define BONUS_DONUT_INNER_SIZE    0.08
#define BONUS_DONUT_OUTER_SIZE    0.14


typedef struct teclas_t{
  GLboolean   up,down,left,right;
}teclas_t;

typedef struct pos_t{
    GLfloat    x,y,z;
}pos_t;

typedef struct objecto_t{
    pos_t    pos;  
    GLfloat  dir; 
    GLfloat  vel; 
}objecto_t;

typedef struct camera_t{
    pos_t    eye;  
    GLfloat  dir_long;  // longitude olhar (esq-dir)
    GLfloat  dir_lat;   // latitude olhar	(cima-baixo)
    GLfloat  fov;
}camera_t;

typedef struct ESTADO{
    camera_t      camera;
    GLint         timer;
    GLint         mainWindow,topSubwindow,navigateSubwindow;
    teclas_t      teclas;
    GLboolean     localViewer;
    GLuint        vista[NUM_JANELAS];
    GLuint        semente;
}ESTADO;

typedef struct MODELO{
    GLuint        texID[NUM_JANELAS][NUM_TEXTURAS];
    GLuint        labirinto[NUM_JANELAS];
    GLuint        chao[NUM_JANELAS];
    objecto_t	  objecto;
    GLint         xMouse;
    GLint         yMouse;
    StudioModel   homer[NUM_JANELAS];   // Modelo Homer
    GLboolean     andar;
    GLuint        prev;
}MODELO;

typedef struct TIPOBONUS{
    GLuint        i;
    GLuint        j;
    char		  tipo;
}TIPOBONUS;

/////////////////////////////////////
//variaveis globais

TIPOBONUS bonus[NUM_BONUS];
GLuint donutsrot;
GLuint totalPoints;

ESTADO estado;
MODELO modelo;

GLuint timeElapsedSincefirstColision;

char mazedata[MAZE_HEIGHT][MAZE_WIDTH+1] = {
  "                  ",
  " ******* ******** ",
  " *c      *     c* ",
  " * * *** *d*    * ",
  " * **  * ** * * * ",
  " *     *      * * ",
  " *          *** * ",
  " *  d        *d * ",
  " *     * *** **** ",
  " * *   *c  *    * ",
  " *  c****  *    * ",
  " ********  **** * ",
  " *            * * ",
  " *     *   d  * * ",
  " ** ** *    *** * ",
  " * c *      *d  * ",
  " *******  **** ** ",
  "                  "
};


////////////////////////////////////
/// Iluminação e materiais


void setLight()
{	
	GLfloat light_pos[4] =	{-5.0, 20.0, -8.0, 0.0};
	GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat light_specular[]=	{ 0.5f, 0.5f, 0.5f, 1.0f };
	
    // ligar iluminação
	glEnable(GL_LIGHTING);

	// ligar e definir fonte de luz 0
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,estado.localViewer);
}

void setMaterial()
{
	GLfloat mat_specular[]= { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat mat_shininess = 104;

    // criação automática das componentes Ambiente e Difusa do material a partir das cores
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	
    // definir de outros parâmetros dos materiais estáticamente
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess); 
}


///////////////////////////////////
//// Redisplays

void redisplayTopSubwindow(int width, int height)
{
	// glViewport(botom, left, width, height)
	// define parte da janela a ser utilizada pelo OpenGL
	glViewport(0, 0, (GLint) width, (GLint) height);
	// Matriz Projeccao
	// Matriz onde se define como o mundo e apresentado na janela
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	gluPerspective(60,(GLfloat)width/height,.5,100);
	// Matriz Modelview
	// Matriz onde são realizadas as tranformacoes dos modelos desenhados
	glMatrixMode(GL_MODELVIEW);

}


void reshapeNavigateSubwindow(int width, int height)
{
	// glViewport(botom, left, width, height)
	// define parte da janela a ser utilizada pelo OpenGL
	glViewport(0, 0, (GLint) width, (GLint) height);
	// Matriz Projeccao
	// Matriz onde se define como o mundo e apresentado na janela
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	gluPerspective(estado.camera.fov,(GLfloat)width/height,0.1,50);
	// Matriz Modelview
	// Matriz onde são realizadas as tranformacoes dos modelos desenhados
	glMatrixMode(GL_MODELVIEW);
}

void reshapeMainWindow(int width, int height)
{
	GLint w,h;
    w = (width-GAP*3)*.5;
    h = (height-GAP*2);    
    glutSetWindow(estado.topSubwindow);
    glutPositionWindow(GAP, GAP);
    glutReshapeWindow(w, h);
    glutSetWindow(estado.navigateSubwindow);
    glutPositionWindow(GAP+w+GAP, GAP);
    glutReshapeWindow(w, h);

}

void strokeCenterString(char *str,double x, double y, double z, double s)
{
	int i,n;
	
	n = strlen(str);
	glPushMatrix();
	  glTranslated(x-glutStrokeLength(GLUT_STROKE_ROMAN,(const unsigned char*)str)*0.5*s,y-119.05*0.5*s,z);
	  glScaled(s,s,s);
	  for(i=0;i<n;i++)
		  glutStrokeCharacter(GLUT_STROKE_ROMAN,(int)str[i]);
	glPopMatrix();

}

/////////////////////////////////////
//Modelo

void detectaBonus(GLfloat nx,GLfloat nz)
{
	GLfloat fx = nx + MAZE_WIDTH/2.0 - 1 + 0.5; //1=Translate Centrar o Labirinto 
	GLfloat fz = nz + MAZE_HEIGHT/2.0 - 1  + 0.5; 
	int ix = fx; //Para retirar os decimais
	int iz = fz;
	bool gotBonus=false;

	GLfloat d;
	
	d = pow(fx - ix -0.5,2) + pow(fz - iz -0.5, 2);

	if (mazedata[iz][ix] == 'd'){
		if (d < BONUS_DONUT_OUTER_SIZE * BONUS_DONUT_OUTER_SIZE){
			totalPoints++;
			gotBonus=true;
		}
	} else if (mazedata[iz][ix] == 'c'){
		if (d < BONUS_TEAPOT_SIZE * BONUS_TEAPOT_SIZE){
			totalPoints+=5;
			gotBonus=true;
		}
	}

	if (gotBonus){
		mazedata[iz][ix]=' ';
		for (int i = 0; i<NUM_BONUS; i++){
			if (iz==bonus[i].j && ix == bonus[i].i){
				bonus[i].tipo = ' ';
			}
		}
	}
}

GLboolean detectaColisao(GLfloat nx,GLfloat nz)
{
	int x = nx + MAZE_WIDTH/2.0 - 1 + 0.5; //1=Translate Centrar o Labirinto 0.5=Translate Centro Cubo 
	int z = nz + MAZE_HEIGHT/2.0 - 1 + 0.5; 

	if (mazedata[z][x] == '*'){
		return GL_TRUE;
	}
	return GL_FALSE;
}

GLboolean detectaColisoesHomer(GLfloat nx,GLfloat nz)
{
	// Detecta os vários pontos do perimetro abdominal do Homer
	return detectaColisao(nx + OBJECTO_RAIO * cos(modelo.objecto.dir), nz - OBJECTO_RAIO * sin(modelo.objecto.dir)) 
		|| detectaColisao(nx + OBJECTO_RAIO * cos(modelo.objecto.dir + M_PI/4), nz - OBJECTO_RAIO * sin(modelo.objecto.dir + M_PI/4)) 
		|| detectaColisao(nx + OBJECTO_RAIO * cos(modelo.objecto.dir - M_PI/4), nz - OBJECTO_RAIO * sin(modelo.objecto.dir - M_PI/4)) 
		|| detectaColisao(nx + OBJECTO_RAIO * cos(modelo.objecto.dir + M_PI/2), nz - OBJECTO_RAIO * sin(modelo.objecto.dir + M_PI/2)) 
		|| detectaColisao(nx + OBJECTO_RAIO * cos(modelo.objecto.dir - M_PI/2), nz - OBJECTO_RAIO * sin(modelo.objecto.dir - M_PI/2))
		|| detectaColisao(nx - OBJECTO_RAIO * cos(modelo.objecto.dir), nz - OBJECTO_RAIO * sin(-modelo.objecto.dir)) 
		|| detectaColisao(nx - OBJECTO_RAIO * cos(modelo.objecto.dir + M_PI/4), nz - OBJECTO_RAIO * sin(-modelo.objecto.dir + M_PI/4)) 
		|| detectaColisao(nx - OBJECTO_RAIO * cos(modelo.objecto.dir - M_PI/4), nz - OBJECTO_RAIO * sin(-modelo.objecto.dir - M_PI/4));

}

void desenhaPoligono(GLfloat a[], GLfloat b[], GLfloat c[], GLfloat  d[], GLfloat normal[], GLfloat tx, GLfloat ty)
{
    glBegin(GL_POLYGON);
        glNormal3fv(normal);
		glTexCoord2f(tx,ty);
        glVertex3fv(a);
		glTexCoord2f(tx + 0.25,ty);
        glVertex3fv(b);
		glTexCoord2f(tx + 0.25,ty + 0.25);
        glVertex3fv(c);
		glTexCoord2f(tx,ty + 0.25);
        glVertex3fv(d);
    glEnd();
}


void desenhaCubo(GLuint TexID, GLfloat tx, GLfloat ty)
{
  GLfloat vertices[][3] = { {-0.5,-0.5,-0.5}, 
                            {0.5,-0.5,-0.5}, 
                            {0.5,0.5,-0.5}, 
                            {-0.5,0.5,-0.5}, 
                            {-0.5,-0.5,0.5},  
                            {0.5,-0.5,0.5}, 
                            {0.5,0.5,0.5}, 
                            {-0.5,0.5,0.5}};
  GLfloat normais[][3] = {  {0,0,-1}, 
							{0,1,0}, 
							{-1,0,0}, 
							{1,0,0}, 
							{0,0,1}, 
							{0,-1,0}};

  glBindTexture(GL_TEXTURE_2D, TexID);
  
  desenhaPoligono(vertices[1],vertices[0],vertices[3],vertices[2],normais[0], tx, ty);
  desenhaPoligono(vertices[2],vertices[3],vertices[7],vertices[6],normais[1], tx, ty);
  desenhaPoligono(vertices[3],vertices[0],vertices[4],vertices[7],normais[2], tx, ty);
  desenhaPoligono(vertices[6],vertices[5],vertices[1],vertices[2],normais[3], tx, ty);
  desenhaPoligono(vertices[4],vertices[5],vertices[6],vertices[7],normais[4], tx, ty);
  desenhaPoligono(vertices[5],vertices[4],vertices[0],vertices[1],normais[5], tx, ty);

  glBindTexture(GL_TEXTURE_2D, NULL);
}


void desenhaBussola(int width, int height)  // largura e altura da janela
{
	// Altera viewport e projecção para 2D (copia de um reshape de um projecto 2D)
	glViewport(width-50,0, 50, 50);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-50, 50, -50, 50);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Blending (transparencias)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);

	//desenha Bckground bussola 
	GLint pontosCirculo = 20;
	GLint raioCirculo = 40;
	GLdouble ang = 2 * M_PI / pontosCirculo;
	GLdouble t=0;
	GLint i;
	glColor4f(0.5,0.5,0.5,0.5);

	glBegin(GL_POLYGON);
	for (i=0; i < pontosCirculo; i++){
		glVertex2f(0 + raioCirculo * cos(t), 0 + raioCirculo * sin(t));
		t+=ang;
	}
	glEnd();

	//desenha bussola 2D
	glColor3f(1,0.4,0.4);
	glPushMatrix();
		glRotated(-GRAUS(estado.camera.dir_long) + 90,0.0,0.0,1.0); 
		strokeCenterString("N", 0, 25, 0 , 0.1); // string, x ,y ,z ,scale
		strokeCenterString("S", 0, -25, 0 , 0.1); // string, x ,y ,z ,scale
		//Triangulo
		glColor3f(1,1,1);
		glBegin(GL_POLYGON);
		glVertex2d(0,15);
		glVertex2d(-8,0);
		glVertex2d(8,0);
		glEnd();
	glPopMatrix();
		
	// ropõe estado
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);

	//repõe projecção chamando redisplay
	reshapeNavigateSubwindow(width, height);

}

void desenhaScoreBoard(int width, int height)  // largura e altura da janela
{
  // Altera viewport e projecção
	glViewport(100,height-50, width-200, 50);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-100, 100, -50, 50);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

  // Blending (transparencias)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

  //Mostra Score
	char score[255];
	sprintf_s (score, 255, "Score: %d",totalPoints);
	strokeCenterString(score, 0, 25, 0 , 0.2); // string, x ,y ,z ,scale

  // ropõe estado
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

  //repõe projecção chamando redisplay
  redisplayTopSubwindow(width, height);
}

void desenhaMenu(int width, int height)  // largura e altura da janela
{
  // Altera viewport e projecção
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, 0, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glDisable(GL_DEPTH_TEST);
	//glClear(GL_DEPTH_BUFFER_BIT);

  // Blending (transparencias)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	//BackGround
	glColor4f(1,1,1,0.2);
	glBegin(GL_POLYGON);
	glVertex2d(10,10);
	glVertex2d(10,height - 10);
	glVertex2d(width - 10,height - 10);
	glVertex2d(width - 10,10);
	glEnd();

	//desenha "Menú"
	glColor4f(1,0.4,0.4, 1);
	strokeCenterString("-: Menu :-", 200, height - 50, 0, 0.3); // string, x ,y ,z ,scale

  // ropõe estado
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

	//repõe projecção chamando redisplay
	redisplayTopSubwindow(width, height);
}

void desenhaMenuItem(int x, int y, int width, int height){
	
}

void desenhaModeloDir(objecto_t obj,int width, int height)
{
  // Altera viewport e projecção
	glViewport(0 ,0, 50, 50);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-50, 50, -50, 50);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

  // Blending (transparencias)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

  //desenha Seta
	glPushMatrix();
		glRotated(GRAUS(modelo.objecto.dir) - 90,0.0,0.0,1.0); 
		//Seta
		glColor3f(1,0.4,0.4);
		glBegin(GL_POLYGON);
			glVertex2d(0,25);
			glVertex2d(-12,10);
			glVertex2d(12,10);
		glEnd();
		glBegin(GL_POLYGON);
			glVertex2d(5,10);
			glVertex2d(5,-25);
			glVertex2d(-5,-25);
			glVertex2d(-5,10);
		glEnd();
	glPopMatrix();

  // ropõe estado
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

  //repõe projecção chamando redisplay
  redisplayTopSubwindow(width, height);
}

void desenhaBonus()
{
	glPushMatrix();
		glTranslatef(-MAZE_WIDTH/2.0 + 1, 0.2, -MAZE_HEIGHT/2.0 + 1); //Posição central do mapa para desenhar os bonus
		for (int idx =0; idx<NUM_BONUS;idx++){
			if (bonus[idx].tipo == 'c'){
				glPushMatrix();
					glColor3f(1,0,0);
					glTranslatef(bonus[idx].i, 0, bonus[idx].j);
					glRotatef(donutsrot, 0,1,0);
					glutSolidTeapot(BONUS_TEAPOT_SIZE);
				glPopMatrix();
			} else if (bonus[idx].tipo == 'd') {
				glPushMatrix();
					glColor3f(0,0,1);
					glTranslatef(bonus[idx].i, 0, bonus[idx].j);
					glRotatef(donutsrot, 0,1,0);
					glutSolidTorus(BONUS_DONUT_INNER_SIZE, BONUS_DONUT_OUTER_SIZE, 10, 30);
				glPopMatrix();
			}
		}
    glPopMatrix();
}

void desenhaAngVisao(camera_t *cam)
{
    GLfloat ratio;
    ratio=(GLfloat)glutGet(GLUT_WINDOW_WIDTH)/glutGet(GLUT_WINDOW_HEIGHT); // proporção 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glPushMatrix();
        glTranslatef(cam->eye.x,OBJECTO_ALTURA,cam->eye.z);
        glColor4f(0,0,1,0.2);
        glRotatef(GRAUS(cam->dir_long),0,1,0);

        glBegin(GL_TRIANGLES);
            glVertex3f(0,0,0);
            glVertex3f(5*cos(RAD(cam->fov*ratio*0.5)),0,-5*sin(RAD(cam->fov*ratio*0.5)));
            glVertex3f(5*cos(RAD(cam->fov*ratio*0.5)),0,5*sin(RAD(cam->fov*ratio*0.5)));
        glEnd();
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void desenhaModelo()
{
    glColor3f(0,1,0);
    glutSolidCube(OBJECTO_ALTURA);
    glPushMatrix();
        glColor3f(1,0,0);
        glTranslatef(0,OBJECTO_ALTURA*0.75,0);
        glRotatef(GRAUS(estado.camera.dir_long-modelo.objecto.dir),0,1,0);
        glutSolidCube(OBJECTO_ALTURA*0.5);
    glPopMatrix();
}

void desenhaLabirinto(GLuint TexID)
{
	GLfloat tx, ty;
	GLuint bonus_idx=0;
	srand(estado.semente);
	// código para desenhar o labirinto
	glColor3f(1,1,1);
	glPushMatrix();
		glTranslatef(-MAZE_WIDTH/2.0 + 1, 0.5, -MAZE_HEIGHT/2.0 + 1);
		for(int i=0;i<=MAZE_HEIGHT;i+=1) {
			for(int j=0;j<=MAZE_WIDTH;j+=1) {
				if (mazedata[j][i]=='*'){
					glPushMatrix();
						glTranslatef(i, 0, j);
						tx = rand()%4 * 0.25;
						ty = rand()%4 * 0.25;
						desenhaCubo(TexID, tx, ty);
					glPopMatrix();
				} else if (mazedata[j][i]=='c' || mazedata[j][i]=='d'){
					bonus[bonus_idx].i = i;
					bonus[bonus_idx].j = j;
					bonus[bonus_idx].tipo = mazedata[j][i];
					bonus_idx++;
				}
			}
		}
	glPopMatrix();
}

#define STEP    1

void desenhaChao(GLfloat dimensao, GLuint texID)
{
    // código para desenhar o chão
    GLfloat i,j;
    glBindTexture(GL_TEXTURE_2D, texID);

    glColor3f(0.5f,0.5f,0.5f);
    for(i=-dimensao;i<=dimensao;i+=STEP)
        for(j=-dimensao;j<=dimensao;j+=STEP)
        {
            glBegin(GL_POLYGON);
                glNormal3f(0,1,0);
                glTexCoord2f(1,1);
                glVertex3f(i+STEP,0,j+STEP);
                glTexCoord2f(0,1);
                glVertex3f(i,0,j+STEP);
                glTexCoord2f(0,0);
                glVertex3f(i,0,j);
                glTexCoord2f(1,0);
                glVertex3f(i+STEP,0,j);
            glEnd();
        }
    glBindTexture(GL_TEXTURE_2D, NULL);
}

/////////////////////////////////////
//navigateSubwindow

void motionNavigateSubwindow(int x, int y)
{
	GLfloat difx = (modelo.xMouse - x) * 0.01;
	GLfloat dify = (modelo.yMouse - y) * 0.01;

	estado.camera.dir_long += difx;
	estado.camera.dir_lat += dify;
	//Não permitir à camara dar a volta por cima 
	if (estado.camera.dir_lat>M_PI/3){estado.camera.dir_lat=M_PI/3;};
	//Não permitir à camara descer abaixo do chão
	if (estado.camera.dir_lat<-M_PI/20){estado.camera.dir_lat=-M_PI/20;};

	modelo.xMouse = x;
	modelo.yMouse = y;
}


void mouseNavigateSubwindow(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON){
		if (state==GLUT_DOWN){
			modelo.xMouse = x;
			modelo.yMouse = y;
			glutMotionFunc(motionNavigateSubwindow);
		}else{
			glutMotionFunc(NULL);
		}
	}
}

void setNavigateSubwindowCamera(camera_t *cam, objecto_t obj)
{
	pos_t center;
	if(estado.vista[JANELA_NAVIGATE])
	{
		//1ª Pessoa
		cam->eye.x=obj.pos.x;
		cam->eye.y=obj.pos.y + .2;
		cam->eye.z=obj.pos.z;
		center.x=obj.pos.x + 1 * cos(estado.camera.dir_long) * cos(estado.camera.dir_lat);
		center.y=obj.pos.y + estado.camera.dir_lat;
		center.z=obj.pos.z - 1 * sin(estado.camera.dir_long) * cos(estado.camera.dir_lat);
	}
	else
	{
 		//3ª Pessoa
		cam->eye.x=obj.pos.x - 1 * cos(estado.camera.dir_long) * cos(estado.camera.dir_lat);
		cam->eye.y=obj.pos.y + estado.camera.dir_lat;
		cam->eye.z=obj.pos.z + 1 * sin(estado.camera.dir_long) * cos(estado.camera.dir_lat);
		center.x=obj.pos.x;
		center.y=obj.pos.y + .2;
		center.z=obj.pos.z;
		//detectar colisões camara/ mundo
		if (detectaColisao(cam->eye.x, cam->eye.z)) {
			if (cam->eye.y < 1){
				cam->eye.y = 1.5;
			}
		}
	}
	gluLookAt(cam->eye.x,cam->eye.y,cam->eye.z,center.x,center.y,center.z,0,1,0);
}


void displayNavigateSubwindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glLoadIdentity();

	setNavigateSubwindowCamera(&estado.camera, modelo.objecto);
    setLight();

	glCallList(modelo.labirinto[JANELA_NAVIGATE]);
	glCallList(modelo.chao[JANELA_NAVIGATE]);

	desenhaBonus();

	if(!estado.vista[JANELA_NAVIGATE])
      {
        glPushMatrix();
            glTranslatef(modelo.objecto.pos.x,modelo.objecto.pos.y,modelo.objecto.pos.z);
            glRotatef(GRAUS(modelo.objecto.dir),0,1,0);
            glRotatef(-90,1,0,0);
            glScalef(SCALE_HOMER,SCALE_HOMER,SCALE_HOMER);
            mdlviewer_display(modelo.homer[JANELA_NAVIGATE]);
        glPopMatrix();
      }

	desenhaBussola(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));

	desenhaScoreBoard(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));

	if(!estado.vista[JANELA_NAVIGATE])
    {
		desenhaMenu(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	}

	glutSwapBuffers();

}

/////////////////////////////////////
//topSubwindow
void setTopSubwindowCamera( camera_t *cam,objecto_t obj)
{
	cam->eye.x=obj.pos.x;
	cam->eye.z=obj.pos.z;
	if(estado.vista[JANELA_TOP])
		gluLookAt(obj.pos.x,CHAO_DIMENSAO*.2,obj.pos.z,obj.pos.x,obj.pos.y,obj.pos.z,0,0,-1);
	else
		gluLookAt(obj.pos.x,CHAO_DIMENSAO*2,obj.pos.z,obj.pos.x,obj.pos.y,obj.pos.z,0,0,-1);
}

void displayTopSubwindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    glLoadIdentity();
    setTopSubwindowCamera(&estado.camera,modelo.objecto);
    setLight();

	glCallList(modelo.labirinto[JANELA_TOP]);
	glCallList(modelo.chao[JANELA_TOP]);
	
	desenhaBonus();

	glPushMatrix();		
        glTranslatef(modelo.objecto.pos.x,modelo.objecto.pos.y,modelo.objecto.pos.z);
        glRotatef(GRAUS(modelo.objecto.dir),0,1,0);
        glRotatef(-90,1,0,0);
        glScalef(SCALE_HOMER,SCALE_HOMER,SCALE_HOMER);
        mdlviewer_display(modelo.homer[JANELA_TOP]);
    glPopMatrix();

	desenhaAngVisao(&estado.camera);
	desenhaModeloDir(modelo.objecto,glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	glutSwapBuffers();
}

/////////////////////////////////////
//mainWindow

void redisplayAll(void)
{
    glutSetWindow(estado.mainWindow);
    glutPostRedisplay();
    glutSetWindow(estado.topSubwindow);
    glutPostRedisplay();
    glutSetWindow(estado.navigateSubwindow);
    glutPostRedisplay();
}

void displayMainWindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glutSwapBuffers();
}

void Timer(int value)
{
    GLfloat nx=0,nz=0;
    GLboolean andar=GL_FALSE, emColisao=GL_FALSE;

	GLuint curr = GetTickCount( );
  // calcula velocidade baseado no tempo passado
	float velocidade= modelo.objecto.vel*(curr - modelo.prev )*0.001;

	glutTimerFunc(estado.timer, Timer, 0);
	modelo.prev = curr;    

	if (modelo.homer[JANELA_NAVIGATE].GetSequence() == 20){
		//Saber quanto tempo passou para animação da queda
		//timeElapsedSincefirstColision += (curr - modelo.prev);
		if (curr-timeElapsedSincefirstColision < 1000){
			redisplayAll();
			return;
		} else {
			modelo.homer[JANELA_NAVIGATE].SetSequence(0);  //muda Sequencia usada pelo homer
		}
	}

	if(estado.teclas.up){
		 nx = modelo.objecto.pos.x + (velocidade * cos(modelo.objecto.dir));
		 nz = modelo.objecto.pos.z + (velocidade * -sin(modelo.objecto.dir));
		// calcula nova posição nx,nz
			if(!detectaColisoesHomer(nx,nz)){
				modelo.objecto.pos.x=nx;
				modelo.objecto.pos.z=nz;
			} else {emColisao=GL_TRUE;};
		andar=GL_TRUE;
	}
	if(estado.teclas.down){
		 nx = modelo.objecto.pos.x - (velocidade * cos(modelo.objecto.dir));
		 nz = modelo.objecto.pos.z - (velocidade * -sin(modelo.objecto.dir));
		// calcula nova posição nx,nz
			if(!detectaColisoesHomer(nx,nz)){
				modelo.objecto.pos.x=nx;
				modelo.objecto.pos.z=nz;
			} else {emColisao=GL_TRUE;};
	    andar=GL_TRUE;
	}
	if(estado.teclas.left){
		// rodar camara e objecto
		modelo.objecto.dir += 5 * velocidade;
		estado.camera.dir_long += 5 * velocidade;
	}
	if(estado.teclas.right){
		// rodar camara e objecto
		modelo.objecto.dir -= 5 * velocidade;
		estado.camera.dir_long -= 5 * velocidade;
	}

	//Verifica Bonus
	if (nx!=0 && nz!=0){
		detectaBonus(nx, nz);
	}

    // Sequencias - 0(parado) 3(andar) 20(choque)
	if (emColisao && modelo.homer[JANELA_NAVIGATE].GetSequence() != 20){
		timeElapsedSincefirstColision = curr;
		modelo.homer[JANELA_NAVIGATE].SetSequence(20);  //muda Sequencia usada pelo homer
	}else{
		if (andar && modelo.homer[JANELA_NAVIGATE].GetSequence() != 3){
			modelo.homer[JANELA_NAVIGATE].SetSequence(3);  //muda Sequencia usada pelo homer
		}
		if (!andar && modelo.homer[JANELA_NAVIGATE].GetSequence() != 0){
			modelo.homer[JANELA_NAVIGATE].SetSequence(0);  //muda Sequencia usada pelo homer
		}
	}

	donutsrot+=velocidade*100;

	redisplayAll();
	
}


void imprime_ajuda(void)
{
  printf("\n\nDesenho de um quadrado\n");
  printf("h,H - Ajuda \n");
  printf("******* Diversos ******* \n");
  printf("l,L - Alterna o calculo luz entre Z e eye (GL_LIGHT_MODEL_LOCAL_VIEWER)\n");
  printf("w,W - Wireframe \n");
  printf("f,F - Fill \n");
  printf("******* Movimento ******* \n");
  printf("up  - Acelera \n");
  printf("down- Trava \n");
  printf("left- Vira para a direita\n");
  printf("righ- Vira para a esquerda\n");
  printf("******* Camara ******* \n");
  printf("F1 - Alterna camara da janela da Esquerda \n");
  printf("F2 - Alterna camara da janela da Direita \n");
  printf("PAGE_UP, PAGE_DOWN - Altera abertura da camara \n");
  printf("botao esquerdo + movimento na Janela da Direita altera o olhar \n");
  printf("ESC - Sair\n");
}


void Key(unsigned char key, int x, int y)
{
	switch (key) {
		case 27:
			exit(1);
			break;
    case 'h' :
    case 'H' :
                imprime_ajuda();
            break;
    case 'l':
    case 'L':
        estado.localViewer=!estado.localViewer;
        break;
    case 'w':
    case 'W':
          glutSetWindow(estado.navigateSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          glDisable(GL_TEXTURE_2D);
          glutSetWindow(estado.topSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          glDisable(GL_TEXTURE_2D);
        break;
    case 's':
    case 'S':
          glutSetWindow(estado.navigateSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glEnable(GL_TEXTURE_2D);
          glutSetWindow(estado.topSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glEnable(GL_TEXTURE_2D);
        break;
	}

}
void SpecialKey(int key, int x, int y)
{
	switch (key) {
		case GLUT_KEY_UP: estado.teclas.up =GL_TRUE;
			break;
		case GLUT_KEY_DOWN: estado.teclas.down =GL_TRUE;
			break;
		case GLUT_KEY_LEFT: estado.teclas.left =GL_TRUE;
			break;
		case GLUT_KEY_RIGHT: estado.teclas.right =GL_TRUE;
			break;
		case GLUT_KEY_F1: estado.vista[JANELA_TOP]=!estado.vista[JANELA_TOP];
			break;
		case GLUT_KEY_F2: estado.vista[JANELA_NAVIGATE]=!estado.vista[JANELA_NAVIGATE];
			break;
		case GLUT_KEY_PAGE_UP: 
                  if(estado.camera.fov>20)
                  {
                    estado.camera.fov--;
                    glutSetWindow(estado.navigateSubwindow);
                    reshapeNavigateSubwindow(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
                    redisplayAll();
                  }
			break;
		case GLUT_KEY_PAGE_DOWN: 
                  if(estado.camera.fov<130)
                  {
                    estado.camera.fov++;
                    glutSetWindow(estado.navigateSubwindow);
                    reshapeNavigateSubwindow(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
                    redisplayAll();
                  }
			break;
	}

}
// Callback para interaccao via teclas especiais (largar na tecla)
void SpecialKeyUp(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP: estado.teclas.up =GL_FALSE;
            break;
    case GLUT_KEY_DOWN: estado.teclas.down =GL_FALSE;
            break;
    case GLUT_KEY_LEFT: estado.teclas.left =GL_FALSE;
            break;
    case GLUT_KEY_RIGHT: estado.teclas.right =GL_FALSE;
            break;
  }
}

////////////////////////////////////
// Inicializações



void createDisplayLists(int janelaID)
{
	modelo.labirinto[janelaID]=glGenLists(2);
	glNewList(modelo.labirinto[janelaID], GL_COMPILE);
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
		  desenhaLabirinto(modelo.texID[janelaID][ID_TEXTURA_CUBOS]);
		glPopAttrib();
	glEndList();

	modelo.chao[janelaID]=modelo.labirinto[janelaID]+1;
	glNewList(modelo.chao[janelaID], GL_COMPILE);
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
		  desenhaChao(CHAO_DIMENSAO,modelo.texID[janelaID][ID_TEXTURA_CHAO]);
		glPopAttrib();
	glEndList();

}


///////////////////////////////////
/// Texturas

AUX_RGBImageRec *LoadBMP(char *Filename)				// Loads A Bitmap Image
{
	FILE *File=NULL;									// File Handle

	if (!Filename)										// Make Sure A Filename Was Given
	{
		return NULL;									// If Not Return NULL
	}

	File=fopen(Filename,"r");							// Check To See If The File Exists

	if (File)											// Does The File Exist?
	{
		fclose(File);									// Close The Handle
		return auxDIBImageLoad(Filename);				// Load The Bitmap And Return A Pointer
	}

	return NULL;										// If Load Failed Return NULL
}

void createTextures(GLuint texID[])
{
    AUX_RGBImageRec *TextureImage[1];					// Create Storage Space For The Texture
    char *image;
    int w, h, bpp;

    glGenTextures(NUM_TEXTURAS,texID);

    memset(TextureImage,0,sizeof(void *)*1);           	// Set The Pointer To NULL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (TextureImage[0]=LoadBMP(NOME_TEXTURA_CUBOS))
    {
        // Create MipMapped Texture
        glBindTexture(GL_TEXTURE_2D, texID[ID_TEXTURA_CUBOS]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);

        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
    }
    else
    {
        printf("Textura %s not Found\n",NOME_TEXTURA_CUBOS);
        exit(0);
    }

    if(	read_JPEG_file(NOME_TEXTURA_CHAO, &image, &w, &h, &bpp))
    {
        glBindTexture(GL_TEXTURE_2D, texID[ID_TEXTURA_CHAO]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
    }else{
        printf("Textura %s not Found\n",NOME_TEXTURA_CHAO);
        exit(0);
    }
	glBindTexture(GL_TEXTURE_2D, NULL);
}

void assignCubeTexture(GLfloat x, GLfloat y){
	
}


void init()
{
    GLfloat amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	estado.timer=100;	
	
	estado.camera.eye.x=0;
	estado.camera.eye.y=OBJECTO_ALTURA*2;
	estado.camera.eye.z=0;
	estado.camera.dir_long=0;
	estado.camera.dir_lat=0;
	estado.camera.fov=60;

	estado.localViewer=1;
	estado.vista[JANELA_TOP]=0;
	estado.vista[JANELA_NAVIGATE]=0;

	modelo.objecto.pos.x=0;
	modelo.objecto.pos.y=OBJECTO_ALTURA*.5;
	modelo.objecto.pos.z=0;
	modelo.objecto.dir=0;
	modelo.objecto.vel=OBJECTO_VELOCIDADE;

	modelo.xMouse=modelo.yMouse=-1;
    modelo.andar=GL_FALSE;

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	//Incompatibilidade com a minha grafica
	//glEnable(GL_LINE_SMOOTH);
	//glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);  // por causa do Scale ao Homer

    if(glutGetWindow()==estado.mainWindow)
        glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    else
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
}

/////////////////////////////////////
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowPosition(10, 10);
	glutInitWindowSize(800+GAP*3, 400+GAP*2);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	if ((estado.mainWindow=glutCreateWindow("Labirinto")) == GL_FALSE)
	    exit(1);

    imprime_ajuda();
  
  // Registar callbacks do GLUT da janela principal
	init();
	glutReshapeFunc(reshapeMainWindow);
	glutDisplayFunc(displayMainWindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);

	// criar a sub window topSubwindow
	estado.topSubwindow=glutCreateSubWindow(estado.mainWindow, GAP, GAP, 400, 400);
	init();
	setLight();
	setMaterial();
    createTextures(modelo.texID[JANELA_TOP]);
	createDisplayLists(JANELA_TOP);

    mdlviewer_init( "homer.mdl", modelo.homer[JANELA_TOP] );

	glutReshapeFunc(redisplayTopSubwindow);
	glutDisplayFunc(displayTopSubwindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);

	
	// criar a sub window navigateSubwindow
	estado.navigateSubwindow=glutCreateSubWindow(estado.mainWindow, 400+GAP, GAP, 400, 800);
	init();
	setLight();
	setMaterial();
	
	createTextures(modelo.texID[JANELA_NAVIGATE]);
	createDisplayLists(JANELA_NAVIGATE);
	mdlviewer_init( "homer.mdl", modelo.homer[JANELA_NAVIGATE] );

	glutReshapeFunc(reshapeNavigateSubwindow);
	glutDisplayFunc(displayNavigateSubwindow);
	glutMouseFunc(mouseNavigateSubwindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);
	
	estado.semente = time(NULL);	
	donutsrot=0;
	totalPoints=0;

	glutMainLoop();
	return 0;

}

