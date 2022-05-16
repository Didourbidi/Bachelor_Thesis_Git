#pragma once
#include "common.h"
#include "algebra.h"

namespace edt
{
	class Renderer;
	struct MeshInstance;
	class Camera;

	class SingleSceneRenderer
	{
	public:
		SingleSceneRenderer(); //constructor
		~SingleSceneRenderer(); //destructor -called when the object gets destroyed

		//functions
		void init(Renderer* renderer, const int& resolution_x, const int& resolution_y, const Camera* camera);

		void drawScene(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, std::vector<MeshInstance*>& machines_monochrome);

	private:
		Renderer* renderer;
		int back_buffer_resolution[2];
		const Camera* camera;
	};
}
