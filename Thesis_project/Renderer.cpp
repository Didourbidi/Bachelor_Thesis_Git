#include "Renderer.h"
#include "common.h"

namespace edt
{
	void Renderer::createMeshDeviceResource(Mesh& mesh)
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

	void Renderer::renderMesh(const MeshInstance& mesh_instance)
	{
		if (matrices_dirty)
			calculateProjViewMatrix();

		if (shader_dirty)
		{
			applyGlobalMatrices();
			shader_dirty = false;
		}

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

	//send 
	void Renderer::send_float3_to_shader(const char* name, const Vector& value)
	{
		// gives a handle to the specific piece of data which we will use in order to access it later
		GLint h_handle = glGetUniformLocation(shader_id, name);

		//send information to shader
		glUniform3f(h_handle, value.x, value.y, value.z);
	}

	void Renderer::send_float_to_shader(const char* name, const float& value)
	{
		// gives a handle to the specific piece of data which we will use in order to access it later
		GLint h_handle = glGetUniformLocation(shader_id, name);

		//send information to shader
		glUniform1f(h_handle, value);
	}

	//take info from txt file to create shader
	bool Renderer::create_shader_from_file(const int& shader_type, const char* file_name, GLuint& shader_id)
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

	bool Renderer::loadShaderProgram(const char* vs_filename, const char* fs_filename, GLuint& shader_id)
	{
		shader_id = glCreateProgram();

		GLuint vertex_shader_id, fragment_shader_id;
		if (!create_shader_from_file(GL_VERTEX_SHADER, vs_filename, vertex_shader_id))
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

	void Renderer::setViewMatrix(const Matrix& view_mat)
	{
		view_matrix = view_mat;
		matrices_dirty = true;
	}

	void Renderer::setProjectionMatrix(const Matrix& proj_mat)
	{
		projection_matrix = proj_mat;
		matrices_dirty = true;
	}

	void Renderer::setActiveShader(const GLuint& new_shader_id)
	{
		if (new_shader_id == shader_id)
			return;

		glUseProgram(new_shader_id);
		shader_id = new_shader_id;
		shader_dirty = true;
	}

	void Renderer::applyGlobalMatrices()
	{
		// Pass the view transform to the shader
		GLint loc_view_mat = glGetUniformLocation(shader_id, "view_matrix");
		glUniformMatrix4fv(loc_view_mat, 1, GL_FALSE, view_matrix.e);

		// Pass the proj transform to the shader
		GLint loc_proj_mat = glGetUniformLocation(shader_id, "proj_matrix");
		glUniformMatrix4fv(loc_proj_mat, 1, GL_FALSE, projection_matrix.e);

		// Pass the viewing transform to the shader
		GLint loc_PV = glGetUniformLocation(shader_id, "proj_view_matrix");
		glUniformMatrix4fv(loc_PV, 1, GL_FALSE, projection_view_matrix.e);
	}

	void Renderer::calculateProjViewMatrix()
	{
		// This finds the combined view-projection matrix
		projection_view_matrix = MatMatMul(projection_matrix, view_matrix);
		matrices_dirty = false;
		shader_dirty = true;
	}

}