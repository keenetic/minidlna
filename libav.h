
#if HAVE_FFMPEG_LIBAVUTIL_AVUTIL_H
#include <ffmpeg/libavutil/avutil.h>
#elif HAVE_LIBAV_LIBAVUTIL_AVUTIL_H
#include <libav/libavutil/avutil.h>
#elif HAVE_LIBAVUTIL_AVUTIL_H
#include <libavutil/avutil.h>
#elif HAVE_FFMPEG_AVUTIL_H
#include <ffmpeg/avutil.h>
#elif HAVE_LIBAV_AVUTIL_H
#include <libav/avutil.h>
#elif HAVE_AVUTIL_H
#include <avutil.h>
#endif

#if HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
#include <ffmpeg/libavcodec/avcodec.h>
#elif HAVE_LIBAV_LIBAVCODEC_AVCODEC_H
#include <libav/libavcodec/avcodec.h>
#elif HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#elif HAVE_FFMPEG_AVCODEC_H
#include <ffmpeg/avcodec.h>
#elif HAVE_LIBAV_AVCODEC_H
#include <libav/avcodec.h>
#elif HAVE_AVCODEC_H
#include <avcodec.h>
#endif

#if HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
#include <ffmpeg/libavformat/avformat.h>
#elif HAVE_LIBAV_LIBAVFORMAT_AVFORMAT_H
#include <libav/libavformat/avformat.h>
#elif HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#elif HAVE_FFMPEG_AVFORMAT_H
#include <ffmpeg/avformat.h>
#elif HAVE_LIBAV_LIBAVFORMAT_H
#include <libav/avformat.h>
#elif HAVE_AVFORMAT_H
#include <avformat.h>
#endif

#ifndef FF_PROFILE_H264_BASELINE
#define FF_PROFILE_H264_BASELINE 66
#endif
#ifndef FF_PROFILE_H264_CONSTRAINED_BASELINE
#define FF_PROFILE_H264_CONSTRAINED_BASELINE 578
#endif
#ifndef FF_PROFILE_H264_MAIN
#define FF_PROFILE_H264_MAIN 77
#endif
#ifndef FF_PROFILE_H264_HIGH
#define FF_PROFILE_H264_HIGH 100
#endif
#ifndef FF_PROFILE_SKIP
#define FF_PROFILE_SKIP -100
#endif

#if LIBAVCODEC_VERSION_MAJOR < 53
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#endif

#if LIBAVUTIL_VERSION_INT < ((50<<16)+(13<<8)+0)
#define av_strerror(x, y, z) snprintf(y, z, "%d", x)
#endif

