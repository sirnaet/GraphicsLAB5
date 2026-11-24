// Experiment 7: Menu-driven program to animate a flag using Bezier curves.
//
// Manual requirements covered:
//   - flagpole and a rectangular flag attached along one edge (the pole side)
//   - the top and bottom flag boundaries are CUBIC BEZIER curves (P0..P3), evaluated
//     with the Bernstein weights; the flag surface is a mesh between the two curves
//   - animation: control points P1, P2, P3 are displaced by a travelling sine wave,
//     updated in a timer callback; P0 never moves, so the flag stays attached
//   - right-click menu: Start, Stop, Reset, Change Wind Speed, Change Colour, Exit
//     (plus a Show/Hide control points option to help explain the curves)
//
// How the animation is kept smooth (review question 90):
//   - small, constant time step (16 ms timer, small phase increment)
//   - the wind amplitude ramps up from 0 after Start instead of jumping
//   - changing wind speed changes only the phase INCREMENT, never the phase itself,
//     so there is no sudden jump in the wave
//
// Build (Linux):  g++ -Wall exp7_bezier_flag.cpp -o exp7 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp7_bezier_flag.cpp -o exp7 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp7_bezier_flag.cpp -o exp7 -lfreeglut -lopengl32 -lglu32

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Pt { double x, y; };

// ---------------------------------------------------------------- layout (window pixels)
const int    WIN_W = 900, WIN_H = 600;
const double POLE_X = 150, POLE_BOTTOM = 40, POLE_TOP = 550;
const double FLAG_TOP = 520, FLAG_W = 450, FLAG_H = 220;
const int    SEGMENTS = 60;                 // samples along each Bezier curve

// ---------------------------------------------------------------- animation state
bool   running = false;
double phase = 0.0;                         // wave phase in radians
double amp = 0.0;                           // 0 = flat flag ... 1 = full wave
int    windLevel = 1;                       // 0 slow, 1 medium, 2 fast
const char*  WIND_NAME[3]  = { "Slow", "Medium", "Fast" };
const double WIND_SPEED[3] = { 0.04, 0.09, 0.18 };   // phase increment per frame
const double WIND_AMP[3]   = { 0.7, 1.0, 1.3 };      // stronger wind = bigger waves

// ---------------------------------------------------------------- colour schemes (3 horizontal bands)
struct Scheme { const char* name; float c[3][3]; };
const Scheme SCHEMES[4] = {
    { "Black / Red / Green", { {0.05f, 0.05f, 0.05f}, {0.75f, 0.05f, 0.10f}, {0.00f, 0.50f, 0.20f} } },
    { "Red / White / Blue",  { {0.80f, 0.10f, 0.10f}, {1.00f, 1.00f, 1.00f}, {0.10f, 0.20f, 0.70f} } },
    { "Orange / White / Green", { {1.00f, 0.50f, 0.10f}, {1.00f, 1.00f, 1.00f}, {0.05f, 0.55f, 0.15f} } },
    { "Blue / Yellow / Red", { {0.10f, 0.25f, 0.75f}, {1.00f, 0.85f, 0.10f}, {0.80f, 0.10f, 0.10f} } }
};
int  schemeIdx = 0;
bool showControl = false;

// ---------------------------------------------------------------- Bezier maths
// Cubic Bezier with Bernstein polynomials:
//   B(t) = (1-t)^3 P0 + 3t(1-t)^2 P1 + 3t^2(1-t) P2 + t^3 P3,   0 <= t <= 1
// B(0) = P0 and B(1) = P3 (the curve starts and ends at the outer control points);
// P1 and P2 only pull the curve towards them.
Pt bezier(const Pt P[4], double t)
{
    double u  = 1.0 - t;
    double b0 = u * u * u;
    double b1 = 3.0 * t * u * u;
    double b2 = 3.0 * t * t * u;
    double b3 = t * t * t;
    return { b0 * P[0].x + b1 * P[1].x + b2 * P[2].x + b3 * P[3].x,
             b0 * P[0].y + b1 * P[1].y + b2 * P[2].y + b3 * P[3].y };
}

