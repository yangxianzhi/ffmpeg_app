#ifndef COMMON_H_
#define COMMON_H_

#ifdef _WIN32
//Windows
extern "C"
{
#include <windows.h>
#include <csignal>
#include <intsafe.h>
#include <stdint.h>  
#include <inttypes.h>  
//#include <config.h>
#include <libavcodec/avcodec.h> 
#include <libavcodec/avfft.h>
#include <libavformat/avformat.h> 
#include <libavdevice/avdevice.h> 
#include <libavutil/imgutils.h> 
#include <libavutil/time.h> 
#include <libavutil/avstring.h> 
#include <libavutil/display.h> 
#include <libavutil/avutil.h> 
#include <libavutil/mem.h>
#include <libswscale/swscale.h> 
#include <libswresample/swresample.h> 
#include <libpostproc/postprocess.h> 
#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif
#include <SDL2/SDL.h> 
#include <SDL2/SDL_syswm.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <config.h>
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h> 
#include <libavdevice/avdevice.h> 
#include <libavfilter/avfilter.h> 
#include <libavutil/imgutils.h> 
#include <libavutil/time.h> 
#include <libavutil/avstring.h> 
#include <libavutil/display.h> 
#include <libavutil/avutil.h> 
#include <libavutil/mem.h>
#include <libswscale/swscale.h> 
#include <libswresample/swresample.h> 
#include <libpostproc/postprocess.h> 
#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif
#include <SDL2/SDL.h>
#include <SDL_syswm.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#ifdef __cplusplus
};
#endif
#endif

#endif