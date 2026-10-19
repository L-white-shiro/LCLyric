#include "lyric_label.h"

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