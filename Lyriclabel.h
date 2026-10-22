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
struct LyricBlock
{
    qsizetype end;
    qint64 startMS;
};
class Lyriclabel:public QWidget
{
private:
    QString L_text;
    QPointF L_current={0,0};
    QSizeF L_Size={0,0};
    qint64 positionMS=0;
    int visibleLyric=0;
    std::vector<qreal>litter;
    const std::vector<LyricBlock>* Block=nullptr;
public:
    qreal L_angle=0;
    bool setshake=0,setopacity=0,setshadow=0;
    int SDoffset=8;
    explicit Lyriclabel(QWidget *parent=nullptr):QWidget(parent){
        setAttribute(Qt::WA_TranslucentBackground,true);
    }
    void Ramdonlitter()
    {
            litter.clear();
            litter.resize(L_text.size());
            for(qreal& real:litter)
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
protected:
    void paintEvent(QPaintEvent *event) override
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
        qreal Amplitude=1.5;//抖动幅度
        qreal Frequency=0.5;//抖动频率
        qreal seconds=positionMS/1000.0;
        qreal round=2.0*3.1415926*Frequency*seconds;
        if(Block)
        {
            for(int i=0;i<std::min(visibleLyric,static_cast<int>(Block->size()));++i)
            {
                const qreal FadeTime=400;//淡入持续时间
                const qreal elapsedMS=positionMS-(*Block)[i].startMS;//相对字块经过时间
                QString NowBlock="";
                if(i>0)NowBlock=L_text.mid((*Block)[i-1].end,(*Block)[i].end-(*Block)[i-1].end);
                else NowBlock=L_text.mid(0,(*Block)[i].end);

                for(int j=0;j<NowBlock.size();++j)
                {
                    //  const qreal CharFadeTime=80;
                    // const qint64 charStartMs=(*Block)[i].startMS + qint64(j)*CharFadeTime;
                    // const qint64 age=positionMS-charStartMs;
                    // const qreal Opacity=std::clamp<qreal>(elapsedMS/CharFadeTime,0,1);//淡入不透明度设置
                    qreal floatY=0,Opacity=0;
                    QColor color;
                    if(setshake)
                    {
                    if(i>0)floatY=Amplitude*std::sin(round+litter[(*Block)[i-1].end+j]);
                    else floatY=Amplitude*std::sin(round+litter[j]);
                    }
                    if(setopacity)Opacity=std::clamp<qreal>(elapsedMS*(NowBlock.size()-j)/FadeTime,0,1);//逐字淡入不透明度
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
                    if(setshake)painter.translate(0.0,floatY);//字幕呼吸抖动
                    if(setshadow) painter.drawText(NowRect.translated(3,3),Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//绘制阴影
                    color=palette().color(QPalette::WindowText);//读取颜色
                    if(!color.isValid()) color=Qt::white;//如果颜色无效，使用白色替代
                    painter.setPen(color);
                    painter.drawText(NowRect,Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//主文字画图
                    painter.restore();
                }
            }
        }
        painter.restore();
    }
};
#endif // LYRICLABEL_H
