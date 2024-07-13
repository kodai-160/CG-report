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

// グローバル変数
int g_mouseX = 0;
int g_mouseY = 0;
int g_rotX = 0;
int g_rotY = 0;
float step_angle = 0.0; // 足の振り角度
float step_angle_change = 0.05;  // 初期の角度変化量
int walking = 0;        // 歩行中かどうかのフラグ

// MQOモデル
MQO_MODEL g_camera, g_right_hand, g_left_hand, g_right_leg, g_left_leg;

// プロトタイプ宣言
void mySetLight(void);
void Draw(void);
void Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void Quit(void);

void mySetLight(void)
{
    GLfloat light_diffuse[] = { 0.9, 0.9, 0.9, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 0.1 };
    GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHT0);
}

void Draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.2f, 1.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    mySetLight();
    glEnable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(0.0, -100.0, -400.0);
    glRotatef(g_rotX, 1, 0, 0);
    glRotatef(g_rotY, 0, 1, 0);

    mqoCallModel(g_camera);

    if (walking) {
        step_angle += step_angle_change;  // 角度を更新
        if (step_angle >= 45 || step_angle <= -45) {
            step_angle_change = -step_angle_change;  // 角度が範囲外になったら反転
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

    glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glutSwapBuffers();
}


void myReshape(int w, int h)
{
    double znear = 10;
    double fovy = 50;
    double zfar = 10000;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, (double)w / h, znear, zfar);
}

void Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'w':
    case 'W':
        walking = !walking;
        if (walking) step_angle = 0;  // 歩き始めたときに角度をリセット
        break;
    case 'q':
    case 'Q':
    case 0x1b:
        exit(0);
    default:
        break;
    }
    glutPostRedisplay(); // 画面更新要求
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
    g_camera = mqoCreateModel("camera.mqo", 1.0);
    g_right_hand = mqoCreateModel("righthand.mqo", 1.0);
    g_left_hand = mqoCreateModel("lefthand.mqo", 1.0);
    g_right_leg = mqoCreateModel("rightleg.mqo", 1.0);
    g_left_leg = mqoCreateModel("leftleg.mqo", 1.0);

    atexit(Quit);

    glutDisplayFunc(Draw);
    glutIdleFunc(Draw);
    glutReshapeFunc(myReshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutMainLoop();

    return 0;
}