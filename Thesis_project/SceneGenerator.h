#pragma once
#include <vector>

namespace edt
{
	struct Mesh;
	struct MeshInstance;
	struct Vector;

	class SceneGenerator
	{
	public:
		void generateScene( Mesh** room_mesh, Mesh** inner_wall_mesh, Mesh** l_shape_mesh, Mesh** machine1_mesh,
			std::vector<MeshInstance*>& walls_rgb, std::vector<MeshInstance*>& walls_monochrome, std::vector<MeshInstance*>& machines_rgb, 
			std::vector<MeshInstance*>& machines_monochrome);

	private:
		static void createMeshInstance( Mesh** mesh, const Vector& position, const Vector& rotation, const Vector& scaling,
			std::vector<MeshInstance*>& instances_rgb, std::vector<MeshInstance*>& instances_monochrome);
	};
}