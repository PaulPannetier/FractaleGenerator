#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "FractaleParam.hpp"
#include "Vector.hpp"
#include "FractalUpdater.hpp"

struct ShaderProgrammSource
{
    std::string vertexSource;
    std::string fragmentSource;
};

static ShaderProgrammSource ParseShader(const std::string& filePath)
{
    std::ifstream stream(filePath);

    enum class ShaderType
    {
        NONE = -1, Vertex = 0, FRAGMENT = 1
    };

    std::string line;
    ShaderType type = ShaderType::NONE;

    std::stringstream ss[2];

    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::Vertex;
            }
            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
        }
        else
        {
            ss[(int)type] << line << '\n';

        }
    }

    return { ss[0].str(), ss[1].str() };
}

static GLuint CompileShader(unsigned int type, const std::string& source)
{
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    //todo : error handleing
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Fail to compile!" << std::endl;
        std::cout << message << std::endl;

        glDeleteShader(id);
        return 0;
    }

    return id;
}

static GLuint CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    GLuint programm = glCreateProgram();
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(programm, vs);
    glAttachShader(programm, fs);
    glLinkProgram(programm);
    glValidateProgram(programm);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return programm;
}

float getDeltaTime()
{
    static float previousSeconds = 0.0f;
    double currentSeconds = glfwGetTime(); // nombre de secondes depuis le d�but de l'ex�cution
    float dt = currentSeconds - previousSeconds;
    previousSeconds = currentSeconds;
    return dt;
}

void onFrameBufferResize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void onMouseMove(GLFWwindow* window, double x, double y)
{
    // nothing yet
}

void onMouseScroll(GLFWwindow* window, double deltaX, double deltaY)
{
    // nothing yet
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1600, 900, "FractalGenerator", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Error in glewInit" << std::endl;
    }
    glewExperimental = GL_TRUE;

    glfwSetScrollCallback(window, onMouseScroll);
    glfwSetFramebufferSizeCallback(window, onFrameBufferResize);
    glfwSetCursorPosCallback(window, onMouseMove);
    //glfwSwapInterval(0); // D�sactiver la VSync

    GLfloat vertices_positions[] =
    {
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, -1.0f,
    };

    GLuint vertices_indices[] =
    {
        0, 1, 2,
        0, 2, 3,
    };

    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER,  sizeof(vertices_positions), vertices_positions, GL_STATIC_DRAW);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint indicesBuffer;
    glGenBuffers(1, &indicesBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertices_indices), vertices_indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, NULL);
    glEnableVertexAttribArray(0);

    ShaderProgrammSource source = ParseShader("Shaders/julia.shader");

    GLuint juliaShader = CreateShader(source.vertexSource, source.fragmentSource);

    FractalUpdater fractalUpdater = FractalUpdater();
    fractalUpdater.init();

    GLint seedLocation = glGetUniformLocation(juliaShader, "seed");
    GLint windowLocation = glGetUniformLocation(juliaShader, "window");
    GLint maxIterLocation = glGetUniformLocation(juliaShader, "maxIter");
    GLint fadeColorLocation = glGetUniformLocation(juliaShader, "fadeColor");

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        float dt = getDeltaTime();
        fractalUpdater.update(dt);

        glUseProgram(juliaShader);

        FractaleParam& param = fractalUpdater.getFractaleParam();
        glUniform2f(seedLocation, param.seed.x, param.seed.y);
        glUniform4f(windowLocation, param.xMin, param.xMax, param.yMin, param.yMax);
        glUniform1i(maxIterLocation, param.maxIter);
        glUniform3f(fadeColorLocation, param.fadeColor.x, param.fadeColor.y, param.fadeColor.z);

        glBindVertexArray(vao);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &positionBuffer);

    glDeleteProgram(juliaShader);

    glfwTerminate();
    return 0;
}