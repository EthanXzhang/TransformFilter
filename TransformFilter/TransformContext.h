#pragma once
#include "stdafx.h"
#include "VideoFrameTransformHandler.h"
#include "VideoFrameTransformHelper.h"
typedef struct TransformContext {
	int w, h;
	int out_map_planes;

	AVDictionary *opts;
	char *w_expr;               ///< width  expression string
	char *h_expr;               ///< height expression string
	char *size_str;
	int cube_edge_length;
	int max_cube_edge_length;
	int max_output_h;
	int max_output_w;
	int input_layout;
	int output_layout;
	int input_stereo_format;
	int output_stereo_format;
	int vflip;
	int planes;
	float input_expand_coef;
	float expand_coef;
	int is_horizontal_offset;
	float fixed_yaw;    ///< Yaw (asimuth) angle, degrees
	float fixed_pitch;  ///< Pitch (elevation) angle, degrees
	float fixed_roll;   ///< Roll (tilt) angle, degrees
	float fixed_cube_offcenter_x; // offcenter projection x
	float fixed_cube_offcenter_y; // offcenter projection y
	float fixed_cube_offcenter_z; // offcenter projection z

								  // openCV-based transform parameters
	VideoFrameTransform* transform;
	int interpolation_alg;
	float width_scale_factor;
	float height_scale_factor;
	int enable_low_pass_filter;
	float kernel_height_scale_factor;
	float min_kernel_half_height;
	float max_kernel_half_height;
	int enable_multi_threading;
	int num_vertical_segments;
	int num_horizontal_segments;
	int adjust_kernel;
	float kernel_adjust_factor;

} TransformContext;
enum var_name {
	VAR_OUT_W, VAR_OW,
	VAR_OUT_H, VAR_OH,
	VARS_NB
};
static const char *const var_names[] = {
	"out_w",  "ow",
	"out_h",  "oh",
	NULL
};