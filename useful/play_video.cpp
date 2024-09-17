#include <iostream>

#ifdef __cplusplus  
extern "C" {
#endif  

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

#include "libavutil/time.h"

#ifdef __cplusplus  
}
#endif  


#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Rect rect;

int thread_quit = 0;
int thread_pause = 0;

//SDL 线程参数
struct FFMpegData{
    AVFormatContext* input_fmt_ctx;
    AVCodecContext*  dec_ctx;
    int video_stream_index;
    AVFrame* frame;
};


int thread_func(void *data)
{
    int ret;
    int64_t start_time = av_gettime();
    AVPacket packet;

    AVFormatContext* input_fmt_ctx = ((FFMpegData*)data)->input_fmt_ctx;
    AVCodecContext*  dec_ctx = ((FFMpegData*)data)->dec_ctx;
    int video_stream_index = ((FFMpegData*)data)->video_stream_index;
    AVFrame* frame = ((FFMpegData*)data)->frame;

    while(!thread_quit) 
    {
        if(thread_pause) {
            SDL_Delay(10);
            continue;
        }

        if((ret = av_read_frame(input_fmt_ctx, &packet)) < 0)
            break;

        if(packet.stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, &packet);

            while(ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);

                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if(ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                    //goto end;
                    break;
                }

                //显示
                SDL_UpdateYUVTexture(texture, &rect,
                                     frame->data[0], frame->linesize[0],
                                     frame->data[1], frame->linesize[1],
                                     frame->data[2], frame->linesize[2]);

                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_RenderPresent(renderer);

                // 控制延时
                AVRational time_base_q = {1,AV_TIME_BASE};
                int64_t pts_time = av_rescale_q(frame->pkt_dts, input_fmt_ctx->streams[video_stream_index]->time_base, time_base_q);
                int64_t now_time = av_gettime() - start_time;
                if(pts_time > now_time)
                    av_usleep(pts_time - now_time);

                av_frame_unref(frame);
            }
        }

        av_packet_unref(&packet);
    }

    //flush decoder
    //FIX: Flush Frames remained in Codec
    while(1) {

        if(thread_pause) {
            break;
        }

        ret = avcodec_send_packet(dec_ctx, &packet); // 发送空包到解码器
        if(ret < 0) 
            break;
    
        while(ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);

            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if(ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                //goto end;
                break;
            }

            //显示
            SDL_UpdateYUVTexture(texture, &rect,
                                 frame->data[0], frame->linesize[0],
                                 frame->data[1], frame->linesize[1],
                                 frame->data[2], frame->linesize[2]);

            //SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_RenderPresent(renderer);

            AVRational time_base_q = {1,AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(frame->pkt_dts, input_fmt_ctx->streams[video_stream_index]->time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if(pts_time > now_time)
                av_usleep(pts_time - now_time);

            av_frame_unref(frame);
        }
    }

    // 通知主线程退出
    SDL_Event eve; eve.type = SDL_QUIT;
    SDL_PushEvent(&eve);

    return 0;
}

int main()
{
    //const char* filename = "LoadingScreen_2.mp4";
    const char* filename = "tellme.mp4";

    // 1  输入流
    AVFormatContext *input_fmt_ctx = NULL;
    const AVCodec *input_codec = NULL;
    int ret;

    if((ret = avformat_open_input(&input_fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if((ret = avformat_find_stream_info(input_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    int video_stream_index = -1;
    for(int idx = 0; idx < input_fmt_ctx->nb_streams; idx++) {
        AVStream* stream = input_fmt_ctx->streams[idx];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = idx;
            break;
        }
    }

    if(video_stream_index < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find video stream\n");
        avformat_close_input(&input_fmt_ctx);
        return 0;
    }

    // 输出所有流信息
    av_dump_format(input_fmt_ctx, 0, NULL, 0);

    //2 解码器
    AVCodecParameters *codecpar = input_fmt_ctx->streams[video_stream_index]->codecpar;
    input_codec = avcodec_find_decoder(codecpar->codec_id);
    if(input_codec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Could not find decoder for codec id \n", codecpar->codec_id);
        avformat_close_input(&input_fmt_ctx);
        return AVERROR(ENOMEM);
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(input_codec);
    if(!dec_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate a decoding context\n");
        avformat_close_input(&input_fmt_ctx);
        return AVERROR(ENOMEM);
    }

    if((ret = avcodec_parameters_to_context(dec_ctx, codecpar)) < 0) {
        avformat_close_input(&input_fmt_ctx);
        avcodec_free_context(&dec_ctx);
        return ret;
    }

    if((ret = avcodec_open2(dec_ctx, input_codec, NULL)) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&input_fmt_ctx);
        return ret;
    }

    //3 解码
    //AVPacket packet;
    AVFrame *frame = av_frame_alloc(); // 解码数据yuv420p

    // 解码后数据, 转换
    int srcW = codecpar->width;
    int srcH = codecpar->height;
    int dstW = srcW;
    int dstH = srcH;

    ///   SDL init start
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("init audio subsysytem failed.");
        return 0;
    }

    int screen_w = dstW;
    int screen_h = dstH;

    ret = SDL_CreateWindowAndRenderer(screen_w, screen_h, SDL_WINDOW_RESIZABLE, &window, &renderer);
    if(ret < 0) {
        SDL_LogError(1, "SDL: SDL_CreateWindowAndRenderer failed.");
        return 0;
    }
    SDL_SetWindowTitle(window, "SDL2.0 Video Sample");
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
    if(!texture) {
        SDL_LogError(1, "SDL: SDL_CreateTexture failed.");
        return 0;
    }

    rect = SDL_Rect{0,0,screen_w, screen_h};

    SDL_Event event;

    FFMpegData data;
    data.input_fmt_ctx = input_fmt_ctx;
    data.dec_ctx = dec_ctx;
    data.frame = frame;
    data.video_stream_index = video_stream_index;

    SDL_Thread *sdl_thread = SDL_CreateThread(thread_func, NULL, &data);

     //SDL init end

    // Event loop

    while(1) 
    {
        SDL_WaitEvent(&event);
        if( event.type == SDL_QUIT)
        {
            thread_quit = 1;
            break;
        }
        else if(event.type == SDL_KEYDOWN) 
        {
            if(event.key.keysym.sym == SDLK_SPACE) {
                thread_pause = !thread_pause;
            }
        }
    }

    SDL_Quit();

    avcodec_free_context(&dec_ctx);
    avformat_close_input(&input_fmt_ctx);
    av_frame_free(&frame);
}
