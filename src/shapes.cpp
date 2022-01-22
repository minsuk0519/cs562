#pragma once
#include <glm/glm.hpp>

#include "helper.hpp"
#include "shapes.hpp"

const float PI = 3.14159265358979f;

helper::vertindex generateSphere()
{
    const int n = 16;

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
    const int n = 16;

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