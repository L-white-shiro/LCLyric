#ifndef LYRIC_LABEL_H
#define LYRIC_LABEL_H

#include <QWidget>
class Lyriclabel:public QWidget
{

private:

public:
    Lyriclabel(){
        setAttribute(Qt::WA_TranslucentBackground,true);
    }
    void clear();
    void setText();

};
#endif // LYRIC_LABEL_H
