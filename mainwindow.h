#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCloseEvent>
#define PLAY_SPD_05 200
#define PLAY_SPD_08 150
#define PLAY_SPD_10 100    //默认
#define PLAY_SPD_12 80
#define PLAY_SPD_15 60
#define PLAY_SPD_20 40
#define PLAY_SPD_30 10
#define PLAY_SPD_40 5

enum
{
    Video_Playing = 0,//播放中
    Video_PlayPause,//暂停播放
    Video_PlayFinish//播放完成
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    int playVideo(char* videoPath);
    void resizeWindow(int width, int height);
    void init();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;//窗口变化事件
    virtual  void  closeEvent(QCloseEvent* event);//窗体关闭事件


private slots:
    void on_selectVideoFile();
    void on_videoPlayCtrl();
    void on_stopPlayVideo();
    void on_autoSize();
    void on_VolumeChange(int volume);
    void on_playSpdChange(int spdVal);
private:
    Ui::MainWindow *ui;
    QLabel* mVideoFile;//用与在状态栏显示选择的文件名
    QLabel* mCurVolume;//显示当前音量
    QAction* mActionPlayCtrl;//视频播/暂停控制
    int mVideoPlaySta;//视频播放状态，正在播放，播放完成，暂停，停止
    int mVideoWidth;//视频宽度
    int mVideoHeight;//视频高度
    int mPlaySpdVal;//视频播放速度的控制值，值越大，播放速度越慢。0<=mPlaySpdVal
};
#endif // MAINWINDOW_H
