//#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>

#include <glew.h>
#include <freeglut.h>

#include "algebra.h"
#include "mesh.h"
#include "common.h"
#include "Camera.h"
#include "ObjFileLoader.h"
#include "mesh_bunny.h"

namespace edt
{
//we tell to the compile that we know that there are warnings here so that it doesn't complain
#pragma warning( push )
//#pragma warning( disable: 4838 )

#pragma warning( pop )

	Camera camera({ 0.0f, 1.0f, 0.0f },{0, 0, 0 }, 60, 1, 10000, 0.75f ); //global camera parameters
	int last_mouse_pos[] = { 0 , 0 };

	GLuint render_targets[] = { 0 , 0 };
	GLuint depth_buffer = 0;
	GLuint frame_buffer;
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };

	// Global transform matrices
	struct 
	{
		Matrix view, projection, projection_view;
	} global_matrices;

	//send 
	void send_float3_to_shader(const GLuint& shader_id, const char* name, const Vector& value)
	{
		// gives a handle to the specific piece of data which we will use in order to access it later
		GLint h_handle = glGetUniformLocation(shader_id, name);

		//send information to shader
		glUniform3f(h_handle, value.x, value.y, value.z);
	}

	void send_float_to_shader(const GLuint& shader_id, const char* name, const float& value)
	{
		// gives a handle to the specific piece of data which we will use in order to access it later
		GLint h_handle = glGetUniformLocation(shader_id, name);

		//send information to shader
		glUniform1f(h_handle, value);
	}

	//take info from txt file to create shader
	bool create_shader_from_file(const int& shader_type, const char* file_name, GLuint& shader_id )
	{
		//store line by line the whole file
		std::string file_string;
		//open the file
		std::ifstream myfile(file_name);

		//check if it opened
		if (myfile.is_open())
		{
			std::string line;
			//you give the info from the line and copy it 
			while (getline(myfile, line))
			{
				file_string += line;
				file_string += '\n';
			}
			myfile.close();
		}
		else
		{
			printf("failed to load file: " << file_name);
			return false;
		}

		GLint success = GL_FALSE;
		shader_id = glCreateShader(shader_type);
		const char* tmp = file_string.c_str();
		//we give to the shader source a huge line that contains the shader
		glShaderSource(shader_id, 1, &tmp, NULL);
		glCompileShader(shader_id);
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			printf("Error in shader!");
			return false;
		}

		return true;
	}

	bool loadShaderProgram(const char* vs_filename, const char* fs_filename, GLuint& shader_id) 
	{
		shader_id = glCreateProgram();

		GLuint vertex_shader_id, fragment_shader_id;
		if (!create_shader_from_file(GL_VERTEX_SHADER, vs_filename, vertex_shader_id ))
		{
			printf("failed creating vertex shader: " << vs_filename);
			return false;
		}

		if (!create_shader_from_file(GL_FRAGMENT_SHADER, fs_filename, fragment_shader_id))
		{
			printf("failed creating fragment shader: " << fs_filename);
			return false;
		}

		glAttachShader(shader_id, vertex_shader_id);
		glAttachShader(shader_id, fragment_shader_id);
		glLinkProgram(shader_id);
		GLint isLinked = GL_FALSE;
		glGetProgramiv(shader_id, GL_LINK_STATUS, &isLinked);

		if (!isLinked)
		{
			printf("Link error in shader program! " << vs_filename << ", " << fs_filename);
			return false;
		}

		return true;
	}

	void createMeshDeviceResource(Mesh& mesh) 
	{
		int data_size_positions = mesh.geometry.positions.size() * sizeof(Vector);
		int data_size_normals = mesh.geometry.normals.size() * sizeof(Vector);
		int data_size_indices = mesh.geometry.triangles.size() * sizeof(Triangle);

		// For storage of state and other buffer objects needed for vertex specification
		glGenVertexArrays(1, &mesh.vertex_array_object);
		glBindVertexArray(mesh.vertex_array_object);

		// Allocate VBO and load mesh data (vertices and normals)
		glGenBuffers(1, &mesh.vertex_buffer_object);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, data_size_positions + data_size_normals, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, data_size_positions, (void*)mesh.geometry.positions.data());
		glBufferSubData(GL_ARRAY_BUFFER, data_size_positions, data_size_normals, (void*)mesh.geometry.normals.data());

		// Allocate index buffer and load mesh indices
		glGenBuffers(1, &mesh.index_buffer_object);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer_object);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_size_indices, (void*)mesh.geometry.triangles.data(), GL_STATIC_DRAW);

		// Define the format of the vertex data
		GLint vPos = glGetAttribLocation(mesh.material.shader_id, "vPos");
		glEnableVertexAttribArray(vPos);
		glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		// Define the format of the vertex data 
		GLint vNorm = glGetAttribLocation(mesh.material.shader_id, "vNorm");
		glEnableVertexAttribArray(vNorm);
		glVertexAttribPointer(vNorm, 3, GL_FLOAT, GL_FALSE, 0, (void*)data_size_positions);

		glBindVertexArray(0);
	}

	void renderMesh(const MeshInstance& mesh_instance) 
	{
		Matrix model_transform;
		//position and rotation are inverted (view matrix is the inverse transform of the camera)

		model_transform = create_Translation_mat(mesh_instance.translation);

		model_transform = MatMatMul(model_transform, create_Rotation_xmat(mesh_instance.rotation.x));
		model_transform = MatMatMul(model_transform, create_Rotation_ymat(mesh_instance.rotation.y));
		model_transform = MatMatMul(model_transform, create_Rotation_zmat(mesh_instance.rotation.z));

		//send model_matrix to shader (graphics card)
		GLint loc_model_mat = glGetUniformLocation(mesh_instance.model->material.shader_id, "model_matrix");
		glUniformMatrix4fv(loc_model_mat, 1, GL_FALSE, model_transform.e);

		// Select current resources 
		glBindVertexArray(mesh_instance.model->vertex_array_object);

		// Draw all triangles
		glDrawElements(GL_TRIANGLES, mesh_instance.model->geometry.triangles.size() * 3, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);

	}

	void loadWalls()
	{
		Mesh* mesh = new Mesh();
		
		bool result = loadOBJ("Models/empty_room.obj", mesh->geometry.positions, &mesh->geometry.normals);
		if (!result)
			return;
		
		for (unsigned short i = 0; i < mesh->geometry.positions.size(); i += 3)
			mesh->geometry.triangles.push_back({ i, i + 1u, i + 2u });

		mesh->material.shader_id = global.shaders.wall;

		createMeshDeviceResource(*mesh);

		MeshInstance* instance = new MeshInstance();
		instance->model = mesh;
		instance->translation = { 0,0,0 };
		instance->rotation = { 0,0,0 };
		instance->scaling = { 1,1,1 };

		global.walls.push_back(instance);
	}

	void loadMachines()
	{
		Mesh* mesh = new Mesh();

		bool result = loadOBJ("Models/machine1.obj", mesh->geometry.positions, &mesh->geometry.normals);
		if (!result)
			return;

		for (unsigned short i = 0; i < mesh->geometry.positions.size(); i += 3)
			mesh->geometry.triangles.push_back({ i, i + 1u, i + 2u });

		mesh->material.shader_id = global.shaders.wall;

		createMeshDeviceResource(*mesh);

		for (size_t i = 0; i < 3; i++)
		{
			MeshInstance* instance = new MeshInstance();
			instance->model = mesh;
			instance->translation = { 0,0,0 };
			instance->rotation = { 0,0,0 };
			instance->scaling = { 1,1,1 };

			global.machines.push_back(instance);
		}

		global.machines[0]->translation = { 5, 1, 10 };
		global.machines[0]->rotation = { 0, 30, 0 };
		global.machines[0]->scaling = { 0.5f, 1.0f ,0.5f };

		global.machines[1]->translation = { -12, 1, 6 };
		global.machines[1]->rotation = { 0, -10, 0 };
		global.machines[1]->scaling = { 0.3f, 1.0f ,0.3f };

		global.machines[2]->translation = { 7, 1, -5 };
		global.machines[2]->rotation = { 0, 75, 0 };
		global.machines[2]->scaling = { 0.7f, 1.0f ,0.7f };
	}

	//doing it once for all the walls 
	void applyGlobalMatrices(const GLuint& shader_id)
	{
		// Pass the view transform to the shader
		GLint loc_view_mat = glGetUniformLocation(shader_id, "view_matrix");
		glUniformMatrix4fv(loc_view_mat, 1, GL_FALSE, global_matrices.view.e);

		// Pass the proj transform to the shader
		GLint loc_proj_mat = glGetUniformLocation(shader_id, "proj_matrix");
		glUniformMatrix4fv(loc_proj_mat, 1, GL_FALSE, global_matrices.projection.e);

		// Pass the viewing transform to the shader
		GLint loc_PV = glGetUniformLocation(shader_id, "proj_view_matrix");
		glUniformMatrix4fv(loc_PV, 1, GL_FALSE, global_matrices.projection_view.e);
	}

	void renderWalls()
	{
		if (global.walls.empty())
			return;

		const GLuint& shader_id = global.walls.front()->model->material.shader_id;

		glUseProgram(shader_id);
		applyGlobalMatrices(shader_id); 
		
		{
			send_float3_to_shader(shader_id, "light.color", { 0.0f , 0.0f , 1.0f });
		}

		// Set render_target [0] as our colour attachement #0
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_targets[0], 0);

		for (MeshInstance* mesh_instance : global.walls)
		{
			renderMesh(*mesh_instance);
		}
	}

	void renderMachines()
	{
		if (global.machines.empty())
			return;

		const GLuint& shader_id = global.machines.front()->model->material.shader_id;

		glUseProgram(shader_id);
		applyGlobalMatrices(shader_id);

		Vector visible_color = { 0.0f , 1.0f , 0.0f }, obscured_color = { 1.0f , 0.0f , 0.0f };
		for (MeshInstance* mesh_instance : global.machines)
		{
			send_float3_to_shader(shader_id, "light.color", mesh_instance->is_visibru ? visible_color : obscured_color );
			renderMesh(*mesh_instance);
		}
	}

	//view and projection matrices are the same for all (walls and machines)
	void calcViewandProjectionMatrices()
	{
		//position and rotation are inverted (view matrix is the inverse transform of the camera)
		global_matrices.view = MatMatMul(create_Rotation_ymat(-1 * camera.rotation.y), create_Translation_mat(ScalarVecMul(-1.0f, camera.position)));
		global_matrices.view = MatMatMul(create_Rotation_xmat(-1 * camera.rotation.x), global_matrices.view);
		
		/*global_matrices.view = MatMatMul(create_Rotation_xmat(-1 * camera.rotation.x), create_Translation_mat(ScalarVecMul(-1.0f, camera.position)));
		global_matrices.view = MatMatMul(create_Rotation_ymat(-1 * camera.rotation.y), global_matrices.view);
		global_matrices.view = MatMatMul(create_Rotation_zmat(-1 * camera.rotation.z), global_matrices.view);*/

		if (global.viewport.is_orthographic)
		{
			global_matrices.projection = construct_parallel_matrix(global.viewport.right, global.viewport.left, global.viewport.top,
				global.viewport.bottom, (float)camera.far_clip, (float)camera.near_clip);
		}
		else
		{
			global_matrices.projection = construct_perspective_matrix((float)camera.FoV, (float)camera.near_clip, (float)camera.far_clip,
				(float)global.viewport.screen_width / (float)global.viewport.screen_height);
		}

		// This finds the combined view-projection matrix
		global_matrices.projection_view = MatMatMul(global_matrices.projection, global_matrices.view);
	}

	void drawScene() 
	{
		// Undo the settings of the visibility test.
		glDrawBuffer(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		//

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		//Z BUFFER
		//enable zbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		//Compare depth
		glDepthFunc(GL_LESS);
		calcViewandProjectionMatrices();
		renderWalls();
		renderMachines();
		glFlush();
	}

	void runVisibilityTest()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
		glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		for (MeshInstance* machonk : global.machines)
		{
			machonk->is_visibru = true;

		}
	}

	void update()
	{
		runVisibilityTest();
		drawScene();
	}

	void changeSize(int w, int h) 
	{
		global.viewport.screen_width = w;
		global.viewport.screen_height = h;
		glViewport(0, 0, global.viewport.screen_width, global.viewport.screen_height);
	}

	void keypress(unsigned char key, int x, int y) 
	{
		float position_step = 1.0f;
		Vector pos_adj = { 0.0f, 0.0f, 0.0f };

		switch (key) {
		case 'w':
			pos_adj.z = -position_step;
			pos_adj = euler_rotate(camera.rotation, pos_adj);
			break;
		case 's':
			pos_adj.z = position_step;
			pos_adj = euler_rotate(camera.rotation, pos_adj);
			break;
		case 'a':
			pos_adj.x = -position_step;
			pos_adj = euler_rotate(camera.rotation, pos_adj);
			break;
		case 'd':
			pos_adj.x = position_step;
			pos_adj = euler_rotate(camera.rotation, pos_adj);
			break;
			//up
		case 'q':
			pos_adj.y = position_step;
			break;
			//down
		case 'e':
			pos_adj.y = -position_step;
			break;
		case 'Q':
			glutLeaveMainLoop();
			break;
		}
		camera.move(pos_adj);
		glutPostRedisplay();
	}

	void mouse_dragged(int mouse_x_pos, int mouse_y_pos)
	{
		//cout << "mouse_x_pos " << mouse_x_pos << " mouse_y_pos " << mouse_y_pos << "\n" << "\n";
		float delta_x = (float)(last_mouse_pos[0] - mouse_x_pos);
		float delta_y = (float)(last_mouse_pos[1] - mouse_y_pos);

		last_mouse_pos[0] = mouse_x_pos;
		last_mouse_pos[1] = mouse_y_pos;

		const float angle_step_x = 0.3f;
		const float angle_step_y = 0.3f;

		float local_y = delta_y * angle_step_y;
		float local_x = delta_x * angle_step_x;

		camera.rotate({ local_y, local_x, 0.0f });
		//cout << "delta_x " << delta_x << " delta_y " << delta_y << "\n";
		//cout << "camera_rotation " << cam.rotation.x << ", " << cam.rotation.y << ", " << cam.rotation.z << std::endl;
		glutPostRedisplay();
	}

	void mouse_button_pressed(int button, int state, int mouse_x_pos, int mouse_y_pos)
	{
		if (button == 0)
		{
			//cout << "button " << button << " state " << state << " mouse_x_pos " << mouse_x_pos << " mouse_y_pos " << mouse_y_pos << "\n" << "\n";
			last_mouse_pos[0] = mouse_x_pos;
			last_mouse_pos[1] = mouse_y_pos;
		}
	}

	void createRenderTargets()
	{
		glGenFramebuffers(1, &frame_buffer);
		//glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

		for (GLuint& renderedTexture : render_targets)
		{
			// The texture we're going to render to
			glGenTextures(1, &renderedTexture);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, renderedTexture);

			// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global.viewport.screen_width, global.viewport.screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

			// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

		// The depth buffer
		glGenRenderbuffers(1, &depth_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, global.viewport.screen_width, global.viewport.screen_height);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

		//glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers
	}

	void init(void) 
	{
		// Compile and link the given shader program (vertex shader and fragment shader)
		loadShaderProgram("Shaders/vertex_shader_wall.txt", "Shaders/fragment_shader_wall.txt", global.shaders.wall);
		loadWalls();
		loadMachines();
		createRenderTargets();
	}

	void cleanUp(void) 
	{
		printf("Running cleanUp function... ");
		// Free openGL resources

		// Free meshes

		printf("Done!\n\n");
	}

	void initGlobals()
	{

	}
}

int main(int argc, char** argv)
{
	// Setup freeGLUT	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(edt::global.viewport.screen_width, edt::global.viewport.screen_height);
	glutCreateWindow("Bachelor Thesis edt");
	glutDisplayFunc(edt::update);
	glutReshapeFunc(edt::changeSize);
	glutKeyboardFunc(edt::keypress);
	glutMotionFunc(edt::mouse_dragged);
	glutMouseFunc(edt::mouse_button_pressed);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// Specify your preferred OpenGL version and profile
	glutInitContextVersion(4, 5);
	//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);	
	glutInitContextProfile(GLUT_CORE_PROFILE);

	// Uses GLEW as OpenGL Loading Library to get access to modern core features as well as extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error: " << glewGetErrorString(err));
		return 1;
	}

	// Output OpenGL version info
	printf("GLEW version: " << glewGetString(GLEW_VERSION));
	printf("OpenGL version: " << (const char*)glGetString(GL_VERSION));
	printf("OpenGL vendor: " << glGetString(GL_VENDOR));

	edt::init();
	glutMainLoop();

	edt::cleanUp();
	return 0;
}