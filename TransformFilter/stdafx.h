// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#include <afxwin.h>
#include <stdio.h>
#include <tchar.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h> 
#include <libavfilter/buffersink.h>  
#include <libavfilter/buffersrc.h>  
#include <libavutil/opt.h>
#include <libavutil/eval.h>
#include <libavutil/avassert.h>
};



// TODO: 在此处引用程序需要的其他头文件
