#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QString>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QByteArray>
#include <QRegularExpression>
#include <vector>
#include <QStringList>
#include <QElapsedTimer>
#include <algorithm>
#include <utility>
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
    QElapsedTimer ElapsedTimer;//计时器
    auto* label=new QLabel("暂无歌词", &window); // 测试用主显示区
    auto* button_in=new QPushButton("显示示例", &window); // 显示按钮
    auto* button_out=new QPushButton("清空", &window); // 清空页面
    auto* timer=new QTimer(&window); // 超时器
    auto* OpenFile=new QPushButton("打开文件", &window);//文件打开按钮
    auto* LyricWord_count=new QLabel(QString::number(visibleLyric), &window); // 测试用歌词字数展示
    auto* LyricLine_count=new QLabel(QString::number(0), &window); // 测试用歌词行数目展示
    QString s="默认歌词";
    QObject::connect(OpenFile,&QPushButton::clicked,&window,
    [&window,&s,timer,&visibleLyric,label,LyricWord_count,&Lines,&visibleLyricLine,LyricLine_count]() { // 打开文件
        QString File=QFileDialog::getOpenFileName(&window,"选择文件",QString(),"文本文件(*.txt);;歌词文件(*.lrc)");
        if(File.isEmpty())
        {
            return;
        }
        QFile file(File);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug() << file.errorString();
            return;
        }
        std::vector<Lyric_Line> Temp_Lyric;
        QByteArray Byte=file.readAll();
        QString Now_Lyric=QString::fromUtf8(Byte);
        if(Now_Lyric.isEmpty())
        {
            return;
        }
        timer->stop();
        s=Now_Lyric;
        visibleLyric=0;
        label->clear();
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
        std::stable_sort(Temp_Lyric.begin(),Temp_Lyric.end(),[](const Lyric_Line& a,const Lyric_Line& b){return a.Time<b.Time;});
        Lines=std::move(Temp_Lyric);
        visibleLyricLine=-1;
        LyricLine_count->setText(QString::number(visibleLyricLine));
    });
    QObject::connect(timer,&QTimer::timeout,&window,
    [&visibleLyric,label,timer,&s,LyricWord_count,&visibleLyricLine,&Lines,&ElapsedTimer,LyricLine_count]() { // 计时器信号槽
        qDebug()<<ElapsedTimer.elapsed();
        if(visibleLyricLine+1<Lines.size() && Lines[visibleLyricLine+1].Time<=ElapsedTimer.elapsed()) // 满足条件，跳到下一行
        {
            label->clear();
            visibleLyric=0;
            int i=visibleLyricLine+1;
            while(i+1<Lines.size() && Lines[i].Time==Lines[i+1].Time) // 相同时间定位到最后一句歌词
            {
                i++;
            }
            timer->start(150);
            visibleLyricLine=i;
            s=Lines[visibleLyricLine].Context;
            LyricLine_count->setText(QString::number(visibleLyricLine));
        }
        if(visibleLyric<s.size() && Lines[visibleLyricLine].Time<=ElapsedTimer.elapsed()) // 行内逐字显示
        {
            visibleLyric++;
            label->setText(s.left(visibleLyric));
            LyricWord_count->setText(QString::number(visibleLyric));
        }
        if(visibleLyric>=s.length() && visibleLyricLine+1>=Lines.size())
        {
            timer->stop();
            visibleLyric=0;
            visibleLyricLine=0;
            return;
        }
    });
    QObject::connect(button_in,&QPushButton::clicked,&window,
    [label,&visibleLyric,timer,&s,LyricWord_count,&ElapsedTimer,&visibleLyricLine,LyricLine_count,&Lines]() { // 按钮1信号槽
        timer->stop();
        int i=0;
        if(Lines.empty()) // 防止段错误，边界检查
        {
        qDebug()<<"无歌词";
        return;
        }
        else
        {
            s=Lines[visibleLyricLine].Context;
        }
        if(Lines[0].Time==0) // 检索0ms时的所有歌词，并暂时性定位到最后一句，前置长度判断防止短路
        {
            while(i+1<Lines.size() &&Lines[i].Time==Lines[i+1].Time)
            {
                i++;
            }
        visibleLyricLine=i;
        }
        if(!s.isEmpty())
        {
            ElapsedTimer.start();
            visibleLyric=0;
            if(visibleLyric<s.length())timer->start(150);
            LyricWord_count->setText(QString::number(visibleLyric));
            LyricLine_count->setText(QString::number(visibleLyricLine));
        }
        else
        {
            label->clear();
            visibleLyric=0;
            visibleLyricLine=-1;
            LyricWord_count->setText(QString::number(visibleLyric));
            LyricLine_count->setText(QString::number(visibleLyricLine));
            return;
        }
    });
    QObject::connect(button_out,&QPushButton::clicked,&window,
    [label,&visibleLyric,timer,LyricWord_count,&visibleLyricLine,LyricLine_count]() {//clear清屏
        timer->stop();
        label->clear();
        visibleLyric=0;
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(visibleLyricLine));
    });
    auto* layout=new QVBoxLayout(&window);
    layout->addWidget(label);
    layout->addWidget(LyricWord_count);
    layout->addWidget(LyricLine_count);
    layout->addWidget(OpenFile);
    layout->addWidget(button_in);
    layout->addWidget(button_out);
    window.show(); // NOLINT
    return app.exec();
}