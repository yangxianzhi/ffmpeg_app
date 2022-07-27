#pragma once
/*
* Copyright (c) 2001 Fabrice Bellard
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /**
  * @file
  * video decoding with libavcodec API example
  *
  * @example decode_video.c
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

#define INBUF_SIZE 4096

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
	FILE *f;
	int i;

#ifndef _WIN32
	f = fopen(filename, "wb");
#else
	fopen_s(&f, filename, "wb");
#endif
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

#pragma pack(2)
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
void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, char *filename, int bpp)
{
	char buf[5] = { 0 };
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	FILE *fp = NULL;

#ifndef _WIN32
	fp = fopen(filename, "wb");
#else
	fopen_s(&fp, filename, "wb");
#endif
	if (fp == NULL) {
		printf("open file failed!\n");
		return;
	}

	bmpheader.bfType = 0x4d42;
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width * height*bpp / 8;

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = width;
	bmpinfo.biHeight = height;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = bpp;
	bmpinfo.biCompression = 0;
	bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
	bmpinfo.biXPelsPerMeter = 100;
	bmpinfo.biYPelsPerMeter = 100;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;

	fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
	fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
	fwrite(pFrameRGB->data[0], width*height*bpp / 8, 1, fp);

	fclose(fp);
}


// Super ineffient but a proof of concept adapted from sources across the intertubes
int avframe_to_jpeg(AVFrame *srcFrame, char *filename) {
	const AVCodec *mjpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	if (!mjpegCodec) {
		printf("could not find mjpeg codec\n");
		return -1;
	}

	AVCodecContext *jpegAVCC = avcodec_alloc_context3(mjpegCodec);
	if (!jpegAVCC) {
		printf("could not allocate jpeg codec context\n");
		return -1;
	}

	jpegAVCC->time_base = AVRational { 1,25 };
	jpegAVCC->width = srcFrame->width;
	jpegAVCC->height = srcFrame->height;
// 	jpegAVCC->coded_width = srcFrame->width;
// 	jpegAVCC->coded_height = srcFrame->height;
	jpegAVCC->pix_fmt = AV_PIX_FMT_YUVJ420P;

	printf("height = %d\n", srcFrame->height);
	printf("width = %d\n", srcFrame->width);

	printf("before opening the jpeg context, jpegAVCC->pix_fmt=%d\n", jpegAVCC->pix_fmt);
	if (avcodec_open2(jpegAVCC, mjpegCodec, NULL) < 0) {
		printf("could not open jpeg codec context.\n");
		exit(1);
		return -1;
	}
	printf("end opened the jpeg context, jpegAVCC->pix_fmt=%d\n", jpegAVCC->pix_fmt);

	AVPacket *packet = av_packet_alloc();
	if (!packet) {
		return false;
	}

	FILE *jpegFile;
	printf("before writing the picture\n");
#ifndef _WIN32
	jpegFile = fopen(filename, "wb");
#else
	fopen_s(&jpegFile, filename, "wb");
#endif

	int got_picture = 0;
	int ret = avcodec_send_frame(jpegAVCC, srcFrame);
	if (ret < 0) {
		printf("Encode Error.\n");
		return -1;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(jpegAVCC, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			break;
		}
		fwrite(packet->data, 1, (size_t)packet->size, jpegFile);
		av_packet_unref(packet);
	}

	fclose(jpegFile);

	av_packet_free(&packet);
	avcodec_close(jpegAVCC);
	return 0;
}

int avframe_to_png(AVFrame *srcFrame, char *filename) {
	const AVCodec *mjpegCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	if (!mjpegCodec) {
		printf("could not find png codec\n");
		return -1;
	}

	AVCodecContext *pngAVCC = avcodec_alloc_context3(mjpegCodec);
	if (!pngAVCC) {
		printf("could not allocate png codec context\n");
		return -1;
	}

	pngAVCC->time_base = AVRational{ 1, 25 };
	pngAVCC->width = srcFrame->width;
	pngAVCC->height = srcFrame->height;
	pngAVCC->coded_width = srcFrame->width;
	pngAVCC->coded_height = srcFrame->height;
	pngAVCC->pix_fmt = AV_PIX_FMT_GRAY8;

	printf("height = %d\n", srcFrame->height);
	printf("width = %d\n", srcFrame->width);

	printf("before opening the png context, jpegAVCC->pix_fmt=%d\n", pngAVCC->pix_fmt);
	if (avcodec_open2(pngAVCC, mjpegCodec, NULL) < 0) {
		printf("could not open png codec context.\n");
		exit(1);
		return -1;
	}
	printf("end opened the png context, pngAVCC->pix_fmt=%d\n", pngAVCC->pix_fmt);

	AVPacket *packet = av_packet_alloc();
	if (!packet) {
		return false;
	}

	FILE *jpegFile;
	printf("before writing the picture\n");
#ifndef _WIN32
	jpegFile = fopen(filename, "wb");
#else
	fopen_s(&jpegFile, filename, "wb");
#endif

	int got_picture = 0;
	int ret = avcodec_send_frame(pngAVCC, srcFrame);
	if (ret < 0) {
		printf("Encode Error.\n");
		return -1;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(pngAVCC, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			break;
		}
		fwrite(packet->data, 1, (size_t)packet->size, jpegFile);
		av_packet_unref(packet);
	}

	fclose(jpegFile);

	av_packet_free(&packet);
	avcodec_close(pngAVCC);
	return 0;
}


int avframe_to_bmp(AVFrame *srcFrame, char *filename) {
	const AVCodec *mjpegCodec = avcodec_find_encoder(AV_CODEC_ID_BMP);
	if (!mjpegCodec) {
		printf("could not find bmp codec\n");
		return -1;
	}

	AVCodecContext *bmpAVCC = avcodec_alloc_context3(mjpegCodec);
	if (!bmpAVCC) {
		printf("could not allocate bmp codec context\n");
		return -1;
	}

	bmpAVCC->time_base = AVRational{ 1, 25 };
	bmpAVCC->width = srcFrame->width;
	bmpAVCC->height = srcFrame->height;
	bmpAVCC->coded_width = srcFrame->width;
	bmpAVCC->coded_height = srcFrame->height;
	bmpAVCC->pix_fmt = AV_PIX_FMT_GRAY8;

	printf("height = %d\n", srcFrame->height);
	printf("width = %d\n", srcFrame->width);

	printf("before opening the bmp context, jpegAVCC->pix_fmt=%d\n", bmpAVCC->pix_fmt);
	if (avcodec_open2(bmpAVCC, mjpegCodec, NULL) < 0) {
		printf("could not open bmp codec context.\n");
		exit(1);
		return -1;
	}
	printf("end opened the bmp context, bmpAVCC->pix_fmt=%d\n", bmpAVCC->pix_fmt);

	AVPacket *packet = av_packet_alloc();
	if (!packet) {
		return false;
	}

	FILE *jpegFile;
	printf("before writing the picture\n");
#ifndef _WIN32
	jpegFile = fopen(filename, "wb");
#else
	fopen_s(&jpegFile, filename, "wb");
#endif

	int got_picture = 0;
	int ret = avcodec_send_frame(bmpAVCC, srcFrame);
	if (ret < 0) {
		printf("Encode Error.\n");
		return -1;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(bmpAVCC, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			break;
		}
		fwrite(packet->data, 1, (size_t)packet->size, jpegFile);
		av_packet_unref(packet);
	}

	fclose(jpegFile);

	av_packet_free(&packet);
	avcodec_close(bmpAVCC);
	return 0;
}

static AVFrame * FrameToRGB(AVFrame *frame)
{
	struct SwsContext *img_convert_ctx; // 图片处理的上下文
	int width = frame->width;
	int height = frame->height;
	AVPixelFormat pix_fmt = AV_PIX_FMT_YUVJ420P;
	//AVPixelFormat pix_fmt = AV_PIX_FMT_YUVJ420P;
	AVPixelFormat rgb_fmt = AV_PIX_FMT_BGR24;
	img_convert_ctx = sws_getContext(width, height, pix_fmt, width, height, rgb_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	int numBytes = av_image_get_buffer_size(rgb_fmt, width, height, 1);
	uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	AVFrame *pFrameRGB = av_frame_alloc();

	//pFrameRGB,是将RGB转换成YUV之后，存储YUV一帧图像用的。 out_buffer是YUV的存储空间。
	//下面两个宽高，是图像的宽高（由于输出的宽高=输入的宽高，所以整个例子中只有一组宽高）。
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, rgb_fmt, width, height, 1);

	//反转图像 ，否则生成的图像是上下调到的
	frame->data[0] += frame->linesize[0] * (height - 1);
	frame->linesize[0] *= -1;
	frame->data[1] += frame->linesize[1] * (height / 2 - 1);
	frame->linesize[1] *= -1;
	frame->data[2] += frame->linesize[2] * (height / 2 - 1);
	frame->linesize[2] *= -1;
	
	/*
	进行转换
	frame->data和pFrameRGB->data分别指输入和输出的buf
	frame->linesize和pFrameRGB->linesize可以看成是输入和输出的每列的byte数
	第4个参数是指第一列要处理的位置
	第5个参数是source slice 的高度
	*/
	sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, height, pFrameRGB->data, pFrameRGB->linesize);

	return pFrameRGB;
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, 	const char *filename)
{
	char buf[1024] = {0};
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);

	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);

		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		/* the picture is allocated by the decoder. no need to free it */
