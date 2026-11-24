// Experiment 8: Menu-driven polygon filling with the scan-line algorithm.
//
// Manual requirements covered:
//   - polygon accepted from the user (left-click vertices) or loaded from a preset
//     (convex, concave, polygon with horizontal edges)
//   - the polygon boundary is displayed before filling
//   - scan-line fill implemented by hand: for every scan line find the edge
//     intersections, sort them by x, and fill between pairs (even-odd rule)
//   - menu: Enter/Reset polygon, Choose colour, Fill, Clear, Exit
//   - the boundary is redrawn over the fill; vertices and fill colour are displayed
//
// Rules used by the algorithm (needed for your explanation):
//   * Horizontal edges are skipped: they are parallel to the scan line, so they have
//     no single intersection point. Their neighbouring edges still produce the
//     correct span ends.
//   * Edge inclusion convention: an edge (y1 < y2) is active for y1 <= y < y2, i.e.
//     the LOWER endpoint is included and the UPPER endpoint is excluded. A vertex
//     shared by two edges is therefore counted once (or twice for a local min/max,
//     which correctly gives a zero-length span or a pair) and never mis-pairs.
//   * Even-odd rule: after sorting, intersections 0-1 are inside, 2-3 are inside, ...
//
// Usage: right-click for the menu. Choose "Enter new polygon", left-click vertices,
// then right-click > "Finish polygon", then "Fill polygon".
//
// Build (Linux):  g++ -Wall exp8_scanline_fill.cpp -o exp8 -lGL -lGLU -lglut
// Build (macOS):  g++ -Wall exp8_scanline_fill.cpp -o exp8 -framework OpenGL -framework GLUT
// Build (MinGW):  g++ -Wall exp8_scanline_fill.cpp -o exp8 -lfreeglut -lopengl32 -lglu32

#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct IPoint { int x, y; };                // integer pixel coordinates (y up)
struct Span   { int y, x0, x1; };           // filled pixels x0..x1 (inclusive) on row y

std::vector<IPoint> poly;                   // polygon vertices in order
bool closed   = false;                      // polygon finished?
bool entering = false;                      // currently collecting vertices?
std::vector<Span> spans;                    // result of the last fill
bool filled = false;

struct NamedColour { const char* name; float rgb[3]; };
const NamedColour COLOURS[6] = {
    { "Red",    {0.90f, 0.15f, 0.15f} }, { "Green",  {0.15f, 0.70f, 0.25f} },
    { "Blue",   {0.20f, 0.35f, 0.90f} }, { "Yellow", {0.95f, 0.85f, 0.15f} },
    { "Orange", {1.00f, 0.55f, 0.10f} }, { "Purple", {0.60f, 0.25f, 0.80f} }
};
int colourIdx = 0;                          // colour chosen in the menu
int filledColourIdx = 0;                    // colour used by the last Fill

std::string message = "Right-click for the menu.";
int winW = 800, winH = 600;

// ---------------------------------------------------------------- scan-line fill
void scanFill()
{
    spans.clear();
    const int n = (int)poly.size();

    int ymin = poly[0].y, ymax = poly[0].y;             // step 1: vertical extent
    for (int i = 1; i < n; ++i) {
        ymin = std::min(ymin, poly[i].y);
        ymax = std::max(ymax, poly[i].y);
    }

    std::vector<double> xs;
    for (int y = ymin; y <= ymax; ++y) {                // step 2: every scan line
        xs.clear();
        for (int i = 0; i < n; ++i) {
            IPoint a = poly[i], b = poly[(i + 1) % n];
            if (a.y == b.y) continue;                   // skip horizontal edges
            if (a.y > b.y) std::swap(a, b);             // make a the LOWER endpoint
            if (y >= a.y && y < b.y) {                  // lower included, upper excluded
                double x = a.x + (double)(y - a.y) * (b.x - a.x) / (b.y - a.y);
                xs.push_back(x);                        // step 3: intersection x
            }
        }
        std::sort(xs.begin(), xs.end());                // step 4: sort left to right

        for (size_t k = 0; k + 1 < xs.size(); k += 2) { // step 5: fill between pairs
            int xa = (int)std::ceil(xs[k]);
            int xb = (int)std::floor(xs[k + 1]);
            if (xa <= xb) spans.push_back({ y, xa, xb });
        }
    }
}

