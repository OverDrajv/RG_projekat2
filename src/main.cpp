#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool Blinn = false;
bool TurnOnTheBrightLights = false;
bool bloom = false;
bool hdr=false;
float exposure = 0.55f;
bool spectatorMode = false;
bool acceleration = false;

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct DirLight{
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

//xwing
glm::vec3 xwingOffset = glm::vec3(0.0f, -3.0f, -10.0f);
glm::vec3 xwingPosition = glm::vec3(0.0f);
glm::vec3 xwingRotation = glm::vec3(0.0f);

glm::vec3 xwingLightOffset = glm::vec3 (0.00577375f ,0.257113f ,-6.42628f);
glm::vec3 xwingLightPosition = glm::vec3(0.0f);
glm::vec3 xwingLightDirection = glm::vec3(0.0f, 0.0f, -1.0f);

glm::vec3 xwingLBO = glm::vec3(-1.47279f, -0.757466f, 5.85636f); //left bottom light

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    programState->camera.Position = glm::vec3(7.0f, -1.5f, 55.0f);
    programState->camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
    xwingPosition = programState->camera.Position + xwingOffset;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // build and compile shaders
    // -------------------------
    Shader xwingShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyBoxShader("resources/shaders/skyBox.vs", "resources/shaders/skyBox.fs");
    Shader starDestroyerShader("resources/shaders/newShader.vs", "resources/shaders/newShader.fs");
    Shader rebelShipShader("resources/shaders/newShader.vs", "resources/shaders/newShader.fs");
    Shader asteroidFieldShader("resources/shaders/oppositeShader.vs", "resources/shaders/newShader.fs");
    Shader lightShader("resources/shaders/lightShader.vs", "resources/shaders/lightShader.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader hdrShader("resources/shaders/hdr.vs","resources/shaders/hdr.fs");
    // load models
    // -----------
    Model xwingModel("resources/objects/xwing/XWing_Woody.obj");
    xwingModel.SetShaderTextureNamePrefix("material.");
    Model starDestroyerModel("resources/objects/starDestroyer/star_destroyer.obj");
    starDestroyerModel.SetShaderTextureNamePrefix("material.");
    Model rebelShipModel("resources/objects/rebelShip/Vehicle_SpaceCraft_SW_CR90-Corvette.obj");
    rebelShipModel.SetShaderTextureNamePrefix("material.");
    Model asteroidFieldModel("resources/objects/asteroidField/asteroid_03_01.obj");
    asteroidFieldModel.SetShaderTextureNamePrefix("material.");

    //skyBox
    float skyBoxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    //light
    float lightVertices[] = {
            //coordinates                     //normals                      //texture
            1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, //top right
            1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  //bottom right
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  //bottom left
            -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f //top left
    };

    unsigned int lightIndices[]={
            0, 1, 3,
            1, 2, 3
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBoxVertices), &skyBoxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    unsigned int lightVAO, lightVBO, lightEBO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glGenBuffers(1, &lightEBO);

    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lightIndices), lightIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);


    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    vector <std::string> faces{
        "resources/textures/skybox/right.png",
        "resources/textures/skybox/left.png",
        "resources/textures/skybox/top.png",
        "resources/textures/skybox/bottom.png",
        "resources/textures/skybox/front.png",
        "resources/textures/skybox/back.png",
    };

    unsigned int cubemapTexture = loadCubemap(faces);
    unsigned int planetTexture = loadTexture("resources/textures/planetrotation.png");
    unsigned  int lightTexture = loadTexture("resources/textures/svetloYellow.png");

    SpotLight spotLight;
    spotLight.position = glm::vec3 (0.00577375,0.257113,-6.42628);
    spotLight.direction = glm::vec3 (0.0f, 0.0f, -1.0f);
    spotLight.ambient = glm::vec3 (0.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.01f;
    spotLight.quadratic = 0.02f;
    spotLight.cutOff = glm::cos(glm::radians(15.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(30.0f));

    DirLight sun;
    sun.direction = glm::vec3(1.0f, 0.0f, 0.0f);
    sun.ambient = glm::vec3(0.2f);
    sun.specular = glm::vec3(0.8f);
    sun.diffuse = glm::vec3 (0.95f);

    //shader configuration
    //
    skyBoxShader.use();
    skyBoxShader.setInt("skybox", 0);
    skyBoxShader.setInt("planetTexture", 1);

    lightShader.use();
    lightShader.setInt("texture", 0);

    blurShader.use();
    blurShader.setInt("image", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.setInt("bloomBlur", 1);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //hdr, postavljamo floating point buffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(TurnOnTheBrightLights){
            spotLight.specular = glm::vec3 (0.9f);
            spotLight.diffuse = glm::vec3 (5.0f, 0.2f, 0.2f);
        }
        else{
            spotLight.specular = glm::vec3 (0.0f);
            spotLight.diffuse = glm::vec3 (0.0f);
        }
        //X-Wing
        xwingShader.use();
        xwingShader.setVec3("dirLight.direction",sun.direction);
        xwingShader.setVec3("dirLight.specular", sun.specular);
        xwingShader.setVec3("dirLight.diffuse", sun.diffuse);
        xwingShader.setVec3("dirLight.ambient", sun.ambient);
        xwingShader.setVec3("viewPosition", programState->camera.Position);
        xwingShader.setFloat("material.shininess", 32.0f);
        xwingShader.setBool("Blinn", Blinn);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        xwingShader.setMat4("projection", projection);
        xwingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,xwingPosition);
        model = glm::scale(model, glm::vec3(0.9f));
        model = glm::rotate(model, glm::radians(xwingRotation.x), glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(xwingRotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
        xwingShader.setMat4("model", model);
        xwingModel.Draw(xwingShader);

        if(!spectatorMode) {
            glm::vec4 xwingLightPosition2 = model * glm::vec4(xwingLightOffset, 1.0f);
            xwingLightPosition = glm::vec3(xwingLightPosition2);
            xwingLightDirection = programState->camera.Front;
        }

        //Star Destroyer
        starDestroyerShader.use();
        starDestroyerShader.setVec3("dirLight.direction",sun.direction);
        starDestroyerShader.setVec3("dirLight.specular", sun.specular);
        starDestroyerShader.setVec3("dirLight.diffuse", sun.diffuse);
        starDestroyerShader.setVec3("dirLight.ambient", sun.ambient);
        starDestroyerShader.setVec3("viewPosition", programState->camera.Position);
        starDestroyerShader.setFloat("material.shininess", 32.0f);
        starDestroyerShader.setBool("Blinn", Blinn);

        starDestroyerShader.setVec3("spotLight.position", xwingLightPosition);
        starDestroyerShader.setVec3("spotLight.direction", xwingLightDirection);
        starDestroyerShader.setVec3("spotLight.ambient", spotLight.ambient);
        starDestroyerShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        starDestroyerShader.setVec3("spotLight.specular", spotLight.specular);
        starDestroyerShader.setFloat("spotLight.constant", spotLight.constant);
        starDestroyerShader.setFloat("spotLight.linear", spotLight.linear);
        starDestroyerShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        starDestroyerShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        starDestroyerShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        starDestroyerShader.setMat4("projection", projection);
        starDestroyerShader.setMat4("view", view);

        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(10.0f, -15.0f, -35.0f));
        model2 = glm::scale(model2, glm::vec3(0.2f));
        starDestroyerShader.setMat4("model", model2);
        starDestroyerModel.Draw(starDestroyerShader);
        //rebel Ship
        rebelShipShader.use();
        rebelShipShader.setVec3("dirLight.direction",sun.direction);
        rebelShipShader.setVec3("dirLight.specular", sun.specular);
        rebelShipShader.setVec3("dirLight.diffuse", sun.diffuse*0.7f);
        rebelShipShader.setVec3("dirLight.ambient", sun.ambient);
        rebelShipShader.setVec3("viewPosition", programState->camera.Position);
        rebelShipShader.setFloat("material.shininess", 32.0f);
        rebelShipShader.setBool("Blinn", Blinn);

        rebelShipShader.setVec3("spotLight.position", xwingLightPosition);
        rebelShipShader.setVec3("spotLight.direction", xwingLightDirection);
        rebelShipShader.setVec3("spotLight.ambient", spotLight.ambient);
        rebelShipShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        rebelShipShader.setVec3("spotLight.specular", spotLight.specular);
        rebelShipShader.setFloat("spotLight.constant", spotLight.constant);
        rebelShipShader.setFloat("spotLight.linear", spotLight.linear);
        rebelShipShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        rebelShipShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        rebelShipShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        rebelShipShader.setMat4("projection", projection);
        rebelShipShader.setMat4("view", view);

        model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(37.0f, 5.0f, +25.0f));
        model2 = glm::scale(model2, glm::vec3(0.15f));
        model2 = glm::rotate(model2, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, -0.5f));
        rebelShipShader.setMat4("model", model2);
        rebelShipModel.Draw(rebelShipShader);

        //asteroid Field
        asteroidFieldShader.use();
        asteroidFieldShader.setVec3("dirLight.direction",sun.direction);
        asteroidFieldShader.setVec3("dirLight.specular", sun.specular);
        asteroidFieldShader.setVec3("dirLight.diffuse", sun.diffuse);
        asteroidFieldShader.setVec3("dirLight.ambient", sun.ambient);
        asteroidFieldShader.setVec3("viewPosition", programState->camera.Position);
        asteroidFieldShader.setFloat("material.shininess", 32.0f);
        asteroidFieldShader.setBool("Blinn", Blinn);

        asteroidFieldShader.setVec3("spotLight.position", xwingLightPosition);
        asteroidFieldShader.setVec3("spotLight.direction", xwingLightDirection);
        asteroidFieldShader.setVec3("spotLight.ambient", spotLight.ambient);
        asteroidFieldShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        asteroidFieldShader.setVec3("spotLight.specular", spotLight.specular);
        asteroidFieldShader.setFloat("spotLight.constant", spotLight.constant);
        asteroidFieldShader.setFloat("spotLight.linear", spotLight.linear);
        asteroidFieldShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        asteroidFieldShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        asteroidFieldShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        asteroidFieldShader.setMat4("projection", projection);
        asteroidFieldShader.setMat4("view", view);

        model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(-10.0f, -15.f, 0.0f));
        asteroidFieldShader.setMat4("model", model2);
        asteroidFieldModel.Draw(asteroidFieldShader);

        //lightTexture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lightTexture);
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        glm::mat4 modellb = glm::translate(model,xwingLBO);
        modellb = glm::scale(modellb, glm::vec3(0.24f));
        lightShader.setMat4("model", modellb);
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glm::mat4 modellt = glm::translate(model,glm::vec3(xwingLBO.x, -xwingLBO.y, xwingLBO.z));
        modellt = glm::scale(modellt, glm::vec3(0.24f));
        lightShader.setMat4("model", modellt);
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glm::mat4 modelrt = glm::translate(model,glm::vec3(-xwingLBO.x, -xwingLBO.y, xwingLBO.z));
        modelrt = glm::scale(modelrt, glm::vec3(0.24f));
        lightShader.setMat4("model", modelrt);
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glm::mat4 modelrb = glm::translate(model,glm::vec3(-xwingLBO.x, xwingLBO.y, xwingLBO.z));
        modelrb = glm::scale(modelrb, glm::vec3(0.24f));
        lightShader.setMat4("model", modelrb);
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        //planetTexture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, planetTexture);

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);
        skyBoxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyBoxShader.setMat4("view", view);
        skyBoxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDepthFunc(GL_LESS);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //bloom part
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //hdr post-processing effect
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setInt("hdr", hdr);
        hdrShader.setInt("bloom", bloom);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteBuffers(1, &lightVBO);
    glDeleteVertexArrays(1, &lightEBO);


    glfwTerminate();
    return 0;
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if(!spectatorMode) {
        xwingPosition = programState->camera.Position + xwingOffset;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);

    if(!spectatorMode) {
        xwingRotation.x = programState->camera.Yaw + 90;
        xwingRotation.y = programState->camera.Pitch;
    }

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
        TurnOnTheBrightLights = !TurnOnTheBrightLights;
    }
    if(key == GLFW_KEY_B && action ==GLFW_PRESS){
        Blinn = !Blinn;
    }
    if(key == GLFW_KEY_H && action == GLFW_PRESS){
        hdr = !hdr;
    }

    if(key == GLFW_KEY_M && action == GLFW_PRESS){
        bloom = !bloom;
    }

    if(key == GLFW_KEY_I && action == GLFW_PRESS){
        spectatorMode = true;
    }

    if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS){
        if(!acceleration){
            acceleration = true;
            programState->camera.MovementSpeed *= 2;
        }
        else{
            acceleration = false;
            programState->camera.MovementSpeed /= 2;
        }
    }
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}