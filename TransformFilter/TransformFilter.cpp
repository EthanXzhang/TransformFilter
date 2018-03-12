// TransformFilter.cpp: 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include "transformfilterguid.h"
#include <streams.h>
#include <olectl.h>
#include "TransformFilter.h"
#include "TransformFilterPage.h"
#include "TransformFilterAdvancePage.h"
#if (1100 > _MSC_VER)
#include <olectlid.h>
#endif
// setup data
//
//注册时候的信息
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_NULL,            // Major type
	&MEDIASUBTYPE_NULL          // Minor type
};
//注册时候的信息
const AMOVIESETUP_PIN psudPins[] =
{
	{
		L"Input",           // String pin name
		FALSE,              // Is it rendered
		FALSE,              // Is it an output
		FALSE,              // Allowed none
		FALSE,              // Allowed many
		&CLSID_NULL,        // Connects to filter
		L"Output",          // Connects to pin
		1,                  // Number of types
		&sudPinTypes }
};

//注册时候的信息
const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_TransformFilter,       // Filter CLSID
	L"Transform Filter",    // Filter name
	MERIT_DO_NOT_USE,        // Its merit
	1,                       // Number of pins
	psudPins                 // Pin details
};

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//注意g_Templates名称是固定的
CFactoryTemplate g_Templates[2] =
{
	{
		L"Transform Filter",
		&CLSID_TransformFilter,
		TransformFilter::CreateInstance,
		NULL,
		&sudFilter
	}
	,
	{
		L"Transform Filter Pages",
		&CLSID_TransformPage,
		TransformFilterPage::CreateInstance,
		NULL,
		NULL,
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

TransformFilter::TransformFilter(TCHAR *tszName, LPUNKNOWN punk) :
	CTransformFilter(tszName, punk, CLSID_TransformFilter)
{
	transform_setting = new Transform360;
	setting= (TransformContext*)malloc(sizeof(TransformContext));
	setting->w_expr = "";
	setting->h_expr = "";
	setting->size_str = NULL;
	setting->is_horizontal_offset = 0;
	setting->cube_edge_length = 0;
	setting->max_cube_edge_length = 960;
	setting->max_output_h = 0;
	setting->max_output_w = 0;
	setting->input_stereo_format = STEREO_FORMAT_MONO;
	setting->output_stereo_format = STEREO_FORMAT_MONO;
	setting->input_layout = LAYOUT_EQUIRECT;//LAYOUT_EQUIRECT
	setting->output_layout = LAYOUT_CUBEMAP_32;//LAYOUT_CUBEMAP_32
	setting->input_expand_coef = 1.01f;
	setting->expand_coef = 1.01f;
	setting->fixed_yaw = 0.0;
	setting->fixed_pitch = 0.0;
	setting->fixed_roll = 0.0;
	setting->interpolation_alg = CUBIC;
	setting->width_scale_factor = 1.0;
	setting->height_scale_factor = 1.0;
	setting->enable_low_pass_filter = 0;
	setting->enable_multi_threading = 0;
	setting->num_vertical_segments = 2;//最高500
	setting->num_horizontal_segments = 1;//最高500
	setting->kernel_height_scale_factor = 1.0;
	setting->min_kernel_half_height = 1.0;
	setting->max_kernel_half_height = 10000.0;
	setting->adjust_kernel = 1;
	setting->kernel_adjust_factor = 1.0;

	setting->out_map_planes = 0;
	transform_setting->frameformat = 0;
}
TransformFilter::~TransformFilter()
{
	delete(transform_setting);
	delete(setting);
}
CUnknown * WINAPI TransformFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	TransformFilter *pNewObject = new TransformFilter(NAME("Transform Filter"), punk);
	if (pNewObject == NULL)
	{
		*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}
HRESULT TransformFilter::CheckInputType(const CMediaType* mtIn)
{
	if (FFmpeg_initFlag)
	{
		return NOERROR;
	}
	// Dynamic format change will never be allowed!
	if (IsStopped() && *mtIn->Type() == MEDIATYPE_Video)
	{
		return NOERROR;
	}
	return E_INVALIDARG;
}
HRESULT TransformFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	HRESULT hr;
	if (FAILED(hr = CheckInputType(mtIn)))
	{
		return hr;
	}

	if (*mtOut->FormatType() != FORMAT_VideoInfo&& *mtOut->FormatType() != FORMAT_VIDEOINFO2)
	{
		return E_INVALIDARG;
	}
	return NOERROR;
}
HRESULT TransformFilter::Transform(IMediaSample *pIn,IMediaSample *pOut)
{
	pIn->GetPointer(&pbufferin);
	pOut->GetPointer(&pbufferout);
	av_image_fill_arrays(pFrameIn->data, pFrameIn->linesize, pbufferin, transform_setting->inputformat, transform_setting->input_w, transform_setting->input_h, 32);
	//对输入为YV12的Frame进行色彩空间转换，变成YUV420P
	if (InputType.subtype == MEDIASUBTYPE_YV12)
	{
		trans_yv12_yuv420p(pFrameIn);
	}
	sws_scale(img_convert_ctx, (const unsigned char* const*)pFrameIn->data, pFrameIn->linesize, 0, pFrameIn->height, pFrameTrans->data, pFrameTrans->linesize);
	/*
	传递给filter进行处理
	*/
	transform_by_filter(pFrameTrans, pFrameOut);
	/*
	处理完的Frame进行一次尺寸拉伸，递交给pFrameOutput，准备送上pinout
	进行一次旋转处理
	*/
	if ((transform_setting->inputformat == AV_PIX_FMT_RGB32&&transform_setting->outputformat != AV_PIX_FMT_BGR24) ||
		(transform_setting->inputformat == AV_PIX_FMT_RGB32&&transform_setting->outputformat != AV_PIX_FMT_RGB32) ||
		(transform_setting->inputformat != AV_PIX_FMT_BGR24&&transform_setting->outputformat == AV_PIX_FMT_BGR24) ||
		(transform_setting->inputformat != AV_PIX_FMT_BGR24&&transform_setting->outputformat == AV_PIX_FMT_RGB32))
	{
		pFrameOut->data[0] += pFrameOut->linesize[0] * (pFrameOut->height - 1);
		pFrameOut->linesize[0] = -pFrameOut->linesize[0];
		pFrameOut->data[1] += pFrameOut->linesize[1] * (pFrameOut->height / 2 - 1);
		pFrameOut->linesize[1] = -pFrameOut->linesize[1];
		pFrameOut->data[2] += pFrameOut->linesize[2] * (pFrameOut->height / 2 - 1);
		pFrameOut->linesize[2] = -pFrameOut->linesize[2];
	}

	pFrameOutput->format = transform_setting->outputformat;
	sws_scale(output_convert_ctx, (const unsigned char* const*)pFrameOut->data, pFrameOut->linesize, 0, pFrameOut->height, pFrameOutput->data, pFrameOutput->linesize);
	if (pFrameOut->data[0] == NULL)
	{
		return NOERROR;
	}
	//test_write(pFrameOutput);
	if (transform_setting->backup_output_type.subtype== MEDIASUBTYPE_IYUV)
	{
		get_yuvbuffer(pbufferout, pFrameOutput);
	}
	else if (transform_setting->outputformat == AV_PIX_FMT_NV12)
	{
		get_nv12buffer(pbufferout, pFrameOutput);
	}
	else if (transform_setting->backup_output_type.subtype == MEDIASUBTYPE_YV12)
	{
		get_yv12buffer(pbufferout, pFrameOutput);
	}
	else if (transform_setting->outputformat == AV_PIX_FMT_BGR24)//测试BGR24
	{
		get_rgb24buffer(pbufferout, pFrameOutput);
	}
	else if (transform_setting->outputformat == AV_PIX_FMT_RGB32)
	{
		get_rgb32buffer(pbufferout, pFrameOutput);
	}
	//FILE *pout;
	//fopen_s(&pout, "G://framein.yuv", "w");
	//fwrite(pbufferout,(1920*1280*3)/2,1,pout);
	//fclose(pout);
	return NOERROR;
}
HRESULT TransformFilter::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	HRESULT hr= CTransformFilter::CompleteConnect(direction, pReceivePin);
	if (direction== PINDIR_INPUT)
	{	
		CMediaType *mtIn;
		mtIn = (CMediaType*)malloc(sizeof(CMediaType));
		pReceivePin->ConnectionMediaType(mtIn);
		if (*mtIn->Subtype() == MEDIASUBTYPE_IYUV)
		{
			save_type(mtIn);
		}
		else if (*mtIn->Subtype() == MEDIASUBTYPE_NV12)
		{
			save_type(mtIn);
		}
		else if (*mtIn->Subtype() == MEDIASUBTYPE_YV12)
		{
			save_type(mtIn);
		}
		else if (*mtIn->Subtype() == MEDIASUBTYPE_RGB24)
		{
			save_type(mtIn);
		}
		else if (*mtIn->Subtype() == MEDIASUBTYPE_RGB32)
		{
			save_type(mtIn);
		}
		InputType = *mtIn;
		if (FFmpeg_initFlag)
		{
			destory_ffmpeg();
		}
		init_Transform360();
		init_all();
		if (init_ffmpeg())
		{				
			if (s->out_map_planes != 2)
			{
				int ret = generate_map(s, pFrameTrans, pFrameOut);
			}
			FFmpeg_initFlag = TRUE;
			return S_OK;
		};
		return E_INVALIDARG;
	}
	/*
	已修改，2017.11.29 20：48
	使用CompleteConnect获取输出媒体类型
	加入if语句，将CheckTransform中函数移植到此处
	*/

	if (direction == PINDIR_OUTPUT)
	{
		AM_MEDIA_TYPE mtOut;
		pReceivePin->ConnectionMediaType(&mtOut);
		transform_setting->backup_output_type = mtOut;
		if (mtOut.subtype== MEDIASUBTYPE_IYUV)
		{
			transform_setting->outputformat = AV_PIX_FMT_YUV420P;
		}
		else if (mtOut.subtype == MEDIASUBTYPE_NV12)
		{
			transform_setting->outputformat = AV_PIX_FMT_NV12;
		}
		else if (mtOut.subtype == MEDIASUBTYPE_YV12)
		{
			transform_setting->outputformat = AV_PIX_FMT_YUV420P;
		}
		else if (mtOut.subtype == MEDIASUBTYPE_RGB24)
		{
			transform_setting->outputformat = AV_PIX_FMT_BGR24;//测试BGR24
		}
		else if (mtOut.subtype == MEDIASUBTYPE_RGB32)
		{
			transform_setting->outputformat = AV_PIX_FMT_RGB32;
		}
		/*
		初始化FrameOut的buffer
		输出到pinOut转换
		填充buffer，初始化转换上下文
		*/
		//pframeout_buffer——》pFrameOut
		pframeout_buffer= (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, transform_setting->output_w, transform_setting->output_h, 1));
		av_image_fill_arrays(pFrameOut->data, pFrameOut->linesize, pframeout_buffer, AV_PIX_FMT_YUV420P, transform_setting->output_w, transform_setting->output_h, 1);
		//pframeoutput_buffer——》pFrameOutput
		output_convert_ctx = sws_getContext(transform_setting->output_w, transform_setting->output_h, AV_PIX_FMT_YUV420P,transform_setting->output_w, transform_setting->output_h, transform_setting->outputformat, SWS_POINT, NULL, NULL, NULL);
		pframeoutput_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(transform_setting->outputformat, transform_setting->output_w, transform_setting->output_h, 1));
		av_image_fill_arrays(pFrameOutput->data, pFrameOutput->linesize, pframeoutput_buffer, transform_setting->outputformat, transform_setting->output_w, transform_setting->output_h, 1);
	}
	return S_OK;
}
HRESULT TransformFilter::BreakConnect(PIN_DIRECTION dir)
{
	HRESULT hr = CTransformFilter::BreakConnect(dir);
	if (dir == PINDIR_INPUT)
	{
		if (FFmpeg_initFlag)
		{
			destory_ffmpeg();
			destory_transform(s);
			return S_OK;
		}
		else
		{
			return S_OK;
		}
	}
	return hr;
}
HRESULT TransformFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < 2)
	{
		*pMediaType = *OutputType[transform_setting->frameformat*2+iPosition];
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}
HRESULT TransformFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(ppropInputRequest, E_POINTER);
	HRESULT hr = NOERROR;
	ppropInputRequest->cBuffers = 1;
	if (transform_setting->outputformat == AV_PIX_FMT_NV12 || transform_setting->outputformat == AV_PIX_FMT_YUV420P)
	{
		ppropInputRequest->cbBuffer = transform_setting->output_w*transform_setting->output_h * 3 / 2;
	}
	else if (transform_setting->outputformat == AV_PIX_FMT_BGR24)//测试BGR24
	{
		ppropInputRequest->cbBuffer = transform_setting->output_w*transform_setting->output_h * 3;
	}
	else if (transform_setting->outputformat == AV_PIX_FMT_RGB32)
	{
		ppropInputRequest->cbBuffer = transform_setting->output_w*transform_setting->output_h * 4;
	}
	ASSERT(ppropInputRequest->cbBuffer);
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(ppropInputRequest, &Actual);
	if (FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable

	if (Actual.cbBuffer < ppropInputRequest->cbBuffer)
	{
		return E_FAIL;
	}

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}

