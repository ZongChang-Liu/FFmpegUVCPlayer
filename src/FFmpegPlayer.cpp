//
// Created by liu_zongchang on 2025/1/13 1:52.
// Email 1439797751@qq.com
// 
//

#include <QImage>
#include "FFmpegPlayer.h"
#include "Microscope_Utils_Log.h"

FFmpegPlayer::FFmpegPlayer(QObject* parent) : QThread(parent)
{
    av_log_set_level(AV_LOG_INFO);
    avformat_network_init();
    avdevice_register_all();
}

FFmpegPlayer::~FFmpegPlayer()
{    avformat_network_deinit();
}

int FFmpegPlayer::openDevice(const QString& path, const int width, const int height, const int fps, const QString& format)
{
    close();
    if (path.isEmpty() || width <= 0 || height <= 0 || fps <= 0) {
        LOG_ERROR("input params is invalid");
        return -1;
    }
    AVDictionary *options = nullptr;

    const std::string size = std::to_string(width) + "x" + std::to_string(height);
    int ret = av_dict_set(&options, "video_size", size.c_str(), 0);
    if (ret < 0) {
        LOG_ERROR("set video size failed {}\n", getErrorString(ret));
       return -1;
    }
    LOG_INFO("set video size {}\n", size.c_str());


    ret = av_dict_set_int(&options, "framerate", fps, 0);
    if (ret < 0) {
        LOG_ERROR("set frameRate failed: {}\n", getErrorString(ret));
        return -1;
    }
    LOG_INFO("set video frameRate {}\n", fps);

    ret = av_dict_set(&options, "input_format", format.toStdString().c_str(), 0);
    if (ret < 0) {
        LOG_ERROR("set video format failed {}\n", getErrorString(ret));
        return -1;
    }
    LOG_INFO("set video format {}\n", format.toStdString().c_str());

    const std::string deviceUrl = "video=" + path.toStdString();
    return open(deviceUrl, options);
}

