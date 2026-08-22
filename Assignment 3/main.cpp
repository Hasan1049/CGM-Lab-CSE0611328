#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

// Fragment Shader: Cyan color
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);\n"  // Cyan
    "}\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Window title = Student ID (not name, per this assignment)
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "0432410005101049", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // ---- Star vertices: built ONLY with triangles ----
    // A 5-pointed star = 10 triangles, each sharing the center point.
    // Outer points (spikes) alternate with inner points (concave corners).
    const int numPoints = 10;       // 5 outer + 5 inner
    float outerRadius = 0.7f;
    float innerRadius = 0.28f;
    float angleStep = 3.14159265f / 5.0f;   // 36 degrees
    float startAngle = 3.14159265f / 2.0f;  // start pointing up

    float outerX[5], outerY[5], innerX[5], innerY[5];
    for (int i = 0; i < 5; i++)
    {
        float outerAngle = startAngle + i * 2 * angleStep;
        outerX[i] = outerRadius * cos(outerAngle);
        outerY[i] = outerRadius * sin(outerAngle);

        float innerAngle = startAngle + angleStep + i * 2 * angleStep;
        innerX[i] = innerRadius * cos(innerAngle);
        innerY[i] = innerRadius * sin(innerAngle);
    }

    // 10 triangles, each: center -> point -> next point (going around)
    float vertices[10 * 3 * 3]; // 10 triangles * 3 vertices * (x,y,z)
    int idx = 0;
    for (int i = 0; i < 5; i++)
    {
        // Triangle 1: center, outer[i], inner[i]
        vertices[idx++] = 0.0f; vertices[idx++] = 0.0f; vertices[idx++] = 0.0f;
        vertices[idx++] = outerX[i]; vertices[idx++] = outerY[i]; vertices[idx++] = 0.0f;
        vertices[idx++] = innerX[i]; vertices[idx++] = innerY[i]; vertices[idx++] = 0.0f;

        // Triangle 2: center, inner[i], outer[(i+1)%5]
        int next = (i + 1) % 5;
        vertices[idx++] = 0.0f; vertices[idx++] = 0.0f; vertices[idx++] = 0.0f;
        vertices[idx++] = innerX[i]; vertices[idx++] = innerY[i]; vertices[idx++] = 0.0f;
        vertices[idx++] = outerX[next]; vertices[idx++] = outerY[next]; vertices[idx++] = 0.0f;
    }

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Yellow background
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 30);   // 10 triangles = 30 vertices

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
