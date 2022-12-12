#ifndef FFMPEG_PLAYER_H_
#define FFMPEG_PLAYER_H_
#include "common.h"
#include <QtCore/QObject>
#include <QtCore/QEvent>

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
#define SUBTITLE_LENGTH 512

typedef struct MyAVPacketList {
	AVPacket pkt;
	struct MyAVPacketList *next;
	int serial;
} MyAVPacketList;

typedef struct PacketQueue {
	MyAVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int64_t duration;
	int abort_request;
	int serial;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
	int frame_size;
	int bytes_per_sec;
} AudioParams;

typedef struct Clock {
	double pts;           /* clock base */
	double pts_drift;     /* clock base minus time at which we updated the clock */
	double last_updated;
	double speed;
	int serial;           /* clock is based on a packet with this serial */
	int paused;
	int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
	AVFrame *frame;
	AVSubtitle sub;
	int serial;
	double pts;           /* presentation timestamp for the frame */
	double duration;      /* estimated duration of the frame */
	int64_t pos;          /* byte position of the frame in the input file */
	int width;
	int height;
	int format;
	AVRational sar;
	int uploaded;
	int flip_v;
} Frame;

typedef struct FrameQueue {
	Frame queue[FRAME_QUEUE_SIZE];
	int rindex;
	int windex;
	int size;
	int max_size;
	int keep_last;
	int rindex_shown;
	SDL_mutex *mutex;
	SDL_cond *cond;
	PacketQueue *pktq;
} FrameQueue;

enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
	AVPacket pkt;
	PacketQueue *queue;
	AVCodecContext *avctx;
	int pkt_serial;
	int finished;
	int packet_pending;
	SDL_cond *empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;
	int64_t next_pts;
	AVRational next_pts_tb;
	SDL_Thread *decoder_tid;
} Decoder;

typedef struct VideoState {
	SDL_Thread *read_tid;
	SDL_Thread *video_refresh_tid;
	AVInputFormat *iformat;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int queue_attachments_req;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	AVFormatContext *ic;
	int realtime;

	Clock audclk;
	Clock vidclk;
	Clock extclk;

	FrameQueue pictq;
	FrameQueue subpq;
	FrameQueue sampq;

	Decoder auddec;
	Decoder viddec;
	Decoder subdec;

	int audio_stream;

	int av_sync_type;

	double audio_clock;
	int audio_clock_serial;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	unsigned int audio_buf1_size;
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	int audio_volume;
	int muted;
	struct AudioParams audio_src;
#if CONFIG_AVFILTER
	struct AudioParams audio_filter_src;
#endif
	struct AudioParams audio_tgt;
	struct SwrContext *swr_ctx;
	int frame_drops_early;
	int frame_drops_late;

	enum ShowMode {
		SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
	} show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext *rdft;
	int rdft_bits;
	FFTSample *rdft_data;
	int xpos;
	double last_vis_time;
	SDL_Texture *vis_texture;
	SDL_Texture *sub_texture;
	SDL_Texture *vid_texture;

	int subtitle_stream;
	AVStream *subtitle_st;
	PacketQueue subtitleq;

	double frame_timer;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int video_stream;
	AVStream *video_st;
	PacketQueue videoq;
	double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
	struct SwsContext *img_convert_ctx;
	struct SwsContext *sub_convert_ctx;
	int eof;

	char *filename;
	int width, height, xleft, ytop;
	int step;

#if CONFIG_AVFILTER
	int vfilter_idx;
	AVFilterContext *in_video_filter;   // the first filter in the video chain
	AVFilterContext *out_video_filter;  // the last filter in the video chain
	AVFilterContext *in_audio_filter;   // the first filter in the audio chain
	AVFilterContext *out_audio_filter;  // the last filter in the audio chain
	AVFilterGraph *agraph;              // audio filter graph
#endif

	int last_video_stream, last_audio_stream, last_subtitle_stream;
	SDL_cond *continue_read_thread;
} VideoState;

class CustomEvent : public QEvent
{
public:
	CustomEvent(QEvent::Type type);
	~CustomEvent();

	int64_t code;        /**< User defined event code */
	void *data1;        /**< User defined data pointer */
	void *data2;        /**< User defined data pointer */

};

typedef void(*log_callback)(void*, int, const char*, va_list);

class ffmpeg_player:public QObject
{
public:
	enum LogLevel
	{
		LOG_QUIET = -1,
		LOG_PANIC,
		LOG_FATAL,
		LOG_ERROR,
		LOG_WARNING,
		LOG_INFO,
		LOG_VERBOSE,
		LOG_DEBUG,
		LOG_TRACE
	};

