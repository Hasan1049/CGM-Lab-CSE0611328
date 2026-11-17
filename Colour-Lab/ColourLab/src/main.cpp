#include "glad.h"
#include "glfw3.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <cmath>

// Colour Lab: a small colour mixing game for kids.
// The left shape shows the target colour. The kid mixes 3 colours from the
// palette on the right and sees the mix on the middle shape.

// Vertex shader: moves every point with the transform matrix
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 transform;\n"
"void main()\n"
"{\n"
"   gl_Position = transform * vec4(aPos, 1.0);\n"
"}\0";

// Fragment shader: paints every point with one colour
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"   FragColor = color;\n"
"}\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 600;
const float PI = 3.14159265f;

// The 4 basic shapes everything is built from
enum { P_RECT, P_CIRCLE, P_TRI, P_TRAP };

unsigned int shaderProgram, VAOs[4], VBOs[4];
int transformLoc, colorLoc, vertexCounts[4];
GLenum drawModes[4];
glm::mat4 projection;   // keeps circles round on a wide window
float animTime = 0.0f;

// The 6 palette colours, in the order of the keys R, B, Y, G, M, C
const glm::vec3 paletteColor[6] = {
    glm::vec3(0.88f, 0.19f, 0.19f), glm::vec3(0.10f, 0.45f, 0.78f), glm::vec3(1.00f, 0.82f, 0.18f),
    glm::vec3(0.18f, 0.63f, 0.27f), glm::vec3(0.41f, 0.24f, 0.84f), glm::vec3(0.27f, 0.67f, 0.98f)
};
const int paletteKeys[6] = { GLFW_KEY_R, GLFW_KEY_B, GLFW_KEY_Y, GLFW_KEY_G, GLFW_KEY_M, GLFW_KEY_C };