int FFmpegPlayer::open(const std::string& url, AVDictionary* options)
{
    if (url.empty()) {
        LOG_ERROR("input params is invalid");
        return -1;
    }

    m_srcFormatContext = avformat_alloc_context();
    if (m_srcFormatContext == nullptr) {
        LOG_ERROR("alloc format context failed: {}\n", getErrorString(AVERROR(ENOMEM)));
        return -1;
    }

    int ret;
    if (url.find("rtsp") != std::string::npos) {
        ret = av_dict_set(&options, "rtsp_transport", "tcp", 0);
        if (ret < 0) {
            LOG_ERROR("set rtsp transport failed: {}\n", getErrorString(ret));
            return -1;
        }
    } else if (url.find("video") != std::string::npos) {
#ifdef _WIN32
        ret = avformat_open_input(&m_srcFormatContext, url.c_str(), av_find_input_format("dshow"), &options);
#elif defined(__linux__)
        ret = avformat_open_input(&_srcFormatContext, url.c_str(), av_find_input_format("v4l2"), &options);
#endif
        if (ret < 0) {
            LOG_ERROR("open device {} failed: {}\n", url.c_str(), getErrorString(ret));
            return -1;
        }
        LOG_INFO("open device {} success\n", url.c_str());
    } else {
        ret = avformat_open_input(&m_srcFormatContext, url.c_str(), nullptr, nullptr);
        if (ret < 0) {
            LOG_ERROR("open url {} failed: {}\n", url.c_str(), getErrorString(ret));
            return -1;
        }
        LOG_INFO("open url {} success\n", url.c_str());
    }


    ret = avformat_find_stream_info(m_srcFormatContext, nullptr);
    if (ret < 0) {
        LOG_ERROR("find stream info failed: {}\n", getErrorString(ret));
        return -1;
    }
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_srcFormatContext->nb_streams; ++i) {
        if (m_srcFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = static_cast<int>(i);
            LOG_INFO("find video stream index: {}\n", i);
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        LOG_ERROR("not find available stream\n");
        return -1;
    }

    av_dump_format(m_srcFormatContext, m_videoStreamIndex, nullptr, 0);

    const AVCodecParameters *codecParameters = m_srcFormatContext->streams[m_videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        if (m_srcFormatContext) {
            avformat_close_input(&m_srcFormatContext);
            m_srcFormatContext = nullptr;
        }
        LOG_ERROR("find codec failed\n");
        return -1;
    }
    LOG_INFO("find codec {}\n", codec->name);

    m_srcCodecContext = avcodec_alloc_context3(codec);
    if (m_srcCodecContext == nullptr) {
        LOG_ERROR("alloc codec context failed\n");
        return -1;
    }

    ret = avcodec_parameters_to_context(m_srcCodecContext, codecParameters);
    if (ret < 0) {
        LOG_ERROR("copy codec parameters to context failed: {}\n", getErrorString(ret));
        return -1;
    }

    ret = avcodec_open2(m_srcCodecContext, codec, nullptr);
    if (ret < 0) {
        LOG_ERROR("open codec failed: {}\n", getErrorString(ret));
        return -1;
    }

    m_width = m_srcCodecContext->width;
    m_height = m_srcCodecContext->height;
    m_fps = m_srcCodecContext->framerate.num / m_srcCodecContext->framerate.den;
    m_format = av_get_pix_fmt_name(m_srcCodecContext->pix_fmt);
    LOG_INFO("video width: {} height: {} fps: {} format: {}\n", m_width, m_height, m_fps, m_format.toStdString().c_str());


    switch (m_srcCodecContext->pix_fmt)
    {
    case AV_PIX_FMT_YUVJ420P:
        m_srcCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        break;
    case AV_PIX_FMT_YUVJ422P:
        m_srcCodecContext->pix_fmt = AV_PIX_FMT_YUV422P;
        break;
    case AV_PIX_FMT_YUVJ444P:
        m_srcCodecContext->pix_fmt = AV_PIX_FMT_YUV444P;
        break;
    case AV_PIX_FMT_YUVJ440P:
        m_srcCodecContext->pix_fmt = AV_PIX_FMT_YUV440P;
        break;
    default:
        break;
    }

    m_swsContext = sws_getContext(m_width, m_height, m_srcCodecContext->pix_fmt,
                                  m_width, m_height, AV_PIX_FMT_RGB24,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (m_swsContext == nullptr) {
        LOG_ERROR("create sws context failed\n");
        return -1;
    }
    m_renderFrame = av_frame_alloc();
    if (m_renderFrame == nullptr) {
        LOG_ERROR("alloc frame failed\n");
        return -1;
    }
    m_renderFrame->format = AV_PIX_FMT_RGB24;
    m_renderFrame->width = m_width;
    m_renderFrame->height = m_height;
    ret = av_frame_get_buffer(m_renderFrame, 0);
    if (ret < 0) {
        LOG_ERROR("alloc frame buffer failed: {}\n", getErrorString(ret));
        return -1;
    }

    m_isOpened = true;
    return 0;
}

int FFmpegPlayer::close()
{
    m_isPlaying = false;
    this->wait();

    if (m_swsContext != nullptr) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_srcCodecContext != nullptr) {
        avcodec_close(m_srcCodecContext);
        avcodec_free_context(&m_srcCodecContext);
        m_srcCodecContext = nullptr;
    }
    if (m_srcFormatContext != nullptr) {
        avformat_close_input(&m_srcFormatContext);
        avformat_free_context(m_srcFormatContext);
        m_srcFormatContext = nullptr;
    }

    if (m_renderFrame != nullptr) {
        av_frame_free(&m_renderFrame);
        m_renderFrame = nullptr;
    }

    if (m_frame != nullptr) {
        delete m_frame;
        m_frame = nullptr;
    }

    m_isOpened = false;
    m_videoStreamIndex = -1;
    m_width = -1;
    m_height = -1;
    m_fps = -1;
    m_format.clear();
    return 0;
}

