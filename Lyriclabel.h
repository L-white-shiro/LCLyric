#ifndef LYRICLABEL_H
#define LYRICLABEL_H
#include<QWidget>
#include<QPainter>
#include<QPoint>
#include<QRandomGenerator>
class Lyriclabel:public QWidget
{
private:
    QString L_text;
    QPointF L_current={0,0};
    QSizeF L_Size={0,0};
public:
    qreal L_angle=0;
    int SDoffset=8;
    explicit Lyriclabel(QWidget *parent=nullptr):QWidget(parent){
        setAttribute(Qt::WA_TranslucentBackground,true);
    }
    void clear(){
        L_text.clear();
        update();
    };
    void setText(const QString& text)
    {
        L_text=text;
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
protected:
    void paintEvent(QPaintEvent *event) override
    {
        if(L_text.isEmpty())return;
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);//几何抗锯齿
        painter.setRenderHint(QPainter::TextAntialiasing);//文字抗锯齿
        painter.setFont(font());//调用字体
        QFontMetrics fm(font());//获取字体信息
        const int totalX=fm.horizontalAdvance(L_text);//取得整行长数据
        const int totalY=fm.height();//宽数据
        int X=qRound(L_current.x())+SDoffset;//起点XY轴
        int Y=qRound(L_current.y())+SDoffset;
        const qreal angel=L_angle;//旋转角度快照
        painter.save();
        const qreal centralX=L_current.x()+L_Size.width()/2;//行旋转中心点
        const qreal centralY=L_current.y()+L_Size.height()/2;
        painter.translate(centralX,centralY);
        painter.rotate(angel);
        painter.translate(-centralX,-centralY);
        for(int i=0;i<L_text.size();++i)
        {
            QColor color(51,51,51,50);//阴影色
            painter.setPen(color);
            QString Nowchar=L_text.at(i);
            const int CharX=fm.horizontalAdvance(Nowchar);//取得单字长度数据
            QRect NowRect(X,Y,CharX,totalY);//内层字体旋转回调
            const qreal AngelX=X+CharX/2;
            const qreal AngelY=Y+totalY/2;
            X+=CharX;
            painter.save();
            painter.translate(AngelX,AngelY);
            painter.rotate(-angel);
            painter.translate(-AngelX,-AngelY);
            painter.drawText(NowRect.translated(3,3),Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//绘制阴影

            color=palette().color(QPalette::WindowText);//读取颜色
            if(!color.isValid()) color=Qt::white;//如果颜色无效，使用白色替代
            painter.setPen(color);
            painter.drawText(NowRect,Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//主文字画图
            painter.restore();
        }
        painter.restore();
    }
};
#endif // LYRICLABEL_H
