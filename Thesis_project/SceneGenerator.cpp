#include "SceneGenerator.h"
#include "mesh.h"
#include "algebra.h"

#include <fstream>

namespace edt
{
	bool SceneGenerator::Vector2::operator == (const Vector2& rhs) const 
	{ 
		return x == rhs.x && y == rhs.y; 
	}

	SceneGenerator::Vector2 SceneGenerator::Vector2::operator + (const Vector2& rhs) const
	{
		return { x + rhs.x, y + rhs.y };
	}

	void SceneGenerator::Vector2::operator += (const Vector2& rhs)
	{
		x += rhs.x;
		y += rhs.y;
	}

	SceneGenerator::Vector2 SceneGenerator::Vector2::operator - (const Vector2& rhs) const
	{
		return { x - rhs.x, y - rhs.y };
	}

	void SceneGenerator::Vector2::operator /= (const float& rhs)
	{
		x /= rhs;
		y /= rhs;
	}

	void SceneGenerator::Vector2::normalize()
	{
		float length = sqrtf(x * x + y * y);
		x /= length;
		y /= length;
	}

	float SceneGenerator::Vector2::cross(const Vector2& rhs) const
	{
		float v = x * rhs.y - y * rhs.x;
		return v;
	}

	float SceneGenerator::Vector2::distanceSq(const Vector2& rhs) const
	{
		return x * x + y * y;
	}

	void SceneGenerator::generateScene(Mesh** room_mesh, Mesh** inner_wall_mesh, Mesh** l_shape_mesh, Mesh** machine1_mesh,
		std::vector<MeshInstance*>& walls_rgb, std::vector<MeshInstance*>& walls_monochrome, std::vector<MeshInstance*>& machines_rgb,
		std::vector<MeshInstance*>& machines_monochrome)
	{
#if 1
		size_t nr_of_scenes = 1;
		for (size_t i_scene = 0; i_scene < nr_of_scenes; i_scene++)
		{
			walls_rgb.clear();
			walls_monochrome.clear();
			machines_rgb.clear();
			machines_monochrome.clear();

			srand((unsigned int)i_scene+0); // Adjust randomizer seed here.
			float grid_size = 100; //number of squares
			size_t number_of_squares = (size_t)(grid_size * grid_size);
			size_t wanted_nr_of_machines = 2 * (size_t)(number_of_squares * 0.5f);
			//size_t wanted_nr_of_machines = 1000;
			float grid_square_size = 16; //meters
			const Vector square_center_position_min = { -grid_square_size * (grid_size * 0.5f - 0.5f), 0.0f, -grid_square_size * (grid_size * 0.5f - 0.5f) };
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

			std::vector<unsigned char> mesh_types(wanted_nr_of_machines);
			std::vector<Vector> mesh_scalings = { { 1.0f, 1.5f, 1.0f } , { 0.5f, 0.5f, 0.5f } };

			auto create_machine_1 = [&l_shape_mesh, &machines_rgb, &machines_monochrome, &mesh_types, &mesh_scalings](const Vector& offset)
			{
				mesh_types[machines_monochrome.size()] = 0;
				float y_rot = -180.0f + rand() % 360; // random rotation around Y.
				SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 2, 0, -6 }), { 0, y_rot, 0 }, mesh_scalings[0], machines_rgb, machines_monochrome);
			};

			auto create_machine_2 = [&l_shape_mesh, &machines_rgb, &machines_monochrome, &mesh_types, &mesh_scalings](const Vector& offset)
			{
				mesh_types[machines_monochrome.size()] = 1;
				float y_rot = -180.0f + rand() % 360; // random rotation around Y.
				SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 6, 0, -4 }), { 0, y_rot, 0 }, mesh_scalings[1], machines_rgb, machines_monochrome);
			};

			for (float i_x = 0; i_x < grid_size; i_x++)
			{
				for (float i_z = 0; i_z < grid_size; i_z++)
				{
					size_t i_spawn_mask = (size_t)(i_z + i_x * grid_size);
					if (spawn_mask[i_spawn_mask] & 1)
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

			const char* file_extension = ".txt";
			const char* file_prefix = "scene_data_";

			std::stringstream ss;
			ss << file_prefix << (i_scene + 1) << file_extension;
			//saveScene(ss.str().c_str(), machines_monochrome, mesh_types, mesh_scalings);
			//loadScene(ss.str().c_str());		
		}
#else
		createMeshInstance(room_mesh, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 0, 0, -10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 0, 0, 10 }, { 0, 0, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { 10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);
		createMeshInstance(inner_wall_mesh, { -10, 0, 0 }, { 0, 90, 0 }, { 1, 1, 1 }, walls_rgb, walls_monochrome);

		auto create_machine_cluster = [&l_shape_mesh, &machine1_mesh, &machines_rgb, &machines_monochrome](const Vector& offset)
		{
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 2, 0, -6 }), { 0, 30, 0 }, { 1.0f, 1.5f, 1.0f }, machines_rgb, machines_monochrome);
			SceneGenerator::createMeshInstance(l_shape_mesh, Add(offset, { 6, 0, -4 }), { 0, -90, 0 }, { 0.5f, 0.5f, 0.5f }, machines_rgb, machines_monochrome);

			SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { 4, 1, 3 }), { 0, 150, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
			SceneGenerator::createMeshInstance(machine1_mesh, Add(offset, { -7, 1, 0 }), { 0, 90, 0 }, { 1.0f, 1.0f, 0.5f }, machines_rgb, machines_monochrome);
		};

		create_machine_cluster({ 10, 0, 10 });
		create_machine_cluster({ 10, 0, -10 });
		create_machine_cluster({ -10, 0, -10 });
		create_machine_cluster({ -10, 0, 10 });
