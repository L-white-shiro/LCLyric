#include "main.h"
#include "Lyriclabel.h"
#include "Network.h"
struct Lyric_Line;
struct LyricBlock;
QRegularExpression Pattern_nonms(
    R"(^\[(\d{2}):([0-5]\d)\](.*)$)"
    );
QRegularExpression Pattern_non_00ms(
    R"(^\[(\d{2}):([0-5]\d)\.(\d{2})\](.*)$)"
    );
QRegularExpression Pattern_non_000ms(
    R"(^\[(\d{2}):([0-5]\d)\.(\d{3})\](.*)$)"
    );
QRegularExpression Pattern_linefirst(
    R"(^\[(\d+),(\d+)\](.*)$)"
    );
QRegularExpression Pattern_everyword(
    R"(\((\d+),(\d+),(\d+)\)([^\(]*))"
    );
using SessionManager=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using Media=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;
using Playbackstatus=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
QThread* Thread=nullptr;
int main(int argc, char* argv[])
{
    int visibleLyric=0;
    int visibleLyricLine=-1;
    std::vector<Lyric_Line> Lines;//动态数组歌词行
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("LCLyric");//设置应用名称
    QWidget window;//顶层窗口
    window.setWindowTitle("LCLyric");
    window.resize(1200, 300);
    QString ConfigDir=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);//应用目录查找存入
    bool creator=QDir().mkpath(ConfigDir);//创建目录
    if(!creator)qDebug()<<"目录创建失败";
    QString FilePath=QDir(ConfigDir).filePath("setting.json");//拼接文件路径
    QWidget LyricWindow;//歌词显示窗口
    QScreen* Screen=QGuiApplication::primaryScreen();//窗口查找

    if(Screen==nullptr)
    {
        qDebug()<<"无可用主页面";
        return 1;
    }
    LyricWindow.setGeometry(Screen->geometry());
    LyricWindow.setWindowFlags(
        Qt::WindowTransparentForInput |
    //    Qt::Window | //需要屏幕录制时开启
        Qt::Tool |//需要屏幕录制时隐藏
        Qt::FramelessWindowHint |
        Qt::WindowStaysOnTopHint
        );
   // LyricWindow.setAttribute(Qt::WA_QuitOnClose,false);//需要屏幕录制时开启
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
    auto* Stack=new QStackedWidget(&window);//页面堆叠
    Stack->addWidget(Page_main);//index 0
    Stack->addWidget(Page_setting);//index 1
    QElapsedTimer ElapsedTimer;//计时器
    auto* label=new Lyriclabel(&LyricWindow); //主显示区
    auto* button_in=new QPushButton("开始",Page_main); //显示按钮
    auto* button_out=new QPushButton("暂停",Page_main); //清空页面
    auto* button_ColorChoose=new QPushButton("选择颜色",Page_setting);//颜色选择
    auto* button_FontChoose=new QPushButton("选择字体",Page_setting);//字体选择
    auto* Size_Input=new QLineEdit(Page_setting);//字号输入
    auto* timer=new QTimer(&window); //超时器
    auto* OpenFile=new QPushButton("本地歌词读取",Page_main);//文件打开按钮
    auto* LyricWord_count=new QLabel(QString::number(visibleLyric),Page_main); //测试用歌词字数展示
    auto* LyricLine_count=new QLabel(QString::number(0),Page_main); //测试用歌词行数目展示
    auto* Lyricshake=new QCheckBox("文字抖动",Page_setting);//字体抖动按钮
    auto* LyricOpacity=new QCheckBox("文字淡入",Page_setting);//淡入按钮
    auto* LyricShadow=new QCheckBox("文字阴影",Page_setting);//阴影按钮
    qint64 SmtcPositionMS=0,Smtctimeline=0;//Smtc接收
    QString SmtcArtist="",SmtcSongsname="",LastSong="";
    bool SmtcPositionchange=0,Smtcsongschange=0;
    bool lyricPlaying=0,Playing=0;//歌曲播放状态
    auto Waiting=new QTimer(&window);
    Waiting->setSingleShot(1);//单次触发，设置切歌防抖避免多次访问
    qint64 lasttime=-1;//超时器用变量
    int Skip=0,flag=0;
    qint64 lastDisplayTime=0,previousI=0,previousword=0;
    std::vector<LyricBlock>lyricblock;//歌词块切分
    auto Thread=QThread::create([&window,&SmtcPositionMS,&SmtcPositionchange,&Smtctimeline,&lyricPlaying
    ,&SmtcArtist,&SmtcSongsname,&LastSong,&Smtcsongschange,&Playing,timer,label,Waiting]()//SMTC
    {
    bool init=false;
    try{
        winrt::init_apartment();
        qDebug()<<"WinRT initialization succeeded\n";
        init=true;
        {
            auto manager=SessionManager::RequestAsync().get();//get获取管理器
            while(!QThread::currentThread()->isInterruptionRequested())
            {
                lyricPlaying=0;
                auto sessionall=manager.GetSessions();//获取最优会话
                for(const auto& session:sessionall)
                {
                    bool change=0;
                    qint64 positionMS=0;
                    if(winrt::to_string(session.SourceAppUserModelId())=="cloudmusic.exe")
                    {
                        if(!session)//非空检查
                        {
                            qDebug()<<"NO current session\n";
                        }
                        else
                        {
                            try
                            {
                                auto appName=session.SourceAppUserModelId();//获取会话名
                                auto propertiesOperation=session.TryGetMediaPropertiesAsync();//异步请求会话属性
                                auto properties=propertiesOperation.get();//获取会话属性
                                auto title=QString::fromStdString(winrt::to_string(properties.Title()));//歌曲名获取
                                QString artist=QString::fromStdString(winrt::to_string(properties.Artist()));//歌手信息获取
                                auto playbackinfo=session.GetPlaybackInfo();//播放状态获取
                                if(playbackinfo)
                                {
                                    auto status=playbackinfo.PlaybackStatus();
                                    if(status==Playbackstatus::Playing)
                                    {
                                        lyricPlaying=1;
                                    }
                                    else if(status==Playbackstatus::Paused)
                                    {
                                        lyricPlaying=0;
                                    }
                                    else if(status==Playbackstatus::Stopped)
                                    {
                                        lyricPlaying=0;
                                    }
                                    else lyricPlaying=0;
                                }
                                auto timeline=session.GetTimelineProperties();//获取时间线
                                if(timeline>0)
                                {
                                    change=1;
                                    auto position=timeline.Position();
                                    auto endtime=timeline.EndTime();
                                    positionMS=std::chrono::duration_cast<std::chrono::milliseconds>(position).count();//毫秒转换
                                    auto endtimeMS=std::chrono::duration_cast<std::chrono::milliseconds>(endtime).count();
                                    if(endtimeMS>0)
                                    {
                                        QMetaObject::invokeMethod(&window,
                                        [&SmtcPositionMS,&SmtcPositionchange,&Smtctimeline,&SmtcArtist,&SmtcSongsname,&Smtcsongschange,lyricPlaying
                                        ,change,positionMS,endtimeMS,artist,title,&LastSong,&Playing,timer,label,Waiting]{
                                            SmtcPositionMS=positionMS;
                                            SmtcPositionchange=change;
                                            Smtctimeline=endtimeMS;
                                            SmtcArtist=artist;
                                            SmtcSongsname=title;
                                            Playing=lyricPlaying;
                                            if(LastSong!=title)
                                            {
                                                timer->stop();
                                                label->clear();
                                                Smtcsongschange=1;
                                                LastSong=title;
                                                Waiting->start(1500);//waiting连续切歌防抖
                                            }
                                        },
                                        Qt::QueuedConnection);
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                            }
                            catch(const winrt::hresult_error& error)
                            {
                                qDebug()<<QString::fromStdString(winrt::to_string(error.message()));
                            }
                        }
                    }
                }
                QThread::msleep(200);
            }
        }
        winrt::uninit_apartment();
        return;
    }
    catch(const winrt::hresult_error& error){
        if(init==true)winrt::uninit_apartment();
        qDebug()<<winrt::to_string(error.message())<<"\n";
        return;
    }
    });
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
    auto ParseStream=[](QTextStream &stream)->std::vector<Lyric_Line>{//文件流输入
        std::vector<Lyric_Line>Temp_Lyric;
        while(!stream.atEnd())
        {
            QString Line=stream.readLine().trimmed();
            auto match=Pattern_linefirst.match(Line);
            if(match.hasMatch())
            {
                Lyric_Line Lyric_Now;
                Lyric_Now.isYRC=1;
                Lyric_Now.Time=match.captured(1).toLongLong();
                Lyric_Now.Duration=match.captured(2).toLongLong();
                QRegularExpressionMatchIterator it=Pattern_everyword.globalMatch(match.captured(3));//行外逐字匹配
                int i=0;
                while(it.hasNext())
                {
                    QRegularExpressionMatch m=it.next();
                    Lyric_Now.wordstartMS[i]=m.captured(1).toLongLong();
                    Lyric_Now.word[i]=m.captured(4);
                    Lyric_Now.Context+=Lyric_Now.word[i];
                    ++i;
                }
                Lyric_Now.block=i;
                Temp_Lyric.push_back(Lyric_Now);
            }
            else
            {
                auto match=Pattern_non_00ms.match(Line);
                if(match.hasMatch())
                {
                    Lyric_Line Lyric_Now;
                    Lyric_Now.Time=60000LL*match.captured(1).toInt()+1000LL*match.captured(2).toInt()+match.captured(3).toInt()*10LL;
                    Lyric_Now.Context=match.captured(4);
                    Temp_Lyric.push_back(Lyric_Now);
                }
                else
                {
                    auto match=Pattern_non_000ms.match(Line);
                    if(match.hasMatch())
                    {
                        Lyric_Line Lyric_Now;
                        Lyric_Now.Time=60000LL*match.captured(1).toInt()+1000LL*match.captured(2).toInt()+match.captured(3).toInt()*1LL;
                        Lyric_Now.Context=match.captured(4);
                        Temp_Lyric.push_back(Lyric_Now);
                    }
                    else
                    {
                        auto match=Pattern_nonms.match(Line);
                        if(match.hasMatch())
                        {
                            Lyric_Line Lyric_Now;
                            Lyric_Now.Time=60000LL*match.captured(1).toInt()+1000LL*match.captured(2).toInt();
                            Lyric_Now.Context=match.captured(3);
                            Temp_Lyric.push_back(Lyric_Now);
                        }
                    }
                }
            }
        }
        std::stable_sort(Temp_Lyric.begin(),Temp_Lyric.end(),[](const Lyric_Line& a,const Lyric_Line& b){return a.Time<b.Time;});

        return Temp_Lyric;
    };
    auto rotatedNew=[](qreal W,qreal H,qreal deg,qreal& Wr,qreal& Hr) {//计算文字框大小
        const qreal rad=qDegreesToRadians(deg);
        const qreal c=std::abs(std::cos(rad));
        const qreal s=std::abs(std::sin(rad));
        Wr=W*c+H*s;
        Hr=W*s+H*c;
    };
    QObject::connect(OpenFile,&QPushButton::clicked,Page_main,//打开文件
    [&window,timer,&visibleLyric,label,LyricWord_count,&Lines,&visibleLyricLine,LyricLine_count,ParseStream,
    &previousI,&previousword,&lasttime,&lastDisplayTime,&flag,&lyricblock]() {
        QString File=QFileDialog::getOpenFileName(&window,"选择文件",QString(),"文本文件(*.txt);;歌词文件(*.lrc)");
        if(File.isEmpty())return;
        QFile file(File);
        if(!file.open(QIODevice::ReadOnly))//打开审查
        {
            qDebug()<<file.errorString();
            return;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);//文件流转码
        std::vector<Lyric_Line>Temp_Lyric=ParseStream(stream);
        file.close();

        if(Temp_Lyric.empty())return;
        //重置状态
        timer->stop();
        visibleLyric=0;
        label->clear();
        previousI=0;
        previousword=0;
        lasttime=-1;
        lastDisplayTime=0;
        flag=0;
        lyricblock.clear();
        Lines=std::move(Temp_Lyric);
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    bool hasplay=0;//歌曲进行中判断，避免用户回退误判
    auto* topmostTimer=new QTimer(&window);
    topmostTimer->setInterval(3000);
    QObject::connect(topmostTimer,&QTimer::timeout,&window,//置顶同步
    [&LyricWindow](){
    LyricWindow.setWindowFlags(
    Qt::WindowTransparentForInput |
    //Qt::Window | //需要屏幕录制时开启
    Qt::Tool |//需要屏幕录制时隐藏
    Qt::FramelessWindowHint |
    Qt::WindowStaysOnTopHint
    );
    LyricWindow.show();
    });
    QObject::connect(timer,&QTimer::timeout,&window,//超时器信号槽
    [&visibleLyric,label,timer,LyricWord_count,&visibleLyricLine,&Lines,&flag,&lyricblock,&hasplay,
    &ElapsedTimer,LyricLine_count,&LyricWindow,&SmtcPositionMS,&SmtcPositionchange,&lasttime,&Playing,rotatedNew,&Skip,&lastDisplayTime,&previousI,&previousword]() {
        if(Playing!=1)
        {
            return;
        }
        int i=-1;//行筛选
        if(SmtcPositionchange)
        {
            if(lasttime<0 || lasttime!=SmtcPositionMS)
            {
            if(SmtcPositionMS<lastDisplayTime && lastDisplayTime-SmtcPositionMS<300 &&
            previousI>=0 && static_cast<std::size_t>(previousI)<Lines.size())//小幅回退检测
            previousword=(lastDisplayTime-Lines[previousI].Time)/150+1;
            lasttime=SmtcPositionMS;
            ElapsedTimer.restart();
            }
            SmtcPositionchange=0;
        }
        qint64 TIMESCAN=ElapsedTimer.elapsed()+SmtcPositionMS;//处理时间戳
        if(hasplay && TIMESCAN<lastDisplayTime)//smtc同步回退判定
        {
            const qint64 backtime=lastDisplayTime-SmtcPositionMS;
            if(backtime<700)//700+认定为用户回退，否则为smtc同步回退
            {
                TIMESCAN=lastDisplayTime;
            }
        }
        lastDisplayTime=TIMESCAN;
        label->setposition(TIMESCAN);
        hasplay=1;
        for(std::size_t I=0;I<Lines.size();++I)
        {
            if(Lines[I].Time>TIMESCAN)break;
            i=I;
        }
        if(previousI>=0 && static_cast<std::size_t>(previousI)<Lines.size() && i<previousI && (Lines[previousI].Time-TIMESCAN)<1000)i=previousI;//保护回调
        previousI=i;
        if(i==-1)
        {
            label->clear();
            return;
        }
        bool IfChange;//换行设定随机歌词位置
        if(visibleLyricLine!=i)
        {
            previousword=0;
            IfChange=1;
            flag=1;
        }
        else IfChange=0;
        visibleLyricLine=i;//有效筛选，进入播放环节
        QString allLine=Lines[visibleLyricLine].Context;
        if(allLine.isEmpty())
        {
            label->clear();
            lyricblock.clear();
            return;
        }
        if(IfChange)
        {
            hasplay=0;
            label->L_angle=QRandomGenerator::global()->bounded(-10, 10);
            QFontMetrics XY(label->font());
            const qreal bx=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;//带边界XY值
            const qreal by=XY.height()+2*label->SDoffset;
            label->resize(LyricWindow.size());
            label->move(0, 0);
            label->setsize(QSizeF(bx, by));
            qreal Wr,Hr;
            rotatedNew(bx,by,label->L_angle,Wr,Hr);//计算实际宽高
            const qreal SW=LyricWindow.width();
            const qreal SH=LyricWindow.height();
            const qreal minCx=Wr/2;
            const qreal maxCx=SW-Wr/2;
            const qreal minCy=Hr/2;
            const qreal maxCy=SH-Hr/2;
            qreal Cx,Cy;
            if(maxCx>=minCx) Cx=minCx+QRandomGenerator::global()->generateDouble()*(maxCx-minCx);//合法范围生成随机坐标
            else  Cx=SW/2;
            if(maxCy>=minCy) Cy=minCy+QRandomGenerator::global()->generateDouble()*(maxCy-minCy);
            else Cy=SH/2;

            label->setorigin(QPointF(Cx-bx/2,Cy-by/2));
        }
        if(flag)
        {
            lyricblock.clear();
            LyricBlock TEMP;
            qsizetype I=0;
            while(I<allLine.size())
            {
                QChar nextChar=allLine.at(I);
                if(nextChar.isLetter() && nextChar.unicode()<128)
                {
                    while(I<allLine.size())
                    {
                        const QChar char1=allLine.at(I);
                        if (!char1.isLetter() || char1.unicode()>=128)break;
                        ++I;
                    }
                }
                else if(nextChar=='(' && I>0)
                {
                    while(I<allLine.size())
                    {
                        const QChar char1=allLine.at(I);
                        ++I;
                        if (char1==')')break;
                    }
                }
                else if(nextChar==u'（' && I>0)
                {
                    while(I<allLine.size())
                    {
                        const QChar char1=allLine.at(I);
                        ++I;
                        if (char1==u'）')break;
                    }
                }
                else if(nextChar==u' ')
                {
                    while(I<allLine.size())
                    {
                        const QChar char1=allLine.at(I);
                        ++I;
                        if (char1== u' ')break;
                    }
                }
                else ++I;
                TEMP.end=I;
                TEMP.startMS=Lines[visibleLyricLine].Time+lyricblock.size()*150;
                lyricblock.push_back(TEMP);
            }
        }
        flag=0;
        qint64 Line_Elapsed=TIMESCAN-Lines[visibleLyricLine].Time;//开始时间
        qint64 LineWord_display=std::max(qint64(0),std::min(std::max(Line_Elapsed/150+1,previousword),qint64(lyricblock.size())));//目前行歌词进度
        int changecompare=visibleLyric;
        visibleLyric=int(LineWord_display);
        if(changecompare!=visibleLyric && visibleLyric!=0 || IfChange)
        {
            if(Lines[visibleLyricLine].isYRC)label->setLyricLine(Lines[visibleLyricLine]);
            else label->setText(allLine,lyricblock,visibleLyric);
            LyricWord_count->setText(QString::number(lyricblock[visibleLyric-1].end));
            LyricLine_count->setText(QString::number(visibleLyricLine+1));
        }
        if(visibleLyricLine+1>=qsizetype(Lines.size()) && visibleLyric>=lyricblock.size() && TIMESCAN-Lines[visibleLyricLine].Time>7000)
        {
            label->clear();
            return;
        }
    });
    int Size=20;//字号初始化
    QFont defaultFont=label->font();
    defaultFont.setPointSize(Size);
    label->setFont(defaultFont);
    Size_Input->setText(QString::number(Size));
    auto* button_file=new QPushButton("输出",Page_setting);
    button_file->hide();
    Thread->setParent(&app);
    Thread->start();
    QObject::connect(button_file,&QPushButton::clicked,Page_setting,[label,FilePath,LyricShadow,Lyricshake,LyricOpacity](){//保存用户设置
        QSaveFile file(FilePath);
        QString Fonttext=label->font().toString();//截取字体信息
        QColor Color=label->palette().color(QPalette::WindowText);//截取颜色信息
        QString ColorText=Color.name(QColor::HexRgb);
        bool shadow=LyricShadow->isChecked();
        bool shake=Lyricshake->isChecked();
        bool opacity=LyricOpacity->isChecked();
        QJsonObject Object;//Json对象
        Object.insert("Version",1);//组织json字段
        Object.insert("Font",Fonttext);
        Object.insert("Color",ColorText);
        Object.insert("LyricShadow",shadow);
        Object.insert("LyricShake",shake);
        Object.insert("LyricOpacity",opacity);
        QJsonDocument document(Object);//Json文件打包对象
        QByteArray Byte=document.toJson(QJsonDocument::Indented);
        bool opened=file.open(QIODevice::WriteOnly);
        if(!opened)
        {
            qDebug()<<"打开失败";
            return;
        }
        qint64 written=file.write(Byte);
        if(written!=Byte.size())//写入字节完整性检查
        {
            qDebug()<<"保存失败"<<file.errorString();
            return;
        }
        bool saved=file.commit();
        if(!saved)//提交成功性检查
        {
            qDebug()<<"保存失败"<<file.errorString();
            return;
        }
        qDebug()<<"保存成功";
    });
    auto LoadSetting=[&]()//读取用户设置
    {
        QFile configFile(FilePath);
        if(!configFile.exists())//无配置json文件
        {
            qDebug()<<"无对应配置文件";
            return;
        }
        if(!configFile.open(QIODevice::ReadOnly))//只读打开失败
        {
            qDebug()<<"只读打开失败"<<configFile.errorString();
            return;
        }
        QByteArray Byte=configFile.readAll();
        if(configFile.error()!=QFileDevice::NoError)//打开失败
        {
            qDebug()<<"读取失败"<<configFile.errorString();
            return;
        }
        QJsonParseError ParseError;
        QJsonDocument document=QJsonDocument::fromJson(Byte,&ParseError);
        if(ParseError.error!=QJsonParseError::NoError)//解析json错误
        {
            qDebug()<<"json解析失败"<<ParseError.errorString();
            return;
        }
        if(!document.isObject())
        {
            qDebug()<<"对象错误";
            return;
        }
        QJsonObject Object=document.object();
        QJsonValue versionValue=Object.value("Version");
        if(!versionValue.isDouble() || versionValue.toDouble()!=1)//字体颜色格式检查
        {
            qDebug()<<"版本错误";
            return;
        }
        QJsonValue fontValue=Object.value("Font");
        if(!fontValue.isString())
        {
            qDebug()<<"字体非字符串";
            return;
        }
        QJsonValue colorValue=Object.value("Color");
        if(!colorValue.isString())
        {
            qDebug()<<"颜色错误";
            return;
        }
        QJsonValue shadowValue=Object.value("LyricShadow");
        if(!shadowValue.isBool())
        {
            qDebug()<<"阴影格式错误";
            return;
        }
        QJsonValue shakeValue=Object.value("LyricShake");
        if(!shakeValue.isBool())
        {
            qDebug()<<"抖动格式错误";
            return;
        }
        QJsonValue opacityValue=Object.value("LyricOpacity");
        if(!opacityValue.isBool())
        {
            qDebug()<<"淡入格式错误";
            return;
        }
        QFont Fontnew=label->font();
        bool fontchange=Fontnew.fromString(fontValue.toString());
        if(fontchange && Fontnew.pointSize()>=8 && Fontnew.pointSize()<=96)//字号大小，写入成功检查
        {
            label->setFont(Fontnew);
            Size=Fontnew.pointSize();
            Size_Input->setText(QString::number(Fontnew.pointSize()));
        }
        QColor colornew(colorValue.toString());
        if(colornew.isValid())//颜色检查
        {
            QPalette setcolor=label->palette();
            setcolor.setColor(QPalette::WindowText,colornew);
            label->setPalette(setcolor);
        }
        LyricShadow->setChecked(shadowValue.toBool());
        LyricOpacity->setChecked(opacityValue.toBool());
        Lyricshake->setChecked(shakeValue.toBool());
        label->update();
    };
    auto* Size_validator=new QIntValidator(8,96,Size_Input);//字号验证器
    Size_Input->setValidator(Size_validator);
    QObject::connect(Size_Input,&QLineEdit::returnPressed,Page_setting,//字号更改
    [label,Size_Input,&Size,&Lines,&visibleLyricLine,&LyricWindow,button_file]{
        if(!Size_Input->hasAcceptableInput())return;
        Size=Size_Input->text().toInt();
        QFont font=label->font();
        font.setPointSize(Size);
        label->setFont(font);
        if(visibleLyricLine!=-1 && static_cast<std::size_t>(visibleLyricLine)<Lines.size())
        {
        QFontMetrics XY(label->font());
        int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;
        int Y=XY.height()+2*label->SDoffset;
        label->resize(LyricWindow.size());
        const int MaxX=std::max(0,LyricWindow.width()-X);//边界条件判断以及纠正
        const int MaxY=std::max(0,LyricWindow.height()-Y);
        const int SafeX=std::min(label->x(),MaxX);
        const int SafeY=std::min(label->y(),MaxY);
        label->move(SafeX, SafeY);
        }
        button_file->click();
    });
    QObject::connect(button_FontChoose,&QPushButton::clicked,Page_setting,
    [&window,label,&Lines,&visibleLyricLine,&LyricWindow,button_file](){//字体选择
        bool Accept=0;
        QFont SelectFont=QFontDialog::getFont(&Accept,label->font(),&window,"选择字体");
            if(!Accept)return;
            if(visibleLyricLine!=-1 && static_cast<std::size_t>(visibleLyricLine)<Lines.size())
            {
            qDebug()<<"true";
            label->setFont(SelectFont);
            QFontMetrics XY(label->font());
            int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;
            int Y=XY.height()+2*label->SDoffset;
            label->resize(LyricWindow.size());
            label->move(0,0);
            const int MaxX=std::max(0, LyricWindow.width()-X);//边界条件判断以及纠正
            const int MaxY=std::max(0, LyricWindow.height()-Y);
            const int SafeX=std::min(label->x(),MaxX);
            const int SafeY=std::min(label->y(),MaxY);
            }
        else label->setFont(SelectFont);
        button_file->click();
    });
    QObject::connect(button_ColorChoose,&QPushButton::clicked,Page_setting,
    [&window,label,button_file](){//颜色选择
        QColor CurrentColor=label->palette().color(QPalette::WindowText);
        QColor SelectColor=QColorDialog::getColor(CurrentColor,&window,"选择颜色");
        if(!SelectColor.isValid())return;
        QPalette Palette=label->palette();//拷贝副本
        Palette.setColor(QPalette::WindowText,SelectColor);//改副本
        label->setPalette(Palette);//赋值
        button_file->click();
    });
    QObject::connect(Lyricshake,&QCheckBox::toggled,[&label,button_file](bool checked){//文字抖动信号槽
        if(checked==1)label->setshake=1;
        else label->setshake=0;
        label->update();
        button_file->click();
    });
    QObject::connect(LyricOpacity,&QCheckBox::toggled,[&label,button_file](bool checked){//文字淡入信号槽
        if(checked==1)label->setopacity=1;
        else label->setopacity=0;
        label->update();
        button_file->click();
    });
    QObject::connect(LyricShadow,&QCheckBox::toggled,[&label,button_file](bool checked){//文字淡入信号槽
        if(checked==1)label->setshadow=1;
        else label->setshadow=0;
        label->update();
        button_file->click();
    });
    QObject::connect(button_in,&QPushButton::clicked,Page_main,
    [label,&visibleLyric,timer,LyricWord_count,&ElapsedTimer,&visibleLyricLine,LyricLine_count,&Lines,&lasttime
    ,&previousI,&previousword,&lastDisplayTime,&flag,&lyricblock]() { // 按钮1信号槽
        timer->stop();
        if(Lines.empty()) // 防止段错误，边界检查
        {
        qDebug()<<"无歌词";
        return;
        }
        visibleLyricLine=-1;
        visibleLyric=0;
        previousI=0;
        previousword=0;
        lasttime=-1;
        lastDisplayTime=0;
        flag=0;
        lyricblock.clear();
        label->clear();
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
        timer->start(16);
        ElapsedTimer.start();
    });
    QObject::connect(button_out,&QPushButton::clicked,Page_main,
    [label,&visibleLyric,timer,LyricWord_count,&visibleLyricLine,LyricLine_count,&previousI,&previousword,
    &lasttime,&lastDisplayTime,&flag,&lyricblock]() {//clear清屏
        timer->stop();
        label->clear();
        visibleLyric=0;
        visibleLyricLine=-1;
        previousI=0;
        previousword=0;
        lasttime=-1;
        lastDisplayTime=0;
        flag=0;
        lyricblock.clear();
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    auto* net=new NetWorkQ(&window);
    QObject::connect(Waiting,&QTimer::timeout,Page_setting,//网络查询
    [net,&SmtcSongsname,&SmtcArtist](){
        net->CloudMusic_Normal(SmtcSongsname,SmtcArtist);
    });
    QObject::connect(net,&NetWorkQ::SIDgetfail,Page_setting,//错误抛出
    [net](const QString& Q){
        qDebug()<<Q;
    });
    QObject::connect(net,&NetWorkQ::CloudAPI_Lyric_Fail,Page_setting,//错误抛出
    [net](const QString& Q){
        qDebug()<<Q;
    });
    QObject::connect(net,&NetWorkQ::CloudAPI_Lyric_Ready,Page_setting,//歌词获取
    [net,
    &Lines,&SmtcSongsname,&SmtcArtist,button_in,timer,&visibleLyric,ParseStream,label,&flag,&visibleLyricLine,&previousI,&previousword,
    &lastDisplayTime,&lasttime,&lyricblock,LyricWord_count,LyricLine_count](const QJsonObject& Object){
    if(Object.value("nolyric").toBool(false))
    {
        qDebug()<<"纯音乐";
        Lyric_Line nolyric_sample;
        nolyric_sample.Context="纯音乐："+SmtcSongsname+"~"+SmtcArtist;
        nolyric_sample.Time=0;
        Lines.clear();
        Lines.push_back(nolyric_sample);
        button_in->click();
        return;
    }
    bool un_yrc=0,un_lrc=0;
    QJsonValue yrclyric=Object.value("yrc");//yrc拆解
    QString text="";
    if(yrclyric.isObject())
    {
        QJsonValue lyric=yrclyric.toObject().value("lyric");
        if(!lyric.isString())
        {
            un_yrc=1;
        }
        else
        {
            text=lyric.toString();
            if(text.isEmpty())
            {
                un_yrc=1;
            }
        }
    }
    else un_yrc=1;
    if(un_yrc==1)
    {
        QJsonValue lrclyric=Object.value("lrc");
        if(lrclyric.isObject())
        {
            QJsonValue lyric=lrclyric.toObject().value("lyric");
            if(!lyric.isString())
            {
                un_lrc=1;
            }
            else
            {
                text=lyric.toString();
                if(text.isEmpty())
                {
                    un_lrc=1;
                }
            }
        }
    }
    if(un_yrc==1 && un_lrc==1)
    {
        QJsonValue lyric=Object.value("lyric");
        text=lyric.toString();
    }
    if(!text.isEmpty())
    {
        QTextStream stream(&text);
        stream.setEncoding(QStringConverter::Utf8);//文件流转码
        std::vector<Lyric_Line>Temp_Lyric=ParseStream(stream);
        if(Temp_Lyric.empty())return;
        //重置状态
        timer->stop();
        visibleLyric=0;
        label->clear();
        visibleLyricLine=-1;
        previousI=0;
        previousword=0;
        lastDisplayTime=0;
        lasttime=-1;
        flag=0;
        lyricblock.clear();
        Lines=std::move(Temp_Lyric);
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
        button_in->click();
    }
    });
    LoadSetting();
    auto* layout=new QVBoxLayout(&window);
    auto* layout_Page_main=new QVBoxLayout(Page_main);
    auto* layout_Page_setting=new QVBoxLayout(Page_setting);
    auto* layout_Page_setting_size1=new QHBoxLayout(Page_setting);
    auto* layout_Page_setting_size2=new QHBoxLayout(Page_setting);
    auto* layout_Page_setting_size3=new QHBoxLayout(Page_setting);
    layout->addWidget(Top_Bar);
    layout->addWidget(Stack);
    layout_Page_main->addWidget(LyricWord_count);
    layout_Page_main->addWidget(LyricLine_count);
    layout_Page_main->addWidget(OpenFile);
    layout_Page_main->addWidget(button_in);
    layout_Page_main->addWidget(button_out);
    layout_Page_setting_size1->addWidget(button_ColorChoose);
    layout_Page_setting_size1->addWidget(button_FontChoose);
    auto* Size_label=new QLabel("字号(pt):",Page_setting);
    layout_Page_setting_size2->addWidget(Size_label);
    layout_Page_setting_size2->addWidget(Size_Input);
    layout_Page_setting_size2->addWidget(button_file);
    layout_Page_setting_size2->addStretch();
    layout_Page_setting_size3->addWidget(LyricShadow);
    layout_Page_setting_size3->addWidget(LyricOpacity);
    layout_Page_setting_size3->addWidget(Lyricshake);
    layout_Page_setting_size3->addStretch();
    layout_Page_setting->addLayout(layout_Page_setting_size1);
    layout_Page_setting->addLayout(layout_Page_setting_size2);
    layout_Page_setting->addLayout(layout_Page_setting_size3);
    LyricWindow.show();
    window.show(); // NOLINT
    int exitcode=app.exec();//exitcode接收主线程事件循环是否结束，返回0把子线程也结束
    Thread->requestInterruption();
    Thread->wait();
    return exitcode;
}