// Build the control points of the top and bottom edges for the current phase.
// P0 (at the pole) is fixed. P1..P3 get a vertical displacement that grows
// towards the free end and is delayed along the flag, so the wave travels
// from the pole to the free edge.
void buildControlPoints(Pt top[4], Pt bot[4])
{
    const double A1 = 14, A2 = 30, A3 = 38;            // wave amplitude at P1, P2, P3 (pixels)
    const double k = amp * WIND_AMP[windLevel];
    const double dx = FLAG_W / 3.0;
    const double botY = FLAG_TOP - FLAG_H;

    top[0] = { POLE_X,          FLAG_TOP };                              // fixed: attached to pole
    top[1] = { POLE_X + dx,     FLAG_TOP + k * A1 * sin(phase) };
    top[2] = { POLE_X + 2 * dx, FLAG_TOP + k * A2 * sin(phase - 1.3) };
    top[3] = { POLE_X + 3 * dx, FLAG_TOP + k * A3 * sin(phase - 2.6) };

    bot[0] = { POLE_X,          botY };                                  // fixed: attached to pole
    bot[1] = { POLE_X + dx,     botY + k * A1 * sin(phase - 0.3) };
    bot[2] = { POLE_X + 2 * dx, botY + k * A2 * sin(phase - 1.6) };
    bot[3] = { POLE_X + 3 * dx, botY + k * A3 * sin(phase - 2.9) };
}

// ---------------------------------------------------------------- drawing
void drawText(double x, double y, const char* s)
{
    glRasterPos2d(x, y);
    for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

void drawPole()
{
    glColor3f(0.35f, 0.7f, 0.3f);                       // ground
    glRectd(0, 0, WIN_W, POLE_BOTTOM);
    glColor3f(0.45f, 0.45f, 0.45f);                     // pole
    glRectd(POLE_X - 5, POLE_BOTTOM, POLE_X + 5, POLE_TOP);
    glColor3f(0.9f, 0.75f, 0.1f);                       // gold ball on top
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(POLE_X, POLE_TOP + 8);
    for (int i = 0; i <= 24; ++i) {
        double a = 2.0 * M_PI * i / 24.0;
        glVertex2d(POLE_X + 9 * cos(a), POLE_TOP + 8 + 9 * sin(a));
    }
    glEnd();
}

void drawFlag()
{
    Pt top[4], bot[4];
    buildControlPoints(top, bot);

    // sample both Bezier curves
    Pt tp[SEGMENTS + 1], bp[SEGMENTS + 1];
    for (int i = 0; i <= SEGMENTS; ++i) {
        double t = (double)i / SEGMENTS;
        tp[i] = bezier(top, t);
        bp[i] = bezier(bot, t);
    }

    // simple shading from the local slope of the top curve (gives a cloth look)
    float shade[SEGMENTS + 1];
    for (int i = 0; i <= SEGMENTS; ++i) {
        int j = (i < SEGMENTS) ? i : i - 1;
        double dy = tp[j + 1].y - tp[j].y;
        double s = 1.0 + 0.08 * dy;
        if (s < 0.6) s = 0.6;
        if (s > 1.2) s = 1.2;
        shade[i] = (float)s;
    }

    // flag surface: 3 horizontal bands, each a quad strip between the two curves
    for (int band = 0; band < 3; ++band) {
        double v0 = band / 3.0, v1 = (band + 1) / 3.0;   // fractions between top and bottom curve
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= SEGMENTS; ++i) {
            glColor3f(SCHEMES[schemeIdx].c[band][0] * shade[i],
                      SCHEMES[schemeIdx].c[band][1] * shade[i],
                      SCHEMES[schemeIdx].c[band][2] * shade[i]);
            glVertex2d(tp[i].x + (bp[i].x - tp[i].x) * v0, tp[i].y + (bp[i].y - tp[i].y) * v0);
            glVertex2d(tp[i].x + (bp[i].x - tp[i].x) * v1, tp[i].y + (bp[i].y - tp[i].y) * v1);
        }
        glEnd();
    }

    // outline
    glColor3f(0, 0, 0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);                              // top Bezier
    for (int i = 0; i <= SEGMENTS; ++i) glVertex2d(tp[i].x, tp[i].y);
    glEnd();
    glBegin(GL_LINE_STRIP);                              // bottom Bezier
    for (int i = 0; i <= SEGMENTS; ++i) glVertex2d(bp[i].x, bp[i].y);
    glEnd();
    glBegin(GL_LINES);                                   // pole edge and free edge
    glVertex2d(tp[0].x, tp[0].y);              glVertex2d(bp[0].x, bp[0].y);
    glVertex2d(tp[SEGMENTS].x, tp[SEGMENTS].y); glVertex2d(bp[SEGMENTS].x, bp[SEGMENTS].y);
    glEnd();

    // optional: show the control polygons and control points
    if (showControl) {
        const Pt* sets[2] = { top, bot };
        for (int s = 0; s < 2; ++s) {
            glColor3f(0.3f, 0.3f, 0.3f);
            glLineWidth(1.0f);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i < 4; ++i) glVertex2d(sets[s][i].x, sets[s][i].y);
            glEnd();
            glPointSize(7.0f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 4; ++i) {
                if (i == 0 || i == 3) glColor3f(0.9f, 0.1f, 0.1f);   // end points: red
                else                  glColor3f(0.1f, 0.1f, 0.9f);   // interior: blue
                glVertex2d(sets[s][i].x, sets[s][i].y);
            }
            glEnd();
        }
        for (int i = 0; i < 4; ++i) {
            char lab[8];
            snprintf(lab, sizeof lab, "P%d", i);
            glColor3f(0, 0, 0);
            drawText(top[i].x + 6, top[i].y + 8, lab);
        }
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawPole();
    drawFlag();

    char buf[200];
    glColor3f(0, 0, 0);
    snprintf(buf, sizeof buf, "State: %s   Wind: %s   Colour: %s   Control points: %s",
             running ? "RUNNING" : "STOPPED", WIND_NAME[windLevel],
             SCHEMES[schemeIdx].name, showControl ? "shown" : "hidden");
    drawText(10, WIN_H - 20, buf);
    drawText(10, WIN_H - 38, "Right-click for the menu (Start / Stop / Reset / Wind speed / Colour / Exit)");

    glutSwapBuffers();
}