#endif
	}

	void SceneGenerator::loadScene(const char* file_name)
	{
		struct Vector
		{
			bool operator == (const Vector& other) const
			{
				return x == other.x && y == other.y && z == other.z;
			}

			Vector operator - () const
			{
				return { -x, -y, -z };
			}

			float x, y, z;
		};

		struct Vector2
		{
			float x, y;
		};

		std::vector<Vector2> hull;

		std::ifstream scene_file(file_name, std::ofstream::binary );
		uint32_t nr_mesh_scalings;
		scene_file >> nr_mesh_scalings;
		std::vector<unsigned char> mesh_types;
		std::vector<Vector> mesh_scalings(nr_mesh_scalings);
		scene_file.read((char*)mesh_scalings.data(), sizeof(Vector) * mesh_scalings.size());
		uint32_t hull_vertices_size;
		scene_file >> hull_vertices_size;
		hull.resize(hull_vertices_size);
		scene_file.read((char*)hull.data(), sizeof(Vector2) * hull.size());
		uint32_t nr_of_machines;
		scene_file >> nr_of_machines;
		mesh_types.resize(nr_of_machines);
		scene_file.read((char*)mesh_types.data(), sizeof(unsigned char) * mesh_types.size());

		struct MachineDef
		{
			Vector scaling;
		};

		struct Machine
		{
			Vector position;
			Vector rotation;
			unsigned def_index;
		};

		std::vector<MachineDef> machine_definitions(mesh_scalings.size());
		for (size_t i = 0; i < machine_definitions.size(); i++)
			machine_definitions[i].scaling = mesh_scalings[i];
		
		std::vector<Machine> machines(nr_of_machines);
		for (size_t i = 0; i < nr_of_machines; i++)
		{
			Machine& machine = machines[i];
			machine.position.y = 0;
			machine.rotation.x = 0;
			machine.rotation.z = 0;

			machine.def_index = mesh_types[i];
			scene_file.read((char*)&machine.position.x, sizeof(float));
			scene_file.read((char*)&machine.position.z, sizeof(float));
			scene_file.read((char*)&machine.rotation.y, sizeof(float));
		}
		scene_file.close();

		struct point2d 
		{
			float x, y;
		};
		struct polygon2d 
		{
			int nvertices;
			point2d* vertices;
		};

		std::vector<polygon2d> machines_2d(machines.size());
		for (size_t i = 0; i < machines_2d.size(); i++)
		{
			polygon2d& poly2d = machines_2d[i];
			const Machine& machine = machines[i];
			poly2d.nvertices = (int)hull.size();
			poly2d.vertices = (point2d*)malloc(sizeof(point2d) * poly2d.nvertices);

			for (size_t j = 0; j < (size_t)poly2d.nvertices; j++)
			{
				point2d position2d;
				position2d.x = hull[j].x * cos(machine.rotation.y) + hull[j].y * sin(machine.rotation.y);
				position2d.x *= mesh_scalings[machine.def_index].x;
				position2d.y = -hull[j].x * sin(machine.rotation.y) + hull[j].y * cos(machine.rotation.y);
				position2d.y *= mesh_scalings[machine.def_index].z;
				poly2d.vertices[j] = position2d;
				poly2d.vertices[j].x += machine.position.x;
				poly2d.vertices[j].y += machine.position.z;
			}
		}
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

	
	void SceneGenerator::saveScene(const char* file_name, const std::vector<MeshInstance*>& machines_monochrome, const std::vector<unsigned char>& mesh_types,
		const std::vector<Vector>& mesh_scalings)
	{
		if (machines_monochrome.empty())
			return;

		std::vector<Vector2> positions;
		const Mesh& mesh = *machines_monochrome.front()->model;

		for (size_t i_0 = 0; i_0 < mesh.geometry.positions.size(); i_0++)
		{
			const Vector& position_3d = mesh.geometry.positions[i_0];
			Vector2 position_2d = { position_3d.x, position_3d.z };
			bool is_unique = true;

			for (const Vector2& other_position_2d : positions)
			{
				if (position_2d == other_position_2d)
				{
					is_unique = false;
					break;
				}
			}

			if (is_unique)
				positions.push_back(position_2d);
		}

		std::vector<Vector2> hull;
		calculateConvexHull(positions, hull);
		std::ofstream scene_file(file_name, std::ofstream::binary | std::ofstream::trunc);
		//scene
		scene_file << (uint32_t)mesh_scalings.size();
		scene_file.write((const char*)mesh_scalings.data(), sizeof(Vector) * mesh_scalings.size());
		//convex hull - 2d representation
		scene_file << (uint32_t)hull.size();
		scene_file.write((const char*)hull.data(), sizeof(Vector2) * hull.size());

		scene_file << (uint32_t)mesh_types.size();
		scene_file.write((const char*)mesh_types.data(), sizeof(unsigned char)* mesh_types.size());
		for (size_t i=0; i < machines_monochrome.size(); i++)
		{
			const MeshInstance* machine = machines_monochrome[i];
			scene_file.write((const char*)&machine->translation.x, sizeof(float) );
			scene_file.write((const char*)&machine->translation.z, sizeof(float));
			scene_file.write((const char*)&machine->rotation.y, sizeof(float));
		}

		scene_file.close();
	}

	void SceneGenerator::calculateConvexHull(const std::vector<Vector2>& all_points, std::vector<Vector2>& hull)
	{
		Vector2 com = { 0, 0 };

		for (const Vector2& position : all_points)
			com += position;
		
		com /= (float)all_points.size(); //center of mass

		size_t i_last_hull_vertex = 0;
		float longest_distance_sq = 0;
		for (size_t i = 0; i < all_points.size(); i++)
		{
			float test_distance_sq = all_points[i].distanceSq(com);
			if (test_distance_sq > longest_distance_sq)
			{
				longest_distance_sq = test_distance_sq;
				i_last_hull_vertex = i;
			}
		}

		hull.push_back(all_points[i_last_hull_vertex]);
		std::vector<bool> hull_vertices_lookup(all_points.size(), false);
		hull_vertices_lookup[i_last_hull_vertex] = true;

		for (;;)
		{
			float biggest_cross_product = 0;
			Vector2 p_0;
			Vector2 dir_0;
			size_t i_p_0 = i_last_hull_vertex;

			for (size_t i = 0; i < all_points.size(); i++)
			{
				if (!hull_vertices_lookup[i])
				{
					p_0 = all_points[i];
					dir_0 = p_0 - hull.back();
					dir_0.normalize();
					i_p_0 = i;

					for (size_t j = 0; j < all_points.size(); j++)
					{
						if (j != i && j != i_last_hull_vertex)
						{
							const Vector2& p_1 = all_points[j];
							Vector2 dir_1 = p_1 - hull.back();
							dir_1.normalize();
							float test_cross = dir_0.cross(dir_1);

							if (test_cross > biggest_cross_product)
							{
								p_0 = p_1;
								dir_0 = dir_1;
								i_p_0 = j;
								biggest_cross_product = test_cross;
							}
						}
					}

					break;
				}
			}

			if (hull_vertices_lookup[i_p_0])
				break;
			else
			{
				hull_vertices_lookup[i_p_0] = true;
				hull.push_back(p_0);
				i_last_hull_vertex = i_p_0;
			}
		}
	}
}