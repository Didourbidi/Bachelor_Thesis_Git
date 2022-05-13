#include "SceneGenerator.h"
#include "mesh.h"
#include "algebra.h"

namespace edt
{
	void SceneGenerator::generateScene(Mesh** room_mesh, Mesh** inner_wall_mesh, Mesh** l_shape_mesh, Mesh** machine1_mesh,
		std::vector<MeshInstance*>& walls_rgb, std::vector<MeshInstance*>& walls_monochrome, std::vector<MeshInstance*>& machines_rgb,
		std::vector<MeshInstance*>& machines_monochrome)
	{
		createMeshInstance(room_mesh, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 0, 0, -10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 0, 0, 10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { -10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);

		auto create_machine_cluster = [&l_shape_mesh, &machine1_mesh, &machines_rgb, &machines_monochrome](const Vector& offset)
		{
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 2, 0, -6 }), { 0, 30, 0 }, { 0.5f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
			SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { 4, 1, 3 }), { 0, 150, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
			SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { -7, 1, 0 }), { 0, 90, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
		};

		create_machine_cluster({ 10, 0, 10 });
		create_machine_cluster({ 10, 0, -10 });
		create_machine_cluster({ -10, 0, -10 });
		create_machine_cluster({ -10, 0, 10 });

	}

	void SceneGenerator::createMeshInstance(Mesh** mesh, const Vector& position, const Vector& rotation, const Vector& scaling,
		std::vector<MeshInstance*>& instances_rgb, std::vector<MeshInstance*>& instances_monochrome)
	{
		MeshInstance* instance_rgb = new MeshInstance();
		instance_rgb->model = mesh[0];
		instance_rgb->translation = position;
		instance_rgb->rotation = rotation;
		instance_rgb->scaling = scaling;

		instances_rgb.push_back(instance_rgb);

		auto* instance_monochrome = new MeshInstance();
		instance_monochrome->model = mesh[1];
		instance_monochrome->translation = instance_rgb->translation;
		instance_monochrome->rotation = instance_rgb->rotation;
		instance_monochrome->scaling = instance_rgb->scaling;

		instances_monochrome.push_back(instance_monochrome);
	}
}