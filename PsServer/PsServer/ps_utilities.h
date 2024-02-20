#pragma once
#include "parasolid_kernel.h"

double PK_VECTOR_dot(PK_VECTOR_t v1, PK_VECTOR_t v2)
{
	return v1.coord[0] * v2.coord[0] + v1.coord[1] * v2.coord[1] + v1.coord[2] * v2.coord[2];
}

PK_VECTOR_t PK_VECTOR_cross(PK_VECTOR_t v1, PK_VECTOR_t v2)
{
	PK_VECTOR_t cross;
	cross.coord[0] = v1.coord[1] * v2.coord[2] - v1.coord[2] * v2.coord[1];
	cross.coord[1] = v1.coord[2] * v2.coord[0] - v1.coord[0] * v2.coord[2];
	cross.coord[2] = v1.coord[0] * v2.coord[1] - v1.coord[1] * v2.coord[0];

	return cross;
}