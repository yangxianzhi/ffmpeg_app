// ffmpeg_video2picture.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
/*将视频转图片*/
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_LIMIT_MACROS
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#define INBUF_SIZE 4096

#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

// 位图文件头（bitmap-file header）包含了图像类型、图像大小、图像数据存放地址和两个保留未使用的字段
typedef struct tagBITMAPFILEHEADER {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

// 位图信息头（bitmap-information header）包含了位图信息头的大小、图像的宽高、图像的色深、压缩说明图像数据的大小和其他一些参数。
typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

// 图片转换并保存
void saveBMP(struct SwsContext *img_convert_ctx, AVFrame *frame, char *filename)
{
	//1 先进行转换,  YUV420=>RGB24:
	int w = frame->width;
	int h = frame->height;


	//int numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, w, h);
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, w, h, 1); // 1按1字节对齐，实际大小。4按4字节对齐，实际大小的4
	uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));


	AVFrame *pFrameRGB = av_frame_alloc();
	/* buffer is going to be written to rawvideo file, no alignment */
	//avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_BGR24, w, h);

	//pFrameRGB,是将RGB转换成YUV之后，存储YUV一帧图像用的。 out_buffer是YUV的存储空间。
	//下面两个宽高，是图像的宽高（由于输出的宽高=输入的宽高，所以整个例子中只有一组宽高）。
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_BGR24, w, h, 1);

	/*
	进行转换
	frame->data和pFrameRGB->data分别指输入和输出的buf
	frame->linesize和pFrameRGB->linesize可以看成是输入和输出的每列的byte数
	第4个参数是指第一列要处理的位置
	第5个参数是source slice 的高度
	*/
	sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, h, pFrameRGB->data, pFrameRGB->linesize);

	//2 构造 BITMAPINFOHEADER
	BITMAPINFOHEADER header;
	header.biSize = sizeof(BITMAPINFOHEADER);

	int bpp = 24;
	header.biWidth = w;
	header.biHeight = h * (-1);
	header.biBitCount = 24;
	header.biCompression = 0;
	header.biSizeImage = (w*bpp + 31) / 32 * 4 * h;
	header.biClrImportant = 0;
	header.biClrUsed = 0;
	header.biXPelsPerMeter = 100;
	header.biYPelsPerMeter = 100;
	header.biPlanes = 1;

	//3 构造文件头
	BITMAPFILEHEADER bmpFileHeader = { 0, };
	//HANDLE hFile = NULL;
	DWORD dwTotalWriten = 0;
	//DWORD dwWriten;

	bmpFileHeader.bfType = 0x4d42; //'BM';
	bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + numBytes;
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	FILE* pf;
	fopen_s(&pf, filename, "wb");
	fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, pf);
	fwrite(&header, sizeof(BITMAPINFOHEADER), 1, pf);
	fwrite(pFrameRGB->data[0], 1, numBytes, pf);
	fclose(pf);


	//释放资源
	//av_free(buffer);
	av_freep(&pFrameRGB[0]);
	av_free(pFrameRGB);
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
	char *filename)
{
	FILE *f;
	int i;

	//f = fopen(filename, "w");
	fopen_s(&f, filename, "w");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

// 对数据进行解码
static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
	struct SwsContext *img_convert_ctx, AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
	//int len;
	int got_frame,len_packet;
	char buf[1024];

	// 对数据进行解码，avctx是编解码器上下文，解码后的帧存放在frame，通过got_frame获取解码后的帧，pkt是输入包
	// len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
	// if (len < 0) {
	len_packet = avcodec_send_packet(avctx, pkt);
	if (len_packet < 0 ) {
		fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
		return len_packet;
	}
	got_frame = avcodec_receive_frame(avctx, frame);

	// 解码成功
	if (!got_frame) {
		printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
		fflush(stdout);

		/* the picture is allocated by the decoder, no need to free it */
		snprintf(buf, sizeof(buf), "%s-%d.bmp", outfilename, *frame_count);

		// 解码后的数据帧保存rgb图片
		saveBMP(img_convert_ctx, frame, buf);

		(*frame_count)++;
	}
	return 0;
}