// 		snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
// 		pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);

		{
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s-%d.jpeg", filename, dec_ctx->frame_number);
			avframe_to_jpeg(frame, buf);
		}

		{
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s-%d.png", filename, dec_ctx->frame_number);
			avframe_to_png(frame, buf);
		}

		{
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s-%d-avctx.bmp", filename, dec_ctx->frame_number);
			avframe_to_bmp(frame, buf);
		}

		{// 保持到bmp
			AVFrame * pFrameRGB = FrameToRGB(frame);
			pFrameRGB->width = frame->width;
			pFrameRGB->height = frame->height;
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s-%d.bmp", filename, dec_ctx->frame_number);
			SaveAsBMP(pFrameRGB, frame->width, frame->height, buf, 24);
			av_freep(&pFrameRGB[0]);
			av_free(pFrameRGB);
		}

		dec_ctx->frame_number++;

		exit(1); //测试 转一张图片
	}
}

static void printlog(void *ptr, int level, const char *fmt, va_list vl)
{
	va_list vl2;
	char line[1024];
	static int print_prefix = 1;

	va_copy(vl2, vl);
	av_log_default_callback(ptr, level, fmt, vl);
	av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
	va_end(vl2);

	printf(line);
}

int main(int argc, char **argv)
{
	const char *filename, *outfilename;
	const AVCodec *codec;
	AVCodecParserContext *parser;
	AVCodecContext *c = NULL;
	FILE *f;
	AVFrame *frame;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t *data;
	size_t data_size;
	int ret;
	AVPacket *pkt;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	filename = argv[1];
	outfilename = argv[2];

	av_log_set_flags(AV_LOG_PRINT_LEVEL);
	av_log_set_level(AV_LOG_TRACE);
// 	av_log_set_callback(printlog);

	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	/* find the MPEG-1 video decoder */
	//codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
	/* find the H264 video decoder */
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	/* For some codecs, such as msmpeg4 and mpeg4, width and height
	   MUST be initialized there because this information is not
	   available in the bitstream. */

	c->pix_fmt = AV_PIX_FMT_YUVJ420P;
	printf("before opening the H264 context, c->pix_fmt=%d\n", c->pix_fmt);
	   /* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}
	printf("end opened the H264 context, c->pix_fmt=%d\n", c->pix_fmt);

	c->frame_number = 0;
#ifndef _WIN32
	f = fopen(filename, "rb");
#else
	fopen_s(&f, filename, "rb");
#endif
	
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	while (!feof(f)) {
		/* read raw data from the input file */
		data_size = fread(inbuf, 1, INBUF_SIZE, f);
		if (!data_size)
			break;

		/* use the parser to split the data into frames */
		data = inbuf;
		while (data_size > 0) {
			ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
				data, (int)data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);

			}
			data += ret;
			data_size -= ret;

			if (pkt->size)
				decode(c, frame, pkt, outfilename);
		}
	}

	/* flush the decoder */
	decode(c, frame, NULL, outfilename);

	fclose(f);

	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}
