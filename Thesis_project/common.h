#pragma once

#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdio.h>

#include <glew.h>
#include <freeglut.h>

namespace edt
{
	struct MeshInstance;

	__declspec(selectany) struct //normaly definitions are not allowed in header files
	{
		struct
		{
			int screen_width = 1024;
			int screen_height = 1024;
		} viewport;

		//int number_of_mips = log2(viewport.screen_width);

		size_t visibility_test_rt_size = 1024;

		struct
		{
			GLuint wall;
			GLuint machine;
		} shaders;

	} global;

	//the name of the function that is being called
#define print(expression) \
	{\
		std::stringstream ss;\
		ss << expression;\
		std::cout << ss.str();\
	}

#define printf(expression) {print( "[" << __FUNCTION__ << "] " << expression << std::endl);}

}