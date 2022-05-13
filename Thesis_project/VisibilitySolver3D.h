#pragma once
#include "common.h"
#include "algebra.h"

namespace edt
{
	class Renderer;
	struct MeshInstance;
	struct Matrix;
	struct Vector;

	class VisibilitySolver3D
	{
	public:
		VisibilitySolver3D(); //constructor
		~VisibilitySolver3D(); //destructor -called when the object gets destroyed

		//functions
		void init(Renderer* renderer, int resolution);
		void runVisibilityTest(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, 
			const Vector& camera_position);

	private:
		struct Pixel
		{
			uint8_t r;
		};

		void createRenderTargets();
		void createViewMatrices();
		Matrix updateViewMatrix(const Matrix& mat, const Matrix& translation_matrix);
		void createProjectionMatrix();
		bool testMachineVisibility(MeshInstance* machine);
		void calculateLastMipLevel();

		GLuint render_target;
		GLuint depth_buffer;
		GLuint frame_buffer;
		GLenum draw_buffers[1];
		Renderer* renderer;
		int resolution;

		Matrix round_view[6];
		Matrix projection_matrix;
		std::vector<Pixel> texture_data;
		size_t last_mip_resolution;
		GLint mip_level;
	};
}
