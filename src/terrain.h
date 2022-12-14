// Author: Alex Hartford
// Program: Experience
// File: Terrain
// Date: April 2022

#ifndef TERRAIN_H
#define TERRAIN_H

#include <glad/glad.h> 

#include "shader.h"
#include "light.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;
using namespace glm;

struct Terrain
{
    float yScale = 16.0f;  // Desired Scale
    float yShift = 16.0f;  // Amount of Shift downwards.
    vec3 bottom;
    vec3 top;
    vec3 dirt;

    // Default Values
    float widthExtent = 128.0f;
    float heightExtent = 128.0f;

    string path;
    unsigned int VAO, VBO, EBO;

    vector<float> vertices;
    vector<unsigned int> indices;

    unsigned int num_strips;
    unsigned int num_tris_per_strip;

    int width, height;

    // [x][z]
    vector<vector<float>> pointsData;

    // Uses Bilinear Interpolation to return the height at a point.
    float heightAt(float worldX, float worldZ)
    {
        // Gets us into pointsData space.
        float x = (worldX + height / 2);
        float z = (worldZ + width / 2);

        int x1 = (int)x;
        int x2 = x1 + 1;
        int z1 = (int)z;
        int z2 = z1 + 1;

        float x_minus_x1 = x - x1;
        float z_minus_z1 = z - z1;
        float x2_minus_x = 1 - x_minus_x1;
        float z2_minus_z = 1 - z_minus_z1;

        float fxz1 = x2_minus_x * pointsData[x1][z1] + x_minus_x1 * pointsData[x2][z1];
        float fxz2 = x2_minus_x * pointsData[x1][z2] + x_minus_x1 * pointsData[x2][z2];

        float fxz = z2_minus_z * fxz1 + z_minus_z1 * fxz2;

        return fxz;
    }

    void init(string p, float y_scale, float y_shift, vec3 b, vec3 t, vec3 d)
    {
        this->bottom = b;
        this->top = t;
        this->dirt = d;
        vertices.clear();
        indices.clear();
        pointsData.clear();
        this->path = p;
        // Load Heightmap
        int nrChannels;
        unsigned char *data = stbi_load(p.c_str(), &width, &height, &nrChannels, 0);
        if(!data)
        {
            cout << "Unable to load Heightmap: " << path << "\n";
        }

        this->yScale = y_scale;
        float yScaleNormalized = yScale / height;
        this->yShift = y_shift;

        this->widthExtent = width / 2;
        this->heightExtent = height / 2;

        for(int i = 0; i < height; ++i)
        {
            vector<float> row;
            for(int j = 0; j < width; ++j)
            {
                unsigned char* texel = data + (j + width * i) * nrChannels;
                unsigned char y = texel[0];
                float vy = (y * yScaleNormalized - yShift);
                row.push_back(vy);
            }
            pointsData.push_back(row);
        }
        stbi_image_free(data);

        // Generate Indices
        for(int i = 0; i < height - 1; ++i)             // For each strip
        {
            for(int j = width - 1; j >= 0; --j)         // For each column (backwards for CCW data)
            {
                indices.push_back(j + width * (i + 0)); // For each side of the strip
                indices.push_back(j + width * (i + 1));
            }
        }

        // Generate Vertices
        for(int i = 0; i < height; ++i)
        {
            for(int j = 0; j < width; ++j)
            {
                float vx = (-height/2.0f + height * i / (float)height); // vx
                float vz = (-width/2.0f + width * j / (float)width);    // vz
                vertices.push_back(vx);
                vertices.push_back(pointsData[i][j]);
                vertices.push_back(vz);

                // Compute Normals, pack into array.
                // With Bounds Checking.
                vec3 v0, v1;
                if(j > 0 && j < pointsData[i].size() - 1)
                {
                    v0 = vec3(0, (pointsData[i][j+1] - pointsData[i][j-1]), 2);
                }
                else
                {
                    v0 = vec3(0, 1, 0);
                }
                if(i > 0 && i < pointsData.size() - 1)
                {
                    v1 = vec3(2, (pointsData[i+1][j] - pointsData[i-1][j]), 0);
                }
                else
                {
                    v1 = vec3(0, 1, 0);
                }
                vec3 normal = normalize(cross(v0, v1));
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            }
        }

        num_strips = height - 1;
        num_tris_per_strip = width * 2;

        // Buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // position attribute
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        glBindVertexArray(VAO);
        // Vertex data
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal data
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    }

    void Draw(Shader &shader, vector<Light> *lights, DirLight *dirLight, FogSystem *fog)
    {
        shader.bind();
        {
            mat4 projection = camera.GetProjectionMatrix();
            mat4 view = camera.GetViewMatrix();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            mat4 model = mat4(1.0f);
            shader.setMat4("model", model);

            shader.setVec3("bottom", bottom);
            shader.setVec3("top", top);
            shader.setVec3("dirt", dirt);

            shader.setFloat("maxFogDistance", fog->maxDistance);
            shader.setFloat("minFogDistance", fog->minDistance);
            shader.setVec4("fogColor", fog->color);

            dirLight->Render(shader);

            shader.setInt("size", lights->size());
            for (int i = 0; i < lights->size(); ++i)
            {
                lights->at(i).Render(shader, i);
            }

            shader.setVec3("viewPos", camera.Position);

            glBindVertexArray(VAO);
            for(unsigned int strip = 0; strip < num_strips; ++strip)
            {
                glDrawElements(GL_TRIANGLE_STRIP,
                               num_tris_per_strip, // Count of elements to be rendered.
                               GL_UNSIGNED_INT,    // Type of EBO data.
                               (void *)(sizeof(unsigned int) * num_tris_per_strip * strip)); // Pointer to starting index of indices.
            }
        }
        shader.unbind();
    }
};

#endif