int main(int argc, char **argv)
{
	int ret;

	//FILE *f;

	const char *filename, *outfilename; // 输入、输出文件路径

	AVFormatContext *fmt_ctx = NULL; // 格式上下文环境(输入文件的上下文环境)

	const AVCodec *codec; // 编解码器
	AVCodecContext *c = NULL; // 编解码器上下文环境(输出文件的上下文环境)

	AVStream *st = NULL; // 多媒体文件中的流
	int stream_index; // 流的index

	int frame_count;
	AVFrame *frame; // 原始帧

	struct SwsContext *img_convert_ctx; // 图片处理的上下文

	//uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	AVPacket avpkt;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	filename = argv[1];
	outfilename = argv[2];

	// 打开输入文件，创建格式上下文环境
	if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", filename);
		exit(1);
	}

	// 检索多媒体文件中的流信息
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	// 打印输入文件的详细信息
	av_dump_format(fmt_ctx, 0, filename, 0);

	av_init_packet(&avpkt);

	// 查找视频流，返回值ret是流的索引号
	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO), filename);
		return ret;
	}

	// 根据索引号获取流
	stream_index = ret;
	st = fmt_ctx->streams[stream_index];

	// 根据流中编码参数中的解码器ID查找解码器
	// codec = avcodec_find_decoder(st->codecpar->codec_id);
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Failed to find %s codec\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return AVERROR(EINVAL);
	}

	// 分配输出文件的上下文环境
	c = avcodec_alloc_context3(NULL);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	// 将视频流的编解码参数直接拷贝到输出上下文环境中
	if ((ret = avcodec_parameters_to_context(c, st->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return ret;
	}


	// 打开解码器
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	/*
		初始化图片转换上下文环境
		sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
						int dstW, int dstH, enum AVPixelFormat dstFormat,
							   int flags, SwsFilter *srcFilter,
							   SwsFilter *dstFilter, const double *param);
		srcW/srcH/srcFormat分别为原始图像数据的宽高和数据类型，数据类型比如AV_PIX_FMT_YUV420、PAV_PIX_FMT_RGB24
		dstW/dstH/dstFormat分别为输出图像数据的宽高和数据类型
		flags是scale算法种类；SWS_BICUBIC、SWS_BICUBLIN、SWS_POINT、SWS_SINC等
		最后3个参数不用管，都设为NULL
	*/
	img_convert_ctx = sws_getContext(c->width, c->height,
		c->pix_fmt,
		c->width, c->height,
		AV_PIX_FMT_BGR24,
		SWS_BICUBIC, NULL, NULL, NULL);

	if (img_convert_ctx == NULL)
	{
		fprintf(stderr, "Cannot initialize the conversion context\n");
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	frame_count = 0;
	// 循环从多媒体文件中读取一帧一帧的数据
	while (av_read_frame(fmt_ctx, &avpkt) >= 0) {

		// 如果读取到的数据包的stream_index和视频流的stream_index一致就进行解码
		if (avpkt.stream_index == stream_index) {
			if (decode_write_frame(outfilename, c, img_convert_ctx, frame, &frame_count, &avpkt, 0) < 0)
				exit(1);
		}

		av_packet_unref(&avpkt);
	}

	/* Some codecs, such as MPEG, transmit the I- and P-frame with a
	   latency of one frame. You must do the following to have a
	   chance to get the last frame of the video. */
	avpkt.data = NULL;
	avpkt.size = 0;
	decode_write_frame(outfilename, c, img_convert_ctx, frame, &frame_count, &avpkt, 1);

	//fclose(f);

	avformat_close_input(&fmt_ctx);

	sws_freeContext(img_convert_ctx);
	avcodec_free_context(&c);
	av_frame_free(&frame);

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
