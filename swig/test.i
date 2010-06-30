%module test;

%{
#include <stdlib.h>
struct vector_s {
	float x, y, z;
};
typedef struct vector_s Vector;
%}

typedef struct vector_s {
	float x, y, z;
} Vector;

%extend Vector {
	Vector(float x, float y, float z) {
		Vector *v = calloc(1, sizeof(*v));
		v->x = x;
		v->y = y;
		v->z = z;
		return v;
	}

	void add(Vector *o) {
		$self->x += o->x;
		$self->y += o->y;
		$self->z += o->z;
	}
};
