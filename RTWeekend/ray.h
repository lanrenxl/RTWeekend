#ifndef RAY_H
#define RAY_H

#include "vec3.h"

// 光线的抽象类
class ray {
public:
	ray() : tm(0.0) {}
	ray(const point3& origin, const vec3& direction, double time = 0.0)
		: orig(origin), dir(direction), tm(time)
	{}

	point3 origin() const	{ return orig; }
	vec3 direction() const	{ return dir; }
	double time() const		{ return tm; }

	point3 at(double t) const {
		return orig + t * dir;
	}

public:
	// 出射点
	point3 orig;
	// 射线方向
	vec3 dir;
	// 光线的时间
	double tm;
};

#endif