STDMETHODIMP TransformFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv, E_POINTER);

	if (riid == IID_TransformFilterInterface) {
		return GetInterface((TransformFilterInterface *)this, ppv);

	}
	else if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages *)this, ppv);

	}

	else {
		return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
	}

} // NonDelegatingQueryInterface

STDMETHODIMP TransformFilter::GetPages(CAUUID *pPages)
{
	if (pPages == NULL)
		return E_POINTER;

	pPages->cElems = 1;
	pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
	{
		return E_OUTOFMEMORY;
	}
	(pPages->pElems)[0]= CLSID_TransformPage;
	return NOERROR;

} // GetPages

STDMETHODIMP TransformFilter::DoSetting(int output_w,int output_h,int inputlayout,int outputlayout,int frameformat,int cubelength,int cubemax,int interpolationALG,BOOL lowpass_enable,BOOL lowpass_MT,int vs,int hs)
{
	setting->w_expr = "";
	setting->h_expr = "";
	setting->size_str = NULL;
	setting->is_horizontal_offset = 0;
	setting->cube_edge_length = cubelength;
	setting->max_cube_edge_length = cubemax;
	setting->max_output_h = output_h;
	setting->max_output_w = output_w;
	setting->input_stereo_format = STEREO_FORMAT_MONO;
	setting->output_stereo_format = STEREO_FORMAT_MONO;
	setting->input_layout =inputlayout;//LAYOUT_EQUIRECT
	setting->output_layout = outputlayout;//LAYOUT_CUBEMAP_32
	setting->input_expand_coef = 1.01f;
	setting->expand_coef = 1.01f;
	setting->fixed_yaw = 0.0;
	setting->fixed_pitch = 0.0;
	setting->fixed_roll = 0.0;
	setting->interpolation_alg = interpolationALG;
	setting->width_scale_factor = 1.0;
	setting->height_scale_factor = 1.0;
	setting->enable_low_pass_filter = lowpass_enable;
	setting->enable_multi_threading = lowpass_MT;
	setting->num_vertical_segments = vs;//最高500
	setting->num_horizontal_segments = hs;//最高500
	setting->kernel_height_scale_factor = 1.0;
	setting->min_kernel_half_height = 1.0;
	setting->max_kernel_half_height = 10000.0;
	setting->adjust_kernel = 1;
	setting->kernel_adjust_factor = 1.0;

	setting->out_map_planes = 0;

	transform_setting->frameformat = frameformat;
	if (FFmpeg_initFlag)
	{
		destory_ffmpeg();
		destory_transform(s);
		init_Transform360();
		init_all();
		init_ffmpeg();
		if (s->out_map_planes != 2)
		{
			int ret = generate_map(s, pFrameTrans, pFrameOut);
		}
		FFmpeg_initFlag = true;
	}
	return NOERROR;
}

STDMETHODIMP TransformFilter::GetSetting(int *output_w, int *output_h, int *inputlayout, int *outputlayout, int *frameformat, int *cubelength, int *cubemax, int *interpolationALG, BOOL *lowpass_enable, BOOL *lowpass_MT, int *vs, int *hs)
{
	*cubelength=setting->cube_edge_length;
	*cubemax=setting->max_cube_edge_length;
	*output_h=setting->max_output_h;
	*output_w=setting->max_output_w;
	*inputlayout=setting->input_layout;//LAYOUT_EQUIRECT
	*outputlayout=setting->output_layout;//LAYOUT_CUBEMAP_32
	*interpolationALG=setting->interpolation_alg;
	*lowpass_enable=setting->enable_low_pass_filter;
	*lowpass_MT=setting->enable_multi_threading;
	*vs=setting->num_vertical_segments;//最高500
	*hs=setting->num_horizontal_segments;//最高500
	*frameformat=transform_setting->frameformat;
	return NOERROR;
}

void TransformFilter::init_all()
{
	cal_outputsize();
	//保存输入pin的媒体格式
	init_outputtype();
	set_outputtype();
}
void TransformFilter::save_type(const CMediaType* mtIn)
{
	if (*mtIn->FormatType() == FORMAT_VIDEOINFO2)
	{
		InVinfo2 = *(VIDEOINFOHEADER2 *)mtIn->pbFormat;	
		AvgTimePerFrame = InVinfo2.AvgTimePerFrame;
		InBinfo = InVinfo2.bmiHeader;
		InHeight = InBinfo.biHeight;
		InWidth = InBinfo.biWidth;
		transform_setting->input_h = InHeight;
		transform_setting->input_w = InWidth;
	}
	else if (*mtIn->FormatType() == FORMAT_VideoInfo)
	{
		InVinfo = *(VIDEOINFOHEADER *)mtIn->pbFormat;
		AvgTimePerFrame = InVinfo.AvgTimePerFrame;
		InBinfo = InVinfo.bmiHeader;
		InHeight = InBinfo.biHeight;
		InWidth = InBinfo.biWidth;
		transform_setting->input_h = InHeight;
		transform_setting->input_w = InWidth;
	}
	if(*mtIn->Subtype() == MEDIASUBTYPE_IYUV)
	{
		transform_setting->inputformat = AV_PIX_FMT_YUV420P;
	}
	else if (*mtIn->Subtype() == MEDIASUBTYPE_NV12)
	{
		transform_setting->inputformat = AV_PIX_FMT_NV12;
	}
	else if (*mtIn->Subtype() == MEDIASUBTYPE_YV12)
	{
		transform_setting->inputformat = AV_PIX_FMT_YUV420P;
	}
	else if (*mtIn->Subtype() == MEDIASUBTYPE_RGB24)
	{
		transform_setting->inputformat = AV_PIX_FMT_RGB24;
	}
	else if (*mtIn->Subtype() == MEDIASUBTYPE_RGB32)
	{
		transform_setting->inputformat = AV_PIX_FMT_RGB32;
	}
}
/*
计算Transform输出的帧分辨率
*/
int TransformFilter::cal_outputsize()
{
	double var_values[VARS_NB], res;
	char *expr;
	int ret;

	var_values[VAR_OUT_W] = var_values[VAR_OW] = NAN;
	var_values[VAR_OUT_H] = var_values[VAR_OH] = NAN;
	/*
	判断3D格式
	*/
	if (s->input_stereo_format == STEREO_FORMAT_GUESS) {
		int aspect_ratio = transform_setting->input_w / transform_setting->input_h;
		if (aspect_ratio == 1)
			s->input_stereo_format = STEREO_FORMAT_TB;
		else if (aspect_ratio == 4)
			s->input_stereo_format = STEREO_FORMAT_LR;
		else
			s->input_stereo_format = STEREO_FORMAT_MONO;
	}

	if (s->output_stereo_format == STEREO_FORMAT_GUESS) {
		if (s->input_stereo_format == STEREO_FORMAT_MONO) {
			s->output_stereo_format = STEREO_FORMAT_MONO;
		}
		else {
			s->output_stereo_format =
				(s->output_layout == LAYOUT_CUBEMAP_23_OFFCENTER)
				? STEREO_FORMAT_LR
				: STEREO_FORMAT_TB;
		}
	}
	/*
	判断立方体块大小
	*/
	if (s->max_cube_edge_length > 0	&& s->cube_edge_length==0) {
		if (s->input_stereo_format == STEREO_FORMAT_LR) {
			s->cube_edge_length = transform_setting->input_w / 8;
		}
		else {
			s->cube_edge_length = transform_setting->input_w / 4;
		}

		// do not exceed the max length supplied
		if (s->cube_edge_length > s->max_cube_edge_length) {
			s->cube_edge_length = s->max_cube_edge_length;
		}
	}

	// ensure cube edge length is a multiple of 16 by rounding down
	// so that macroblocks do not cross cube edge boundaries
	s->cube_edge_length = s->cube_edge_length - (s->cube_edge_length % 16);

	if (s->cube_edge_length > 0)
	{
		/*
		计算cubemap32的长宽
		*/
		if (s->input_layout == LAYOUT_EQUIRECT&&s->output_layout == LAYOUT_CUBEMAP_32)
		{
			transform_setting->output_w = s->cube_edge_length * 3;
			transform_setting->output_h = s->cube_edge_length * 2;

		}
		/*
		计算纵列格式cubemap长宽
		*/
		else if (s->input_layout == LAYOUT_EQUIRECT&&s->output_layout == LAYOUT_CUBEMAP_23_OFFCENTER)
		{
			transform_setting->output_w = s->cube_edge_length * 2;
			transform_setting->output_h = s->cube_edge_length * 3;
		}
		else if (s->input_layout == LAYOUT_CUBEMAP_32&&s->output_layout == LAYOUT_EQUIRECT)
		{
			transform_setting->output_h = transform_setting->input_h;
			transform_setting->output_w = (transform_setting->input_w / 3) * 4;
		}
	}

	else
	{
		var_values[VAR_OUT_W] = var_values[VAR_OW] = NAN;
		var_values[VAR_OUT_H] = var_values[VAR_OH] = NAN;

		av_expr_parse_and_eval(&res, (expr = s->w_expr), var_names, var_values, NULL, NULL, NULL, NULL, NULL, 0, NULL);
		s->w = var_values[VAR_OUT_W] = var_values[VAR_OW] = res;
		if ((ret = av_expr_parse_and_eval(&res, (expr = s->h_expr),
			var_names, var_values,
			NULL, NULL, NULL, NULL, NULL, 0, NULL)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR,
				"Error when evaluating the expression '%s'.\n"
				"Maybe the expression for out_w:'%s' or for out_h:'%s' is self-referencing.\n",
				expr, s->w_expr, s->h_expr);
			return ret;
		}
		s->h = var_values[VAR_OUT_H] = var_values[VAR_OH] = res;
		/* evaluate again the width, as it may depend on the output height */
		if ((ret = av_expr_parse_and_eval(&res, (expr = s->w_expr),
			var_names, var_values,
			NULL, NULL, NULL, NULL, NULL, 0, NULL)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR,
				"Error when evaluating the expression '%s'.\n"
				"Maybe the expression for out_w:'%s' or for out_h:'%s' is self-referencing.\n",
				expr, s->w_expr, s->h_expr);
			return ret;
		}
		s->w = res;
		transform_setting->output_w = s->w;
		transform_setting->output_h = s->h;
	}
	/*
	判断3D格式，若真，则翻倍长或宽
	*/
	if (s->output_stereo_format == STEREO_FORMAT_TB)
	{
		transform_setting->output_h *= 2;
		s->h *= 2;
	}
	else if (s->output_stereo_format == STEREO_FORMAT_LR)
	{
		transform_setting->output_w *= 2;
		s->w *= 2;
	}
	return 0;
}
bool TransformFilter::init_ffmpeg()
{
	avfilter_register_all();

	pFrameIn = av_frame_alloc();
	pFrameIn->format = transform_setting->inputformat;
	pFrameIn->key_frame = 1;
	pFrameIn->width = transform_setting->input_w;
	pFrameIn->height = transform_setting->input_h;

	pFrameTrans = av_frame_alloc();
	pFrameTrans->format = AV_PIX_FMT_YUV420P;
	pFrameTrans->key_frame = 1;
	pFrameTrans->width = transform_setting->input_w;
	pFrameTrans->height = transform_setting->input_h;
	pFrameTrans->sample_aspect_ratio.num = 1;
	pFrameTrans->pict_type = AV_PICTURE_TYPE_I;

	pFrameOut = av_frame_alloc();
	pFrameOut->format = AV_PIX_FMT_YUV420P;
	pFrameOut->key_frame = 1;
	pFrameOut->width = transform_setting->output_w;
	pFrameOut->height = transform_setting->output_h;

	pFrameOutput = av_frame_alloc();
	pFrameOutput->format = transform_setting->outputformat;
	pFrameOutput->key_frame = 1;
	pFrameOutput->width = transform_setting->output_w;
	pFrameOutput->height = transform_setting->output_h;

	//初始化转换frame的buffer缓存和准备转换上下文
	pframetrans_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, transform_setting->input_w, transform_setting->input_h, 32));
	av_image_fill_arrays(pFrameTrans->data, pFrameTrans->linesize, pframetrans_buffer,AV_PIX_FMT_YUV420P, transform_setting->input_w, transform_setting->input_h, 32);

	if (AvgTimePerFrame < 10000)
	{
		time_base.num = 60;
		time_base.den = 1200000;
	}
	else
	{
		time_base.num = 1;
		time_base.den = 90000;
	}
	//转换上下文，原始格式=transform_setting->inputformat,目标格式=AV_PIX_FMT_YUV420P
	//输入到filterIn的转换
	img_convert_ctx = sws_getContext(transform_setting->input_w, transform_setting->input_h, transform_setting->inputformat,
		transform_setting->input_w, transform_setting->input_h, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
	//翻转RGB图像的转换
	middle_convert = sws_getContext(transform_setting->input_w, transform_setting->input_h, AV_PIX_FMT_YUV420P,
		transform_setting->input_w, transform_setting->input_h, transform_setting->inputformat, SWS_POINT, NULL, NULL, NULL);
	return true;
}
int TransformFilter::destory_ffmpeg()
{
	if (FFmpeg_initFlag)
	{
		av_frame_free(&pFrameIn);
		av_frame_free(&pFrameTrans);
		av_frame_free(&pFrameOut);
		av_frame_free(&pFrameOutput);
		sws_freeContext(img_convert_ctx);
		if (output_convert_ctx != NULL)
		{
			sws_freeContext(output_convert_ctx);
		}		
		sws_freeContext(middle_convert);
		FFmpeg_initFlag = FALSE;
		return 0;
	}
	else
	{
		return -1;
	}
}
/*
执行转换的处理函数
*/
int TransformFilter::transform_by_filter(AVFrame *in, AVFrame *out)
{
	//if (s->out_map_planes != 2)
	//{
	//	int ret = generate_map(s, in, out);
	//	if (ret != 0)
	//	{
	//		av_frame_free(&in);
	//		return ret;
	//	}
	//}
	uint8_t *in_data, *out_data;
	int out_map_plane;
	int in_w, in_h, out_w, out_h;
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)out->format);
	for (int plane = 0; plane < s->planes; ++plane)
	{
		in_data = in->data[plane];
		av_assert1(in_data);
		out_data = out->data[plane];
		out_map_plane = (plane == 1 || plane == 2) ? 1 : 0;

		out_w = out->width;
		out_h = out->height;
		in_w = in->width;
		in_h = in->height;

		if (plane >= 1)
		{
			update_plane_sizes(desc, &in_w, &in_h, &out_w, &out_h);
		}

		if (!VideoFrameTransform_transformFramePlane(
			s->transform,
			in_data,
			out_data,
			in_w,
			in_h,
			in->linesize[plane],
			out_w,
			out_h,
			out->linesize[plane],
			out_map_plane,
			plane)) {
			return AVERROR(EINVAL);
		}
	}
	////av_frame_free(&in);
	return 0;
}
void TransformFilter::get_yuvbuffer(byte* pout,AVFrame *pf)
{
	int i, j, k;
	for (i = 0; i < pf->height; i++)
	{
		memcpy(pout + pf->width*i, pf->data[0] + pf->linesize[0] * i,
			pf->width);
	}
	for (j = 0; j < pf->height / 2; j++)
	{
		memcpy(pout + pf->width*i + pf->width / 2 * j, pf->data[1] + pf->linesize[1] * j, pf->width / 2);
	}
	for (k = 0; k < pf->height / 2; k++)
	{
		memcpy(pout + pf->width*i + pf->width / 2 * j + pf->width / 2 * k, pf->data[2] + pf->linesize[2] * k, pf->width / 2);
	}
}
void TransformFilter::get_yv12buffer(byte* pout, AVFrame *pf)
{
	int i, j, k;
	for (i = 0; i < pf->height; i++)
	{
		memcpy(pout + pf->width*i, pf->data[0] + pf->linesize[0] * i,	pf->width);
	}
	for (j = 0; j < pf->height / 2; j++)
	{
		memcpy(pout + pf->width*i + pf->width / 2 * j, pf->data[2] + pf->linesize[2] * j, pf->width / 2);
	}
	for (k = 0; k < pf->height / 2; k++)
	{
		memcpy(pout + pf->width*i + pf->width / 2 * j + pf->width / 2 * k, pf->data[1] + pf->linesize[1] * k, pf->width / 2);
	}
}
void TransformFilter::get_nv12buffer(byte* pout, AVFrame *pf)
{	
	//int ret;
	//ret = av_image_copy_to_buffer((unsigned char*)pout, pf->width*pf->height*3/2,(const uint8_t* const *)pf->data, (const int*)pf->linesize,AV_PIX_FMT_NV12, pf->width, pf->height, 1);
	memcpy(pout, pf->data[0], pf->linesize[0]*pf->height );
	memcpy(pout+ pf->linesize[0] * pf->height, pf->data[1], pf->linesize[1] * pf->height/2);
}
void TransformFilter::get_rgb32buffer(byte* pout, AVFrame *pf)
{
	memcpy(pout, pf->data[0],pf->width*pf->height*4);
}
void TransformFilter::get_rgb24buffer(byte* pout, AVFrame *pf)
{
	memcpy(pout, pf->data[0], pf->width*pf->height * 3-1);
}
void TransformFilter::set_outputtype()
{
	for (int i = 0; i < 10; i=i+2)
	{
		VIDEOINFOHEADER* vheader = (VIDEOINFOHEADER*)OutputType[i]->pbFormat;
		vheader->rcSource.left = 0;
		vheader->rcSource.right = transform_setting->output_w;
		vheader->rcSource.top = 0;
		vheader->rcSource.bottom = transform_setting->output_h;
		vheader->rcTarget.left = 0;
		vheader->rcTarget.right = transform_setting->output_w;
		vheader->rcTarget.top = 0;
		vheader->rcTarget.bottom = transform_setting->output_h;
		if (i < 6)
		{
			vheader->dwBitRate = transform_setting->output_w*transform_setting->output_h * (3 / 2) * (10000000 / AvgTimePerFrame);
			vheader->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 3 / 2;
		}
		else if(i == 6)
		{
			vheader->dwBitRate = transform_setting->output_w*transform_setting->output_h * 3 * (10000000 / AvgTimePerFrame);
			vheader->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 3;
		}
		else if (i == 8)
		{
			vheader->dwBitRate = transform_setting->output_w*transform_setting->output_h * 4 * (10000000 / AvgTimePerFrame);
			vheader->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 4;
		}
		vheader->dwBitRate = 0;
		vheader->dwBitErrorRate = 0;
		vheader->AvgTimePerFrame = AvgTimePerFrame;
		vheader->bmiHeader.biSize = 40;
		vheader->bmiHeader.biWidth = transform_setting->output_w;
		vheader->bmiHeader.biHeight = transform_setting->output_h;
		if (i == 0||i==1)
		{
			vheader->bmiHeader.biCompression = 0x56555949;//IYUV
			vheader->bmiHeader.biPlanes = 3;
			vheader->bmiHeader.biBitCount = 12;
		}
		else if (i == 2 || i == 3)
		{
			vheader->bmiHeader.biCompression = 0x3231564e;//NV12
			vheader->bmiHeader.biPlanes = 2;
			vheader->bmiHeader.biBitCount = 12;
		}
		else if (i == 4 || i == 5)
		{
			vheader->bmiHeader.biCompression = 0x32315659;//YV12
			vheader->bmiHeader.biPlanes = 3;
			vheader->bmiHeader.biBitCount = 12;
		}
		else if (i == 6 || i == 7)
		{
			vheader->bmiHeader.biCompression = 0x00000000;//RGB24
			vheader->bmiHeader.biPlanes = 1;
			vheader->bmiHeader.biBitCount = 24;
		}
		else if (i == 8 || i == 9)
		{
			vheader->bmiHeader.biCompression = 0x00000000;//RGB32
			vheader->bmiHeader.biPlanes =1;
			vheader->bmiHeader.biBitCount = 32;
		}			
		vheader->bmiHeader.biXPelsPerMeter = 0;
		vheader->bmiHeader.biYPelsPerMeter = 0;
		vheader->bmiHeader.biClrUsed = 0;
		vheader->bmiHeader.biClrImportant = 0;
	}

	for (int i = 1; i < 10;i=i+2)
	{
		VIDEOINFOHEADER2* vheader2 = (VIDEOINFOHEADER2*)OutputType[i]->pbFormat;
		vheader2->rcSource.left = 0;
		vheader2->rcSource.right = transform_setting->output_w;
		vheader2->rcSource.top = 0;
		vheader2->rcSource.bottom = transform_setting->output_h;
		vheader2->rcTarget.left = 0;
		vheader2->rcTarget.right = transform_setting->output_w;
		vheader2->rcTarget.top = 0;
		vheader2->rcTarget.bottom = transform_setting->output_h;
		if (i < 7)
		{
			//vheader2->dwBitRate = transform_setting->output_w*transform_setting->output_h * (3 / 2) * (10000000 / AvgTimePerFrame);
			vheader2->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 3 / 2;
		}
		else if(i == 7)
		{
			vheader2->dwBitRate = transform_setting->output_w*transform_setting->output_h * 3 * (10000000 / AvgTimePerFrame);
			vheader2->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 3 ;
		}
		else if (i == 9)
		{
			vheader2->dwBitRate = transform_setting->output_w*transform_setting->output_h * 4 * (10000000 / AvgTimePerFrame);
			vheader2->bmiHeader.biSizeImage = transform_setting->output_w*transform_setting->output_h * 4;
		}
		vheader2->dwBitRate = 0;
		vheader2->dwBitErrorRate = 0;
		vheader2->AvgTimePerFrame = AvgTimePerFrame;
		vheader2->bmiHeader.biSize = 40;
		vheader2->bmiHeader.biWidth = transform_setting->output_w;
		vheader2->bmiHeader.biHeight = transform_setting->output_h;
		if (i == 0 || i == 1)
		{
			vheader2->bmiHeader.biCompression = 0x56555949;//IYUV	
			vheader2->bmiHeader.biPlanes = 3;
			vheader2->bmiHeader.biBitCount = 12;
		}
		else if (i == 2 || i == 3)
		{
			vheader2->bmiHeader.biCompression = 0x3231564e;//NV12
			vheader2->bmiHeader.biPlanes = 2;
			vheader2->bmiHeader.biBitCount = 12;
		}
		else if (i == 4 || i == 5)
		{
			vheader2->bmiHeader.biCompression = 0x32315659;//YV12
			vheader2->bmiHeader.biPlanes = 3;
			vheader2->bmiHeader.biBitCount = 12;
		}
		else if (i == 6 || i == 7)
		{
			vheader2->bmiHeader.biCompression = 0x00000000;//RGB24
			vheader2->bmiHeader.biPlanes = 1;
			vheader2->bmiHeader.biBitCount = 24;
		}
		else if (i == 8 || i == 9)
		{
			vheader2->bmiHeader.biCompression = 0x00000000;//RGB32
			vheader2->bmiHeader.biPlanes = 1;
			vheader2->bmiHeader.biBitCount = 32;
		}
		vheader2->bmiHeader.biXPelsPerMeter = 0;
		vheader2->bmiHeader.biYPelsPerMeter = 0;
		vheader2->bmiHeader.biClrUsed = 0;
		vheader2->bmiHeader.biClrImportant = 0;

		vheader2->dwInterlaceFlags = 0x00000081;
		vheader2->dwCopyProtectFlags = 0x00000000;
		vheader2->dwPictAspectRatioX = 0x00000002;
		vheader2->dwPictAspectRatioY = 0x00000001;
		vheader2->dwControlFlags = 0x00000000;
	}
}
void TransformFilter::init_outputtype()
{
	OutputType[0] = new CMediaType;
	OutputType[0]->SetType(&MEDIATYPE_Video);
	OutputType[0]->SetSubtype(&MEDIASUBTYPE_IYUV);
	OutputType[0]->SetFormatType(&FORMAT_VideoInfo);
	OutputType[0]->bFixedSizeSamples = TRUE;
	OutputType[0]->SetTemporalCompression(FALSE);
	OutputType[0]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[0]->cbFormat = 88;
	OutputType[0]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER));

	OutputType[1] = new CMediaType;
	OutputType[1]->SetType(&MEDIATYPE_Video);
	OutputType[1]->SetSubtype(&MEDIASUBTYPE_IYUV);
	OutputType[1]->SetFormatType(&FORMAT_VIDEOINFO2);
	OutputType[1]->bFixedSizeSamples = TRUE;
	OutputType[1]->SetTemporalCompression(FALSE);
	OutputType[1]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[1]->cbFormat = 112;
	OutputType[1]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER2));

	OutputType[2] = new CMediaType;
	OutputType[2]->SetType(&MEDIATYPE_Video);
	OutputType[2]->SetSubtype(&MEDIASUBTYPE_NV12);
	OutputType[2]->SetFormatType(&FORMAT_VideoInfo);
	OutputType[2]->bFixedSizeSamples = TRUE;
	OutputType[2]->SetTemporalCompression(FALSE);
	OutputType[2]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[2]->cbFormat = 88;
	OutputType[2]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER));

	OutputType[3] = new CMediaType;
	OutputType[3]->SetType(&MEDIATYPE_Video);
	OutputType[3]->SetSubtype(&MEDIASUBTYPE_NV12);
	OutputType[3]->SetFormatType(&FORMAT_VIDEOINFO2);
	OutputType[3]->bFixedSizeSamples = TRUE;
	OutputType[3]->SetTemporalCompression(FALSE);
	OutputType[3]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[3]->cbFormat = 112;
	OutputType[3]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER2));

	OutputType[4] = new CMediaType;
	OutputType[4]->SetType(&MEDIATYPE_Video);
	OutputType[4]->SetSubtype(&MEDIASUBTYPE_YV12);
	OutputType[4]->SetFormatType(&FORMAT_VideoInfo);
	OutputType[4]->bFixedSizeSamples = TRUE;
	OutputType[4]->SetTemporalCompression(FALSE);
	OutputType[4]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[4]->cbFormat = 88;
	OutputType[4]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER));

	OutputType[5] = new CMediaType;
	OutputType[5]->SetType(&MEDIATYPE_Video);
	OutputType[5]->SetSubtype(&MEDIASUBTYPE_YV12);
	OutputType[5]->SetFormatType(&FORMAT_VIDEOINFO2);
	OutputType[5]->bFixedSizeSamples = TRUE;
	OutputType[5]->SetTemporalCompression(FALSE);
	OutputType[5]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3 / 2);
	OutputType[5]->cbFormat = 112;
	OutputType[5]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER2));

	OutputType[6] = new CMediaType;
	OutputType[6]->SetType(&MEDIATYPE_Video);
	OutputType[6]->SetSubtype(&MEDIASUBTYPE_RGB24);
	OutputType[6]->SetFormatType(&FORMAT_VideoInfo);
	OutputType[6]->bFixedSizeSamples = TRUE;
	OutputType[6]->SetTemporalCompression(FALSE);
	OutputType[6]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3);
	OutputType[6]->cbFormat = 88;
	OutputType[6]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER));

	OutputType[7] = new CMediaType;
	OutputType[7]->SetType(&MEDIATYPE_Video);
	OutputType[7]->SetSubtype(&MEDIASUBTYPE_RGB24);
	OutputType[7]->SetFormatType(&FORMAT_VIDEOINFO2);
	OutputType[7]->bFixedSizeSamples = TRUE;
	OutputType[7]->SetTemporalCompression(FALSE);
	OutputType[7]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 3);
	OutputType[7]->cbFormat = 112;
	OutputType[7]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER2));

	OutputType[8] = new CMediaType;
	OutputType[8]->SetType(&MEDIATYPE_Video);
	OutputType[8]->SetSubtype(&MEDIASUBTYPE_RGB32);
	OutputType[8]->SetFormatType(&FORMAT_VideoInfo);
	OutputType[8]->bFixedSizeSamples = TRUE;
	OutputType[8]->SetTemporalCompression(FALSE);
	OutputType[8]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 4);
	OutputType[8]->cbFormat = 88;
	OutputType[8]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER));

	OutputType[9] = new CMediaType;
	OutputType[9]->SetType(&MEDIATYPE_Video);
	OutputType[9]->SetSubtype(&MEDIASUBTYPE_RGB32);
	OutputType[9]->SetFormatType(&FORMAT_VIDEOINFO2);
	OutputType[9]->bFixedSizeSamples = TRUE;
	OutputType[9]->SetTemporalCompression(FALSE);
	OutputType[9]->SetSampleSize(transform_setting->output_w*transform_setting->output_h * 4);
	OutputType[9]->cbFormat = 112;
	OutputType[9]->pbFormat = (BYTE*)malloc(sizeof(VIDEOINFOHEADER2));
}
void TransformFilter::test_write(AVFrame *pf)
{	
	//测试用写入点
	FILE *fpout;
	fopen_s(&fpout, "G://framein.yuv", "w");
	fwrite(pf->data[0], pf->width*pf->height, 1, fpout);
	fwrite(pf->data[1], pf->width*pf->height / 2, 1, fpout);
	//fwrite(pf->data[2], pf->width*pf->height / 4, 1, fpout);
	fclose(fpout);
}
void TransformFilter::trans_yv12_yuv420p(AVFrame* pf)
{
	uint8_t *p;
	p = pf->data[1];
	pf->data[1] = pf->data[2];
	pf->data[2] = p;
}