	Q_OBJECT

public:
	ffmpeg_player();
	~ffmpeg_player();

public:
	void set_log_callback(log_callback cb);
	void init(void* hwnd);
	
	/*set timeout (in microseconds) of socket TCP I/O operations*/
	void set_rtsp_connect_timeout(const char* timeout = "15000000");
	void set_rtsp_max_delay(const char* delay = "500000");
	/*Underlying protocol send/receive buffer size*/
	void set_rtsp_buffer_size(const char* size = "102400");
	/*set RTSP transport protocols: udp, tcp ,http, udp_multicast*/
	void set_rtsp_transport(const char* transport = "udp");
	void set_subtitle_font(const char* font_file_path);
	/*font color value is 000000~FFFFFF*/
	void set_subtitle_font_color(const char* font_color = "FFFFFF");
	/*alpha value is 0.0~1.0*/
	void set_subtitle_font_color_alpha(double alpha = 1);
	void set_subtitle_font_size(unsigned int size = 30);
	void set_log_level(LogLevel level);
	void set_no_data_time_duration(int sec);
	void set_sdl_window_size(int w, int h);
	void set_window_hiden_flag(int flag);
	void set_video_stretch(int flag);

	bool start(const char *url);
	void stop();
	void set_video_subtitle(const char* subtitle);

protected:
	virtual void customEvent(QEvent *event);

private:
	void init_sdl(void* hwnd);
	void init_dynload();
	void init_opts(void);
	void uninit_opts(void);
	void do_exit(VideoState *is);
	void print_all_libs_info(int flags, int level);
	void print_program_info(int flags, int level);
	void print_buildconf(int flags, int level);
	void stream_close(VideoState *is);
	void packet_queue_destroy(PacketQueue *q);
	void frame_queue_destory(FrameQueue *f);
	void decoder_abort(Decoder *d, FrameQueue *fq);
	void decoder_destroy(Decoder *d);
	void packet_queue_flush(PacketQueue *q);
	void frame_queue_unref_item(Frame *vp);
	void packet_queue_abort(PacketQueue *q);
	void frame_queue_signal(FrameQueue *f);
	int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
	int packet_queue_init(PacketQueue *q);
	void init_clock(Clock *c, int *queue_serial);
	void set_clock(Clock *c, double pts, int serial);
	double get_clock(Clock *c);
	void set_clock_at(Clock *c, double pts, int serial, double time);
	VideoState *stream_open(const char *filename, AVInputFormat *iformat);
	void print_error(const char *filename, int err);
	AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts);
	AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id, AVFormatContext *s, AVStream *st, const AVCodec *codec);
	int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);
	int is_realtime(AVFormatContext *s);
	void calculate_display_rect(SDL_Rect *rect,	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
		int pic_width, int pic_height, AVRational pic_sar);
	void set_default_window_size(int width, int height, AVRational sar);
	int stream_component_open(ffmpeg_player *pPlayer, int stream_index);
	void stream_component_close(VideoState *is, int stream_index);
	int packet_queue_put(PacketQueue *q, AVPacket *pkt);
	int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
	int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
	int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);
	void step_to_next_frame(VideoState *is);
	void stream_toggle_pause(VideoState *is);
	void toggle_pause(VideoState *is);
	void toggle_mute(VideoState *is);
	int frame_queue_nb_remaining(FrameQueue *f);
	void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes);
	double get_rotation(AVStream *st);
	int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels,
		int wanted_sample_rate, struct AudioParams *audio_hw_params);
	void frame_queue_next(FrameQueue *f);
	Frame *frame_queue_peek_readable(FrameQueue *f);
	int audio_decode_frame(VideoState *is);
	void update_sample_display(VideoState *is, short *samples, int samples_size);
	void sync_clock_to_slave(Clock *c, Clock *slave);
	int synchronize_audio(VideoState *is, int nb_samples);
	int get_master_sync_type(VideoState *is);
	double get_master_clock(VideoState *is);
	void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);
	int decoder_start(Decoder *d, int(*fn)(void *), void *arg);
	void packet_queue_start(PacketQueue *q);
	int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub);
	int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
	inline int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
	inline int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
		enum AVSampleFormat fmt2, int64_t channel_count2);
	Frame *frame_queue_peek_writable(FrameQueue *f);
	void frame_queue_push(FrameQueue *f);
	int get_video_frame(VideoState *is, AVFrame *frame);
	int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
	void toggle_full_screen(VideoState *is);
	void update_volume(VideoState *is, int sign, double step);
	void stream_cycle_channel(int codec_type);
	void toggle_audio_display(VideoState *is);
	void seek_chapter(VideoState *is, int incr);
	int64_t frame_queue_last_pos(FrameQueue *f);
	void check_external_clock_speed(VideoState *is);
	void set_clock_speed(Clock *c, double speed);
	void video_display(VideoState *is);
	Frame *frame_queue_peek_next(FrameQueue *f);
	Frame *frame_queue_peek_last(FrameQueue *f);
	Frame *frame_queue_peek(FrameQueue *f);
	double vp_duration(VideoState *is, Frame *vp, Frame *nextvp);
	double compute_target_delay(double delay, VideoState *is);
	void update_video_pts(VideoState *is, double pts, int64_t pos, int serial);
	int video_open(VideoState *is);
	void video_audio_display(VideoState *s);
	void video_image_display(VideoState *is);
	inline int compute_mod(int a, int b);
	inline void fill_rectangle(int x, int y, int w, int h);
	int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture);
	int upload_texture(SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx);
	void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode);
	void renderer_background();
	//void refresh_loop_wait_event(SDL_Event *event);
	void stop_impl(int code);

