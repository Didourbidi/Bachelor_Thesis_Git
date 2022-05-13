#include "algebra.h"
#include "mesh.h"
#include "common.h"
#include "Camera.h"
#include "ObjFileLoader.h"
#include "mesh_bunny.h"
#include "Renderer.h"
#include "VisibilitySolver3D.h"
#include "SceneGenerator.h"

namespace edt
{
//we tell to the compiler that we know that there are warnings here so that it doesn't complain
#pragma warning( push )
//#pragma warning( disable: 4838 )

#pragma warning( pop )

	Camera camera({ 0.0f, 0.0f, 0.0f },{0, 0, 0 }, 60, 0.1f, 10000, 1.0f ); //global camera parameters
	int last_mouse_pos[] = { 0 , 0 };

	struct 
	{
		Mesh* room[2];
		Mesh* inner_wall_1[2];
		Mesh* l_shape[2];
		Mesh* machine1[2];
	} meshes;

	std::vector<MeshInstance*> walls;
	std::vector<MeshInstance*> machines;
	std::vector<MeshInstance*> walls_monochrome;
	std::vector<MeshInstance*> machines_monochrome;
	Renderer renderer;
	VisibilitySolver3D solver_3d;
	SceneGenerator scene_generator;

	// Global transform matrices
	struct 
	{
		Matrix view, projection, projection_view;
	} global_matrices;

	bool loadMesh(const char* path, Mesh** meshes_out)
	{
		Mesh* mesh_rgb = new Mesh();

		bool result = loadOBJ(path, mesh_rgb->geometry.positions, &mesh_rgb->geometry.normals);
		if (!result)
			return false;

		for (unsigned short i = 0; i < mesh_rgb->geometry.positions.size(); i += 3)
			mesh_rgb->geometry.triangles.push_back({ i, i + 1u, i + 2u });

		mesh_rgb->material.shader_id = global.shaders.rgb;
		renderer.createMeshDeviceResource(*mesh_rgb);

		meshes_out[0] = mesh_rgb;

		Mesh* mesh_monochrome = new Mesh();
		mesh_monochrome->geometry.positions = mesh_rgb->geometry.positions;
		mesh_monochrome->geometry.triangles = mesh_rgb->geometry.triangles;
		mesh_monochrome->material.shader_id = global.shaders.monochrome;

		renderer.createMeshDeviceResource(*mesh_monochrome);

		meshes_out[1] = mesh_monochrome;

		return true;
	}

	void renderWalls()
	{
		if (walls.empty())
			return;

		const GLuint& shader_id = walls.front()->model->material.shader_id;
		renderer.setActiveShader(shader_id);
		
		{
			renderer.send_float3_to_shader("material.color", { 0.6f , 0.6f , 0.6f });
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
		for (int i = 0 ; i<(int)machines.size(); i++ )
		{
			MeshInstance* mesh_instance = machines[i];
			renderer.send_float3_to_shader("material.color", machines_monochrome[i]->is_visible ? visible_color : obscured_color );
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
		solver_3d.runVisibilityTest(walls_monochrome, machines_monochrome, camera.position);
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
		//cout << "button " << button << " state " << state << " mouse_x_pos " << mouse_x_pos << " mouse_y_pos " << mouse_y_pos << "\n" << "\n";
		last_mouse_pos[0] = mouse_x_pos;
		last_mouse_pos[1] = mouse_y_pos;
	}



	void init(void) 
	{
		// Compile and link the given shader program (vertex shader and fragment shader)
		renderer.loadShaderProgram("Shaders/vertex_shader_rgb.txt", "Shaders/fragment_shader_rgb.txt", global.shaders.rgb);
		renderer.loadShaderProgram("Shaders/vertex_shader_monochrome.txt", "Shaders/fragment_shader_monochrome.txt", global.shaders.monochrome);
		loadMesh("Models/machine1.obj", meshes.machine1);
		loadMesh("Models/test_room.obj", meshes.room);
		loadMesh("Models/Lshape.obj", meshes.l_shape);
		loadMesh("Models/inner_wall.obj", meshes.inner_wall_1);

		solver_3d.init(&renderer, global.visibility_test_rt_size);
		scene_generator.generateScene(meshes.room, meshes.inner_wall_1, meshes.l_shape, meshes.machine1, walls, walls_monochrome, machines, machines_monochrome);
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