int TransformFilter::generate_map(TransformContext *s, AVFrame *in, AVFrame *out) {
	int ret = 0;
	/*
	根据输出链路获得输出的格式
	*/
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)in->format);//帧格式结构体，描述图像的plane及存储格式
	s->planes = av_pix_fmt_count_planes((AVPixelFormat)in->format);//获得格式的plane路数
	s->out_map_planes = 2;
	/*
	使用context初始化参数信息，传递给handle
	*/
	FrameTransformContext frame_transform_ctx;
	frame_transform_ctx.input_layout = (Layout)s->input_layout,
		frame_transform_ctx.output_layout = (Layout)s->output_layout,
		frame_transform_ctx.input_stereo_format = (StereoFormat)s->input_stereo_format,
		frame_transform_ctx.output_stereo_format = (StereoFormat)s->output_stereo_format,
		frame_transform_ctx.vflip = s->vflip,
		frame_transform_ctx.input_expand_coef = s->input_expand_coef,
		frame_transform_ctx.expand_coef = s->expand_coef,
		frame_transform_ctx.interpolation_alg = (InterpolationAlg)s->interpolation_alg,
		frame_transform_ctx.width_scale_factor = s->width_scale_factor,
		frame_transform_ctx.height_scale_factor = s->height_scale_factor,
		frame_transform_ctx.fixed_yaw = s->fixed_yaw,
		frame_transform_ctx.fixed_pitch = s->fixed_pitch,
		frame_transform_ctx.fixed_roll = s->fixed_roll,
		frame_transform_ctx.fixed_cube_offcenter_x = s->fixed_cube_offcenter_x,
		frame_transform_ctx.fixed_cube_offcenter_y = s->fixed_cube_offcenter_y,
		frame_transform_ctx.fixed_cube_offcenter_z = s->fixed_cube_offcenter_z,
		frame_transform_ctx.is_horizontal_offset = s->is_horizontal_offset,
		frame_transform_ctx.enable_low_pass_filter = s->enable_low_pass_filter,
		frame_transform_ctx.kernel_height_scale_factor = s->kernel_height_scale_factor,
		frame_transform_ctx.min_kernel_half_height = s->min_kernel_half_height,
		frame_transform_ctx.max_kernel_half_height = s->max_kernel_half_height,
		frame_transform_ctx.enable_multi_threading = s->enable_multi_threading,
		frame_transform_ctx.num_vertical_segments = s->num_vertical_segments,
		frame_transform_ctx.num_horizontal_segments = s->num_horizontal_segments,
		frame_transform_ctx.adjust_kernel = s->adjust_kernel,
		frame_transform_ctx.kernel_adjust_factor = s->kernel_adjust_factor;
	/*
	将Transform的context绑定到filter的context
	*/
	s->transform = VideoFrameTransform_new(&frame_transform_ctx);
	if (!s->transform) {
		return AVERROR(ENOMEM);
	}
	/*
	s->out_map_planes=2
	循环调用generateMap
	*/
	int in_w, in_h, out_w, out_h;
	for (int plane = 0; plane < s->out_map_planes; ++plane) {
		out_w = out->width;
		out_h = out->height;
		in_w = in->width;
		in_h = in->height;

		if (plane == 1) {
			update_plane_sizes(desc, &in_w, &in_h, &out_w, &out_h);
		}

		if (!VideoFrameTransform_generateMapForPlane(
			s->transform, in_w, in_h, out_w, out_h, plane)) {
			return AVERROR(EINVAL);
		}
	}
	return 0;
}
void TransformFilter::update_plane_sizes(const AVPixFmtDescriptor* desc, int* in_w, int* in_h, int* out_w, int* out_h)
{
	*in_w = FF_CEIL_RSHIFT(*in_w, desc->log2_chroma_w);
	*in_h = FF_CEIL_RSHIFT(*in_h, desc->log2_chroma_h);
	*out_w = FF_CEIL_RSHIFT(*out_w, desc->log2_chroma_w);
	*out_h = FF_CEIL_RSHIFT(*out_h, desc->log2_chroma_h);
}
void TransformFilter::init_Transform360()
{
	s = (TransformContext*)malloc(sizeof(TransformContext));
	*s = *setting;
}
int TransformFilter::VideoFrameTransform_transformFramePlane(VideoFrameTransform* transform,uint8_t* inputData,uint8_t* outputData,int inputWidth,int inputHeight,int inputWidthWithPadding,int outputWidth,int outputHeight,int outputWidthWithPadding,int transformMatPlaneIndex,int imagePlaneIndex) 
{
	return transform->transformFramePlane(inputData,outputData,inputWidth,inputHeight,inputWidthWithPadding,outputWidth,outputHeight,outputWidthWithPadding,transformMatPlaneIndex,imagePlaneIndex);
}
VideoFrameTransform* TransformFilter::VideoFrameTransform_new(FrameTransformContext* ctx) 
{
	return new VideoFrameTransform(ctx);
}
void TransformFilter::VideoFrameTransform_delete(VideoFrameTransform* transform) 
{
	delete transform;
}
int TransformFilter::VideoFrameTransform_generateMapForPlane(VideoFrameTransform* transform,int inputWidth,int inputHeight,int outputWidth,int outputHeight,int transformMatPlaneIndex) 
{
	return transform->generateMapForPlane(inputWidth,inputHeight,outputWidth,outputHeight,transformMatPlaneIndex);
}
void TransformFilter::destory_transform(TransformContext *s)
{
	VideoFrameTransform_delete(s->transform);
	s->transform = NULL;
}
void TransformFilter::read_frame_nv12(AVFrame *in)
{
	FILE *finput;
	fopen_s(&finput,"G://CUBIC_nv12.yuv","r+");
	uint8_t *pb = (uint8_t*)malloc(sizeof(char)*(1920 * 1280 * 3) / 2);
	fread(pb,sizeof(char),(1920*1280*3)/2,finput);
	av_image_fill_arrays(in->data, in->linesize, pb, AV_PIX_FMT_NV12, 1920, 1280, 1);
	fclose(finput);
}
/******************************Public Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
*
*
*
* History:
*
\**************************************************************************/
//****************************DLL注册入口***********************************
//DllEntryPoint
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD dwReason,
	LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}
//extern "C" _declspec(dllexport)BOOL DLLRegisterServer;