#pragma once
// ffmpeg_video2picture.cpp : ���ļ����� "main" ����������ִ�н��ڴ˴���ʼ��������
//
/*����ƵתͼƬ*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// λͼ�ļ�ͷ��bitmap-file header��������ͼ�����͡�ͼ���С��ͼ�����ݴ�ŵ�ַ����������δʹ�õ��ֶ�
typedef struct tagBITMAPFILEHEADER {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

// λͼ��Ϣͷ��bitmap-information header��������λͼ��Ϣͷ�Ĵ�С��ͼ��Ŀ�ߡ�ͼ���ɫ�ѹ��˵��ͼ�����ݵĴ�С������һЩ������
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

// ͼƬת��������
void saveBMP(struct SwsContext *img_convert_ctx, AVFrame *frame, char *filename)
{
	//1 �Ƚ���ת��,  YUV420=>RGB24:
	int w = frame->width;
	int h = frame->height;


	//int numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, w, h);
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, w, h, 1); // 1��1�ֽڶ��룬ʵ�ʴ�С��4��4�ֽڶ��룬ʵ�ʴ�С��4
	uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));


	AVFrame *pFrameRGB = av_frame_alloc();
	/* buffer is going to be written to rawvideo file, no alignment */
	//avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_BGR24, w, h);

	//pFrameRGB,�ǽ�RGBת����YUV֮�󣬴洢YUVһ֡ͼ���õġ� out_buffer��YUV�Ĵ洢�ռ䡣
	//����������ߣ���ͼ��Ŀ�ߣ���������Ŀ��=����Ŀ�ߣ���������������ֻ��һ���ߣ���
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_BGR24, w, h, 0);

	/*
	����ת��
	frame->data��pFrameRGB->data�ֱ�ָ����������buf
	frame->linesize��pFrameRGB->linesize���Կ���������������ÿ�е�byte��
	��4��������ָ��һ��Ҫ�����λ��
	��5��������source slice �ĸ߶�
	*/
	sws_scale(img_convert_ctx, frame->data, frame->linesize,
		0, h, pFrameRGB->data, pFrameRGB->linesize);

	//2 ���� BITMAPINFOHEADER
	BITMAPINFOHEADER header;
	header.biSize = sizeof(BITMAPINFOHEADER);


	header.biWidth = w;
	header.biHeight = h * (-1);
	header.biBitCount = 24;
	header.biCompression = 0;
	header.biSizeImage = 0;
	header.biClrImportant = 0;
	header.biClrUsed = 0;
	header.biXPelsPerMeter = 0;
	header.biYPelsPerMeter = 0;
	header.biPlanes = 1;

	//3 �����ļ�ͷ
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


	//�ͷ���Դ
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

// �����ݽ��н���
static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
	struct SwsContext *img_convert_ctx, AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
	//int len;
	int got_frame, len_packet;
	char buf[1024];

	// �����ݽ��н��룬avctx�Ǳ�����������ģ�������֡�����frame��ͨ��got_frame��ȡ������֡��pkt�������
	// len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
	// if (len < 0) {
	len_packet = avcodec_send_packet(avctx, pkt);
	if (len_packet < 0) {
		fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
		return len_packet;
	}
	got_frame = avcodec_receive_frame(avctx, frame);

	// ����ɹ�
	if (got_frame) {
		printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
		fflush(stdout);

		/* the picture is allocated by the decoder, no need to free it */
		snprintf(buf, sizeof(buf), "%s-%d.bmp", outfilename, *frame_count);

		// ����������֡����rgbͼƬ
		saveBMP(img_convert_ctx, frame, buf);

		(*frame_count)++;
	}
	return 0;
}


int main(int argc, char **argv)
{
	int ret;

	//FILE *f;

	const char *filename, *outfilename; // ���롢����ļ�·��

	AVFormatContext *fmt_ctx = NULL; // ��ʽ�����Ļ���(�����ļ��������Ļ���)

	const AVCodec *codec; // �������
	AVCodecContext *c = NULL; // ������������Ļ���(����ļ��������Ļ���)

	AVStream *st = NULL; // ��ý���ļ��е���
	int stream_index; // ����index

	int frame_count;
	AVFrame *frame; // ԭʼ֡

	struct SwsContext *img_convert_ctx; // ͼƬ�����������

	//uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	AVPacket avpkt;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	filename = argv[1];
	outfilename = argv[2];

	// �������ļ���������ʽ�����Ļ���
	if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", filename);
		exit(1);
	}

	// ������ý���ļ��е�����Ϣ
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	// ��ӡ�����ļ�����ϸ��Ϣ
	av_dump_format(fmt_ctx, 0, filename, 0);

	av_init_packet(&avpkt);

	// ������Ƶ��������ֵret������������
	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO), filename);
		return ret;
	}

	// ���������Ż�ȡ��
	stream_index = ret;
	st = fmt_ctx->streams[stream_index];

	// �������б�������еĽ�����ID���ҽ�����
	codec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!codec) {
		fprintf(stderr, "Failed to find %s codec\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return AVERROR(EINVAL);
	}

	// ��������ļ��������Ļ���
	c = avcodec_alloc_context3(NULL);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	// ����Ƶ���ı�������ֱ�ӿ�������������Ļ�����
	if ((ret = avcodec_parameters_to_context(c, st->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return ret;
	}


	// �򿪽�����
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	/*
		��ʼ��ͼƬת�������Ļ���
		sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
						int dstW, int dstH, enum AVPixelFormat dstFormat,
							   int flags, SwsFilter *srcFilter,
							   SwsFilter *dstFilter, const double *param);
		srcW/srcH/srcFormat�ֱ�Ϊԭʼͼ�����ݵĿ�ߺ��������ͣ��������ͱ���AV_PIX_FMT_YUV420��PAV_PIX_FMT_RGB24
		dstW/dstH/dstFormat�ֱ�Ϊ���ͼ�����ݵĿ�ߺ���������
		flags��scale�㷨���ࣻSWS_BICUBIC��SWS_BICUBLIN��SWS_POINT��SWS_SINC��
		���3���������ùܣ�����ΪNULL
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
	// ѭ���Ӷ�ý���ļ��ж�ȡһ֡һ֡������
	while (av_read_frame(fmt_ctx, &avpkt) >= 0) {

		// �����ȡ�������ݰ���stream_index����Ƶ����stream_indexһ�¾ͽ��н���
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

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

// ����ʹ�ü���: 
//   1. ʹ�ý��������Դ�������������/�����ļ�
//   2. ʹ���Ŷ���Դ�������������ӵ�Դ�������
//   3. ʹ��������ڲ鿴���������������Ϣ
//   4. ʹ�ô����б��ڲ鿴����
//   5. ת������Ŀ��>���������Դ����µĴ����ļ�����ת������Ŀ��>�����������Խ����д����ļ���ӵ���Ŀ
//   6. ��������Ҫ�ٴδ򿪴���Ŀ����ת�����ļ���>���򿪡�>����Ŀ����ѡ�� .sln �ļ�
