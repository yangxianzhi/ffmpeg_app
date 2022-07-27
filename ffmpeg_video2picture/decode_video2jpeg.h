#pragma once
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS


#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

#define MAX_PATH 256
/**
 * ��AVFrame(YUV420��ʽ)����ΪJPEG��ʽ��ͼƬ
 *
 * @param width YUV420�Ŀ�
 * @param height YUV42�ĸ�
 *
 */
int MyWriteJPEG(AVFrame* pFrame, int width, int height, int iIndex)
{
	// ����ļ�·��
	char out_file[MAX_PATH] = { 0 };
	sprintf_s(out_file, sizeof(out_file), "%s%d.jpg", "E:/temp/", iIndex);

	// ����AVFormatContext����
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	// ��������ļ���ʽ
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	// ��������ʼ��һ���͸�url��ص�AVIOContext
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// ����һ����stream
	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	// ���ø�stream����Ϣ
	AVCodecContext* pCodecCtx = pAVStream->codec;

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

	// ���ҽ�����
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		printf("Codec not found.");
		return -1;
	}
	// ����pCodecCtx�Ľ�����ΪpCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	// ��AVPacket�����㹻��Ŀռ�
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

int Work_Save2JPG()
{
	int videoStream = -1;
	AVCodecContext *pCodecCtx;
	AVFormatContext *pFormatCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameRGB;
	const char *filename = "bigbuckbunny_480x272.h264";
	AVPacket packet;
	int frameFinished;
	int PictureSize;
	uint8_t *outBuff;

	//ע��������
	//av_register_all();
	//// ��ʼ������ģ��
	//avformat_network_init();
	// ����AVFormatContext
	pFormatCtx = avformat_alloc_context();

	//����Ƶ�ļ�
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
		printf("av open input file failed!\n");
		exit(1);
	}

	//��ȡ����Ϣ
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("av find stream info failed!\n");
		exit(1);
	}
	//��ȡ��Ƶ��
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		printf("find video stream failed!\n");
		exit(1);
	}

	// Ѱ�ҽ�����
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("avcode find decoder failed!\n");
		exit(1);
	}

	//�򿪽�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("avcode open failed!\n");
		exit(1);
	}

	//Ϊÿ֡ͼ������ڴ�
	pFrame = avcodec_alloc_frame();
	pFrameRGB = avcodec_alloc_frame();
	if ((pFrame == NULL) || (pFrameRGB == NULL)) {
		printf("avcodec alloc frame failed!\n");
		exit(1);
	}

	// ȷ��ͼƬ�ߴ�
	PictureSize = avpicture_get_size(AV_PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
	outBuff = (uint8_t*)av_malloc(PictureSize);
	if (outBuff == NULL) {
		printf("av malloc failed!\n");
		exit(1);
	}
	avpicture_fill((AVPicture *)pFrameRGB, outBuff, AV_PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);

	//����ͼ��ת��������
// 	SwsContext *pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
// 		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUVJ420P,
// 		SWS_BICUBIC, NULL, NULL, NULL);

	int i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			if (frameFinished) {
				// ����Ϊjpegʱ����Ҫ��תͼ��
				static bool b1 = true;
				if (b1) {
					MyWriteJPEG(pFrame, pCodecCtx->width, pCodecCtx->height, i++);
					b1 = false;
				}
			}
		}
		else {
			int a = 2;
			int b = a;
		}

		av_free_packet(&packet);
	}

//	sws_freeContext(pSwsCtx);

	av_free(pFrame);
	av_free(pFrameRGB);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

int main(int argc, char **argv)
{
	Work_Save2JPG();
}