// Tiny 5 x 7 pictures of the letters shown on the palette buttons
const char* letters[6][7] = {
    { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" },   // R
    { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." },   // B
    { "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.." },   // Y
    { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###." },   // G
    { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" },   // M
    { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." }    // C
};

// The secret 3 colours (palette numbers) of each shape: sunflower, apple, fish, balloon
const int NUM_SHAPES = 4;
const int solution[NUM_SHAPES][3] = { {2, 2, 0}, {0, 0, 4}, {5, 5, 3}, {4, 4, 5} };

const glm::vec3 emptyColor(0.88f, 0.89f, 0.92f), navyColor(0.16f, 0.18f, 0.35f), skyColor(0.45f, 0.78f, 1.00f);

// Game state
bool finished = false;
int currentShape = 0, chosen[3], chosenCount = 0;
glm::vec3 targetColor;
float lastPressTime[6] = { -10.0f, -10.0f, -10.0f, -10.0f, -10.0f, -10.0f };
bool keyWasDown[GLFW_KEY_LAST + 1] = { false };

// Sends the points of one basic shape to the GPU
void createShape(int type, const float* data, int vertexCount, GLenum mode)
{
    glGenVertexArrays(1, &VAOs[type]);
    glGenBuffers(1, &VBOs[type]);
    glBindVertexArray(VAOs[type]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[type]);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    vertexCounts[type] = vertexCount;
    drawModes[type] = mode;
}

// Draws one basic shape. The group matrix places the whole picture,
// then every part gets its own translate, rotate and scale.
void draw(int type, const glm::mat4& group, float x, float y, float w, float h, glm::vec3 c, float angle = 0.0f, float alpha = 1.0f)
{
    glm::mat4 model = glm::translate(group, glm::vec3(x, y, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));

    glm::mat4 transform = projection * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform4f(colorLoc, c.r, c.g, c.b, alpha);
    glBindVertexArray(VAOs[type]);
    glDrawArrays(drawModes[type], 0, vertexCounts[type]);
}

// Grows a little and comes back, used for the button "pop"
float popScale(float age, float duration, float amount)
{
    return (age >= duration) ? 1.0f : 1.0f + amount * std::sin(age / duration * PI);
}

// The target colour of a shape: the average of its 3 secret colours
glm::vec3 targetColorOf(int index)
{
    glm::vec3 sum(0.0f);
    for (int i = 0; i < 3; i++)
        sum += paletteColor[solution[index][i]];
    return sum / 3.0f;
}

// The player's mix: the average of the colours picked so far (grey if none)
glm::vec3 mixOfChosen()
{
    glm::vec3 sum(0.0f);
    for (int i = 0; i < chosenCount; i++)
        sum += paletteColor[chosen[i]];
    return (chosenCount == 0) ? emptyColor : sum / (float)chosenCount;
}

// Loads a shape: clears the picked colours and sets its target colour
void startShape(int index)
{
    if (index >= NUM_SHAPES)
    {
        finished = true;   // all shapes are done, the cup is shown
        return;
    }
    finished = false;
    currentShape = index;
    chosenCount = 0;
    targetColor = targetColorOf(index);
}

// True when the player's mix has the same colour as the target
bool mixMatchesTarget()
{
    glm::vec3 d = glm::abs(mixOfChosen() - targetColor);
    return d.r < 0.01f && d.g < 0.01f && d.b < 0.01f;
}

// True only on the frame when a key goes down, so holding a key does nothing extra
bool keyPressedOnce(GLFWwindow* window, int key)
{
    bool down = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = down && !keyWasDown[key];
    keyWasDown[key] = down;
    return pressed;
}

// Reads the game keys: colours, Backspace, Enter and Space
void handleGameKeys(GLFWwindow* window)
{
    bool colourKey[6];
    for (int i = 0; i < 6; i++)
        colourKey[i] = keyPressedOnce(window, paletteKeys[i]);
    bool backspaceKey = keyPressedOnce(window, GLFW_KEY_BACKSPACE);
    bool enterKey = keyPressedOnce(window, GLFW_KEY_ENTER);
    bool spaceKey = keyPressedOnce(window, GLFW_KEY_SPACE);

    // After the cup, Space starts the game again
    if (finished)
    {
        if (spaceKey)
            startShape(0);
        return;
    }

    // Colour keys add a colour (3 at most)
    for (int i = 0; i < 6; i++)
        if (colourKey[i] && chosenCount < 3)
        {
            chosen[chosenCount++] = i;
            lastPressTime[i] = animTime;
        }

    // Backspace removes the last colour
    if (backspaceKey && chosenCount > 0)
        chosenCount--;

    // Enter with 3 colours: a match goes to the next shape, otherwise the same shape starts again
    if (enterKey && chosenCount == 3)
        startShape(mixMatchesTarget() ? currentShape + 1 : currentShape);
}

// Draws one of the four pictures inside a box from -0.5 to 0.5.
// "body" is the part the player colours, the other parts have fixed colours.
void drawShapeIcon(int index, const glm::mat4& group, glm::vec3 body)
{
    glm::vec3 black(0.12f, 0.12f, 0.14f), white(1.0f), green(0.20f, 0.55f, 0.22f), brown(0.42f, 0.26f, 0.13f);

    if (index == 0)
    {
        // Sunflower: the petals turn slowly, the black centre and the stem stay fixed
        draw(P_RECT, group, 0.0f, -0.25f, 0.06f, 0.5f, green);
        draw(P_CIRCLE, group, 0.13f, -0.30f, 0.22f, 0.10f, green, 30.0f);
        draw(P_CIRCLE, group, -0.13f, -0.20f, 0.22f, 0.10f, green, -30.0f);
        for (int i = 0; i < 12; i++)
        {
            float a = i * 30.0f + animTime * 25.0f;
            draw(P_CIRCLE, group, 0.26f * std::cos(glm::radians(a)), 0.14f + 0.26f * std::sin(glm::radians(a)), 0.26f, 0.13f, body, a);
        }
        draw(P_CIRCLE, group, 0.0f, 0.14f, 0.30f, 0.30f, black);
    }
    else if (index == 1)
    {
        // Apple: the body is made from 3 overlapping circles
        draw(P_CIRCLE, group, -0.12f, -0.02f, 0.50f, 0.50f, body);
        draw(P_CIRCLE, group, 0.12f, -0.02f, 0.50f, 0.50f, body);
        draw(P_CIRCLE, group, 0.0f, -0.12f, 0.46f, 0.46f, body);
        draw(P_RECT, group, 0.0f, 0.30f, 0.05f, 0.16f, brown, -12.0f);
        draw(P_CIRCLE, group, 0.13f, 0.30f, 0.20f, 0.10f, green, 25.0f);
        draw(P_CIRCLE, group, -0.20f, 0.10f, 0.12f, 0.07f, white, 35.0f, 0.4f);   // shine
    }
    else if (index == 2)
    {
        // Fish: faces right, the body is coloured and the eye is fixed
        draw(P_TRI, group, -0.33f, 0.0f, 0.40f, 0.26f, body, -90.0f);
        draw(P_TRI, group, 0.02f, 0.25f, 0.28f, 0.18f, body);
        draw(P_TRI, group, 0.0f, -0.24f, 0.20f, 0.12f, body, 180.0f);
        draw(P_CIRCLE, group, 0.04f, 0.0f, 0.66f, 0.46f, body);
        draw(P_CIRCLE, group, 0.22f, 0.06f, 0.13f, 0.13f, white);
        draw(P_CIRCLE, group, 0.235f, 0.06f, 0.07f, 0.07f, black);
    }
    else
    {
        // Balloon: the string is fixed
        draw(P_RECT, group, 0.0f, -0.42f, 0.015f, 0.26f, glm::vec3(0.35f, 0.35f, 0.40f));
        draw(P_TRI, group, 0.0f, -0.27f, 0.10f, 0.09f, body, 180.0f);
        draw(P_CIRCLE, group, 0.0f, 0.10f, 0.60f, 0.70f, body);
        draw(P_CIRCLE, group, -0.14f, 0.28f, 0.12f, 0.20f, white, 20.0f, 0.4f);   // shine
    }
}

// Places a shape on screen. Only the fish moves: its whole body goes up and
// down along the y axis, nothing else is moved or rotated.
glm::mat4 makeShapeGroup(int index, float cx, float cy, float size)
{
    float moveY = (index == 2) ? std::sin(animTime * 2.0f) * 0.12f * size : 0.0f;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy + moveY, 0.0f));
    return glm::scale(m, glm::vec3(size, size, 1.0f));
}

// Draws a card with a coloured border
void drawPanel(float cx, float cy, float w, float h, glm::vec3 border)
{
    glm::mat4 identity(1.0f);
    draw(P_RECT, identity, cx, cy, w, h, border);
    draw(P_RECT, identity, cx, cy, w - 0.04f, h - 0.04f, glm::vec3(1.0f, 0.98f, 0.92f));
}

// Draws a letter from the tiny pixel table, one small square per pixel
void drawLetter(const glm::mat4& g, int index, float cell, glm::vec3 color)
{
    for (int row = 0; row < 7; row++)
        for (int col = 0; col < 5; col++)
            if (letters[index][row][col] == '#')
                draw(P_RECT, g, (col - 2) * cell, (3 - row) * cell, cell * 0.95f, cell * 0.95f, color);
}

// Draws one dot per shape at the top left, the dot of the current shape bounces up and down
void drawProgress(float cx, float cy)
{
    glm::mat4 identity(1.0f);
    glm::vec3 yellow(1.0f, 0.80f, 0.15f), green(0.15f, 0.72f, 0.32f);
    for (int i = 0; i < NUM_SHAPES; i++)
    {
        float x = cx + (i - (NUM_SHAPES - 1) / 2.0f) * 0.18f, y = cy;
        if (i == currentShape)
            y += std::fabs(std::sin(animTime * 5.0f)) * 0.04f;
        draw(P_CIRCLE, identity, x, y, 0.115f, 0.115f, navyColor);
        draw(P_CIRCLE, identity, x, y, 0.09f, 0.09f, (i == NUM_SHAPES - 1) ? green : yellow);
    }
}

// Draws the playing screen: target panel, player panel, palette and progress dots
void drawGameScreen()
{
    glm::mat4 identity(1.0f);
    float ax = -1.12f, bx = 0.0f, paletteX = 1.12f;

    // Left panel shows the target colour, middle panel shows the player's mix
    drawPanel(ax, 0.0f, 1.0f, 1.0f, navyColor);
    drawPanel(bx, 0.0f, 1.0f, 1.0f, navyColor);
    drawShapeIcon(currentShape, makeShapeGroup(currentShape, ax, 0.0f, 0.8f), targetColor);
    drawShapeIcon(currentShape, makeShapeGroup(currentShape, bx, 0.0f, 0.8f), mixOfChosen());

    // Right panel is the colour palette, a button pops (scales up) when its key is pressed
    drawPanel(paletteX, 0.0f, 1.0f, 0.9f, navyColor);
    for (int i = 0; i < 6; i++)
    {
        float pop = popScale(animTime - lastPressTime[i], 0.25f, 0.3f);
        glm::mat4 g = glm::translate(identity, glm::vec3(paletteX + ((i % 3) - 1) * 0.32f, 0.2f - (i / 3) * 0.4f, 0.0f));
        g = glm::scale(g, glm::vec3(pop, pop, 1.0f));
        draw(P_RECT, g, 0.0f, 0.0f, 0.27f, 0.27f, navyColor);
        draw(P_RECT, g, 0.0f, 0.0f, 0.25f, 0.25f, paletteColor[i]);
        drawLetter(g, i, 0.026f, glm::vec3(0.13f, 0.14f, 0.20f));
    }

    drawProgress(ax, 0.76f);
}

// Draws the winning cup, it just stands still in the middle
void drawCup()
{
    glm::mat4 g = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.05f, 0.0f)), glm::vec3(1.5f, 1.5f, 1.0f));
    glm::vec3 gold(1.0f, 0.78f, 0.15f), darkGold(0.85f, 0.60f, 0.10f);
    draw(P_CIRCLE, g, -0.30f, 0.20f, 0.28f, 0.28f, gold);       // the two handles are rings:
    draw(P_CIRCLE, g, 0.30f, 0.20f, 0.28f, 0.28f, gold);        // a gold circle with a sky coloured hole
    draw(P_CIRCLE, g, -0.30f, 0.20f, 0.16f, 0.16f, skyColor);
    draw(P_CIRCLE, g, 0.30f, 0.20f, 0.16f, 0.16f, skyColor);
    draw(P_TRAP, g, 0.0f, 0.15f, 0.56f, 0.45f, gold, 180.0f);   // bowl: the trapezoid turned upside down
    draw(P_CIRCLE, g, 0.0f, -0.05f, 0.34f, 0.28f, gold);        // rounded bottom of the bowl
    draw(P_RECT, g, -0.14f, 0.13f, 0.04f, 0.26f, glm::vec3(1.0f), 0.0f, 0.5f);   // shine
    draw(P_RECT, g, 0.0f, -0.22f, 0.10f, 0.20f, darkGold);      // stem
    draw(P_RECT, g, 0.0f, -0.36f, 0.46f, 0.10f, darkGold);      // base
}

