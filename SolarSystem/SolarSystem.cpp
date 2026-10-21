//////////////////////////////////////////////////////////////////////////////
//
//  SolarSystem.cpp
//  任务一：日地月运动模型动画
//
//////////////////////////////////////////////////////////////////////////////

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

#include <iostream>
#include <vector>
#include <cmath>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 12.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 球体 VAO/VBO/EBO
GLuint sphereVAO, sphereVBO, sphereEBO;
unsigned int indexCount;

// 运动参数
const float SUN_RADIUS = 1.5f;
const float EARTH_RADIUS = 0.5f;
const float MOON_RADIUS = 0.2f;
const float EARTH_ORBIT_RADIUS = 6.0f;
const float MOON_ORBIT_RADIUS = 1.5f;
const float EARTH_ORBIT_SPEED = 0.6f;   
const float MOON_ORBIT_SPEED = 4.8f; 
const float MOON_ORBIT_TILT = 23.5f;   // 月球轨道面相对黄道面的倾角

// 生成球体网格：位置 + 法线 + 纹理坐标
void generateSphere(float radius, int sectors, int stacks,
                    std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    const float PI = 3.14159265359f;
    float sectorStep = 2.0f * PI / sectors;
    float stackStep = PI / stacks;

    for (int i = 0; i <= stacks; ++i)
    {
        float stackAngle = PI / 2.0f - i * stackStep;  
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * sectorStep;

            // 位置
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            // 法线（单位向量）
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;

            // 纹理坐标
            float u = (float)j / sectors;
            float v = (float)i / stacks;

            vertices.push_back(x);  vertices.push_back(y);  vertices.push_back(z);
            vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
            vertices.push_back(u);  vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i)
    {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j)
        {
            if (i != 0)
            {
                indices.push_back(k1 + j);
                indices.push_back(k2 + j);
                indices.push_back(k1 + j + 1);
            }
            if (i != (stacks - 1))
            {
                indices.push_back(k1 + j + 1);
                indices.push_back(k2 + j);
                indices.push_back(k2 + j + 1);
            }
        }
    }
}

void initial(void)
{
    //进行球体顶点和三角面片的计算
    // 生成球体网格
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(1.0f, 40, 30, vertices, indices); // 单位球，后续用 model 矩阵缩放
    indexCount = (unsigned int)indices.size();

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
}

void reshaper(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// 用统一颜色绘制球体
void drawSphere(Shader& shader, const glm::mat4& model, const glm::vec3& color)
{
    shader.use();
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main()
{
    glfwInit(); // 初始化GLFW
    // OpenGL版本为3.3，主次版本号均设为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // 使用核心模式(无需向后兼容性)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // 将窗口的上下文设置为当前线程的主上下文
    glfwMakeContextCurrent(window);
    // 初始化GLAD，加载OpenGL函数指针地址的函数
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    initial();
    //窗口大小改变时调用reshaper函数
    glfwSetFramebufferSizeCallback(window, reshaper);
    glfwSetFramebufferSizeCallback(window, reshaper);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

     //简单的纯色着色器
    Shader solidShader("shader/solid.vs", "shader/solid.fs");

    std::cout << "=== 日地月运动模型 ===" << std::endl;
    std::cout << "WASD: 移动摄像机 | 鼠标: 旋转视角 | 滚轮: 缩放 | ESC: 退出" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // 深空背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 投影和视图矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        solidShader.use();
        solidShader.setMat4("projection", projection);
        solidShader.setMat4("view", view);

        float earthAngle = currentFrame * EARTH_ORBIT_SPEED;
        float moonAngle = currentFrame * MOON_ORBIT_SPEED;

        // 太阳
        glm::mat4 sun = glm::scale(glm::mat4(1.0f), glm::vec3(SUN_RADIUS));
        drawSphere(solidShader, sun, glm::vec3(1.0f, 0.85f, 0.0f)); // 金黄色

        // 地球：
        glm::mat4 earth = glm::translate(glm::mat4(1.0f), glm::vec3(EARTH_ORBIT_RADIUS, 0.0f, 0.0f));
        earth = glm::rotate(glm::mat4(1.0f), earthAngle, glm::vec3(0.0f, 1.0f, 0.0f)) * earth;
        earth = glm::scale(earth, glm::vec3(EARTH_RADIUS));
        drawSphere(solidShader, earth, glm::vec3(0.2f, 0.4f, 1.0f)); // 蓝色

        // 月亮
        glm::mat4 moon = glm::translate(glm::mat4(1.0f), glm::vec3(MOON_ORBIT_RADIUS, 0.0f, 0.0f));
        moon = glm::rotate(glm::mat4(1.0f), moonAngle, glm::vec3(0.0f, 1.0f, 0.0f)) * moon;
        moon = glm::rotate(glm::mat4(1.0f), glm::radians(MOON_ORBIT_TILT), glm::vec3(1.0f, 0.0f, 0.0f)) * moon;
        moon = earth * moon;
        moon = glm::scale(moon, glm::vec3(MOON_RADIUS));
        drawSphere(solidShader, moon, glm::vec3(0.7f, 0.7f, 0.7f)); // 灰色

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
