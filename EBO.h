#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include<glad/glad.h>
#include<vector>

class EBO
{
public:
	
	GLuint ID;
	
	// Construct EBO function
	EBO(std::vector<GLuint>& indices);

	// Binding
	void Bind();

	// Unbinding
	void Unbind();

	// Deleting
	void Delete();

};

#endif
