#include<QApplication>
#include<QWidget>
#include<QLabel>
#include<QPushButton>
#include<QVBoxLayout>
#include<QTimer>
#include<QString>
#include<QFile>
#include<QFileDialog>
#include<QDebug>
#include<QByteArray>
#include<QRegularExpression>
#include<vector>
#include<QStringList>
#include<QElapsedTimer>
#include<algorithm>
#include<utility>
#include<QGuiApplication>
#include<QScreen>
struct Lyric_Line
{
    qint64 Time;
    QString Context;
};
QRegularExpression Pattern(
    R"(^\[(\d{2}):([0-5]\d)\.(\d{2})\](.*)$)"
    );
int main(int argc, char* argv[])
{
    int visibleLyric=0;
    int visibleLyricLine=-1;
    std::vector<Lyric_Line> Lines;//动态数组歌词行
    QApplication app(argc, argv);
    QWidget window;//顶层窗口
    window.setWindowTitle("Lyric");
    window.resize(500, 180);
    QWidget LyricWindow;//歌词显示窗口
    QScreen* Screen=QGuiApplication::primaryScreen();
    if(Screen==nullptr)
    {
        qDebug()<<"无可用主页面";
        return 1;
    }
    LyricWindow.setGeometry(Screen->geometry());
    LyricWindow.setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::WindowStaysOnTopHint |
        Qt::Tool |
        Qt::WindowTransparentForInput |
        Qt::WindowDoesNotAcceptFocus);
    LyricWindow.setAttribute(Qt::WA_TranslucentBackground,true);
    LyricWindow.setAttribute(Qt::WA_ShowWithoutActivating,true);
    QElapsedTimer ElapsedTimer;//计时器
    auto* label=new QLabel(&LyricWindow); // 主显示区
    label->move(100,100);
    auto* button_in=new QPushButton("显示示例", &window); // 显示按钮
    auto* button_out=new QPushButton("清空", &window); // 清空页面
    auto* timer=new QTimer(&window); // 超时器
    auto* OpenFile=new QPushButton("打开文件", &window);//文件打开按钮
    auto* LyricWord_count=new QLabel(QString::number(visibleLyric), &window); // 测试用歌词字数展示
    auto* LyricLine_count=new QLabel(QString::number(0), &window); // 测试用歌词行数目展示
    QObject::connect(OpenFile,&QPushButton::clicked,&window,
    [&window,timer,&visibleLyric,label,LyricWord_count,&Lines,&visibleLyricLine,LyricLine_count]() { // 打开文件
        QString File=QFileDialog::getOpenFileName(&window,"选择文件",QString(),"文本文件(*.txt);;歌词文件(*.lrc)");
        if(File.isEmpty())return;
        QFile file(File);
        if(!file.open(QIODevice::ReadOnly))//打开审查
        {
            qDebug() << file.errorString();
            return;
        }
        std::vector<Lyric_Line> Temp_Lyric;
        QByteArray Byte=file.readAll();
        if(file.error()!=QFileDevice::NoError)//读取审查
        {
            qDebug()<< file.errorString();
            return;
        }
        QString Now_Lyric=QString::fromUtf8(Byte);
        if(Now_Lyric.isEmpty())return;
        QStringList Lyric_row=Now_Lyric.split('\n'); // 截取歌词
        LyricWord_count->setText(QString::number(visibleLyric));
        for(const QString &Now_row:Lyric_row)
        {
            QString matcher=Now_row.trimmed();
            auto match=Pattern.match(matcher);
            if(match.hasMatch())
            {
                Lyric_Line Lyric_Now;
                Lyric_Now.Time=60000LL*match.captured(1).toInt()+1000LL*match.captured(2).toInt()+match.captured(3).toInt()*10LL;
                Lyric_Now.Context=match.captured(4);
                Temp_Lyric.push_back(Lyric_Now);
            }
        }
        if(Temp_Lyric.size()==0)
        {
            return;
        }
        timer->stop();
        visibleLyric=0;
        label->clear();
        std::stable_sort(Temp_Lyric.begin(),Temp_Lyric.end(),[](const Lyric_Line& a,const Lyric_Line& b){return a.Time<b.Time;});
        Lines=std::move(Temp_Lyric);
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    QObject::connect(timer,&QTimer::timeout,&window,
    [&visibleLyric,label,timer,LyricWord_count,&visibleLyricLine,&Lines,&ElapsedTimer,LyricLine_count]() { // 计时器信号槽
        int i=-1;//行筛选
        qint64 TIMESCAN=ElapsedTimer.elapsed();
        for(std::size_t I=0;I<Lines.size();++I)
        {
            if(Lines[I].Time>TIMESCAN)break;
            i=I;
        }
        if(i==-1)return;
        visibleLyricLine=i;//有效筛选，进入播放环节
        qint64 Line_Elapsed=TIMESCAN-Lines[visibleLyricLine].Time;//开始时间
        qint64 LineWord_display=std::min(Line_Elapsed/150+1,Lines[visibleLyricLine].Context.size());//目前行歌词进度
        label->setText(Lines[visibleLyricLine].Context.left(LineWord_display));
        label->adjustSize();
        LyricWord_count->setText(QString::number(LineWord_display));
        LyricLine_count->setText(QString::number(visibleLyricLine+1));
        visibleLyric=int(LineWord_display);
        if(Lines.size()!=0 && visibleLyricLine+1>=Lines.size() && LineWord_display>=Lines[visibleLyricLine].Context.size())
        {
            timer->stop();
            return;
        }
    });
    QObject::connect(button_in,&QPushButton::clicked,&window,
    [label,&visibleLyric,timer,LyricWord_count,&ElapsedTimer,&visibleLyricLine,LyricLine_count,&Lines]() { // 按钮1信号槽
        timer->stop();
        if(Lines.empty()) // 防止段错误，边界检查
        {
        qDebug()<<"无歌词";
        return;
        }
        label->clear();
        visibleLyricLine=-1;
        visibleLyric=0;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
        timer->start(50);
        ElapsedTimer.start();
    });
    QObject::connect(button_out,&QPushButton::clicked,&window,
    [label,&visibleLyric,timer,LyricWord_count,&visibleLyricLine,LyricLine_count]() {//clear清屏
        timer->stop();
        label->clear();
        visibleLyric=0;
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    auto* layout=new QVBoxLayout(&window);
    layout->addWidget(LyricWord_count);
    layout->addWidget(LyricLine_count);
    layout->addWidget(OpenFile);
    layout->addWidget(button_in);
    layout->addWidget(button_out);
    LyricWindow.show();
    window.show(); // NOLINT
    return app.exec();
}