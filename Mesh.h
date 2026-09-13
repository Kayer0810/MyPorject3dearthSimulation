#ifndef MESH_CLASS_H
#define MESH_CLASS_H



#include <string>
#include <vector>
#include "VAO.h"
#include "EBO.h"
#include "Camera.h"
#include "Texture.h"



class Mesh
{
public:
	
	//Adding Vertices elements
	std::vector <Vertex> vertices;
	std::vector <GLuint> indices;
	std::vector <Texture> textures;

	
	GLenum drawMode;

	VAO VAO;

	Mesh(std::vector <Vertex>& vertices, std::vector <GLuint>& indices, std::vector <Texture>& textures,
		GLenum drawMode = GL_TRIANGLES);

	void Draw(Shader& shader, Camera& camera);


};

#endif
