#include "algebra.h"
#include "mesh.h"
#include "common.h"
#include "Camera.h"
#include "ObjFileLoader.h"
#include "mesh_bunny.h"
#include "Renderer.h"
#include "VisibilitySolver3D.h"

namespace edt
{
//we tell to the compiler that we know that there are warnings here so that it doesn't complain
#pragma warning( push )
//#pragma warning( disable: 4838 )

#pragma warning( pop )

	Camera camera({ 0.0f, 0.0f, 0.0f },{0, 0, 0 }, 60, 1, 10000, 1.0f ); //global camera parameters
	int last_mouse_pos[] = { 0 , 0 };

	std::vector<MeshInstance*> walls;
	std::vector<MeshInstance*> machines;
	Renderer renderer;
	VisibilitySolver3D solver_3d;

	// Global transform matrices
	struct 
	{
		Matrix view, projection, projection_view;
	} global_matrices;

	void loadWalls()
	{
		Mesh* mesh = new Mesh();
		
		bool result = loadOBJ("Models/empty_room.obj", mesh->geometry.positions, &mesh->geometry.normals);
		if (!result)
			return;
		
		for (unsigned short i = 0; i < mesh->geometry.positions.size(); i += 3)
			mesh->geometry.triangles.push_back({ i, i + 1u, i + 2u });

		mesh->material.shader_id = global.shaders.wall;

		renderer.createMeshDeviceResource(*mesh);

		MeshInstance* instance = new MeshInstance();
		instance->model = mesh;
		instance->translation = { 0,0,0 };
		instance->rotation = { 0,0,0 };
		instance->scaling = { 1,1,1 };

		walls.push_back(instance);
	}

	void loadMachines()
	{
		Mesh* mesh = new Mesh();

		bool result = loadOBJ("Models/Lshape.obj", mesh->geometry.positions, &mesh->geometry.normals);
		if (!result)
			return;

		for (unsigned short i = 0; i < mesh->geometry.positions.size(); i += 3)
			mesh->geometry.triangles.push_back({ i, i + 1u, i + 2u });

		mesh->material.shader_id = global.shaders.wall;

		renderer.createMeshDeviceResource(*mesh);

		for (size_t i = 0; i < 1; i++)
		{
			MeshInstance* instance = new MeshInstance();
			instance->model = mesh;
			instance->translation = { 0,0,0 };
			instance->rotation = { 0,0,0 };
			instance->scaling = { 1,1,1 };

			machines.push_back(instance);
		}

		machines[0]->translation = { 5, 1, 10 };
		machines[0]->rotation = { 0, 30, 0 };
		machines[0]->scaling = { 0.5f, 1.0f ,0.5f };
		/*
		global.machines[1]->translation = { -12, 1, 6 };
		global.machines[1]->rotation = { 0, -10, 0 };
		global.machines[1]->scaling = { 0.3f, 1.0f ,0.3f };

		global.machines[2]->translation = { 7, 1, -5 };
		global.machines[2]->rotation = { 0, 75, 0 };
		global.machines[2]->scaling = { 0.7f, 1.0f ,0.7f };*/
	}

	void renderWalls()
	{
		if (walls.empty())
			return;

		const GLuint& shader_id = walls.front()->model->material.shader_id;
		renderer.setActiveShader(shader_id);
		
		{
			renderer.send_float3_to_shader("light.color", { 0.0f , 0.0f , 1.0f });
		}

		for (MeshInstance* mesh_instance : walls)
		{
			renderer.renderMesh(*mesh_instance);
		}
	}

	void renderMachines()
	{
		if (machines.empty())
			return;

		const GLuint& shader_id = machines.front()->model->material.shader_id;

		renderer.setActiveShader(shader_id);

		Vector visible_color = { 0.0f , 1.0f , 0.0f }, obscured_color = { 1.0f , 0.0f , 0.0f };
		for (MeshInstance* mesh_instance : machines)
		{
			renderer.send_float3_to_shader("light.color", mesh_instance->is_visible ? visible_color : obscured_color );
			renderer.renderMesh(*mesh_instance);
		}
	}

	void drawScene() 
	{
		renderer.setViewMatrix(camera.get_view_matrix());
		renderer.setProjectionMatrix(camera.get_projection_matrix());

		// Undo the settings of the visibility test.
		glDrawBuffer(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glDepthMask(GL_TRUE); //enable to write to depth buffer

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		//Z BUFFER
		//enable zbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		//Compare depth
		glDepthFunc(GL_LESS);

		renderWalls();
		renderMachines();
		glFlush();
	}

	void update()
	{
		solver_3d.runVisibilityTest(walls, machines, camera.position);
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



	void init(void) 
	{
		// Compile and link the given shader program (vertex shader and fragment shader)
		renderer.loadShaderProgram("Shaders/vertex_shader_wall.txt", "Shaders/fragment_shader_wall.txt", global.shaders.wall);
		loadWalls();
		loadMachines();
		solver_3d.init(&renderer, global.visibility_test_rt_size);
	}

	void cleanUp(void) 
	{
		printf("Running cleanUp function... ");
		// Free openGL resources

		// Free meshes

		printf("Done!\n\n");
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