#include <iostream>
#include <map>

#ifdef __cplusplus  
extern "C" {
#endif  

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>

#include <libswresample/swresample.h>

#ifdef __cplusplus  
}
#endif  


#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"


//  audio sample rates map from FFMPEG to SDL (only non planar)
static std::map<int,int> AUDIO_FORMAT_MAP = {
   // AV_SAMPLE_FMT_NONE = -1,
    {AV_SAMPLE_FMT_U8,  AUDIO_U8    },
    {AV_SAMPLE_FMT_S16, AUDIO_S16SYS},
    {AV_SAMPLE_FMT_S32, AUDIO_S32SYS},
    {AV_SAMPLE_FMT_FLT, AUDIO_F32SYS}        
};


#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio    //48000 * (32/8)

unsigned int audioLen = 0;
unsigned char *audioChunk = NULL;
unsigned char *audioPos = NULL;

void fill_audio(void * udata, Uint8 * stream, int len)
{
    SDL_memset(stream, 0, len);

    if(audioLen == 0)
        return;

    len = (len>audioLen ? audioLen : len);

    SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);

    audioPos += len;
    audioLen -= len;
}

int main()
{
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

    // 找到音频流(不需要遍历)
    if((ret = av_find_best_stream(input_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &input_codec, 0)) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in the input file\n");
        return ret;
    }
    int audio_stream_index = ret;


    // 输出所有流信息
    av_dump_format(input_fmt_ctx, audio_stream_index, NULL, 0);


    //2 解码器
    AVCodecContext *dec_ctx = avcodec_alloc_context3(input_codec);
    if(!dec_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate a decoding context\n");
        avformat_close_input(&input_fmt_ctx);
        return AVERROR(ENOMEM);
    }

    AVCodecParameters *codecpar = input_fmt_ctx->streams[audio_stream_index]->codecpar;
    if((ret = avcodec_parameters_to_context(dec_ctx, codecpar)) < 0){
        avformat_close_input(&input_fmt_ctx);
        avcodec_free_context(&dec_ctx);
        return ret;
    }

    if((ret = avcodec_open2(dec_ctx, input_codec, NULL)) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&input_fmt_ctx);
        return ret;
    }

    // 重采样contex
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;   //声音格式  SDL仅支持部分音频格式
    int out_sample_rate = /*48000; */ dec_ctx->sample_rate;  //采样率
    int out_channels =    /*1;  */    dec_ctx->channels;     //通道数
    int out_nb_samples = /*1024;  */  dec_ctx->frame_size;
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);   //输出buff
    unsigned char *outBuff = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * out_channels);
    uint64_t out_chn_layout =  av_get_default_channel_layout(dec_ctx->channels); //AV_CH_LAYOUT_STEREO;  //通道布局 输出双声道

    SwrContext *au_convert_ctx = swr_alloc_set_opts(NULL,
                           out_chn_layout,                    /*out*/
                           out_sample_fmt,                    /*out*/
                           out_sample_rate,                   /*out*/
                           dec_ctx->channel_layout,           /*in*/                         
                           dec_ctx->sample_fmt,               /*in*/
                           dec_ctx->sample_rate,              /*in*/
                           0,
                           NULL);

    swr_init(au_convert_ctx);


    ///   SDL 
    if(SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log("init audio subsysytem failed.");
        return 0;
    }

    SDL_AudioSpec wantSpec;
    wantSpec.freq = out_sample_rate;
     // 和SwrContext的音频重采样参数保持一致
    wantSpec.format = AUDIO_FORMAT_MAP[out_sample_fmt];   
    wantSpec.channels = out_channels;
    wantSpec.silence = 0;
    wantSpec.samples = out_nb_samples;
    wantSpec.callback = fill_audio;
    wantSpec.userdata = dec_ctx;

    if(SDL_OpenAudio(&wantSpec, NULL) < 0) {
        printf("can not open SDL!\n");
        return -1;
    }

    SDL_PauseAudio(0);

    //3 解码
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    
    while(1) 
    {
        if((ret = av_read_frame(input_fmt_ctx, &packet)) < 0)
            break;

        if(packet.stream_index == audio_stream_index) 
        {
            ret = avcodec_send_packet(dec_ctx, &packet); // 送一帧到解码器

            while(ret >= 0) 
            {
                ret = avcodec_receive_frame(dec_ctx, frame);  // 从解码器取得解码后的数据

                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if(ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                    goto end;
                }

                //static FILE *p = fopen("xxx.pcm", "wb");
                //fwrite(p, frame->buf)

                ret = swr_convert(au_convert_ctx, &outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
                if(ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while converting\n");
                    goto end;
                }

                //static FILE *outFile = fopen("xxx.pcm", "wb");
                //fwrite(outBuff, 1, out_buffer_size, outFile); 

                while(audioLen > 0)
                    SDL_Delay(1);

                audioChunk = (unsigned char *)outBuff;
                audioPos = audioChunk;
                audioLen = out_buffer_size;


                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }

    // 音频不需要 flush ? 

end:
    SDL_Quit();

    avcodec_free_context(&dec_ctx);
    avformat_close_input(&input_fmt_ctx);
    av_frame_free(&frame);

    swr_free(&au_convert_ctx);
}