// Compiles one shader and prints the error log if something is wrong
unsigned int compileShader(GLenum type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

int main()
{
    // Start GLFW and ask for OpenGL 3.3 core
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);   // fixed size keeps the layout safe

    // Create the window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Colour Lab", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load the OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Compile both shaders and link them into one program
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    transformLoc = glGetUniformLocation(shaderProgram, "transform");
    colorLoc = glGetUniformLocation(shaderProgram, "color");

    // Points of the basic shapes: rectangle (2 triangles), triangle pointing up, trapezoid
    float rectVertices[] = { 0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f,
                             0.5f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
    float triVertices[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f };
    float trapVertices[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.3f, 0.5f, 0.0f,
                             -0.5f, -0.5f, 0.0f, 0.3f, 0.5f, 0.0f, -0.3f, 0.5f, 0.0f };

    // Circle: a triangle fan, the centre first and then 48 points around it
    const int SEGMENTS = 48;
    float circleVertices[(SEGMENTS + 2) * 3] = { 0.0f, 0.0f, 0.0f };
    for (int i = 0; i <= SEGMENTS; i++)
    {
        circleVertices[(i + 1) * 3 + 0] = 0.5f * std::cos(2.0f * PI * i / SEGMENTS);
        circleVertices[(i + 1) * 3 + 1] = 0.5f * std::sin(2.0f * PI * i / SEGMENTS);
    }

    // Send all four shapes to the GPU
    createShape(P_RECT, rectVertices, 6, GL_TRIANGLES);
    createShape(P_CIRCLE, circleVertices, SEGMENTS + 2, GL_TRIANGLE_FAN);
    createShape(P_TRI, triVertices, 3, GL_TRIANGLES);
    createShape(P_TRAP, trapVertices, 6, GL_TRIANGLES);

    // Blending is needed for the see-through shine spots
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Projection that keeps circles round on the wide window
    projection = glm::ortho(-(float)SCR_WIDTH / SCR_HEIGHT, (float)SCR_WIDTH / SCR_HEIGHT, -1.0f, 1.0f);

    // Show the controls in the terminal
    std::cout << "Colour Lab controls\n"
              << "  R B Y G M C : pick a colour (3 colours max)\n"
              << "  Backspace   : remove the last colour\n"
              << "  Enter       : check your mix\n"
              << "  Space       : play again (after the cup)\n"
              << "  Esc         : quit" << std::endl;

    startShape(0);

    // Game loop: read the keys, then draw the current screen
    while (!glfwWindowShouldClose(window))
    {
        animTime = (float)glfwGetTime();
        processInput(window);
        handleGameKeys(window);

        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);   // plain background colour
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        if (finished)
            drawCup();
        else
            drawGameScreen();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    for (int i = 0; i < 4; i++)
    {
        glDeleteVertexArrays(1, &VAOs[i]);
        glDeleteBuffers(1, &VBOs[i]);
    }
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// Closes the window when Esc is pressed
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Keeps the OpenGL viewport in step with the window size
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
