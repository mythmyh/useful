#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QTime>
#include <QElapsedTimer>
#include <QIcon>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>
#include <QSlider>
#include <QComboBox>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug("ffmpeg versioin is: %u", avcodec_version());
    QImage *img=new QImage;
    img->load("C:\\Users\\21635\\Documents\\screen.jpg");
    ui->labelVideo->setPixmap(QPixmap::fromImage(*img));
    init();

    //char *s=(char*)"C:\\Users\\21635\\Documents\\ffm\\chose.mp4";
   //playVideo(s);

}


MainWindow::~MainWindow()
{
    delete ui;
}

//初始化
void MainWindow::init()
{
    mVideoWidth = this->width();
    mVideoHeight = this->height();

    //设置视频label的宽高为窗体宽高
    ui->labelVideo->setGeometry(0, 0, 640, 480);

    //初始化播放速度
    mPlaySpdVal = PLAY_SPD_40;

    //初播放状态wei
    mVideoPlaySta = Video_PlayFinish;

    /*****在状态栏添加一个用来显示视频路径的label************/
    //用于显示已经选择的视频文件路径
    mVideoFile = new QLabel(this);
    mVideoFile->setToolTip("视频文件");
    ui->statusBar->addWidget(mVideoFile);

    /*****在工具栏添加选择动作************/
    //创建一个选择视频文件动作
    QAction* pActionSelectVideoFile = new QAction(QIcon(QPixmap(":/selectVideo.png")), "选择");

    //设置鼠标悬停提示文本
    pActionSelectVideoFile->setToolTip("选择视频文件");

    //将动作添加到工具栏
    ui->mainToolBar->addAction(pActionSelectVideoFile);

    //连接动作的点击事件
    QObject::connect(pActionSelectVideoFile, SIGNAL(triggered(bool)), this, SLOT(on_selectVideoFile()));

    /*****在工具栏添加播放/暂停动作************/
    //创建一个播放/暂停视频动作
    mActionPlayCtrl = new QAction(QIcon(QPixmap(":/play.png")), "播放");

    //设置鼠标悬停提示文本
    mActionPlayCtrl->setToolTip("播放视频");

    //将动作添加到工具栏
    ui->mainToolBar->addAction(mActionPlayCtrl);

    //连接动作的点击事件
    QObject::connect(mActionPlayCtrl, SIGNAL(triggered(bool)), this, SLOT(on_videoPlayCtrl()));

    /*****在工具栏添加停止动作************/
    //创建一个停止播放视频动作
    QAction* pActionStopPlay = new QAction(QIcon(QPixmap(":/stop.png")), "停止");

    //设置鼠标悬停提示文本
    pActionStopPlay->setToolTip("停止播放");

    //将动作添加到工具栏
    ui->mainToolBar->addAction(pActionStopPlay);

    //连接动作的点击事件
    QObject::connect(pActionStopPlay, SIGNAL(triggered(bool)), this, SLOT(on_stopPlayVideo()));

    /*****在工具栏添加窗体大小适配视频宽高动作************/
    //创建一个停止播放视频动作
    QAction* pActionAutoSize = new QAction(QIcon(QPixmap(":/autoSize.png")), "自适应");

    //设置鼠标悬停提示文本
    pActionAutoSize->setToolTip("窗体自适应视频宽高");

    //将动作添加到工具栏
    ui->mainToolBar->addAction(pActionAutoSize);

    //连接动作的点击事件
    QObject::connect(pActionAutoSize, SIGNAL(triggered(bool)), this, SLOT(on_autoSize()));

    //添加分隔符
    ui->mainToolBar->addSeparator();

    //添加播放速度下拉选择
    QStringList plsySpdItems;
    plsySpdItems << "0.5";
    plsySpdItems << "0.8";
    plsySpdItems << "1";
    plsySpdItems << "1.2";
    plsySpdItems << "1.5";
    plsySpdItems << "2";
    plsySpdItems << "3";
    plsySpdItems << "4";
    QComboBox* comboBoxPlaySpd = new QComboBox();
    comboBoxPlaySpd->setToolTip("播放速度");
    comboBoxPlaySpd->addItems(plsySpdItems);//添加倍速选项
    comboBoxPlaySpd->setCurrentIndex(2);//设置默认速度1
    comboBoxPlaySpd->setEditable(false);//设置为不可编辑

    //设置样式
    //comboBoxPlaySpd->setStyleSheet("QComboBox { border:1px solid black; min-width: 30px;} QComboBox QAbstractItemView::item {min-height:20px; background-color: rgb(0, 0, 0);} QFrame { border: 1px solid black; }");
    ui->mainToolBar->addWidget(comboBoxPlaySpd);
    QObject::connect(comboBoxPlaySpd, SIGNAL(currentIndexChanged(int)), this, SLOT(on_playSpdChange(int)));

    //添加分隔符
    ui->mainToolBar->addSeparator();

    //添加播放声音滑动条到工具栏
    QString sliderStyle = "QSlider::groove:horizontal{ \
            height: 6px; \
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: , stop: 0 rgb(124, 124, 124), stop: 1.0 rgb(72, 71, 71)); \
   } \
    QSlider::handle:horizontal { \
            width: 5px; \
            background: rgb(0, 160, 230); \
            margin: -6px 0px -6px 0px; \
            border-radius: 9px; \
   }";
    QSlider* slider = new QSlider(Qt::Orientation::Horizontal, this);
    slider->setToolTip("音量");
    slider->setMaximumWidth(100);//设置最大值
    slider->setRange(0, 100);//设置滑动范围
    slider->setStyleSheet(sliderStyle);//设置样式
    ui->mainToolBar->addWidget(slider);
    //连接动作的滑动事件
    QObject::connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(on_VolumeChange(int)));

    /*****在工具栏添加一个用来显示音量的label************/
    mCurVolume = new QLabel("音量60");
    mCurVolume->setToolTip("当前音量值");
    ui->mainToolBar->addWidget(mCurVolume);


}

