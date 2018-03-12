#pragma once
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <Dvdmedia.h>
#include "interface.h"
#include "TransformContext.h"
#include "VideoFrameTransformHelper.h"
class Transform360;
class TransformFilter :public CTransformFilter,public ISpecifyPropertyPages,public TransformFilterInterface
{
private:
	/*
	FFmpeg Filter相关变量
	*/
	enum AVPixelFormat pix_fmts[2]={AV_PIX_FMT_YUV420P,AV_PIX_FMT_NONE };
	AVFrame *pFrameIn, *pFrameTrans,*pFrameOut,*pFrameOutput,*pFrameRGBRotate, *pFrameRGBOutRotate;
	AVRational time_base;
	int ret;
	char args[512],*error;
	UINT8 *pbufferin, *pbufferout,*pframeout_buffer, *pframeoutput_buffer,*pframetrans_buffer,*pframe_RGB_rotate_buffer, *pframe_RGB_trans_buffer;
	struct SwsContext *img_convert_ctx,*output_convert_ctx,*middle_convert;

	CMediaType InputType,*OutputType[10];
	REFERENCE_TIME AvgTimePerFrame;
	VIDEOINFOHEADER2 InVinfo2;
	VIDEOINFOHEADER InVinfo;
	BITMAPINFOHEADER InBinfo,OutBinfo;
	long OutBuffer,InWidth, InHeight, OutWidth, OutHeight;

	BOOL FFmpeg_initFlag=FALSE,Filter_initFlag=FALSE;
	int max_cube_edge_length=0;
	Transform360 *transform_setting;

	TransformContext *s,*setting;
	bool transformctx_flag = false;
public:
	TransformFilter(TCHAR *tszName, LPUNKNOWN punk);
	~TransformFilter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT CheckInputType(const CMediaType* mtIN);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties);
	virtual HRESULT BreakConnect(PIN_DIRECTION dir);

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP GetPages(CAUUID *pPages);
	STDMETHODIMP DoSetting(int output_w, int output_h, int inputlayout, int outputlayout, int frameformat, int cubelength, int cubemax, int interpolationALG, BOOL lowpass_enable, BOOL lowpass_MT, int vs, int hs);
	STDMETHODIMP GetSetting(int *output_w, int *output_h, int *inputlayout, int *outputlayout, int *frameformat, int *cubelength, int *cubemax, int *interpolationALG, BOOL *lowpass_enable, BOOL *lowpass_MT, int *vs, int *hs);

	void save_type(const CMediaType* mtIn);
	bool init_ffmpeg();
	void init_all();
	int destory_ffmpeg();
	int cal_outputsize();
	int transform_by_filter(AVFrame *in, AVFrame *out);
	void get_yuvbuffer(byte* pout, AVFrame *pf);
	void get_yv12buffer(byte* pout,AVFrame *pf);
	void get_nv12buffer(byte* pout, AVFrame *pf);
	void get_rgb32buffer(byte* pout, AVFrame *pf);
	void get_rgb24buffer(byte* pout, AVFrame *pf);
	void set_outputtype();
	void init_outputtype();
	void trans_yv12_yuv420p(AVFrame* pf);
	void test_write(AVFrame *pf);
	int generate_map(TransformContext *s, AVFrame *in, AVFrame *out);
	void update_plane_sizes(const AVPixFmtDescriptor* desc, int* in_w, int* in_h, int* out_w, int* out_h);
	void init_Transform360();
	int VideoFrameTransform_transformFramePlane(VideoFrameTransform* transform,uint8_t* inputData,uint8_t* outputData,int inputWidth,int inputHeight,int inputWidthWithPadding,int outputWidth,int outputHeight,int outputWidthWithPadding,int transformMatPlaneIndex,int imagePlaneIndex);
	VideoFrameTransform* VideoFrameTransform_new(FrameTransformContext* ctx);
	void VideoFrameTransform_delete(VideoFrameTransform* transform);
	int VideoFrameTransform_generateMapForPlane(VideoFrameTransform* transform,int inputWidth,int inputHeight,int outputWidth,int outputHeight,int transformMatPlaneIndex);
	void TransformFilter::destory_transform(TransformContext *s);

	void read_frame_nv12(AVFrame *in);
	DECLARE_IUNKNOWN;
};


class Transform360
{
public:
	AVPixelFormat inputformat;
	AVPixelFormat outputformat;
	int frameformat;
	AM_MEDIA_TYPE backup_output_type;
	int input_w;
	int input_h;
	int output_w;
	int output_h;
	int cube_edge_length;
	int max_cube_edgt_length;
	int w_subdivisions;
	int h_subdivisions;
	char w_expr[4];               ///< width  expression string
	char h_expr[4];               ///< height expression string
};
