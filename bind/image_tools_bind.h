#ifndef GODOT_IMAGE_TOOLS_BIND_H
#define GODOT_IMAGE_TOOLS_BIND_H

#include "core/image.h"

class _ImageTools : public Object {
	GDCLASS(_ImageTools, Object);

private:
	static _ImageTools *singleton;

protected:
	static void _bind_methods();

public:
	static _ImageTools *get_singleton() { return singleton; }

public:
	enum KernelConnectivity {
		KERNEL_FOUR_WAY,
		KERNEL_EIGHT_WAY,
	};

public:
	// Image methods
	Ref<Image> rotate_90_cw(Ref<Image> p_image);
	Ref<Image> rotate_90_ccw(Ref<Image> p_image);
	void replace_color(Ref<Image> p_image, const Color &p_color, const Color &p_with_color);
	Ref<Image> bucket_fill(Ref<Image> p_image, const Point2 &p_at, const Color &p_fill_color, bool p_fill_image = true, KernelConnectivity p_kc = KERNEL_FOUR_WAY);
	void resize_hqx(Ref<Image> p_image, int p_scale);

	// Pixel methods
	bool has_pixel(Ref<Image> p_image, int x, int y);
	bool has_pixelv(Ref<Image> p_image, const Vector2 &p_pos);
	Variant get_pixel_or_null(Ref<Image> p_image, int x, int y);
	Variant get_pixelv_or_null(Ref<Image> p_image, const Vector2 &p_pos);

	_ImageTools();
};

VARIANT_ENUM_CAST(_ImageTools::KernelConnectivity);

#endif