// all vertices on one line (or identical)? then there is no area to fill
bool isDegenerate()
{
    const size_t n = poly.size();
    size_t j = 1;
    while (j < n && poly[j].x == poly[0].x && poly[j].y == poly[0].y) ++j;
    if (j == n) return true;                            // all points identical
    for (size_t i = 0; i < n; ++i) {
        long long cr = (long long)(poly[j].x - poly[0].x) * (poly[i].y - poly[0].y)
                     - (long long)(poly[j].y - poly[0].y) * (poly[i].x - poly[0].x);
        if (cr != 0) return false;
    }
    return true;
}

// ---------------------------------------------------------------- drawing
void drawText(int x, int y, const char* s)
{
    glRasterPos2i(x, y);
    for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // filled spans (one pixel-high rectangle per span)
    if (filled) {
        glColor3fv(COLOURS[filledColourIdx].rgb);
        for (size_t i = 0; i < spans.size(); ++i)
            glRecti(spans[i].x0, spans[i].y, spans[i].x1 + 1, spans[i].y + 1);
    }

    // boundary drawn OVER the fill so the outline stays clear
    if (!poly.empty()) {
        glColor3f(0, 0, 0);
        glLineWidth(2.0f);
        glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
        for (size_t i = 0; i < poly.size(); ++i) glVertex2i(poly[i].x, poly[i].y);
        glEnd();

        glPointSize(7.0f);
        glColor3f(0.8f, 0, 0);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < poly.size(); ++i) glVertex2i(poly[i].x, poly[i].y);
        glEnd();
    }

    // text: status, colour, vertices
    char buf[160];
    glColor3f(0, 0, 0);
    drawText(10, winH - 20, "Right-click: menu (Enter polygon / Finish / Fill / Clear / Colour / Exit)");
    snprintf(buf, sizeof buf, "Mode: %s    Fill colour: %s",
             entering ? "ENTERING VERTICES (left-click)" : (closed ? "polygon ready" : "no polygon"),
             COLOURS[colourIdx].name);
    drawText(10, winH - 38, buf);
    glColor3fv(COLOURS[colourIdx].rgb);                 // colour swatch
    glRecti(10, winH - 62, 40, winH - 46);

    glColor3f(0, 0, 0);
    drawText(winW - 190, winH - 20, "Vertices:");
    for (size_t i = 0; i < poly.size() && i < 20; ++i) {
        snprintf(buf, sizeof buf, "%zu: (%d, %d)", i + 1, poly[i].x, poly[i].y);
        drawText(winW - 190, winH - 38 - 16 * (int)i, buf);
    }
    if (poly.size() > 20) drawText(winW - 190, winH - 38 - 16 * 20, "...");

    glColor3f(0.7f, 0.1f, 0.1f);
    drawText(10, 12, message.c_str());

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);                             // 1 unit = 1 pixel, y up
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ---------------------------------------------------------------- input
void mouse(int button, int state, int mx, int my)
{
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    if (!entering) {
        message = "Not in input mode: choose 'Enter new polygon' from the menu first.";
    } else if (poly.size() >= 50) {
        message = "Maximum of 50 vertices reached. Choose 'Finish polygon'.";
    } else {
        poly.push_back({ mx, winH - my });              // flip y: window y is downward
        char buf[80];
        snprintf(buf, sizeof buf, "Vertex %zu added at (%d, %d).", poly.size(), mx, winH - my);
        message = buf;
    }
    glutPostRedisplay();
}

enum {
    M_ENTER = 1, M_FINISH, M_FILL, M_CLEAR, M_RESET,
    M_LOAD_CONVEX = 10, M_LOAD_CONCAVE, M_LOAD_HORIZ,
    M_COL0 = 20,
    M_EXIT = 99
};