void FFmpegPlayer::playDevice(const QString& path, int width, int height, int fps, const QString& format)
{
    close();
    if (path.isEmpty() || width <= 0 || height <= 0 || fps <= 0) {
        LOG_ERROR("input params is invalid");
        return;
    }
    AVDictionary *options = nullptr;

    const std::string size = std::to_string(width) + "x" + std::to_string(height);
    int ret = av_dict_set(&options, "video_size", size.c_str(), 0);
    if (ret < 0) {
        LOG_ERROR("set video size failed {}\n", getErrorString(ret));
        return;
    }
    LOG_INFO("set video size {}\n", size.c_str());


    ret = av_dict_set_int(&options, "framerate", fps, 0);
    if (ret < 0) {
        LOG_ERROR("set frameRate failed: {}\n", getErrorString(ret));
        return;
    }
    LOG_INFO("set video frameRate {}\n", fps);

    ret = av_dict_set(&options, "input_format", format.toStdString().c_str(), 0);
    if (ret < 0) {
        LOG_ERROR("set video format failed {}\n", getErrorString(ret));
        return;
    }
    LOG_INFO("set video format {}\n", format.toStdString().c_str());

    const std::string deviceUrl = "video=" + path.toStdString();
    playUrl(deviceUrl, options);
}

void FFmpegPlayer::playUrl(const std::string& url, AVDictionary* options)
{
    m_srcPath = QString::fromStdString(url);
    m_options = options;
    start();
}

void FFmpegPlayer::run()
{
    if(open(m_srcPath.toStdString(),m_options) != 0)
    {
        LOG_ERROR("open device failed\n");
        return;
    }

    AVPacket *packet = av_packet_alloc();
    if (packet == nullptr) {
        LOG_ERROR("alloc packet failed\n");
        return;
    }
    AVFrame *frame = av_frame_alloc();;
    if (frame == nullptr) {
        LOG_ERROR("alloc frame failed\n");
        return;
    }
    int errorCount = 0;
    m_isPlaying = true;
    m_frame = new QImage(m_width, m_height, QImage::Format_RGB888);
    while (m_isPlaying && m_isOpened) {
        if (errorCount > ERROR_TIMES_MAX) {
            LOG_ERROR("read frame error count > {}\n", ERROR_TIMES_MAX);
            break;
        }
        //读取包
        if (int ret = av_read_frame(m_srcFormatContext, packet); ret == 0) {
            //判断是不是视频流
            if (packet->stream_index == m_videoStreamIndex) {
                ret = avcodec_send_packet(m_srcCodecContext, packet);
                if (ret == AVERROR(EAGAIN)) {
                    LOG_WARN("decode send packet eagain: {}\n", getErrorString(ret));
                    errorCount++;
                    continue;
                }
                if (ret < 0) {
                    LOG_WARN("decode send packet failed: {}\n", getErrorString(ret));
                    errorCount++;
                    continue;
                }
                ret = avcodec_receive_frame(m_srcCodecContext, frame);
                if (ret < 0) {
                    LOG_WARN("decode receive frame failed: {}\n", getErrorString(ret));
                    errorCount++;
                    continue;
                }

                ret = sws_scale(m_swsContext, frame->data, frame->linesize, 0,
                        frame->height, m_renderFrame->data, m_renderFrame->linesize);
                if (ret < 0) {
                    LOG_WARN("decode sws scale failed: {}\n", getErrorString(ret));
                    errorCount++;
                    av_frame_unref(frame);
                    continue;
                }
                memcpy(m_frame->bits(), m_renderFrame->data[0], m_renderFrame->width * m_renderFrame->height * 3);
                Q_EMIT sigFrameReaded(m_frame->copy());
                errorCount = 0;
            }
        } else if (ret == AVERROR_EOF) {
            LOG_INFO("read frame eof\n");
            errorCount++;
        } else if (ret < 0) {
            LOG_ERROR("read frame failed: {}\n", getErrorString(ret));
            errorCount++;
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    LOG_INFO("thread loop end\n");
}