// ---------------------------------------------------------------- animation
void timer(int)
{
    if (running) {
        phase += WIND_SPEED[windLevel];              // small constant step
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI; // sine is periodic, so wrapping is seamless
        amp += 0.02;                                 // ramp wind up smoothly after Start
        if (amp > 1.0) amp = 1.0;
        glutPostRedisplay();
    }
    glutTimerFunc(16, timer, 0);                     // about 60 updates per second
}

// ---------------------------------------------------------------- menu
enum {
    MENU_START = 1, MENU_STOP, MENU_RESET, MENU_CONTROL,
    MENU_WIND_SLOW = 10, MENU_WIND_MED, MENU_WIND_FAST,
    MENU_COL_0 = 20, MENU_COL_1, MENU_COL_2, MENU_COL_3,
    MENU_EXIT = 99
};

void menuHandler(int v)
{
    switch (v) {
    case MENU_START:   running = true;  break;
    case MENU_STOP:    running = false; break;               // flag freezes in its current pose
    case MENU_RESET:   running = false; phase = 0; amp = 0;  // flat flag, default settings
                       windLevel = 1; schemeIdx = 0; break;
    case MENU_CONTROL: showControl = !showControl; break;
    case MENU_WIND_SLOW: case MENU_WIND_MED: case MENU_WIND_FAST:
        windLevel = v - MENU_WIND_SLOW; break;
    case MENU_COL_0: case MENU_COL_1: case MENU_COL_2: case MENU_COL_3:
        schemeIdx = v - MENU_COL_0; break;
    case MENU_EXIT:    exit(0);
    }
    glutPostRedisplay();
}

void createMenus()
{
    int wind = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Slow",   MENU_WIND_SLOW);
    glutAddMenuEntry("Medium", MENU_WIND_MED);
    glutAddMenuEntry("Fast",   MENU_WIND_FAST);

    int col = glutCreateMenu(menuHandler);
    for (int i = 0; i < 4; ++i) glutAddMenuEntry(SCHEMES[i].name, MENU_COL_0 + i);

    glutCreateMenu(menuHandler);
    glutAddMenuEntry("Start", MENU_START);
    glutAddMenuEntry("Stop",  MENU_STOP);
    glutAddMenuEntry("Reset", MENU_RESET);
    glutAddSubMenu("Change Wind Speed", wind);
    glutAddSubMenu("Change Colour", col);
    glutAddMenuEntry("Show / hide control points", MENU_CONTROL);
    glutAddMenuEntry("Exit", MENU_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);       // fixed logical coordinates, scaled to the window
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Exp 7: Bezier flag animation");
    glClearColor(0.6f, 0.82f, 0.95f, 1);                 // sky
    createMenus();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}