#ifndef LYRICLABEL_H
#define LYRICLABEL_H
#include<QWidget>
#include<QPainter>
#include<QPoint>
#include<QRandomGenerator>
#include<QElapsedTimer>
#include<algorithm>
#include<vector>
#include<cmath>
struct Lyric_Line
{
    qint64 Time,Duration,block=1;
    QString Context;
    qint64 wordstartMS[1000],wordduration[1000];
    QString word[1000];
    bool isYRC=0;
};
struct LyricBlock
{
    qsizetype end;
    qint64 startMS,blocktime[1000];
};
class Lyriclabel:public QWidget
{
private:
    QString L_text;
    QPointF L_current={0,0};
    QSizeF L_Size={0,0};
    qint64 positionMS=0;
    int visibleLyric=0;
    Lyric_Line Line;
    std::vector<qreal>Xlitter,Ylitter;
    const std::vector<LyricBlock>* Block=nullptr;
public:
    qreal L_angle=0;
    bool setshake=0,setopacity=0,setshadow=0;
    int SDoffset=8;
    qreal FadeTime=300;//淡入时间
    explicit Lyriclabel(QWidget *parent=nullptr):QWidget(parent){
        setAttribute(Qt::WA_TranslucentBackground,true);
    }
    void Ramdonlitter()
    {
            Xlitter.clear();
            Xlitter.resize(L_text.size());
            for(qreal& real:Xlitter)
            {
                real=QRandomGenerator::global()->generateDouble()*2.0*3.1415926;
            }
            Ylitter.clear();
            Ylitter.resize(L_text.size());
            for(qreal& real:Ylitter)
            {
                real=QRandomGenerator::global()->generateDouble()*2.0*3.1415926;
            }
    }
    void clear(){
        L_text.clear();
        Block=nullptr;
        update();
    };
    void setText(const QString& text,const std::vector<LyricBlock>& blocks,int visible)
    {
        if(L_text!=text)
        {
            L_text=text;
            Ramdonlitter();
        }
        else L_text=text;
        Block=&blocks;
        visibleLyric=visible;
        update();
    }
    void setorigin(const QPointF& F)
    {
        L_current=F;
        update();
    }
    void setsize(const QSizeF& S)
    {
        L_Size=S;
        update();
    }
    void setposition(const qint64& P)
    {
        positionMS=P;
        update();
    }
    void setLyricLine(const Lyric_Line& L)
    {
        Line=L;
        yrcsetText(L);
        update();
    }
    void yrcsetText(const Lyric_Line& L)
    {
        if(L_text!=L.Context)
        {
            L_text=L.Context;
            Ramdonlitter();
        }
        else L_text=L.Context;
        update();
    }
    qreal getFadetime()
    {
        return FadeTime;
    }
    void setFadetime(const qreal& F)
    {
        FadeTime=F;
        update();
    }
protected:
    void paintEvent(QPaintEvent *event) override//绘制逻辑重写
    {

        if(L_text.isEmpty())return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);//几何抗锯齿
        painter.setRenderHint(QPainter::TextAntialiasing);//文字抗锯齿
        painter.setFont(font());//调用字体
        QFontMetrics fm(font());//获取字体信息
        const int totalY=fm.height();//宽数据
        int X=qRound(L_current.x())+SDoffset;//起点XY轴
        int Y=qRound(L_current.y())+SDoffset;
        const qreal angel=L_angle;//旋转角度快照
        painter.save();
        const qreal centralX=L_current.x()+L_Size.width()/2.0;//行旋转中心点
        const qreal centralY=L_current.y()+L_Size.height()/2.0;
        painter.translate(centralX,centralY);
        painter.rotate(angel);
        painter.translate(-centralX,-centralY);
        qreal Amplitude=1.0;//抖动幅度
        qreal Frequency=0.4;//抖动频率
        qreal seconds=positionMS/1000.0;
        qreal round=2.0*3.1415926*Frequency*seconds;//抖动周期模拟
        if(Block)
        {
            for(int i=0;i<std::min(visibleLyric,static_cast<int>(Block->size()));++i)
            {
                const qreal elapsedMS=positionMS-(*Block)[i].startMS;//相对字块经过时间
                QString NowBlock="";
                if(i>0)NowBlock=L_text.mid((*Block)[i-1].end,(*Block)[i].end-(*Block)[i-1].end);
                else NowBlock=L_text.mid(0,(*Block)[i].end);
                for(int j=0;j<NowBlock.size();++j)
                {
                    qreal floatX=0,floatY=0,Opacity=0;
                    QColor color;
                    if(setshake)
                    {
                    if(i>0)floatY=Amplitude*std::sin(round+Ylitter[(*Block)[i-1].end+j]);
                    else floatY=Amplitude*std::sin(round+Ylitter[j]);
                    if(i>0)floatX=Amplitude*std::sin(round+Xlitter[(*Block)[i-1].end+j]);
                    else floatX=Amplitude*std::sin(round+Xlitter[j]);
                    }
                    if(setopacity)Opacity=std::clamp<qreal>(elapsedMS/FadeTime,0,1);//逐字淡入不透明度
                    if(positionMS-(*Block)[visibleLyric-1].startMS>7000)Opacity=0;
                    QString Nowchar=NowBlock.at(j);
                    const int CharX=fm.horizontalAdvance(Nowchar);//取得单字长度数据
                    QRect NowRect(X,Y,CharX,totalY);//内层字体旋转回调
                    const qreal AngelX=X+CharX/2;
                    const qreal AngelY=Y+totalY/2;
                    X+=CharX;
                    painter.save();
                    if(setopacity)painter.setOpacity(Opacity);
                    if(setshadow)
                    {
                        QColor color(51,51,51,150);//阴影色
                        painter.setPen(color);
                    }
                    painter.translate(AngelX,AngelY);
                    painter.rotate(-angel);
                    painter.translate(-AngelX,-AngelY);
                    if(setshake)
                    {
                        painter.translate(0.0,floatY);//字幕呼吸抖动
                        painter.translate(floatX,0.0);
                    }
                    if(setshadow) painter.drawText(NowRect.translated(3,3),Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//绘制阴影
                    color=palette().color(QPalette::WindowText);//读取颜色
                    if(!color.isValid()) color=Qt::white;//如果颜色无效，使用白色替代
                    painter.setPen(color);
                    painter.drawText(NowRect,Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//主文字画图
                    painter.restore();
                }
            }
        }
        else if(!Line.Context.isEmpty())
        {
            FadeTime=300;
            Block=nullptr;
            int k=0;
            for(int i=0;i<Line.block;++i)
            {
                QString NowBlock="";
                bool flag=0;
                for(int j=0;j<Line.word[i].size();++j)
                {
                    ++k;
                    qreal floatX=0,floatY=0,Opacity=0;
                    QColor color;
                    if(setshake)
                    {
                        floatY=Amplitude*std::sin(round+Ylitter[k]);
                        floatX=Amplitude*std::sin(round+Xlitter[k]);
                    }
                    if(visibleLyric>0 &&positionMS-Line.wordstartMS[i]+FadeTime<0)
                    {
                        flag=1;
                        break;
                    }
                    if(setopacity)Opacity=std::clamp<qreal>((positionMS-Line.wordstartMS[i]+FadeTime)/FadeTime,0,1);//逐字淡入不透明度
                    QString Nowchar=Line.word[i].at(j);
                    const int CharX=fm.horizontalAdvance(Nowchar);//取得单字长度数据
                    QRect NowRect(X,Y,CharX,totalY);//内层字体旋转回调
                    const qreal AngelX=X+CharX/2;
                    const qreal AngelY=Y+totalY/2;
                    X+=CharX;
                    painter.save();
                    if(setopacity)painter.setOpacity(Opacity);
                    if(setshadow)
                    {
                        QColor color(51,51,51,150);//阴影色
                        painter.setPen(color);
                    }
                    painter.translate(AngelX,AngelY);
                    painter.rotate(-angel);
                    painter.translate(-AngelX,-AngelY);
                    if(setshake)
                    {
                        painter.translate(0.0,floatY);//字幕呼吸抖动
                        painter.translate(floatX,0.0);
                    }
                    if(setshadow) painter.drawText(NowRect.translated(3,3),Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//绘制阴影
                    color=palette().color(QPalette::WindowText);//读取颜色
                    if(!color.isValid()) color=Qt::white;//如果颜色无效，使用白色替代
                    painter.setPen(color);
                    painter.drawText(NowRect,Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//主文字画图
                    painter.restore();
                }
                if(flag==1)break;
            }
        }

        painter.restore();
    }
};
#endif // LYRICLABEL_H
