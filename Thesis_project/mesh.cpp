#include <stdlib.h>
#include "mesh.h"
#include <string.h>
#include "common.h"
#include <math.h>

namespace edt
{
	void MeshInstance::calculateTransform() 
	{
		//position and rotation are inverted (view matrix is the inverse transform of the camera)	
		transform = create_Translation_mat(translation);
		transform = MatMatMul(transform, create_Scaling_mat(scaling));
		transform = MatMatMul(transform, create_Rotation_xmat(rotation.x));
		transform = MatMatMul(transform, create_Rotation_ymat(rotation.y));
		transform = MatMatMul(transform, create_Rotation_zmat(rotation.z));
	}
}