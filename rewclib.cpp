#include <ruby.h>

#define EWC_TYPE MEDIASUBTYPE_RGB24
#include "ewclib.h"

VALUE rewclib_open(VALUE self, VALUE width, VALUE height, VALUE fps) {
	if (EWC_Open(NUM2INT(width), NUM2INT(height), NUM2INT(fps))) {
		return T_FALSE;
	}
	rb_iv_set(self, "@width", width);
	return T_TRUE;
}

VALUE rewclib_close(VALUE self) {
	return INT2NUM(EWC_Close());
}

/**
 * upsidedown: true/false
 * channel: Returns the specified channel
 *   0: Red
 *   1: Green
 *   2: Blue
 *   3: All 3 channels in the order RGB
 */
VALUE rewclib_image(VALUE self, VALUE upsidedown, VALUE channel) {
	int size = EWC_GetBufferSize(0);
	char *buffer = (char *) malloc(size);  // The image is BGR
	if (EWC_GetImage(0, buffer)) {
		free(buffer);
		return T_NIL;
	}

	int width = NUM2INT(rb_iv_get(self, "@width"));
	int height = (size/3)/width;
	int c_channel = NUM2INT(channel);

	int size2 = size/((c_channel == 3)? 1 : 3);
	char *buffer2 = (char *) malloc(size2);

	// Ugly, but optimized (avoid "if" condition check in for loop, which is slow)
	if (c_channel == 3) {
		if (TYPE(upsidedown) == T_NIL || TYPE(upsidedown) == T_FALSE) {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					buffer2[(y*width + x)*3 + 0] = buffer[(y*width + x)*3 + 2];
					buffer2[(y*width + x)*3 + 1] = buffer[(y*width + x)*3 + 1];
					buffer2[(y*width + x)*3 + 2] = buffer[(y*width + x)*3 + 0];
				}
			}
		} else {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					buffer2[((height - y - 1)*width + x)*3 + 0] = buffer[(y*width + x)*3 + 2];
					buffer2[((height - y - 1)*width + x)*3 + 1] = buffer[(y*width + x)*3 + 1];
					buffer2[((height - y - 1)*width + x)*3 + 2] = buffer[(y*width + x)*3 + 0];
				}
			}
		}
	} else {
		if (TYPE(upsidedown) == T_NIL || TYPE(upsidedown) == T_FALSE) {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					buffer2[y*width + x] = buffer[(y*width + x)*3 + (2 - c_channel)];
				}
			}
		} else {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					buffer2[(height - y - 1)*width + x] = buffer[(y*width + x)*3 + (2 - c_channel)];
				}
			}
		}
	}

	VALUE ret = rb_str_new(buffer2, size2);
	free(buffer);
	free(buffer2);
	return ret;
}

/**
 * Parts that have black color on the foreground will be painted by the background.
 * Otherwise, the two will be blended. alpha = 1 means only foreground painted.
 */
VALUE rewclib_blend(VALUE self, VALUE foreground, VALUE alpha) {
	int size = EWC_GetBufferSize(0);
	unsigned char *buffer = (unsigned char *) malloc(size);
	if (EWC_GetImage(0, buffer)) {
		free(buffer);
		return T_NIL;
	}

	// The image is upside-down and is BGR
	// Convert it to normal and RGB
	int width = NUM2INT(rb_iv_get(self, "@width"));
	int height = (size/3)/width;
	unsigned char *buffer2 = (unsigned char *) malloc(size);
	unsigned char *foreground_ptr = (unsigned char *) RSTRING_PTR(foreground);
	double c_alpha = NUM2DBL(alpha);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char cf, cb;

			cf = foreground_ptr[((height - y - 1)*width + x)*3 + 0];
			cb = buffer[(y*width + x)*3 + 2];
			buffer2[((height - y - 1)*width + x)*3 + 0] = (cf == 0)? cb : (cf*c_alpha + (1 - c_alpha)*cb);

			cf = foreground_ptr[((height - y - 1)*width + x)*3 + 1];
			cb = buffer[(y*width + x)*3 + 1];
			buffer2[((height - y - 1)*width + x)*3 + 1] = (cf == 0)? cb : (cf*c_alpha + (1 - c_alpha)*cb);

			cf = foreground_ptr[((height - y - 1)*width + x)*3 + 2];
			cb = buffer[(y*width + x)*3 + 0];
			buffer2[((height - y - 1)*width + x)*3 + 2] = (cf == 0)? cb : (cf*c_alpha + (1 - c_alpha)*cb);
		}
	}

	VALUE ret = rb_str_new((char *) buffer2, size);
	free(buffer);
	free(buffer2);
	return ret;
}

void Init_rewclib() {
	VALUE Rewclib = rb_define_class("Rewclib", rb_cObject);
	rb_define_method(Rewclib, "open",  (VALUE (__cdecl *)(...)) rewclib_open,  3);
	rb_define_method(Rewclib, "close", (VALUE (__cdecl *)(...)) rewclib_close, 0);
	rb_define_method(Rewclib, "image", (VALUE (__cdecl *)(...)) rewclib_image, 2);
	rb_define_method(Rewclib, "blend", (VALUE (__cdecl *)(...)) rewclib_blend, 2);
}
