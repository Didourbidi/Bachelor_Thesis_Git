#pragma once
#include "common.h"
#include "algebra.h"

#include <thread>


#if 0
#define RENDER_TARGET_PIXEL_FORMAT GL_RED_INTEGER
#define RENDER_TARGET_PIXEL_DATA_TYPE GL_UNSIGNED_INT
#define RENDER_TARGET_INTERNAL_FORMAT GL_R16
#define PIXEL_TYPE unsigned
#else
#define RENDER_TARGET_PIXEL_FORMAT GL_RED_INTEGER
#define RENDER_TARGET_PIXEL_DATA_TYPE GL_INT
#define RENDER_TARGET_INTERNAL_FORMAT GL_R32I
#define PIXEL_TYPE int
#endif

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
			PIXEL_TYPE r;
		};

		struct View
		{
			Matrix mat;
			Vector fwd;
		};

		struct InstanceInfo
		{
			Vector dir_to_root;
		};

		void createRenderTargets();
		void createViewMatrices();
		Matrix updateViewMatrix(const Matrix& mat, const Matrix& translation_matrix);
		void createProjectionMatrix();
		void fetchRenderTargetData(bool wait_workers);

		bool testFrustum(MeshInstance* machine, const Vector& cam_fwd, const Vector& dir_to_root);
		void testVisibilities( const std::vector<MeshInstance*>& machines, const std::vector<size_t>& index_lookup);
		void test_pixels_interleaved(const std::vector<MeshInstance*>& machines, const size_t& interleave, 
			const std::vector<size_t>& index_lookup);

		GLuint render_target;
		GLuint depth_buffer;
		GLuint frame_buffer;
		GLenum draw_buffers[1];
		Renderer* renderer;
		int resolution;

		View round_view[6];
		Matrix projection_matrix;
		std::vector<Pixel> texture_data;
		std::vector<std::thread> workers;
		size_t nr_workers;
	};
}
