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

		void loadScene(const char* file_name);

	private:
		struct Vector2
		{
			bool operator == (const Vector2& rhs) const;
			Vector2 operator + (const Vector2& rhs) const;
			void operator += (const Vector2& rhs);
			Vector2 operator - (const Vector2& rhs) const;
			void operator /= (const float& rhs);
			void normalize();
			float cross(const Vector2& rhs) const;
			float distanceSq(const Vector2& rhs) const;
			float x, y;
		};

		static void createMeshInstance( Mesh** mesh, const Vector& position, const Vector& rotation, const Vector& scaling,
			std::vector<MeshInstance*>& instances_rgb, std::vector<MeshInstance*>& instances_monochrome);

		void saveScene(const std::vector<MeshInstance*>& machines_monochrome, const std::vector<unsigned char>& mesh_types, 
			const std::vector<Vector>& mesh_scalings);

		void calculateConvexHull(const std::vector<Vector2>& all_points, std::vector<Vector2>& hull );

	};
}