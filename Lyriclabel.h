#ifndef LYRICLABEL_H
#define LYRICLABEL_H
#include<QWidget>
#include<QPainter>
class Lyriclabel:public QWidget
{
private:
    QString m_text;
public:
    int SDoffset=8;
    explicit Lyriclabel(QWidget *parent=nullptr):QWidget(parent){
        setAttribute(Qt::WA_TranslucentBackground,true);
    }
    void clear(){
        m_text.clear();
        update();
    };
    void setText(const QString& text)
    {
        m_text=text;
        update();
    }
protected:
    void paintEvent(QPaintEvent *event) override
    {
        if(m_text.isEmpty())return;
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);//几何抗锯齿
        painter.setRenderHint(QPainter::TextAntialiasing);//文字抗锯齿

        painter.setFont(font());//调用字体
        QFontMetrics fm(font());//获取字体信息
        const int totalX=fm.horizontalAdvance(m_text);//取得整行长数据
        const int totalY=fm.height();//宽数据
        int X=SDoffset;
        int Y=SDoffset;
        int YS=(height()-totalY)/2;
        for(int i=0;i<m_text.size();++i)
        {
            QColor color(51,51,51,50);//阴影色
            painter.setPen(color);
            QString Nowchar=m_text.at(i);
            const int CharX=fm.horizontalAdvance(Nowchar);//取得单字长度数据
            QRect NowRect(X,0,CharX,totalY);
            X+=CharX;
            painter.drawText(NowRect.translated(3,3),Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//绘制阴影

            color=palette().color(QPalette::WindowText);//读取颜色
            if(!color.isValid()) color=Qt::white;//如果颜色无效，使用白色替代
            painter.setPen(color);
            painter.drawText(NowRect,Qt::AlignLeft|Qt::AlignVCenter,Nowchar);//主文字画图
        }

    }
};
#endif // LYRICLABEL_H
