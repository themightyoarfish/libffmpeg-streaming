#ifndef AVUTILS_HPP_L0JIDQTW
#define AVUTILS_HPP_L0JIDQTW

#include <chrono>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

constexpr int KB = 1024;

namespace avutils {

static std::string av_strerror(int errnum) {
  std::vector<char> v(KB);
  ::av_strerror(errnum, v.data(), v.size());
  return std::string(v.begin(), v.end());
}

static int initialize_avformat_context(AVFormatContext *&fctx,
                                       const char *format_name) {
  return avformat_alloc_output_context2(&fctx, nullptr, format_name, nullptr);
}

static void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx,
                             double width, double height, int fps,
                             int target_bitrate = 0, int gop_size = 12) {
  const AVRational dst_fps = {fps, 1};

  codec_ctx->codec_tag = 0;
  if (target_bitrate > 0) {
    codec_ctx->bit_rate = target_bitrate;
  }
  // does nothing, unfortunately
  codec_ctx->thread_count = 1;
  codec_ctx->codec_id = AV_CODEC_ID_H264;
  codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_ctx->width = width;
  codec_ctx->height = height;
  codec_ctx->gop_size = gop_size;
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx->framerate = dst_fps;
  codec_ctx->time_base = av_inv_q(dst_fps);
}

static int initialize_codec_stream(AVStream *&stream,
                                   AVCodecContext *&codec_ctx,
                                   AVCodec *&codec) {
  int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
  if (ret < 0) {
    /* std::cout << "Could not initialize stream codec parameters!" <<
     * std::endl; */
    /* exit(1); */
    return ret;
  }

  AVDictionary *codec_options = nullptr;
  av_dict_set(&codec_options, "profile", "high", 0);
  av_dict_set(&codec_options, "preset", "ultrafast", 0);
  av_dict_set(&codec_options, "tune", "zerolatency", 0);

  // open video encoder
  ret = avcodec_open2(codec_ctx, codec, &codec_options);
  return ret;
}

static SwsContext *initialize_sample_scaler(AVCodecContext *codec_ctx,
                                            double width, double height) {
  SwsContext *swsctx = sws_getContext(width, height, AV_PIX_FMT_BGR24, width,
                                      height, codec_ctx->pix_fmt, SWS_BICUBIC,
                                      nullptr, nullptr, nullptr);
  return swsctx;
}

static AVFrame *allocate_frame_buffer(AVCodecContext *codec_ctx, double width,
                                      double height) {
  AVFrame *frame = av_frame_alloc();

  std::uint8_t *framebuf = new uint8_t[av_image_get_buffer_size(
      codec_ctx->pix_fmt, width, height, 1)];
  av_image_fill_arrays(frame->data, frame->linesize, framebuf,
                       codec_ctx->pix_fmt, width, height, 1);
  frame->width = width;
  frame->height = height;
  frame->format = static_cast<int>(codec_ctx->pix_fmt);

  return frame;
}

static int write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx,
                       AVFrame *frame) {
  AVPacket pkt = {0};
  /* av_init_packet(&pkt); */

  int ret = avcodec_send_frame(codec_ctx, frame);
  if (ret < 0) {
    std::cout << "Error sending frame to codec context!" << std::endl;
    return ret;
  }

  ret = avcodec_receive_packet(codec_ctx, &pkt);
  if (ret < 0) {
    std::cout << "Error receiving packet from codec context!" << std::endl;
    return ret;
  }

  av_interleaved_write_frame(fmt_ctx, &pkt);
  return ret;
}

static void generatePattern(cv::Mat &image, unsigned char i) {
  image.setTo(cv::Scalar(255, 255, 255));
  float perc_height = 1.0 * i / 255;
  float perc_width = 1.0 * i / 255;
  image.row(perc_height * image.rows).setTo(cv::Scalar(0, 0, 0));
  image.col(perc_width * image.cols).setTo(cv::Scalar(0, 0, 0));
}
} // namespace avutils
#endif /* end of include guard: AVUTILS_HPP_L0JIDQTW */