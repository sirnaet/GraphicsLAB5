// Experiment 2: Draw a colour cube and spin it using OpenGL transformation matrices.
//
// Manual requirements covered:
//   - 8 vertices at +-1 on each axis, 6 faces with consistent (counter-clockwise) ordering
//   - a different colour for each face
//   - depth testing enabled, perspective (or orthographic) projection
//   - rotation about ONE axis to start with (Y), extended to two or three axes with x / y / z
//   - keyboard controls: pause, resume, change speed
//   - model transform (glRotatef), view transform (gluLookAt), projection (gluPerspective)
//     are kept separate and commented
//
// Controls
//   SPACE        pause / resume       (p = pause, r = resume)
//   + / -        faster / slower
//   x y z        toggle rotation about X / Y / Z (start: Y only)
//   o            toggle perspective / orthographic projection
//   arrow keys   move the camera around the cube (use these for two viewing angles)
//   ESC          quit
//
// Build (Linux):  g++ -Wall exp2_spin_cube.cpp -o exp2 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp2_spin_cube.cpp -o exp2 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp2_spin_cube.cpp -o exp2 -lfreeglut -lopengl32 -lglu32

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---- Cube geometry: 8 corners ----
const GLfloat V[8][3] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},   // back  (z = -1)
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}    // front (z = +1)
};

// ---- 6 faces, vertices listed counter-clockwise as seen from OUTSIDE the cube ----
const int F[6][4] = {
    {4,5,6,7},   // front  (+z)
    {1,0,3,2},   // back   (-z)
    {0,4,7,3},   // left   (-x)
    {5,1,2,6},   // right  (+x)
    {3,7,6,2},   // top    (+y)
    {0,1,5,4}    // bottom (-y)
};

// ---- One colour per face ----
const GLfloat C[6][3] = {
    {1,0,0},   // front  red
    {0,1,0},   // back   green
    {0,0,1},   // left   blue
    {1,1,0},   // right  yellow
    {1,0,1},   // top    magenta
    {0,1,1}    // bottom cyan
};

// ---- Animation state ----
float angle   = 0.0f;       // current rotation angle in degrees
float speed   = 1.0f;       // degrees added per frame
bool  running = true;
bool  rotX = false, rotY = true, rotZ = false;   // one axis first (Y)

// ---- View state ----
bool   perspective = true;
double camAzimuth   = 25.0;   // degrees around the Y axis
double camElevation = 20.0;   // degrees above the XZ plane
const double CAM_DIST = 8.0;
int winW = 700, winH = 700;

void drawCube()
{
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; ++f) {
        glColor3fv(C[f]);
        for (int k = 0; k < 4; ++k) glVertex3fv(V[F[f][k]]);
    }
    glEnd();

    glColor3f(0, 0, 0);                          // black outline
    glLineWidth(2.0f);
    for (int f = 0; f < 6; ++f) {
        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < 4; ++k) glVertex3fv(V[F[f][k]]);
        glEnd();
    }
}

void drawText(int x, int y, const char* s)
{
    // temporary 2D pixel projection just for text, so the 3D set-up is untouched
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glColor3f(0, 0, 0);
    glRasterPos2i(x, y);
    for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

void setProjection()
{
    // PROJECTION transform: how the 3D view volume is mapped to the 2D screen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (double)winW / winH;
    if (perspective) gluPerspective(45.0, aspect, 1.0, 50.0);
    else             glOrtho(-3 * aspect, 3 * aspect, -3, 3, 1.0, 50.0);
    glMatrixMode(GL_MODELVIEW);
}

void display()
{
    // Clear colour AND depth every frame. Old depth values would otherwise
    // make the cube's new position fail the depth test and be partly hidden.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setProjection();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // VIEW transform: place the camera on a sphere around the cube
    double az = camAzimuth * M_PI / 180.0, el = camElevation * M_PI / 180.0;
    gluLookAt(CAM_DIST * cos(el) * sin(az), CAM_DIST * sin(el), CAM_DIST * cos(el) * cos(az),
              0, 0, 0,
              0, 1, 0);

    // MODEL transform: spin the cube.
    // OpenGL multiplies each call on the right, so the LAST rotation written is
    // applied to the vertices FIRST (Z, then Y, then X). Changing the order
    // changes the final orientation because 3D rotations do not commute.
    if (rotX) glRotatef(angle, 1, 0, 0);
    if (rotY) glRotatef(angle, 0, 1, 0);
    if (rotZ) glRotatef(angle, 0, 0, 1);

    drawCube();

    char buf[160];
    snprintf(buf, sizeof buf, "Angle: %.0f deg   Speed: %.1f deg/frame   Axes: %s%s%s   %s   %s",
             angle, speed,
             rotX ? "X " : "", rotY ? "Y " : "", rotZ ? "Z" : "",
             perspective ? "Perspective" : "Orthographic",
             running ? "RUNNING" : "PAUSED");
    drawText(10, winH - 20, buf);

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0, 0, w, h);
}

void timer(int)
{
    if (running) {
        angle += speed;
        if (angle >= 360.0f) angle -= 360.0f;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);                 // about 60 updates per second
}

void keyboard(unsigned char key, int, int)
{
    switch (key) {
    case ' ': running = !running; break;
    case 'p': running = false; break;
    case 'r': running = true;  break;
    case '+': case '=': speed += 0.5f; break;
    case '-': speed -= 0.5f; if (speed < 0.0f) speed = 0.0f; break;
    case 'x': rotX = !rotX; break;
    case 'y': rotY = !rotY; break;
    case 'z': rotZ = !rotZ; break;
    case 'o': perspective = !perspective; break;
    case 27:  exit(0);
    }
    glutPostRedisplay();
}

void special(int key, int, int)
{
    switch (key) {
    case GLUT_KEY_LEFT:  camAzimuth   -= 5; break;
    case GLUT_KEY_RIGHT: camAzimuth   += 5; break;
    case GLUT_KEY_UP:    camElevation += 5; if (camElevation >  85) camElevation =  85; break;
    case GLUT_KEY_DOWN:  camElevation -= 5; if (camElevation < -85) camElevation = -85; break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);   // request a depth buffer
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Exp 2: Spinning colour cube");
    glEnable(GL_DEPTH_TEST);                     // nearer faces hide farther ones
    glClearColor(0.95f, 0.95f, 0.95f, 1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}