#ifdef CONFIG_AVFILTER
	int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
		AVFilterContext *source_ctx, AVFilterContext *sink_ctx);
	int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame);
	int configure_audio_filters(VideoState *is, const char *afilters, int force_output_format);
#endif /* CONFIG_AVFILTER */

	static int decode_interrupt_cb(void *ctx);
	static void sigterm_handler(int sig);
	static int lockmgr(void **mtx, enum AVLockOp op);
	static int read_thread(void *arg);
	static void sdl_audio_callback(void *opaque, Uint8 *stream, int len);
	static int audio_thread(void *arg);
	static int video_thread(void *arg);
	static int subtitle_thread(void *arg);
	static int video_refresh_thread(void *arg);

protected Q_SLOTS :
	double video_refresh(void *opaque, double remaining_time);

signals:
	void player_stoped(char* file_name, int stop_code);
	void player_playing(char* file_name);
	
private:
	AVDictionary *sws_dict;
	AVDictionary *swr_opts;
	AVDictionary *format_opts;
	AVDictionary *codec_opts;
	AVDictionary *resample_opts;
	SDL_Window   *window;
	SDL_Renderer *renderer;
	SDL_RendererInfo renderer_info;
	SDL_AudioDeviceID audio_dev;
	const char **vfilters_list;
	const char *window_title;
	const char* wanted_stream_spec[AVMEDIA_TYPE_NB];

	/*set audio filters*/
	char *afilters;

	/*force audio decoder*/
	const char *audio_codec_name;

	/*force subtitle decoder*/
	const char *subtitle_codec_name;

	/*force video decoder*/
	const char *video_codec_name;

	/*borderless window*/
	int borderless;

	int show_status;
	int genpts;
	int find_stream_info;
	int seek_by_bytes;
	AVPacket flush_pkt;
	int default_width;
	int default_height;
	int64_t start_time;
	int audio_disable;
	int video_disable;
	int subtitle_disable;
	int infinite_buffer;

	/*set number of times the playback shall be looped*/
	int loop;

	/*"play  \"duration\" seconds of audio/video"*/
	int64_t duration;

	/*exit at the end*/
	int autoexit;
	int lowres;

	/*non spec compliant optimizations*/
	int fast;

	/*automatically rotate video*/
	int autorotate;

	/*let decoder reorder pts 0=off 1=on -1=auto*/
	int decoder_reorder_pts;

	/*drop frames when cpu is too slow*/
	int framedrop;
	
	/*force full screen*/
	int is_full_screen;

	/*force max window*/
	int is_max_window;

	int nb_vfilters;

	/*allow double click mouse / sdl_key_f  to full screen*/
	int allow_full_screen;

	int screen_width;
	int screen_height;

	/*disable graphical display*/
	int display_disable;
	double rdftspeed;

	VideoState::ShowMode show_mode;
	VideoState *m_pVideoState;

	/*readed last packet time*/
	int64_t last_packet_readed_time;
	int64_t no_data_time_duration;
	int is_window_hiden;
	int is_video_stretch;

#ifdef CONFIG_AVFILTER
	char font[256];
	char fontcolor[16];
	char boxcolor[16];
	char subtitle[SUBTITLE_LENGTH];
	double fontcolor_alpha;
	double boxcolor_alpha;
	unsigned int fontsize;
	int shadowy;
	int box;
	int x;
	int y;
	int is_subtitle_change;
#endif
};

#endif