//延时, 不能直接sleep延时，UI主线程不能直接被阻塞，不然会有问题的
void delay(int ms)
{
    QElapsedTimer stopTime;
    stopTime.start();
    //qDebug()<<"start:"<<QTime::currentTime().toString("HH:mm:ss.zzz");
    while(stopTime.elapsed() < ms)//stopTime.elapsed()返回从start开始到现在的毫秒数
    {
        QCoreApplication::processEvents();
    }
    //qDebug()<<"stop :"<<QTime::currentTime().toString("HH:mm:ss.zzz");
}

//重新设置窗体和视频Label的宽高
void MainWindow::resizeWindow(int width, int height)
{
    if(width<1)
    {
       width = this->width();
    }

    if(height<1)
    {
        height = this->height();
    }

    this->setGeometry(100, 100, width, height);
    ui->labelVideo->setGeometry(0, 0, width, height);
}

//窗体变化事件
void MainWindow::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);

    //让视频Label跟随窗体变化
    ui->labelVideo->resize(this->size());
}

//使用FFmpeg播放视频
int MainWindow::playVideo(char* videoPath)
{
    unsigned char* buf;
    int isVideo = -1;
    int ret, gotPicture;
    unsigned int i, streamIndex = 0;
   const AVCodec *pCodec;
    AVPacket *pAVpkt;
    AVCodecContext *pAVctx;
    AVFrame *pAVframe, *pAVframeRGB;
    AVFormatContext* pFormatCtx;
    struct SwsContext* pSwsCtx;
    AVStream * st;

    //创建AVFormatContext
    pFormatCtx = avformat_alloc_context();

    //初始化pFormatCtx
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0)
    {
        qDebug("avformat_open_input err.");
        return -1;
    }

    //获取音视频流数据信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        avformat_close_input(&pFormatCtx);
        qDebug("avformat_find_stream_info err.");
        return -2;
    }

    //找到视频流的索引
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            streamIndex = i;
            isVideo = 0;
            break;
        }
    }

    //没有视频流就退出
    if (isVideo == -1)
    {
        avformat_close_input(&pFormatCtx);
        qDebug("nb_streams err.");
        return -3;
    }

    //获取视频流编码
    pAVctx = avcodec_alloc_context3(NULL);;

    //查找解码器
    avcodec_parameters_to_context(pAVctx, pFormatCtx->streams[streamIndex]->codecpar);
    pCodec = avcodec_find_decoder(pAVctx->codec_id);
    if (pCodec == NULL)
    {
        avcodec_close(pAVctx);
        avformat_close_input(&pFormatCtx);
        qDebug("avcodec_find_decoder err.");
        return -4;
    }

    //初始化pAVctx
    if (avcodec_open2(pAVctx, pCodec, NULL) < 0)
    {
        avcodec_close(pAVctx);
        avformat_close_input(&pFormatCtx);
        qDebug("avcodec_open2 err.");
        return -5;
    }

    //初始化pAVpkt
   // pAVpkt = (AVPacket *)av_malloc(sizeof(AVPacket));

    //初始化数据帧空间
    pAVframe = av_frame_alloc();
    pAVframeRGB = av_frame_alloc();
    qDebug("width,%d,height:%d",pAVctx->width,pAVctx->height);
    //创建图像数据存储buf
    //av_image_get_buffer_size一帧大小
    buf = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, pAVctx->width, pAVctx->height, 1));
    av_image_fill_arrays(pAVframeRGB->data, pAVframeRGB->linesize, buf, AV_PIX_FMT_RGB32, pAVctx->width, pAVctx->height, 1);

    //根据视频宽高重新调整窗口大小 视频宽高 pAVctx->width, pAVctx->height
    //resizeWindow(pAVctx->width, pAVctx->height);
    qDebug("helloworld %d %d",pAVctx->pix_fmt, AV_PIX_FMT_YUYV422);
    //初始化pSwsCtx
    pSwsCtx = sws_getContext(pAVctx->width, pAVctx->height, pAVctx->pix_fmt, pAVctx->width, pAVctx->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    //循环读取视频数据
    for(;;)
    {

        if(mVideoPlaySta == Video_Playing)//正在播放
        {

            pAVpkt = (AVPacket *)av_malloc(sizeof(AVPacket));
          //  qDebug("helloworld2");
            ret =av_read_frame(pFormatCtx, pAVpkt);
            st=pFormatCtx->streams[pAVpkt->stream_index];

            if(ret!= 0)//读取一帧未解码的数据
            {
                continue;
                }

            if (st->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
                {
                        avcodec_send_packet(pAVctx, pAVpkt);
                        ret=avcodec_receive_frame(pAVctx,pAVframe);


                     //   qDebug("helloworld4");
                        if(ret ==0){
                           int a= sws_scale(pSwsCtx, (const unsigned char* const*)pAVframe->data, pAVframe->linesize, 0, pAVctx->height, pAVframeRGB->data, pAVframeRGB->linesize);

                            qDebug("sws_scale %d %d %d",a,pAVframe->display_picture_number,pAVframe->sample_rate);

                            QImage img((uchar*)pAVframeRGB->data[0], pAVctx->width, pAVctx->height, QImage::Format_RGB32);
                            ui->labelVideo->setPixmap(QPixmap::fromImage(img));
                            delay(mPlaySpdVal);//播放延时
                         //   qDebug("helloworld");
                          //  break;
                        }



                    //解码一帧视频数据


                }


        }
        else if(mVideoPlaySta == Video_PlayFinish)//播放结束
        {
            break;
        }
        else//暂停
        {
            delay(300);
        }
    }

    //释放资源
    sws_freeContext(pSwsCtx);
    av_frame_free(&pAVframeRGB);
    av_frame_free(&pAVframe);
    avcodec_close(pAVctx);
    avformat_close_input(&pFormatCtx);

    mVideoPlaySta = Video_PlayFinish;
    qDebug()<<"play finish!";

    return 0;
}

//选择视频文件
void MainWindow::on_selectVideoFile()
{
    QString path = QFileDialog::getOpenFileName(this,"选择视频文件","D:\\Test\\Video\\","MP4视频(*.mp4);;AVI视频(*.avi);;WMV视频(*.wmv)");
    if(!path.isEmpty())
    {
        mVideoFile->setText(path);
        on_videoPlayCtrl();
    }
}

//视频播放/暂停控制
void MainWindow::on_videoPlayCtrl()
{
    QString videoFilePath = mVideoFile->text();
    if(videoFilePath.isEmpty())
    {
        return;
    }

    //判断文件是都存在(以防被删除了)
//    QFileInfo fileInfo(videoFilePath);
//    if(fileInfo.isFile())
//    {
//        QMessageBox::information(this, "提示", "找不到文件:"+videoFilePath);
//        return;
//    }

    //播放和暂停切换
    if(mActionPlayCtrl->text() == "播放")
    {
        mActionPlayCtrl->setText("暂停");
        mActionPlayCtrl->setToolTip("暂停视频");
        mActionPlayCtrl->setIcon(QIcon(QPixmap(":/pause.png")));

        if(mVideoPlaySta == Video_PlayFinish)
        {
            mVideoPlaySta = Video_Playing;
            playVideo(videoFilePath.toLocal8Bit().data());
            on_stopPlayVideo();
        }
        else
        {
            mVideoPlaySta = Video_Playing;
        }
    }
    else
    {
        mActionPlayCtrl->setText("播放");
        mActionPlayCtrl->setToolTip("播放视频");
        mActionPlayCtrl->setIcon(QIcon(QPixmap(":/play.png")));

        if(mVideoPlaySta == Video_Playing)
        {
            mVideoPlaySta = Video_PlayPause;
        }
    }
}

//停止播放视频
void MainWindow::on_stopPlayVideo()
{
    mVideoPlaySta = Video_PlayFinish;
    mActionPlayCtrl->setText("播放");
    mActionPlayCtrl->setToolTip("播放视频");
    mActionPlayCtrl->setIcon(QIcon(QPixmap(":/play.png")));
}

//窗体自适应视频宽高
void MainWindow::on_autoSize()
{
    resizeWindow(mVideoWidth, mVideoHeight);
}

//窗体关闭事件
void MainWindow::closeEvent(QCloseEvent* event)//窗体关闭事件
{
    //没有在播放视频时直接退出
    if(mVideoPlaySta != Video_PlayFinish)
    {
        if(QMessageBox::Yes == QMessageBox::information(this, "提示", "确认关闭？", QMessageBox::Yes, QMessageBox::No))
        {
            mVideoPlaySta = Video_PlayFinish;
            //event->accept();
        }
        else
        {
            event->ignore();//忽略，不关闭
        }
    }
}

//播放音量变化
void MainWindow::on_VolumeChange(int volume)
{
    mCurVolume->setText("音量"+QString::number(volume));
}

//播放速度变化
void MainWindow::on_playSpdChange(int spdVal)
{
    switch(spdVal)
    {
        case 0://0.5
        mPlaySpdVal = PLAY_SPD_20;
            break;
        case 1://0.8
            mPlaySpdVal = PLAY_SPD_08;
            break;
        case 2://1
            mPlaySpdVal = PLAY_SPD_10;
            break;
        case 3://1.2
            mPlaySpdVal = PLAY_SPD_12;
            break;
        case 4://1.5
            mPlaySpdVal = PLAY_SPD_15;
            break;
        case 5://2
            mPlaySpdVal = PLAY_SPD_20;
            break;
        case 6://3
            mPlaySpdVal = PLAY_SPD_30;
            break;
        case 7://4
            mPlaySpdVal = PLAY_SPD_40;
            break;
        default:
            break;
    }
}

