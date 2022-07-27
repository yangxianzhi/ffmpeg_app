#pragma once
#include <iostream>
#include <cstdio>
#include <cstring>

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
//#include <jpeglib.h>
}

using namespace std;

char errbuf[256];
char timebuf[256];
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = NULL;
static const char *src_filename = NULL;
static const char *output_dir = NULL;
static int video_stream_idx = -1;
static AVFrame *frame = NULL;
static AVFrame *pFrameRGB = NULL;
static AVPacket pkt;
static struct SwsContext *pSWSCtx = NULL;
static int video_frame_count = 0;

/* Enable or disable frame reference counting. You are not supposed to support
* both paths in your application but pick the one most appropriate to your
* needs. Look for the use of refcount in this example to see what are the
* differences of API usage between them. */
static int refcount = 0;

// static void jpg_save(uint8_t *pRGBBuffer, int iFrame, int width, int height)
// {
// 
// 	struct jpeg_compress_struct cinfo;
// 
// 	struct jpeg_error_mgr jerr;
// 
// 	char szFilename[1024];
// 	int row_stride;
// 
// 	FILE *fp;
// 	JSAMPROW row_pointer[1]; // 一行位图
// 	cinfo.err = jpeg_std_error(&jerr);
// 	jpeg_create_compress(&cinfo);
// 
// 	sprintf_s(szFilename, 256, "%s/image-%03d.jpg", output_dir, iFrame); //图片名字为视频名+号码
// 	//fp = fopen(szFilename, "wb");
// 	fopen_s(&fp, szFilename, "wb");
// 
// 	if (fp == NULL)
// 		return;
// 
// 	jpeg_stdio_dest(&cinfo, fp);
// 
// 	cinfo.image_width = width; // 为图的宽和高，单位为像素
// 	cinfo.image_height = height;
// 	cinfo.input_components = 3; // 在此为1,表示灰度图， 如果是彩色位图，则为3
// 	cinfo.in_color_space = JCS_RGB; //JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像
// 
// 	jpeg_set_defaults(&cinfo);
// 	jpeg_set_quality(&cinfo, 80, 1);
// 
// 	jpeg_start_compress(&cinfo, TRUE);
// 
// 	row_stride = cinfo.image_width * 3; //每一行的字节数,如果不是索引图,此处需要乘以3
// 
// 	// 对每一行进行压缩
// 	while (cinfo.next_scanline < cinfo.image_height)
// 	{
// 		row_pointer[0] = &(pRGBBuffer[cinfo.next_scanline * row_stride]);
// 		jpeg_write_scanlines(&cinfo, row_pointer, 1);
// 	}
// 
// 	jpeg_finish_compress(&cinfo);
// 	jpeg_destroy_compress(&cinfo);
// 
// 	fclose(fp);
// }
#define MAX_PATH 256
/**
 * 将AVFrame(YUV420格式)保存为JPEG格式的图片
 *
 * @param width YUV420的宽
 * @param height YUV42的高
 *
 */
int MyWriteJPEG(AVFrame* pFrame, int width, int height, int iIndex)
{
	// 输出文件路径
	char out_file[MAX_PATH] = { 0 };
	sprintf_s(out_file, sizeof(out_file), "%s%d.jpg", "E:/temp/", iIndex);

	// 分配AVFormatContext对象
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	// 设置输出文件格式
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	// 创建并初始化一个和该url相关的AVIOContext
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// 构建一个新stream
	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	// 设置该stream的信息
	AVCodecContext* pCodecCtx = pAVStream->codec;

	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);

	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[1]->codecpar);

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// Begin Output some information
	av_dump_format(pFormatCtx, 0, out_file, 1);
	// End Output some information

	// 查找解码器
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		printf("Codec not found.");
		return -1;
	}
	// 设置pCodecCtx的解码器为pCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	// 给AVPacket分配足够大的空间
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// 
	int got_picture = 0;
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if (ret < 0) {
		printf("Encode Error.\n");
		return -1;
	}
	if (got_picture == 1) {
		//pkt.stream_index = pAVStream->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);

	printf("Encode Successful.\n");

	if (pAVStream) {
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

static int decode_packet(int *got_frame, int cached)
{
	int ret = 0;
	int decoded = pkt.size;
	//*got_frame = 0;
	int len_packet;

	if (pkt.stream_index == video_stream_idx)
	{
		/* decode video frame */
		//ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
		//if (ret < 0)

		len_packet = avcodec_send_packet(video_dec_ctx, &pkt);
		if (len_packet < 0) {

			fprintf(stderr, "Error decoding video frame (%s)n", av_make_error_string(errbuf, sizeof(errbuf), ret));
			return ret;
		}
		*got_frame = avcodec_receive_frame(video_dec_ctx, frame);
		if (!*got_frame)
		{
			if (frame->width != width || frame->height != height ||
				frame->format != pix_fmt)
			{
				/* To handle this change, one could call av_image_alloc again and
				* decode the following frames into another rawvideo file. */
				fprintf(stderr, "Error: Width, height and pixel format have to be "
					"constant in a rawvideo file, but the width, height or "
					"pixel format of the input video changed:n"
					"old: width = %d, height = %d, format = %sn"
					"new: width = %d, height = %d, format = %sn",
					width, height, av_get_pix_fmt_name(pix_fmt),
					frame->width, frame->height,
					av_get_pix_fmt_name(AVPixelFormat(frame->format)));
				return -1;
			}

			video_frame_count++;
			static int iFrame = 0;
			if (frame->key_frame == 1) //如果是关键帧
			{
				sws_scale(pSWSCtx, frame->data, frame->linesize, 0,
					video_dec_ctx->height,
					pFrameRGB->data, pFrameRGB->linesize);
				// 保存到磁盘
				iFrame++;
				//jpg_save(pFrameRGB->data[0], iFrame, width, height);
				MyWriteJPEG(pFrameRGB, iFrame, width, height);
			}
		}
	}
	/* If we use frame reference counting, we own the data and need
	* to de-reference it when we don't use it anymore */
	if (*got_frame && refcount)
		av_frame_unref(frame);
	return decoded;
}

static int open_codec_context(int *stream_idx,
	AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream *st;
	AVCodec *dec = NULL;
	AVDictionary *opts = NULL;
	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0)
	{
		fprintf(stderr, "Could not find %s stream in input file '%s'n",
			av_get_media_type_string(type), src_filename);
		return ret;
	}
	else
	{
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];
		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec)
		{
			fprintf(stderr, "Failed to find %s codecn",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}
		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx)
		{
			fprintf(stderr, "Failed to allocate the %s codec contextn",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}
		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0)
		{
			fprintf(stderr, "Failed to copy %s codec parameters to decoder contextn",
				av_get_media_type_string(type));
			return ret;
		}
		/* Init the decoders, with or without reference counting */
		av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
		if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0)
		{
			fprintf(stderr, "Failed to open %s codecn",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}
	return 0;
}

