#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <Windows.h>
#include "color.h"
#include "framebuffer.h"
#include "objReader.h"
#include "uniforms.h"
#include "shaders.h"
#include "triangle.h"
#include "camera.h"
#include "primitiveAssembly.h"
#include "rasterize.h"
#include "matrixes.h"
#include "fragment.h"
#include "planet.h"
#include "Spaceship.h"

Planet planets[] = {
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.5f, 0.5f, 0.5f),
     false},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(150.0f),
     0.9f,
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(1.0f, 1.0f, 1.0f),
     true},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(400.0f),
     0.1f,
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(1.5f, 1.5f, 1.5f),
     false},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(600.0f),
     0.8f,
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(1.0f, 1.0f, 1.0f),
     false},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(1000.0f),
     0.4f,
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.5f, 0.5f, 0.5f),
     false},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(1300.0f),
     0.1f,
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 0.0f),
     glm::vec3(1.0f, 1.0f, 1.0f),
     false},
    {glm::radians(0.0f),
     glm::radians(0.0f),
     glm::radians(0.0f),
     0.2f,
     glm::vec3(1.0f, 0.3f, 0.0f),
     glm::vec3(1.0f, 0.3f, 0.0f),
     glm::vec3(0.4f, 0.4f, 0.4f),
     false},
};

float rotationAngle = glm::radians(0.0f);
const float SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
const float SCREEN_HEIGHT = GetSystemMetrics(SM_CYSCREEN);
int frame = 0;
Framebuffer framebuffer = Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT);

Color clearColor = Color(0, 0, 0);
glm::vec3 light = glm::vec3(0.0f, 1.0f, 0.0f);
Color mainColor = Color(255, 255, 255);
const std::string modelPath = "../models/sphere.obj";
const std::string modelShipPath = "../models/ship.obj";

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;
std::vector<Face> faces;
std::vector<Vertex> verticesArray;

std::vector<glm::vec3> spaceShipVertices;
std::vector<glm::vec3> spaceShipNormals;
std::vector<Face> spaceShipFaces;
std::vector<Vertex> spaceShipVerticesArray;

Uniforms uniforms;

glm::mat4 model = glm::mat4(1);
glm::mat4 view = glm::mat4(1);
glm::mat4 projection = glm::mat4(1);

bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "Error: No se puedo inicializar SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("SR 2: Flat Shading", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Error: No se pudo crear una ventana SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Error: No se pudo crear SDL_Renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void render(Primitive polygon, int planetIndex, std::vector<Vertex> verticesArray)
{

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);

    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices;
    for (const Vertex &vertex : verticesArray)
    {
        transformedVertices.push_back(vertexShader(vertex, uniforms));
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledVertices = primitiveAssembly(polygon, transformedVertices);

    // 3. Rasterization
    std::vector<Fragment> fragments = rasterize(polygon, assembledVertices, SCREEN_WIDTH, SCREEN_HEIGHT, planets[planetIndex].currentPos, framebuffer.getMainColor());

    // 4. Fragment Shader
    for (Fragment &fragment : fragments)
    {
        // Apply the fragment shader to compute the final color
        std::vector<Fragment> fragments;
        fragments = fragmentShader(fragment, frame, planetIndex);
        if (fragments.size() > 0)
            for (Fragment &f : fragments)
            {
                framebuffer.point(f);
            }
    }
}

