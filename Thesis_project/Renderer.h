#pragma once
#include "mesh.h"
#include "common.h"
#include "algebra.h"

namespace edt
{
	class Renderer
	{
	public:
		void createMeshDeviceResource(Mesh& mesh);
		void prepareBatch(const MeshInstance& mesh_instance);
		void batchRenderMesh(const MeshInstance& mesh_instance);
		void renderMesh(const MeshInstance& mesh_instance);
		void renderMeshInstances(const Mesh& mesh, const int& count);
		void send_float3_to_shader(const char* name, const Vector& value);
		void send_float_to_shader(const char* name, const float& value);
		void send_int_to_shader(const char* name, const int& value);
		void send_matrices_to_shader(const char* name, const Matrix* p_values, const int& count);

		bool loadShaderProgram(const char* vs_filename, const char* fs_filename, GLuint& shader_id);

		void setViewMatrix(const Matrix& view_mat);
		void setProjectionMatrix(const Matrix& proj_mat);
		void setActiveShader(const GLuint& new_shader_id);

	private:
		void applyGlobalMatrices();
		void calculateProjViewMatrix();

		Matrix view_matrix;
		Matrix projection_matrix;
		Matrix projection_view_matrix;
		GLint batch_model_matrix_handle;

		bool create_shader_from_file(const int& shader_type, const char* file_name, GLuint& shader_id);
		bool matrices_dirty; //if true - recalculate
		bool shader_dirty; //if true - refeed variables etc

		GLuint shader_id; //active shader

	};
}