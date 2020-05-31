#include "image_tools.h"
#include "thirdparty/hqx/HQ2x.hh"
#include "thirdparty/hqx/HQ3x.hh"

Ref<Image> ImageTools::rotate_90_cw(Ref<Image> p_image) {
    const int iw = p_image->get_width();
    const int ih = p_image->get_height();

	Ref<Image> r_image = memnew(Image);
	r_image->create(ih, iw, p_image->has_mipmaps(), p_image->get_format());

	p_image->lock();
	r_image->lock();

	for (int y = 0; y < ih; ++y) {
		for (int x = 0; x < iw; ++x) {
            r_image->set_pixel(y, iw-x-1, p_image->get_pixel(x, y));
		}
	}

	r_image->unlock();
	p_image->unlock();

    return r_image;
}

Ref<Image> ImageTools::rotate_90_ccw(Ref<Image> p_image) {
    const int iw = p_image->get_width();
    const int ih = p_image->get_height();

	Ref<Image> r_image = memnew(Image);
	r_image->create(ih, iw, p_image->has_mipmaps(), p_image->get_format());

	p_image->unlock();
	r_image->lock();

	for (int y = 0; y < ih; ++y) {
		for (int x = 0; x < iw; ++x) {
            r_image->set_pixel(y, x, p_image->get_pixel(x, y));
		}
	}

	r_image->unlock();
	p_image->unlock();

    return r_image;
}

void ImageTools::replace_color(Ref<Image> p_image, const Color &p_color, const Color &p_with_color) {

	if (p_color == p_with_color) {
		return;
	}

	p_image->lock();

	for (int y = 0; y < p_image->get_height(); ++y) {
		for (int x = 0; x < p_image->get_width(); ++x) {

			if (p_image->get_pixel(x, y) == p_color) {
				p_image->set_pixel(x, y, p_with_color);
			}
		}
	}
	p_image->unlock();
}

Ref<Image> ImageTools::bucket_fill(Ref<Image> p_image, const Point2 &p_at, const Color &p_fill_color, bool p_fill_image, KernelConnectivity p_kc) {

	// Based on flood-fill algorithm
	// Runs up to x35 faster compared to GDScript implementation

	if (!has_pixelv(p_image, p_at)) {
		return Ref<Image>();
	}
	p_image->lock();

	Color filling_color = p_image->get_pixelv(p_at);

	int width = p_image->get_width();
	int height = p_image->get_height();
	bool mipmaps = p_image->has_mipmaps();
	Image::Format format = p_image->get_format();

	Ref<Image> fill_image = memnew(Image);
	fill_image->create(width, height, mipmaps, format);
	fill_image->lock();

	Vector2 at;
	Vector2 pos = p_at;
	Color pixel;

	Vector<Vector2> kernel;
	switch (p_kc) {
		case KERNEL_FOUR_WAY: {
			kernel.push_back(Vector2(1, 0));
			kernel.push_back(Vector2(0, -1));
			kernel.push_back(Vector2(-1, 0));
			kernel.push_back(Vector2(0, 1));
		} break;

		case KERNEL_EIGHT_WAY: {
			kernel.push_back(Vector2(1, 0));
			kernel.push_back(Vector2(1, -1));
			kernel.push_back(Vector2(0, -1));
			kernel.push_back(Vector2(-1, -1));
			kernel.push_back(Vector2(-1, 0));
			kernel.push_back(Vector2(-1, 1));
			kernel.push_back(Vector2(0, 1));
			kernel.push_back(Vector2(1, 1));
		} break;
	}

	List<Vector2> to_fill;
	to_fill.push_back(pos);

	while (!to_fill.empty()) {
		pos = to_fill.front()->get();
		to_fill.pop_front();

		for (int i = 0; i < kernel.size(); ++i) {

			const Vector2 &dir = kernel[i];
			at = pos + dir;

			if (has_pixelv(fill_image, at)) {
				pixel = fill_image->get_pixelv(at);
				if (pixel.a > 0.0)
					continue; // already filled
			}
			if (has_pixelv(fill_image, at)) {
				pixel = p_image->get_pixelv(at);
			} else {
				continue;
			}
			if (pixel == filling_color) {
				fill_image->set_pixelv(at, p_fill_color);
				to_fill.push_back(at);
			}
		}
	}
	if (p_fill_image) {
		// Fill the actual image (no undo)
		// else just return filled area as a new image
		Rect2 fill_rect(0, 0, width, height);
		p_image->blend_rect(fill_image, fill_rect, Point2());
	}

	fill_image->unlock();
	p_image->unlock();

	return fill_image;
}

void ImageTools::resize_hqx(Ref<Image> p_image, int p_scale) {
	ERR_FAIL_COND(p_scale < 2);
	ERR_FAIL_COND(p_scale > 3);
	
	bool used_mipmaps = p_image->has_mipmaps();

	Image::Format current = p_image->get_format();
	if (current != Image::FORMAT_RGBA8) {
		p_image->convert(Image::FORMAT_RGBA8);
	}
	PoolVector<uint8_t> dest;
	PoolVector<uint8_t> src = p_image->get_data();
	
	const int new_width = p_image->get_width() * p_scale;
	const int new_height = p_image->get_height() * p_scale;
	dest.resize(new_width * new_height * 4);
	{
		PoolVector<uint8_t>::Read r = src.read();
		PoolVector<uint8_t>::Write w = dest.write();
		
		ERR_FAIL_COND(!r.ptr());
		
		HQx *hqx;
		if (p_scale == 2) {
			hqx = memnew(HQ2x);
		} else if (p_scale == 3) {	
			hqx = memnew(HQ3x);
		} else {
			hqx = memnew(HQ2x); // Fallback to HQ2x in all cases.
		}
		hqx->resize((const uint32_t *)r.ptr(), p_image->get_width(), p_image->get_height(), (uint32_t *)w.ptr());
		memdelete(hqx);
	}
	p_image->create(new_width, new_height, false, Image::FORMAT_RGBA8, dest);

	if (used_mipmaps) {
		p_image->generate_mipmaps();
	}
}

bool ImageTools::has_pixel(Ref<Image> p_image, int x, int y) {
	return get_pixel_or_null(p_image, x, y);
}

bool ImageTools::has_pixelv(Ref<Image> p_image, const Vector2 &p_pos) {
	return get_pixelv_or_null(p_image, p_pos);
}

bool ImageTools::get_pixel_or_null(Ref<Image> p_image, int x, int y, Color* r_pixel) {
	if (x >= 0 && x < p_image->get_width() && y >= 0 && y < p_image->get_height()) {
		if (r_pixel) {
			*r_pixel = p_image->get_pixel(x, y);
		}
		return true;
	}
	return false;
}

bool ImageTools::get_pixelv_or_null(Ref<Image> p_image, const Vector2 &p_pos, Color* r_pixel) {
	return get_pixel_or_null(p_image, p_pos.x, p_pos.y, r_pixel);
}