int main(int argv, char **args)
{
    framebuffer.setClearColor(clearColor);
    framebuffer.setMainColor(mainColor);
    framebuffer.setLight(light);

    if (!init())
    {
        return 1;
    }

    Spaceship spaceship;
    spaceship.worldPos = {0.0f, 0.0f, 20.0f};
    spaceship.scaleFactor = {0.03f, 0.03f, 0.03f};
    spaceship.rotationAngle = 90.0f;
    spaceship.rotationSpeed = 2.0f;
    spaceship.movementSpeed = 1.0f;

    framebuffer.clear();

    loadOBJ(modelPath, vertices, normals, faces);
    verticesArray = setupVertexArray(vertices, normals, faces);

    loadOBJ(modelShipPath, spaceShipVertices, spaceShipNormals, spaceShipFaces);
    spaceShipVerticesArray = setupVertexArray(spaceShipVertices, spaceShipNormals, spaceShipFaces);

    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Rotar alrededor del eje Y

    // Inicializar cámara
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 30.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Matriz de proyección
    uniforms.projection = createProjectionMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Matriz de viewport
    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Matriz de vista

    int speed = 2;
    int counter = 0;

    bool isRunning = true;
    while (isRunning)
    {
        frame += 1;
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                isRunning = false;
            }
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RIGHT:
                    if (spaceship.rotationAngle - spaceship.rotationSpeed == 0.0f)
                    {
                        spaceship.rotationAngle = 360.0f;
                    }
                    else
                    {
                        spaceship.rotationAngle -= spaceship.rotationSpeed;
                    }
                    break;

                case SDLK_LEFT:
                    if (spaceship.rotationAngle + spaceship.rotationSpeed > 360.0f)
                    {
                        spaceship.rotationAngle = spaceship.rotationSpeed;
                    }
                    else
                    {
                        spaceship.rotationAngle += spaceship.rotationSpeed;
                    }
                    break;

                case SDLK_UP:
                    spaceship.worldPos.z -= spaceship.movementSpeed * sin(glm::radians(spaceship.rotationAngle));
                    spaceship.worldPos.x += spaceship.movementSpeed * cos(glm::radians(spaceship.rotationAngle));
                    break;

                case SDLK_DOWN:
                    spaceship.worldPos.z += spaceship.movementSpeed * sin(glm::radians(spaceship.rotationAngle));
                    spaceship.worldPos.x -= spaceship.movementSpeed * cos(glm::radians(spaceship.rotationAngle));
                    break;
                case SDLK_r:
                    spaceship.worldPos = {0.0f, 0.0f, 30.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_0:
                    spaceship.worldPos = {planets[0].currentPos.x, 0.0f, planets[0].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    /*camera.targetPosition = planets[0].currentPos;
                    camera.cameraPosition.z = planets[0].currentPos.z + 5;*/
                    break;
                case SDLK_1:
                    spaceship.worldPos = {planets[1].currentPos.x, 0.0f, planets[1].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_2:
                    spaceship.worldPos = {planets[2].currentPos.x, 0.0f, planets[2].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_3:
                    spaceship.worldPos = {planets[3].currentPos.x, 0.0f, planets[3].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_4:
                    spaceship.worldPos = {planets[4].currentPos.x, 0.0f, planets[4].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_5:
                    spaceship.worldPos = {planets[5].currentPos.x, 0.0f, planets[5].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_6:
                    spaceship.worldPos = {planets[6].currentPos.x, 0.0f, planets[6].currentPos.z + 1.0f};
                    spaceship.rotationAngle = 90.0f;
                    break;
                case SDLK_q:
                    isRunning = false;
                    break;
                }
            }
        }

        framebuffer.clear();

        // Crear la matriz de vista usando el objeto cámara
        uniforms.view = glm::lookAt(
            camera.cameraPosition, // The position of the camera
            camera.targetPosition, // The point the camera is looking at
            camera.upVector        // The up vector defining the camera's orientation
        );

        glm::mat4 spaceshipTranslation = glm::translate(glm::mat4(1.0f), spaceship.worldPos);
        glm::mat4 spaceshipScale = glm::scale(glm::mat4(1.0f), spaceship.scaleFactor);
        glm::mat4 spaceshipRotation = glm::rotate(glm::mat4(1.0f), glm::radians(spaceship.rotationAngle-90.0f), rotationAxis);
        uniforms.model = spaceshipTranslation * spaceshipRotation * spaceshipScale;

        float d = 5.0f; // Distancia de camara a nave

        // Determinar posición de la cámara
        float cameraAngle;
        float dx;
        float dz;

        if (spaceship.rotationAngle > 0 && spaceship.rotationAngle <= 90)
        {
            // I cuadrante: de 1 a 90
            cameraAngle = spaceship.rotationAngle;
            dz = d * sin(glm::radians(cameraAngle));
            dx = d * cos(glm::radians(cameraAngle));
            camera.cameraPosition = {spaceship.worldPos.x - dx, 1.0f, spaceship.worldPos.z + dz};
        }
        else if (spaceship.rotationAngle > 90 && spaceship.rotationAngle <= 180)
        {
            // II cuadrante: de 91 a 180
            cameraAngle = spaceship.rotationAngle - 90.0f;
            dz = d * cos(glm::radians(cameraAngle));
            dx = d * sin(glm::radians(cameraAngle));
            camera.cameraPosition = {spaceship.worldPos.x + dx, 1.0f, spaceship.worldPos.z + dz};
        }

        else if (spaceship.rotationAngle > 180 && spaceship.rotationAngle <= 270)
        {
            // III cuadrante: de 181 a 270
            cameraAngle = spaceship.rotationAngle - 180.0f;
            dz = d * sin(glm::radians(cameraAngle));
            dx = d * cos(glm::radians(cameraAngle));
            camera.cameraPosition = {spaceship.worldPos.x + dx, 1.0f, spaceship.worldPos.z - dz};
        }

        else if (spaceship.rotationAngle > 270 && spaceship.rotationAngle <= 360)
        {
            // IV cuadrante: de 271 a 360
            cameraAngle = spaceship.rotationAngle - 270.0f;
            dz = d * cos(glm::radians(cameraAngle));
            dx = d * sin(glm::radians(cameraAngle));
            camera.cameraPosition = {spaceship.worldPos.x - dx, 1.0f, spaceship.worldPos.z - dz};
        }

        camera.targetPosition = spaceship.worldPos;

        render(Primitive::TRIANGLES, 100, spaceShipVerticesArray);

        int planetsSize = sizeof(planets) / sizeof(planets[0]);

        for (int i = 0; i < planetsSize - 1; i++)
        {
            Planet model = planets[i];

            model.translation.x = model.rotationRadius;

            if (model.translation.x != 0.0f)
            {
                model.translation.x = model.translation.x * cos(glm::radians(rotationAngle * model.speedFactor));
                model.translation.z = model.translation.z - (model.rotationRadius * sin(glm::radians(rotationAngle * model.speedFactor)));
            }

            if (model.hasMoon)
            {
                Planet moon = planets[planetsSize - 1];
                moon.translation.x = model.translation.x + 1.0f * cos(glm::radians(rotationAngle * 2));
                moon.translation.z = model.translation.z - (1.0f * sin(glm::radians(rotationAngle * 2)));
                moon.translation.y = model.translation.y;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle / 10), rotationAxis);

                glm::mat4 translation = glm::translate(glm::mat4(1.0f), moon.translation);
                glm::mat4 scale = glm::scale(glm::mat4(1.0f), moon.scale);

                // Calcular la matriz de modelo
                uniforms.model = translation * rotation * scale;
                render(Primitive::TRIANGLES, planetsSize - 1, verticesArray);
                planets[planetsSize - 1].currentPos = moon.translation;

                framebuffer.renderBuffer(renderer);
            }

            rotationAngle += 1.0f;
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), rotationAxis);

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model.translation);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), model.scale);

            // Calcular la matriz de modelo
            uniforms.model = translation * rotation * scale;
            render(Primitive::TRIANGLES, i, verticesArray);
            planets[i].currentPos = model.translation;

            framebuffer.renderBuffer(renderer);
        }
        SDL_RenderPresent(renderer);
    }

    return 0;
}