#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

// 可求交的世界物体
class hittable_list : public hittable
{
public:
	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) { add(object); }
	
	void clear() { objects.clear(); }
	void add(shared_ptr<hittable> object) { objects.push_back(object); }

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;
	virtual bool bounding_box(double time0, double time1, aabb& output_box) const override;

public:
	std::vector<shared_ptr<hittable> > objects;
};

// 对世界求交
bool hittable_list::hit(const ray& r, double t_min, double t_max, hit_record& rec) const
{
	// 初始化遍历
	hit_record temp_rec;
	// 输出bool
	bool hit_anything = false;
	auto closest_so_far = t_max;

	// 遍历所有物体 调用物体的hit函数求交
	// closet_so_far用来获取最近的一个相交记录
	for (const auto& object : objects)
	{
		if (object->hit(r, t_min, closest_so_far, temp_rec))
		{
			hit_anything = true;
			closest_so_far = temp_rec.t;
			rec = temp_rec;
		}
	}
	return hit_anything;
}

bool hittable_list::bounding_box(double time0, double time1, aabb& output_box) const
{
	if (objects.empty()) return false;

	aabb temp_box;
	bool first_box = true;

	for (const auto& object : objects)
	{
		if (!object->bounding_box(time0, time1, temp_box)) return false;
		output_box = first_box ? temp_box : surrounding_box(output_box, temp_box);
		first_box = false;
	}
	return true;
}

#endif
