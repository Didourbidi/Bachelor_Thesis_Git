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
		void renderMesh(const MeshInstance& mesh_instance);
		void send_float3_to_shader(const char* name, const Vector& value);
		void send_float_to_shader(const char* name, const float& value);

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

		bool create_shader_from_file(const int& shader_type, const char* file_name, GLuint& shader_id);
		bool matrices_dirty; //if true - recalculate
		bool shader_dirty; //if true - refeed variables etc

		GLuint shader_id; //active shader

	};
}