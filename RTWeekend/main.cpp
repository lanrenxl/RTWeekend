#include "camera.h"
#include "rtweekend.h"
#include "color.h"
#include "hittable_list.h"
#include "moving_sphere.h"
#include "sphere.h"
#include "aarect.h"
#include "box.h"
#include "material.h"
#include "constant_medium.h"
#include "bvh.h"
#include "pdf.h"

#include <iostream>


// 最重要的函数 射出光线r 对可求交的世界物体world进行遍历求交
// 递归函数的递归条件为depth
color ray_color(
	const ray& r, 
	const color& background, 
	const hittable& world, 
	shared_ptr<hittable> lights, 
	int depth
) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	// If the ray hits nothing, return the background color.
	if (!world.hit(r, 0.001, infinity, rec))
		return background;
	scatter_record srec;
	color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
	if (!rec.mat_ptr->scatter(r, rec, srec))
		return emitted;
	if (srec.is_specular) {
		return srec.attenuation
			* ray_color(srec.specular_ray, background, world, lights, depth - 1);
	}

	auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
	mixture_pdf p(light_ptr, srec.pdf_ptr);

	ray scattered = ray(rec.p, p.generate(), r.time());
	auto pdf_val = p.value(scattered.direction());

	return emitted
		+ srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
		* ray_color(scattered, background, world, lights, depth - 1) / pdf_val;
}

hittable_list random_scene() {
	hittable_list world;

	//auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	//world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));
	auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

	for (int a = 0; a < 11; a++) {
		for (int b = 0; b < 11; b++) {
			auto choose_mat = random_double();
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					auto center2 = center + vec3(0, random_double(0, 0.5), 0);

					world.add(make_shared<moving_sphere>(center, center2, 0.0, 1.0, 0.2, sphere_material));
				}
				else if (choose_mat < 0.95) {
					// metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_double(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

	auto world_bvh = hittable_list(make_shared<bvh_node>(world, 0.0, 1.0));
	return world_bvh;
	//return world;
}

hittable_list two_spheres() {
	hittable_list objects;

	auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));

	objects.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
	objects.add(make_shared<sphere>(point3(0,  10, 0), 10, make_shared<lambertian>(checker)));
	auto world_bvh = hittable_list(make_shared<bvh_node>(objects, 0.0, 1.0));
	return world_bvh;
	//return objects;
}

hittable_list two_perlin_spheres() {
	hittable_list objects;

	auto pertext = make_shared<noise_texture>(4);
	objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	return objects;
}

hittable_list earth()
{
	auto earth_texture = make_shared<image_texture>
		("D:/document/CODE/vs2019/RTWeekend/RTWeekend/earthmap.jpg");
	auto earth_surface = make_shared<lambertian>(earth_texture);
	auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

	return hittable_list(globe);
}

hittable_list simple_light() {
	hittable_list objects;

	shared_ptr<noise_texture> pertext = make_shared<noise_texture>(4);
	objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	shared_ptr<diffuse_light> difflight = make_shared<diffuse_light>(color(4, 4, 4));
	objects.add(make_shared<xy_rect>(3, 5, 1, 3, -2, difflight));
	objects.add(make_shared<sphere>(point3(0, 2, 4), 2, difflight));

	return hittable_list(make_shared<bvh_node>(objects, 0.0, 1.0));
}

hittable_list cornell_box() {
	hittable_list objects;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
	objects.add(make_shared<flip_face>(make_shared<xz_rect>(213, 343, 227, 332, 554, light)));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
	objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

	shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
	shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), aluminum);
	box1 = make_shared<rotate_y>(box1, 15);
	box1 = make_shared<translate>(box1, vec3(265, 0, 295));
	objects.add(box1);

	auto glass = make_shared<dielectric>(1.5);
	objects.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

	return hittable_list(make_shared<bvh_node>(objects, 0, 1));
}

hittable_list final_scene() {
	hittable_list boxes1;
	auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

	const int boxes_per_side = 20;
	for (int i = 0; i < boxes_per_side; i++) {
		for (int j = 0; j < boxes_per_side; j++) {
			auto w = 100.0;
			auto x0 = -1000.0 + i * w;
			auto z0 = -1000.0 + j * w;
			auto y0 = 0.0;
			auto x1 = x0 + w;
			auto y1 = random_double(1, 101);
			auto z1 = z0 + w;

			boxes1.add(make_shared<box>(point3(x0, y0, z0), point3(x1, y1, z1), ground));
		}
	}

	hittable_list objects;

	objects.add(make_shared<bvh_node>(boxes1, 0, 1));

	auto light = make_shared<diffuse_light>(color(7, 7, 7));
	objects.add(make_shared<xz_rect>(123, 423, 147, 412, 554, light));

	auto center1 = point3(400, 400, 200);
	auto center2 = center1 + vec3(30, 0, 0);
	auto moving_sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
	objects.add(make_shared<moving_sphere>(center1, center2, 0, 1, 50, moving_sphere_material));

	objects.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
	objects.add(make_shared<sphere>(
		point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
		));

	auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<dielectric>(1.5));
	objects.add(boundary);
	objects.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
	boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
	objects.add(make_shared<constant_medium>(boundary, .0001, color(1, 1, 1)));

	auto emat = make_shared<lambertian>(make_shared<image_texture>("D:/document/CODE/vs2019/RTWeekend/RTWeekend/earthmap.jpg"));
	objects.add(make_shared<sphere>(point3(400, 200, 400), 100, emat));
	auto pertext = make_shared<noise_texture>(0.1);
	objects.add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

	hittable_list boxes2;
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		boxes2.add(make_shared<sphere>(point3::random(0, 165), 10, white));
	}

	objects.add(make_shared<translate>(
		make_shared<rotate_y>(
			make_shared<bvh_node>(boxes2, 0.0, 1.0), 15),
		vec3(-100, 270, 395)
		)
	);

	return hittable_list(make_shared<bvh_node>(objects, 0, 1));
}

int main() {

	// Image 渲染图形属性
	// 长宽比
	// Image

	const auto aspect_ratio = 1.0 / 1.0;
	const int image_width = 100;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 20;
	const int max_depth = 50;

	// World

	auto world = cornell_box();
	shared_ptr<hittable> lights =
		//  make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>());
		make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>());
	color background(0, 0, 0);

	// Camera

	point3 lookfrom(278, 278, -800);
	point3 lookat(278, 278, 0);
	vec3 vup(0, 1, 0);
	auto dist_to_focus = 10.0;
	auto aperture = 0.0;
	auto vfov = 40.0;
	auto time0 = 0.0;
	auto time1 = 1.0;

	camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, time0, time1);

	// Render 渲染循环
	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			color pixel_color(0, 0, 0);
			for (int s = 0; s <samples_per_pixel; ++s)
			{
				auto u = (i + random_double()) / (image_width + 1);
				auto v = (j + random_double()) / (image_height + 1);
				ray r = cam.get_ray(u, v);
				pixel_color += ray_color(r, background, world, lights, max_depth);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
	}

	std::cerr << "\nDone.\n";
}