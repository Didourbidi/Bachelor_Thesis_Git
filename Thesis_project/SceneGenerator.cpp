#include "SceneGenerator.h"
#include "mesh.h"
#include "algebra.h"

namespace edt
{
	void SceneGenerator::generateScene(Mesh** room_mesh, Mesh** inner_wall_mesh, Mesh** l_shape_mesh, Mesh** machine1_mesh,
		std::vector<MeshInstance*>& walls_rgb, std::vector<MeshInstance*>& walls_monochrome, std::vector<MeshInstance*>& machines_rgb,
		std::vector<MeshInstance*>& machines_monochrome)
	{
		float grid_size = 100; //number of squares
		size_t number_of_squares = (size_t)(grid_size * grid_size);
		size_t wanted_nr_of_machines = 2 * (size_t) (number_of_squares * 0.5f);
		//size_t wanted_nr_of_machines = 10;
		float grid_square_size = 16; //meters
		const Vector square_center_position_min = {-grid_square_size * (grid_size * 0.5f - 0.5f), 0.0f, -grid_square_size * (grid_size * 0.5f - 0.5f) }; 
		Vector square_center_position = square_center_position_min;

		std::vector<uint8_t> spawn_mask(number_of_squares, 0u);
		size_t nr_to_spawn = wanted_nr_of_machines;
		while (nr_to_spawn)
		{
			size_t i = rand() % number_of_squares;
			uint8_t i_machine = rand() % 2 ? 1 : 2;
			if (!(spawn_mask[i] & i_machine))
			{
				spawn_mask[i] |= i_machine;
				nr_to_spawn--;
			}
		}

		auto create_machine_1 = [&l_shape_mesh, &machines_rgb, &machines_monochrome](const Vector& offset)
		{
			float y_rot = -180.0f + rand() % 360; // random rotation around Y.
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 2, 0, -6 }), { 0, y_rot, 0 }, { 1.0f, 1.5f, 1.0f }, machines_rgb, machines_monochrome);
		};

		auto create_machine_2 = [&l_shape_mesh, &machines_rgb, &machines_monochrome](const Vector& offset)
		{
			float y_rot = -180.0f + rand() % 360; // random rotation around Y.
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 6, 0, -4 }), { 0, y_rot, 0 }, { 0.5f, 0.5f, 0.5f }, machines_rgb, machines_monochrome);
		};

		for(float i_x = 0; i_x <grid_size; i_x++)
		{
			for (float i_z = 0; i_z < grid_size; i_z++)
			{
				size_t i_spawn_mask = (size_t)(i_z + i_x * grid_size);
				if(spawn_mask[i_spawn_mask] & 1)
					create_machine_1(square_center_position);

				if (spawn_mask[i_spawn_mask] & 2)
					create_machine_2(square_center_position);

				square_center_position.z += grid_square_size;
			}
			//every time we pass a row we go to the next
			square_center_position.x += grid_square_size;
			//once all z are done we reset it to min
			square_center_position.z = square_center_position_min.z; 
		}

		createMeshInstance(room_mesh, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		//createMeshInstance(inner_wall_mesh, { 0, 0, -10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		//createMeshInstance(inner_wall_mesh, { 0, 0, 10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		//createMeshInstance(inner_wall_mesh, { 10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		//createMeshInstance(inner_wall_mesh, { -10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);

		/*auto create_machine_cluster = [&l_shape_mesh, &machine1_mesh, &machines_rgb, &machines_monochrome](const Vector& offset)
		{
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 2, 0, -6 }), { 0, 30, 0 }, { 1.0f, 1.5f, 1.0f }, machines_rgb, machines_monochrome);
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 6, 0, -4 }), { 0, -90, 0 }, { 0.5f, 0.5f, 0.5f }, machines_rgb, machines_monochrome);

			//SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { 4, 1, 3 }), { 0, 150, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
			//SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { -7, 1, 0 }), { 0, 90, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
		};

		create_machine_cluster({ 10, 0, 10 });
		create_machine_cluster({ 10, 0, -10 });
		create_machine_cluster({ -10, 0, -10 });
		create_machine_cluster({ -10, 0, 10 });*/

	}

	void SceneGenerator::createMeshInstance(Mesh** mesh, const Vector& position, const Vector& rotation, const Vector& scaling,
		std::vector<MeshInstance*>& instances_rgb, std::vector<MeshInstance*>& instances_monochrome)
	{
		MeshInstance* instance_rgb = new MeshInstance();
		instance_rgb->model = mesh[0];
		instance_rgb->translation = position;
		instance_rgb->rotation = rotation;
		instance_rgb->scaling = scaling;
		instance_rgb->calculateTransform();

		instances_rgb.push_back(instance_rgb);

		auto* instance_monochrome = new MeshInstance();
		instance_monochrome->model = mesh[1];
		instance_monochrome->translation = instance_rgb->translation;
		instance_monochrome->rotation = instance_rgb->rotation;
		instance_monochrome->scaling = instance_rgb->scaling;
		instance_monochrome->calculateTransform();

		instances_monochrome.push_back(instance_monochrome);
	}
}