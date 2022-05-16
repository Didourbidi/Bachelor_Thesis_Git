#pragma once
#include "common.h"
#include "algebra.h"

namespace edt
{
	class Renderer;
	struct MeshInstance;
	class Camera;

	class PanoramicSceneRenderer
	{
	public:
		PanoramicSceneRenderer(); //constructor
		~PanoramicSceneRenderer(); //destructor -called when the object gets destroyed

		//functions
		void init(Renderer* renderer, const int& resolution_x, const int& resolution_y, const Camera* camera);

		void drawScene(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, std::vector<MeshInstance*>& machines_monochrome);

	private:
		struct Viewport
		{
			int x, y;
			Vector rotation_adjustment;
		};
		
		void createViewports();

		Renderer* renderer;
		int back_buffer_resolution[2];
		int viewport_resolution[2];
		const Camera* camera;
		Viewport viewports[6];
	};
}
