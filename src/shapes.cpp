#pragma once
#include <glm/glm.hpp>

#include "helper.hpp"
#include "shapes.hpp"

const float PI = 3.14159265358979f;

helper::vertindex generateSphere()
{
    const int n = 64;

    helper::vertindex result;
    
    for (int i = 0; i <= n * 2; i++) 
    {
        float s = i * 2.0f * PI / float(n * 2);
        for (int j = 0; j <= n; j++) 
        {
            float t = j * PI / float(n);
            float x = cos(s) * sin(t);
            float y = sin(s) * sin(t);
            float z = cos(t);

            //position
            result.vertices.push_back(x);
            result.vertices.push_back(y);
            result.vertices.push_back(z);

            //normal
            result.vertices.push_back(x);
            result.vertices.push_back(y);
            result.vertices.push_back(z);

            //texcoord
            result.vertices.push_back(s / (2 * PI));
            result.vertices.push_back(t / PI);
            
            if (i > 0 && j > 0) 
            {
                result.indices.push_back((i - 1) * (n + 1) + (j - 1));
                result.indices.push_back((i - 1) * (n + 1) + (j));
                result.indices.push_back((i) * (n + 1) + (j));

                result.indices.push_back((i - 1) * (n + 1) + (j - 1));
                result.indices.push_back((i) * (n + 1) + (j));
                result.indices.push_back((i) * (n + 1) + (j - 1));
            }
        }
    }

    return result;
}

helper::vertindex generateSphereposonly()
{
    //need more precision for calculating texture coordinate
    const int n = 64;

    helper::vertindex result;

    for (int i = 0; i <= n * 2; i++)
    {
        float s = i * 2.0f * PI / float(n * 2);
        for (int j = 0; j <= n; j++)
        {
            float t = j * PI / float(n);
            float x = cos(s) * sin(t);
            float y = sin(s) * sin(t);
            float z = cos(t);

            result.vertices.push_back(x);
            result.vertices.push_back(y);
            result.vertices.push_back(z);

            if (i > 0 && j > 0)
            {
                result.indices.push_back((i - 1) * (n + 1) + (j - 1));
                result.indices.push_back((i) * (n + 1) + (j));
                result.indices.push_back((i - 1) * (n + 1) + (j));

                result.indices.push_back((i - 1) * (n + 1) + (j - 1));
                result.indices.push_back((i) * (n + 1) + (j - 1));
                result.indices.push_back((i) * (n + 1) + (j));
            }
        }
    }

    return result;
}

helper::vertindex generateBox()
{
    helper::vertindex result;

    result.vertices = {
        0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

       -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
       -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
       -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
       -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

        0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
       -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
       -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,

       -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
        0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
       -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
        0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,

        0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
       -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
       -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

       -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
       -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
        0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
        0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
    };

    result.indices = {
        0, 2, 1,
        2, 3, 1,

        4, 6, 5,
        6, 7, 5,

        8,10, 9,
       10,11, 9,

       12,14,13,
       14,15,13,

       16,18,17,
       18,19,17,

       20,22,21,
       22,23,21,
    };

    return result;
}

helper::vertindex generateBoxposonly()
{
    helper::vertindex result;

    result.vertices = {
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f,-0.5f,
        0.5f,-0.5f, 0.5f,
        0.5f,-0.5f,-0.5f,

       -0.5f, 0.5f,-0.5f,
       -0.5f, 0.5f, 0.5f,
       -0.5f,-0.5f,-0.5f,
       -0.5f,-0.5f, 0.5f,

        0.5f, 0.5f, 0.5f,
       -0.5f, 0.5f, 0.5f,
        0.5f, 0.5f,-0.5f,
       -0.5f, 0.5f,-0.5f,

       -0.5f,-0.5f, 0.5f,
        0.5f,-0.5f, 0.5f,
       -0.5f,-0.5f,-0.5f,
        0.5f,-0.5f,-0.5f,

        0.5f, 0.5f, 0.5f,
        0.5f,-0.5f, 0.5f,
       -0.5f, 0.5f, 0.5f,
       -0.5f,-0.5f, 0.5f,

       -0.5f, 0.5f,-0.5f,
       -0.5f,-0.5f,-0.5f,
        0.5f, 0.5f,-0.5f,
        0.5f,-0.5f,-0.5f,
    };

    result.indices = {
        0, 2, 1,
        2, 3, 1,

        4, 6, 5,
        6, 7, 5,

        8,10, 9,
       10,11, 9,

       12,14,13,
       14,15,13,

       16,18,17,
       18,19,17,

       20,22,21,
       22,23,21,
    };

    return result;
}
