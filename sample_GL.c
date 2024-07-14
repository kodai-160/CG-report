#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include "GLMetaseq.h"

// テクスチャの定義
#define imageWidth 256
#define imageHeight 256
unsigned char tailTexImage[imageHeight][imageWidth][3];
unsigned char screenTexImage[imageHeight][imageWidth][3];
GLuint tailTexName;
GLuint screenTexName;

const float roomSize = 100.0f; //部屋のサイズ

// グローバル変数
int g_mouseX = 0;
int g_mouseY = 0;
int g_rotX = 0;
int g_rotY = 0;
float step_angle = 0.0; // 足の振り角度
float step_angle_change = 0.1;  // 初期の角度変化量
int walking = 0;        // 歩行中かどうかのフラグ
float g_fovy = 50.0;    // 視野角
float g_posX = 0.0;     // X座標の位置
float g_posY = -100.0;     // Y座標の位置
float g_posZ = 0;
GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };  // 光源の位置

// MQOモデル
MQO_MODEL g_camera, g_head, g_right_hand, g_left_hand, g_right_leg, g_left_leg;

// プロトタイプ宣言
void mySetLight(void);
void updateLightPosition(void);
void Draw(void);
void myReshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void SpecialKeys(int key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void Quit(void);

void updateLightPosition(void) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(g_posX, g_posY, -400.0 + g_posZ);
    glRotatef(g_rotX, 1, 0, 0);
    glRotatef(g_rotY, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, light_position);
    light_position[2] = 1.0;  // Z位置を固定
    light_position[3] = 1.0;  // 方向光源ではなく位置光源として
    glPopMatrix();
}

void mySetLight(void)
{
    GLfloat light_diffuse[] = { 0.9, 0.9, 0.9, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 0.1 };

    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHT0);
}

void setModelMaterial(MQO_MODEL model) {
    GLfloat mat_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat mat_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

// PPM画像の読み込み
void readPPMImage(char* filename, unsigned char image[imageHeight][imageWidth][3])
{
    FILE* fp;
    if (fopen_s(&fp, filename, "rb") != 0) {
        fprintf(stderr, "Cannot open ppm file %s\n", filename);
        exit(1);
    }

    char header[100];
    fgets(header, sizeof(header), fp);
    fgets(header, sizeof(header), fp);
    fgets(header, sizeof(header), fp);
    fread(image, 1, imageWidth * imageHeight * 3, fp);
    fclose(fp);
}

// テクスチャの設定
void setUpTexture(void)
{
    readPPMImage("desert.ppm", tailTexImage);
    readPPMImage("screen.ppm", screenTexImage);

    // 床のテクスチャ設定
    glGenTextures(1, &tailTexName);
    glBindTexture(GL_TEXTURE_2D, tailTexName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tailTexImage);

    // スクリーンのテクスチャ設定
    glGenTextures(1, &screenTexName);
    glBindTexture(GL_TEXTURE_2D, screenTexName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, screenTexImage);
}

void drawRoom() {
    glEnable(GL_TEXTURE_2D);

    // 床
    glBindTexture(GL_TEXTURE_2D, tailTexName);
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 0.0); glVertex3f(roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 1.0); glVertex3f(roomSize, -5.0, roomSize);
    glTexCoord2f(0.0, 1.0); glVertex3f(-roomSize, -5.0, roomSize);
    glEnd();

    // スクリーン
    glBindTexture(GL_TEXTURE_2D, screenTexName);
    glColor3f(1.0, 1.0, 1.0);
    float screenHeight = 45.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 0.0); glVertex3f(roomSize, -5.0, -roomSize);
    glTexCoord2f(1.0, 1.0); glVertex3f(roomSize, screenHeight, -roomSize);
    glTexCoord2f(0.0, 1.0); glVertex3f(-roomSize, screenHeight, -roomSize);
    glEnd();

    glEnd();

}

void Draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.2f, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    drawRoom();

    glTranslatef(g_posX, g_posY, -400.0 + g_posZ);
    glRotatef(g_rotX, 1, 0, 0);
    glRotatef(g_rotY, 0, 1, 0);

    updateLightPosition();
    mySetLight();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    mqoCallModel(g_camera);
    mqoCallModel(g_head);

    if (walking) {
        step_angle += step_angle_change;
        if (step_angle >= 45 || step_angle <= -45) {
            step_angle_change = -step_angle_change;
        }
    }

    // 右手の描画
    glPushMatrix();
    glTranslatef(-65.0, 60.0, 20.0);
    glTranslatef(0, 10, 0);
    glRotatef(step_angle, 1, 0, 0);
    glTranslatef(0, -10, 0);
    mqoCallModel(g_right_hand);
    glPopMatrix();

    // 左足の描画
    glPushMatrix();
    glTranslatef(-20.0, -40.5, 15.0);
    glTranslatef(0, 20, 0);
    glRotatef(-step_angle, 1, 0, 0);
    glTranslatef(0, -20, 0);
    mqoCallModel(g_left_leg);
    glPopMatrix();

    // 左手の描画
    glPushMatrix();
    glTranslatef(62.0, 60.0, 20.0);
    glTranslatef(0, 10, 0);
    glRotatef(-step_angle, 1, 0, 0);
    glTranslatef(0, -10, 0);
    mqoCallModel(g_left_hand);
    glPopMatrix();

    // 右足の描画
    glPushMatrix();
    glTranslatef(20.0, -40.5, 12.0);
    glTranslatef(0, 20, 0);
    glRotatef(step_angle, 1, 0, 0);
    glTranslatef(0, -20, 0);
    mqoCallModel(g_right_leg);
    glPopMatrix();

    glutSwapBuffers();
}

void myReshape(int w, int h)
{
    double znear = 10;
    double zfar = 10000;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_fovy, (double)w / h, znear, zfar);
}

void Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'w':
    case 'W':
        walking = !walking;
        if (walking) step_angle = 0;
        break;
    case '+':
        g_fovy = max(10.0, g_fovy - 5.0);
        break;
    case '-':
        g_fovy = min(100.0, g_fovy + 5.0);
        break;
    case 'q':
    case 'Q':
    case 0x1b:
        exit(0);
    default:
        break;
    }
    glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_UP:
        if (g_posZ + 50.0 <= -100) {  // 上方向に移動するときの上限をチェック
            g_posZ += 10.0;
            printf("Moving forward, g_posZ: %f\n", g_posZ);
        }
        break;
    case GLUT_KEY_DOWN:
        if (g_posZ - 50.0 >= -1000) {  // 下方向に移動するときの下限をチェック
            g_posZ -= 10.0;
            printf("Moving backward, g_posZ: %f\n", g_posZ);
        }
        break;
    case GLUT_KEY_LEFT:
        g_posX -= 10.0;
        break;
    case GLUT_KEY_RIGHT:
        g_posX += 10.0;
        break;
    }
    glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        g_mouseX = x;
        g_mouseY = y;
    }
}

void Motion(int x, int y)
{
    int xd, yd;

    xd = x - g_mouseX;
    yd = y - g_mouseY;

    g_rotX += yd;
    g_rotY += xd;

    g_mouseX = x;
    g_mouseY = y;
}

void Quit(void)
{
    mqoDeleteModel(g_camera);
    mqoDeleteModel(g_head);
    mqoDeleteModel(g_right_hand);
    mqoDeleteModel(g_left_hand);
    mqoDeleteModel(g_right_leg);
    mqoDeleteModel(g_left_leg);
    mqoCleanup();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitWindowPosition(100, 50);
    glutInitWindowSize(640, 480);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("MQO Loader for OpenGL");

    mqoInit();
    g_camera = mqoCreateModel("camera1.mqo", 1.0);
    g_head = mqoCreateModel("head.mqo", 1.0);
    g_right_hand = mqoCreateModel("righthand.mqo", 1.0);
    g_left_hand = mqoCreateModel("lefthand.mqo", 1.0);
    g_right_leg = mqoCreateModel("rightleg.mqo", 1.0);
    g_left_leg = mqoCreateModel("leftleg.mqo", 1.0);

    atexit(Quit);

    glutDisplayFunc(Draw);
    glutIdleFunc(Draw);
    glutReshapeFunc(myReshape);
    setUpTexture();
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKeys);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutMainLoop();

    return 0;
}