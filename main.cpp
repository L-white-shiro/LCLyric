#include<QApplication>
#include<QWidget>
#include<QLabel>
#include<QPushButton>
#include<QVBoxLayout>
#include<QHBoxLayout>
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
#include<QRandomGenerator>
#include<QFontMetrics>
#include<QColorDialog>
#include<QFontDialog>
#include<QLineEdit>
#include<QIntValidator>
#include<QStackedWidget>
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
    window.setWindowTitle("LCLyric");
    window.resize(1200, 300);
    QWidget LyricWindow;//歌词显示窗口
    QScreen* Screen=QGuiApplication::primaryScreen();//窗口查找
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
    auto* Top_Bar=new QWidget(&window);//顶部按钮栏
    auto* Top_layout=new QHBoxLayout(Top_Bar);
    auto* Top_button_main=new QPushButton("主页面",&window);//主页面切换按钮
    auto* Top_button_setting=new QPushButton("设置",&window);//设置页面切换按钮
    Top_Bar->setFixedHeight(60);
    Top_button_main->setMinimumSize(100,60);
    Top_button_setting->setMinimumSize(100,60);
    Top_layout->addWidget(Top_button_main);
    Top_layout->addWidget(Top_button_setting);
    Top_layout->addStretch();
    auto* Page_main=new QWidget(&window);//主页面
    auto* Page_setting=new QWidget(&window);//设置页面
    auto* Stack=new QStackedWidget(&window); //页面堆叠
    Stack->addWidget(Page_main);//index 0
    Stack->addWidget(Page_setting);//index 1
    QObject::connect(Top_button_setting,&QPushButton::clicked,&window,//翻设置页面
    [Stack,Top_button_main,Top_button_setting](){
        Stack->setCurrentIndex(1);
        Top_button_main->setEnabled(true);
        Top_button_setting->setEnabled(false);
    });
    QObject::connect(Top_button_main,&QPushButton::clicked,&window,//翻主页面
    [Stack,Top_button_main,Top_button_setting](){
        Stack->setCurrentIndex(0);
        Top_button_main->setEnabled(false);
        Top_button_setting->setEnabled(true);
    });
    Stack->setCurrentIndex(0);
    Top_button_main->setEnabled(false);
    QElapsedTimer ElapsedTimer;//计时器
    auto* label=new QLabel(&LyricWindow); // 主显示区
    label->setStyleSheet("font-weight:bold;");
    auto* button_in=new QPushButton("显示示例", Page_main); // 显示按钮
    auto* button_out=new QPushButton("清空", Page_main); // 清空页面
    auto* button_ColorChoose=new QPushButton("选择颜色",Page_setting);//颜色选择
    auto* button_FontChoose=new QPushButton("选择字体",Page_setting);//字体选择
    auto* Size_Input=new QLineEdit(Page_setting);//字号输入
    auto* timer=new QTimer(&window); //超时器
    auto* OpenFile=new QPushButton("打开文件",Page_main);//文件打开按钮
    auto* LyricWord_count=new QLabel(QString::number(visibleLyric), Page_main); // 测试用歌词字数展示
    auto* LyricLine_count=new QLabel(QString::number(0),Page_main); // 测试用歌词行数目展示
    QObject::connect(OpenFile,&QPushButton::clicked,Page_main,
    [&window,timer,&visibleLyric,label,LyricWord_count,&Lines,&visibleLyricLine,LyricLine_count]() { //打开文件
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
    [&visibleLyric,label,timer,LyricWord_count,&visibleLyricLine,&Lines,&ElapsedTimer,LyricLine_count,&LyricWindow]() { //超时器信号槽
        int i=-1;//行筛选
        qint64 TIMESCAN=ElapsedTimer.elapsed();
        for(std::size_t I=0;I<Lines.size();++I)
        {
            if(Lines[I].Time>TIMESCAN)break;
            i=I;
        }
        if(i==-1)return;
        bool IfChange;//换行设定随机歌词位置
        if(visibleLyricLine!=i)IfChange=1;
        else IfChange=0;
        visibleLyricLine=i;//有效筛选，进入播放环节
        if(IfChange)
        {
            QFontMetrics XY(label->font());
            int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context);
            int Y=XY.height();
            label->resize(X,Y);
            const int RandomX=QRandomGenerator::global()->bounded(std::max(0,LyricWindow.width()-X)+1);
            const int RandomY=QRandomGenerator::global()->bounded(std::max(0,LyricWindow.height()-Y)+1);
            label->move(RandomX,RandomY);
        }
        qint64 Line_Elapsed=TIMESCAN-Lines[visibleLyricLine].Time;//开始时间
        qint64 LineWord_display=std::min(Line_Elapsed/150+1,Lines[visibleLyricLine].Context.size());//目前行歌词进度
        label->setText(Lines[visibleLyricLine].Context.left(LineWord_display));
        LyricWord_count->setText(QString::number(LineWord_display));
        LyricLine_count->setText(QString::number(visibleLyricLine+1));
        visibleLyric=int(LineWord_display);
        if(Lines.size()!=0 && visibleLyricLine+1>=Lines.size() && LineWord_display>=Lines[visibleLyricLine].Context.size())
        {
            timer->stop();
            return;
        }
    });
    int Size=20;//字号初始化
    QFont font=label->font();
    font.setPointSize(Size);
    label->setFont(font);
    Size_Input->setText(QString::number(Size));//字号输入
    auto* Size_validator=new QIntValidator(8,96,Size_Input);//字号验证器
    Size_Input->setValidator(Size_validator);
    QObject::connect(Size_Input,&QLineEdit::returnPressed,Page_setting,//字号更改
    [label,Size_Input,&Size,&Lines,&visibleLyricLine,&LyricWindow]{
        if(!Size_Input->hasAcceptableInput())return;
        Size=Size_Input->text().toInt();
        QFont font=label->font();
        font.setPointSize(Size);
        label->setFont(font);
        if(visibleLyricLine!=-1 && static_cast<std::size_t>(visibleLyricLine)<Lines.size())
        {
        QFontMetrics XY(label->font());
        int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context);
        int Y=XY.height();
        label->resize(X,Y);
        const int MaxX=std::max(0,LyricWindow.width()-X);//边界条件判断以及纠正
        const int MaxY=std::max(0,LyricWindow.height()-Y);
        const int SafeX=std::min(label->x(),MaxX);
        const int SafeY=std::min(label->y(),MaxY);
        label->move(SafeX, SafeY);
        }
    });
    QObject::connect(button_FontChoose,&QPushButton::clicked,Page_setting,
    [&window,label,&Lines,&visibleLyricLine,&LyricWindow](){//字体选择
        bool Accept=0;
        QFont SelectFont=QFontDialog::getFont(&Accept,label->font(),&window,"选择字体");
            if(!Accept)return;
            if(visibleLyricLine!=-1 && static_cast<std::size_t>(visibleLyricLine)<Lines.size())
            {
            qDebug()<<"true";
            label->setFont(SelectFont);
            QFontMetrics XY(label->font());
            int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context);
            int Y=XY.height();
            label->resize(X,Y);
            const int MaxX=std::max(0, LyricWindow.width()-X);//边界条件判断以及纠正
            const int MaxY=std::max(0, LyricWindow.height()-Y);
            const int SafeX=std::min(label->x(),MaxX);
            const int SafeY=std::min(label->y(),MaxY);
            label->move(SafeX, SafeY);
            }
            else label->setFont(SelectFont);
    });
    QObject::connect(button_ColorChoose,&QPushButton::clicked,Page_setting,
    [&window,label](){//颜色选择
        QColor CurrentColor=label->palette().color(QPalette::WindowText);
        QColor SelectColor=QColorDialog::getColor(CurrentColor,&window,"选择颜色");
        if(!SelectColor.isValid())return;
        QPalette Palette=label->palette();//拷贝副本
        Palette.setColor(QPalette::WindowText,SelectColor);//改副本
        label->setPalette(Palette);//赋值
    });
    QObject::connect(button_in,&QPushButton::clicked,Page_main,
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
    QObject::connect(button_out,&QPushButton::clicked,Page_main,
    [label,&visibleLyric,timer,LyricWord_count,&visibleLyricLine,LyricLine_count]() {//clear清屏
        timer->stop();
        label->clear();
        visibleLyric=0;
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    auto* layout=new QVBoxLayout(&window);
    auto* layout_Page_main=new QVBoxLayout(Page_main);
    auto* layout_Page_setting=new QVBoxLayout(Page_setting);
    auto* layout_Page_setting_size = new QHBoxLayout;
    layout->addWidget(Top_Bar);
    layout->addWidget(Stack);
    layout_Page_main->addWidget(LyricWord_count);
    layout_Page_main->addWidget(LyricLine_count);
    layout_Page_main->addWidget(OpenFile);
    layout_Page_main->addWidget(button_in);
    layout_Page_main->addWidget(button_out);
    layout_Page_setting->addWidget(button_ColorChoose);
    layout_Page_setting->addWidget(button_FontChoose);
    auto* Size_label=new QLabel("字号(pt):",Page_setting);
    layout_Page_setting_size->addWidget(Size_label);
    layout_Page_setting_size->addWidget(Size_Input);
    layout_Page_setting_size->addStretch();
    layout_Page_setting->addLayout(layout_Page_setting_size);
    LyricWindow.show();
    window.show(); // NOLINT
    return app.exec();
}