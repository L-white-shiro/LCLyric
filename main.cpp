#include "main.h"
#include "Lyriclabel.h"
struct Lyric_Line
{
    qint64 Time;
    QString Context;
};
QRegularExpression Pattern_nonms(
    R"(^\[(\d{2}):([0-5]\d)\](.*)$)"
    );
QRegularExpression Pattern_non_00ms(
    R"(^\[(\d{2}):([0-5]\d)\.(\d{2})\](.*)$)"
    );
QRegularExpression Pattern_non_000ms(
    R"(^\[(\d{2}):([0-5]\d)\.(\d{3})\](.*)$)"
    );
struct songsStructinfo
{
    qint64 SID,albumID,Duration;
    QString Artists_name[20],Songs_Name;
};

using SessionManager=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using Media=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;
using Playbackstatus=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
QThread* Thread=nullptr;
int main(int argc, char* argv[])
{
    int visibleLyric=0;
    int visibleLyricLine=-1;
    std::vector<songsStructinfo>songsInfo;//动态数组歌词信息
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
    qDebug()<<ConfigDir;
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
    auto* Networktrial=new QPushButton(Page_setting);
    Networktrial->hide();
    qint64 SmtcPositionMS=0,Smtctimeline=0;//Smtc接收
    QString SmtcArtist="",SmtcSongsname="",LastSong="";
    bool SmtcPositionchange=0,Smtcsongschange=0;
    bool lyricPlaying=0;//歌曲播放状态
    auto Thread=QThread::create([&window,&SmtcPositionMS,&SmtcPositionchange,&Smtctimeline,&lyricPlaying
    ,&SmtcArtist,&SmtcSongsname,&LastSong,&Smtcsongschange,Networktrial]()//SMTC
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
                                        [&SmtcPositionMS,&SmtcPositionchange,&Smtctimeline,&SmtcArtist,&SmtcSongsname,&Smtcsongschange
                                        ,change,positionMS,endtimeMS,artist,title,&LastSong,Networktrial]{
                                            SmtcPositionMS=positionMS;
                                            SmtcPositionchange=change;
                                            Smtctimeline=endtimeMS;
                                            SmtcArtist=artist;
                                            SmtcSongsname=title;
                                            if(LastSong!=title)
                                            {
                                                Smtcsongschange=1;
                                                LastSong=title;
                                                Networktrial->click();
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
    Thread->setParent(&app);
    Thread->start();
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
    auto* label=new Lyriclabel(&LyricWindow); //主显示区
    auto* button_in=new QPushButton("显示示例", Page_main); //显示按钮
    auto* button_out=new QPushButton("清空", Page_main); //清空页面
    auto* button_ColorChoose=new QPushButton("选择颜色",Page_setting);//颜色选择
    auto* button_FontChoose=new QPushButton("选择字体",Page_setting);//字体选择
    auto* Size_Input=new QLineEdit(Page_setting);//字号输入
    auto* timer=new QTimer(&window); //超时器
    auto* OpenFile=new QPushButton("打开文件",Page_main);//文件打开按钮
    auto* LyricWord_count=new QLabel(QString::number(visibleLyric), Page_main); // 测试用歌词字数展示
    auto* LyricLine_count=new QLabel(QString::number(0),Page_main); // 测试用歌词行数目展示
    auto ParseStream=[](QTextStream &stream)->std::vector<Lyric_Line>{//文件流输入
        std::vector<Lyric_Line>Temp_Lyric;
        while(!stream.atEnd())
        {
            QString Line=stream.readLine().trimmed();
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
        std::stable_sort(Temp_Lyric.begin(),Temp_Lyric.end(),[](const Lyric_Line& a,const Lyric_Line& b){return a.Time<b.Time;});
        return Temp_Lyric;
    };
    QObject::connect(OpenFile,&QPushButton::clicked,Page_main,//打开文件
    [&window,timer,&visibleLyric,label,LyricWord_count,&Lines,&visibleLyricLine,LyricLine_count,ParseStream]() {
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

        Lines=std::move(Temp_Lyric);
        visibleLyricLine=-1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
    });
    qint64 lasttime=0;
    QObject::connect(timer,&QTimer::timeout,&window,//超时器信号槽
    [&visibleLyric,label,timer,LyricWord_count,&visibleLyricLine,&Lines,
    &ElapsedTimer,LyricLine_count,&LyricWindow,&SmtcPositionMS,&SmtcPositionchange,&lasttime,&lyricPlaying]() {
        if(lyricPlaying!=1)
        {
        return;
        }
        int i=-1;//行筛选
        qint64 TIMESCAN=ElapsedTimer.elapsed()+SmtcPositionMS;
        if(SmtcPositionchange && lasttime!=SmtcPositionMS)
        {
        TIMESCAN=SmtcPositionMS;
        ElapsedTimer.restart();
        SmtcPositionchange=0;
        lasttime=SmtcPositionMS;
        }
        for(std::size_t I=0;I<Lines.size();++I)
        {
            if(Lines[I].Time>TIMESCAN)break;
            i=I;
        }
        if(i==-1)
        {
            label->clear();
            return;
        }
        bool IfChange;//换行设定随机歌词位置
        if(visibleLyricLine!=i)IfChange=1;
        else IfChange=0;
        visibleLyricLine=i;//有效筛选，进入播放环节
        if(IfChange)
        {
            QFontMetrics XY(label->font());
            int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;
            int Y=XY.height()+2*label->SDoffset;
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
        if(Lines.size()!=0 && visibleLyricLine+1>=Lines.size() && LineWord_display>=Lines[visibleLyricLine].Context.size()&&TIMESCAN-Lines[visibleLyricLine].Time>7000)
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
    QObject::connect(button_file,&QPushButton::clicked,Page_setting,[label,FilePath](){//保存用户设置
        QSaveFile file(FilePath);
        QString Fonttext=label->font().toString();//截取字体信息
        QColor Color=label->palette().color(QPalette::WindowText);//截取颜色信息
        QString ColorText=Color.name(QColor::HexRgb);
        QJsonObject Object;//Json对象
        Object.insert("Version",1);//组织json字段
        Object.insert("Font",Fonttext);
        Object.insert("Color",ColorText);
        QJsonDocument document(Object);//Json文件打包对象
        QByteArray Byte=document.toJson(QJsonDocument::Indented);
        qDebug().noquote()<<Byte;
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
    };
    LoadSetting();
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
        int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;
        int Y=XY.height()+2*label->SDoffset;
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
            int X=XY.horizontalAdvance(Lines[visibleLyricLine].Context)+2*label->SDoffset;
            int Y=XY.height()+2*label->SDoffset;
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
    [label,&visibleLyric,timer,LyricWord_count,&ElapsedTimer,&visibleLyricLine,LyricLine_count,&Lines,&lasttime,&lyricPlaying]() { // 按钮1信号槽
        timer->stop();
        if(Lines.empty()) // 防止段错误，边界检查
        {
        qDebug()<<"无歌词";
        return;
        }
        label->clear();
        visibleLyricLine=-1;
        visibleLyric=0;
        lasttime=0;
        lyricPlaying=1;
        LyricWord_count->setText(QString::number(visibleLyric));
        LyricLine_count->setText(QString::number(0));
        timer->start(16);
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
    auto* NetworkRequset=new QNetworkAccessManager(&window);
    QObject::connect(Networktrial,&QPushButton::clicked,Page_setting,//网络查询
    [NetworkRequset,&songsInfo,ParseStream,&Lines,&visibleLyricLine,LyricWord_count,timer,&visibleLyric,label,LyricLine_count,
    &SmtcArtist,&Smtctimeline,&SmtcSongsname,button_in](){
        QUrl url("https://music.163.com/api/search/get/web");//查询歌曲初始URL
        QUrlQuery query;//查询参数拼接
        query.addQueryItem("s",SmtcSongsname+" "+SmtcArtist);
        query.addQueryItem("type","1");
        query.addQueryItem("limit","10");
        query.addQueryItem("offset","0");
        url.setQuery(query);
        QNetworkRequest Request(url);
        qDebug()<<url.toString();
        QNetworkReply* reply=NetworkRequset->get(Request);//先拿到 reply，再连接信号
        songsInfo.clear();
        QObject::connect(reply,&QNetworkReply::finished,NetworkRequset,
        [reply,&songsInfo,NetworkRequset,ParseStream,&Lines,&visibleLyricLine,LyricWord_count,timer,&visibleLyric,label,
        LyricLine_count,button_in,&SmtcArtist,&SmtcSongsname](){
            if(reply->error()!=QNetworkReply::NoError)//联网检查
            {
                qDebug()<<"网络错误"<<reply->errorString();
                reply->deleteLater();
                return;
            }
            QByteArray Bytes=reply->readAll();//读取内容
            reply->deleteLater();
            QJsonParseError Error;
            auto document=QJsonDocument::fromJson(Bytes,&Error);//Json转换
            if(Error.error!=QJsonParseError::NoError)//解析json错误
            {
                qDebug()<<"网络返回json文件解析失败"<<Error.errorString();
                return;
            }
            if(!document.isObject())
            {
                qDebug()<<"网络返回对象错误";
                return;
            }
            QJsonObject Object=document.object();
            QJsonValue Result=Object.value("result");//扒result
            if(!Result.isObject())
            {
                qDebug()<<"result根节点非对象";
                return;
            }
            QJsonValue songs=Result.toObject().value("songs");//扒songs
            if(!songs.isArray())
            {
                qDebug()<<"songs根节点非数组";
                return;
            }
            auto songsarray=songs.toArray();
            if(songsarray.isEmpty())
            {
                qDebug()<<"候选列表为空";
                return;
            }
            for(const auto& songsNOW:songsarray)
            {
                songsStructinfo TempSongs;//临时储存单曲信息
                QJsonObject SongsObject=songsNOW.toObject();
                QJsonValue Album=SongsObject.value("album");//扒album
                QJsonValue Album_Id=Album.toObject().value("id");//扒albumid
                QJsonValue Songs_ID=SongsObject.value("id");//扒歌曲id
                QJsonValue Songs_Name=SongsObject.value("name");
                QJsonValue Duration=SongsObject.value("duration");//扒持续时间
                QJsonValue Artist=SongsObject.value("artists");//扒总体艺术家信息
                auto ArtistArray=Artist.toArray();
                TempSongs.SID=Songs_ID.toVariant().toLongLong();
                TempSongs.albumID=Album_Id.toVariant().toLongLong();
                TempSongs.Duration=Duration.toVariant().toLongLong();
                TempSongs.Songs_Name=Songs_Name.toVariant().toString();
                qDebug().noquote()<<TempSongs.SID<<"|"<<TempSongs.albumID<<"|"<<TempSongs.Duration<<"|"<<TempSongs.Songs_Name<<"|";
                int j=0;
                for(const auto& ArtistNOW:ArtistArray)
                {
                    QJsonObject Artists=ArtistNOW.toObject();
                    QJsonValue Artist_Name=Artists.value("name");//扒艺术家名字
                    TempSongs.Artists_name[j]=Artist_Name.toString();
                    qDebug().noquote()<<"|"<<TempSongs.Artists_name[j];
                    ++j;
                }
                songsInfo.push_back(TempSongs);
            }
            QUrl song_url("https://music.163.com/api/song/media"); //查询歌词URL
            QUrlQuery song_Query;
            song_Query.addQueryItem("id",QString::number(songsInfo[0].SID));
            song_url.setQuery(song_Query);
            QNetworkRequest songsRequset(song_url);
            QNetworkReply* song_reply=NetworkRequset->get(songsRequset);
            qDebug()<<song_url.toString();
            QObject::connect(song_reply,&QNetworkReply::finished,NetworkRequset,//歌词获取
            [song_reply,&songsInfo,NetworkRequset,ParseStream,&Lines,&visibleLyricLine,LyricWord_count,timer,
            &visibleLyric,label,LyricLine_count,button_in,&SmtcArtist,&SmtcSongsname](){
                if(song_reply->error()!=QNetworkReply::NoError)//联网检查
                {
                    qDebug()<<"歌词获取时网络错误"<<song_reply->errorString();
                    song_reply->deleteLater();
                    return;
                }
                QByteArray Bytes=song_reply->readAll();//读取内容
                song_reply->deleteLater();
                QJsonParseError Error;
                auto document=QJsonDocument::fromJson(Bytes,&Error);//Json转换
                if(Error.error!=QJsonParseError::NoError)//解析json错误
                {
                    qDebug()<<"歌词网络返回json文件解析失败"<<Error.errorString();
                    return;
                }
                if(!document.isObject())
                {
                    qDebug()<<"歌词网络返回对象错误";
                    return;
                }
                QJsonObject Object=document.object();
                qDebug().noquote()<<QString::fromUtf8(Bytes);
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
                QJsonValue lyric=Object.value("lrc").toObject().value("lyric");
                if(!lyric.isString())//兼容直接放在根节点
                lyric=Object.value("lyric");
                if(!lyric.isString())
                {
                    qDebug()<<"lyric根节点非字符串"<<Object.keys();
                    return;
                }
                QString text=lyric.toString();
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
                    Lines=std::move(Temp_Lyric);
                    visibleLyricLine=-1;
                    LyricWord_count->setText(QString::number(visibleLyric));
                    LyricLine_count->setText(QString::number(0));
                    button_in->click();
                }
            });
        });
    });
    Networktrial->click();
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
    layout_Page_setting_size->addWidget(button_file);
    layout_Page_setting_size->addStretch();
    layout_Page_setting->addLayout(layout_Page_setting_size);
    LyricWindow.show();
    window.show(); // NOLINT
    int exitcode=app.exec();//exitcode接收主线程事件循环是否结束，返回0把子线程也结束
    Thread->requestInterruption();
    Thread->wait();
    return exitcode;
}