static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt)
{
	int i;
	struct sample_fmt_entry
	{
		enum AVSampleFormat sample_fmt;
		const char *fmt_be, *fmt_le;
	} sample_fmt_entries[] = {
	{AV_SAMPLE_FMT_U8, "u8", "u8"},
	{AV_SAMPLE_FMT_S16, "s16be", "s16le"},
	{AV_SAMPLE_FMT_S32, "s32be", "s32le"},
	{AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
	{AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
	};
	*fmt = NULL;
	for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++)
	{
		struct sample_fmt_entry *entry = &sample_fmt_entries[i];
		if (sample_fmt == entry->sample_fmt)
		{
			*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
			return 0;
		}
	}
	fprintf(stderr,
		"sample format %s is not supported as output formatn",
		av_get_sample_fmt_name(sample_fmt));
	return -1;
}

int main(int argc, char **argv)
{
	av_register_all();
	int ret = 0, got_frame;
	int numBytes = 0;
	uint8_t *buffer = NULL;
	if (argc != 3 && argc != 4)
	{
		fprintf(stderr, "usage: %s [-refcount] input_file ouput_dirn"
			"API example program to show how to read frames from an input file.n"
			"This program reads frames from a file, decodes them, and writes bmp keyframesn"
			"If the -refcount option is specified, the program use then"
			"reference counting frame system which allows keeping a copy ofn"
			"the data for longer than one decode call.n"
			"n",
			argv[0]);
		exit(1);
	}

	if (argc == 4 && !strcmp(argv[1], "-refcount"))
	{
		refcount = 1;
		argv++;
	}

	src_filename = argv[1];
	output_dir = argv[2];

	/* open input file, and allocate format context */
	if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
	{
		fprintf(stderr, "Could not open source file %sn", src_filename);
		exit(1);
	}

	/* retrieve stream information */
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
	{
		fprintf(stderr, "Could not find stream informationn");
		exit(1);
	}

	if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0)
	{
		video_stream = fmt_ctx->streams[video_stream_idx];
		/* allocate image where the decoded image will be put */
		width = video_dec_ctx->width;
		height = video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
	}
	else
	{
		goto end;
	}

	/* dump input information to stderr */
	av_dump_format(fmt_ctx, 0, src_filename, 0);
	if (!video_stream)
	{
		fprintf(stderr, "Could not find video stream in the input, abortingn");
		ret = 1;
		goto end;
	}

	pFrameRGB = av_frame_alloc();
	//numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, width, height);
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, width, height, 1);
	buffer = (uint8_t *)av_malloc(numBytes);
	//avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_BGR24, width, height);
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_BGR24, width, height, 1);
	pSWSCtx = sws_getContext(width, height, pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

	frame = av_frame_alloc();
	if (!frame)
	{
		fprintf(stderr, "Could not allocate framen");
		ret = AVERROR(ENOMEM);
		goto end;
	}

	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	if (video_stream)
		printf("Demuxing video from file '%s' to dir: %sn", src_filename, output_dir);

	/* read frames from the file */
	while (av_read_frame(fmt_ctx, &pkt) >= 0)
	{
		AVPacket orig_pkt = pkt;
		do
		{
			ret = decode_packet(&got_frame, 0);
			if (ret < 0)
				break;
			pkt.data += ret;
			pkt.size -= ret;
		} while (pkt.size > 0);
		av_packet_unref(&orig_pkt);
	}

	/* flush cached frames */
	pkt.data = NULL;
	pkt.size = 0;

end:
	if (video_dec_ctx)
		avcodec_free_context(&video_dec_ctx);
	if (fmt_ctx)
		avformat_close_input(&fmt_ctx);
	if ((void*)buffer != NULL)
		av_free((void*)buffer);
	if (pFrameRGB)
		av_frame_free(&pFrameRGB);
	if (frame)
		av_frame_free(&frame);
	return ret < 0;
}