void loadPreset(const std::vector<IPoint>& p, const char* name)
{
    poly = p; closed = true; entering = false;
    spans.clear(); filled = false;
    message = std::string("Loaded: ") + name + ". Choose 'Fill polygon'.";
}

void menuHandler(int v)
{
    switch (v) {
    case M_ENTER:
        poly.clear(); spans.clear(); filled = false; closed = false; entering = true;
        message = "Left-click to add vertices (at least 3), then menu > Finish polygon.";
        break;
    case M_FINISH:
        if (!entering)                 message = "Nothing to finish. Choose 'Enter new polygon' first.";
        else if (poly.size() < 3) {
            char buf[80];
            snprintf(buf, sizeof buf, "Need at least 3 vertices (you have %zu).", poly.size());
            message = buf;
        } else { closed = true; entering = false; message = "Polygon closed. Choose 'Fill polygon'."; }
        break;
    case M_FILL:
        if (!closed || poly.size() < 3)  message = "Define and finish a polygon (3+ vertices) before filling.";
        else if (isDegenerate())         message = "Invalid polygon: all vertices lie on one line.";
        else {
            scanFill();
            filled = true;
            filledColourIdx = colourIdx;
            char buf[100];
            snprintf(buf, sizeof buf, "Filled with %s: %zu horizontal spans.",
                     COLOURS[colourIdx].name, spans.size());
            message = buf;
        }
        break;
    case M_CLEAR:
        spans.clear(); filled = false;
        message = "Fill cleared (boundary kept).";
        break;
    case M_RESET:
        poly.clear(); spans.clear(); filled = false; closed = false; entering = false;
        message = "Reset. Choose 'Enter new polygon' or load a test polygon.";
        break;
    case M_LOAD_CONVEX:
        loadPreset({ {400, 500}, {560, 400}, {500, 200}, {300, 200}, {240, 400} }, "convex pentagon");
        break;
    case M_LOAD_CONCAVE:
        loadPreset({ {200, 150}, {400, 450}, {600, 150}, {400, 250} }, "concave dart");
        break;
    case M_LOAD_HORIZ:   // T-shaped: several horizontal edges, vertices shared with horizontal edges
        loadPreset({ {200, 400}, {600, 400}, {600, 300}, {450, 300},
                     {450, 150}, {350, 150}, {350, 300}, {200, 300} }, "T-shape with horizontal edges");
        break;
    case M_EXIT: exit(0);
    default:
        if (v >= M_COL0 && v < M_COL0 + 6) {
            colourIdx = v - M_COL0;
            message = std::string("Fill colour set to ") + COLOURS[colourIdx].name +
                      ". Choose 'Fill polygon' to apply it.";
        }
    }
    glutPostRedisplay();
}

void createMenus()
{
    int col = glutCreateMenu(menuHandler);
    for (int i = 0; i < 6; ++i) glutAddMenuEntry(COLOURS[i].name, M_COL0 + i);

    int load = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Convex pentagon", M_LOAD_CONVEX);
    glutAddMenuEntry("Concave dart", M_LOAD_CONCAVE);
    glutAddMenuEntry("T-shape (horizontal edges)", M_LOAD_HORIZ);

    glutCreateMenu(menuHandler);
    glutAddMenuEntry("Enter new polygon (left-click vertices)", M_ENTER);
    glutAddMenuEntry("Finish polygon", M_FINISH);
    glutAddSubMenu("Load test polygon", load);
    glutAddSubMenu("Choose colour", col);
    glutAddMenuEntry("Fill polygon", M_FILL);
    glutAddMenuEntry("Clear fill", M_CLEAR);
    glutAddMenuEntry("Reset all", M_RESET);
    glutAddMenuEntry("Exit", M_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Exp 8: Scan-line polygon fill");
    glClearColor(1, 1, 1, 1);
    createMenus();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}