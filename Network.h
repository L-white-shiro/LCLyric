#ifndef NETWORK_H
#define NETWORK_H
#include<QLabel>
#include<QUrl>
#include<QUrlQuery>
#include<QNetworkRequest>
#include<QNetworkReply>
#include<QWidget>
#include<vector>
#include<QJsonObject>
#include<QJsonValue>
#include<QJsonArray>
#include<QNetworkProxy>
#include "AES.h"
#endif // NETWORK_H
struct songsStructinfo
{
    qint64 SID,albumID,Duration;
    QString Artists_name[50],Songs_Name;
};
class NetWorkQ:public QObject
{
    Q_OBJECT;
private:
    QLabel* Debuglabel=nullptr;
public:
    QNetworkAccessManager* NetworkRequset=nullptr;
    std::vector<songsStructinfo>songsInfo;//动态数组歌词信息
    qint64 SID=0;
    explicit NetWorkQ(QObject* parent=nullptr)//父对象构造
    {
        NetworkRequset=new QNetworkAccessManager(this);
        connect(this,&NetWorkQ::SIDReady,this,[this](const qint64& S)
        {
            SID=S;
            ClodeMusic_Lyric(SID);
        });
        connect(this,&NetWorkQ::CloudAPI_Lyric_Fail,this,[this](const QString& err)
        {
            qDebug()<<err;
            ClodeMusic_NormalLyric(SID);
        });
    }
    void setDebug(QLabel* &L)
    {
        Debuglabel=L;
    }
    void CloudMusic_Normal(QString& SmtcSongsname,QString& SmtcArtist,qint64& Time)//歌曲id查询，调用cloudmusic_lyric_requeset
    {
        QUrl url("https://music.163.com/api/search/get/web");//查询歌曲初始URL
        QUrlQuery query;//查询参数请求头拼接
        query.addQueryItem("s",SmtcSongsname+" "+SmtcArtist);
        query.addQueryItem("type","1");
        query.addQueryItem("limit","10");
        query.addQueryItem("offset","0");
        url.setQuery(query);
        QNetworkRequest Request(url);
        qDebug()<<url.toString();
        NetworkRequset->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        QNetworkReply* reply=NetworkRequset->get(Request);//先拿到 reply，再连接信号
        QObject::connect(reply,&QNetworkReply::finished,NetworkRequset,
        [reply,this,Time](){
        songsInfo.clear();
        if(reply->error()!=QNetworkReply::NoError)//联网检查
        {
            Debuglabel->setText("网络错误");
            emit(SIDgetfail(reply->errorString()));
            reply->deleteLater();
            return;
        }
        QByteArray Bytes=reply->readAll();//读取内容
        reply->deleteLater();
        QJsonParseError Error;
        auto document=QJsonDocument::fromJson(Bytes,&Error);//Json转换
        if(Error.error!=QJsonParseError::NoError)//解析json错误
        {
            Debuglabel->setText("网络返回json文件解析失败");
            return;
        }
        if(!document.isObject())
        {
            Debuglabel->setText("网络返回对象错误");
            return;
        }
        QJsonObject Object=document.object();
        QJsonValue Result=Object.value("result");//扒result
        if(!Result.isObject())
        {
            Debuglabel->setText("result根节点非对象");
            return;
        }
        QJsonValue songs=Result.toObject().value("songs");//扒songs
        if(!songs.isArray())
        {
            Debuglabel->setText("songs根节点非数组");
            return;
        }
        auto songsarray=songs.toArray();
        if(songsarray.isEmpty())
        {
            Debuglabel->setText("候选列表为空");
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
                if(j>=50)break;
            }
            songsInfo.push_back(TempSongs);
        }
        int target=0;
        for(const auto& SongsNow:songsInfo)
        {
            if(std::abs(SongsNow.Duration-Time)<1500)break;
            ++target;
        }
        emit(SIDReady(songsInfo[target].SID));
        });
    }
    void ClodeMusic_NormalLyric(qint64 SID)//歌词查询
    {
        QUrl song_url("https://music.163.com/api/song/media"); //查询歌词URL
        QUrlQuery song_Query;
        song_Query.addQueryItem("id",QString::number(SID));
        song_url.setQuery(song_Query);
        QNetworkRequest songsRequset(song_url);
        QNetworkReply* song_reply=NetworkRequset->get(songsRequset);
        qDebug()<<song_url.toString();
        QObject::connect(song_reply,&QNetworkReply::finished,NetworkRequset,//歌词获取
        [song_reply,this](){
        if(song_reply->error()!=QNetworkReply::NoError)//联网检查
        {
            Debuglabel->setText("songs根节点非数组");
            qDebug()<<"歌词获取时网络错误"<<song_reply->errorString();
            emit(CloudAPI_Lyric_Fail(song_reply->errorString()));
            return;
        }
        QByteArray Bytes=song_reply->readAll();//读取内容
        qDebug().noquote()<<QString::fromUtf8(Bytes);
        song_reply->deleteLater();
        QJsonParseError Error;
        QJsonDocument document=QJsonDocument::fromJson(Bytes,&Error);//Json转换
        if(Error.error!=QJsonParseError::NoError)//解析json错误
        {
            Debuglabel->setText("歌词网络返回json文件解析失败"+song_reply->errorString());
            emit(CloudAPI_Lyric_Fail(song_reply->errorString()));
            return;
        }
        if(!document.isObject())
        {
            Debuglabel->setText("歌词网络返回对象错误"+song_reply->errorString());
            emit(CloudAPI_Lyric_Fail(song_reply->errorString()));
            return;
        }
        emit(CloudAPI_Lyric_Ready(document.object()));
        });
    }
    void ClodeMusic_Lyric(qint64 SID)//YRC逐字歌词查询
    {
        QJsonObject song_Query;//请求拼装
        song_Query.insert("id",QString::number(SID));
        song_Query.insert("cp",false);
        song_Query.insert("tv",0);//翻译歌词
        song_Query.insert("lv",1);//老式lyr
        song_Query.insert("rv",0);
        song_Query.insert("kv",0);
        song_Query.insert("yv",1);//逐字歌词
        song_Query.insert("ytv",0);//扩展版本
        song_Query.insert("yrv",0);
        QString text=QJsonDocument(song_Query).toJson(QJsonDocument::Compact);//空格压缩
        QString api_path="/api/song/lyric/v1";//路径模板
        QString digest_input="nobody"+api_path+"use"+text+"md5forencrypt";//加密调用
        QByteArray digest=QCryptographicHash::hash(digest_input.toUtf8(),QCryptographicHash::Md5).toHex();
        QString blob=api_path+"-36cd479b6b5-"+text+"-36cd479b6b5-"+QString::fromUtf8(digest);//明文拼接
        std::string encryptedHex;
        try//AES加密
        {
            QByteArray key="e82ckenh8dichen8";
            std::vector<BYTE>keyVec(key.begin(),key.end());
            encryptedHex=AES128_ECB_Encrypt(blob.toStdString(),keyVec);
        }
        catch(const std::exception& e)
        {
            emit(CloudAPI_YRCLyric_Fail(QString("AES 加密失败: %1").arg(e.what())));
            return;
        }
        QUrl url("https://interface3.music.163.com/eapi/song/lyric/v1");
        QNetworkRequest request(url);//Header设置
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader("Origin","https://music.163.com");
        request.setRawHeader("Referer","https://music.163.com/");
        request.setRawHeader("User-Agent","Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        QByteArray postBody="params="+QByteArray::fromStdString(encryptedHex);//URL请求发送
        QNetworkReply* reply=NetworkRequset->post(request, postBody);
        QObject::connect(reply,&QNetworkReply::finished,NetworkRequset,//歌词获取
        [reply,this](){
        if(reply->error()!=QNetworkReply::NoError)//联网检查
        {
            Debuglabel->setText("歌词获取时网络错误"+reply->errorString());
            emit(CloudAPI_YRCLyric_Fail(reply->errorString()));
            return;
        }
        QByteArray Bytes=reply->readAll();//读取内容
        qDebug().noquote()<<QString::fromUtf8(Bytes);
        reply->deleteLater();
        QJsonParseError Error;
        QJsonDocument document=QJsonDocument::fromJson(Bytes,&Error);//Json转换
        if(Error.error!=QJsonParseError::NoError)//解析json错误
        {
            Debuglabel->setText("歌词网络返回json文件解析失败"+reply->errorString());
            emit(CloudAPI_YRCLyric_Fail(reply->errorString()));
            return;
        }
        if(!document.isObject())
        {
            Debuglabel->setText("歌词网络返回对象错误"+reply->errorString());
            emit(CloudAPI_YRCLyric_Fail(reply->errorString()));
            return;
        }
        emit(CloudAPI_Lyric_Ready(document.object()));
    });
    }
signals:
    void SIDReady(qint64 SID);
    void SIDgetfail(QString error);
    void CloudAPI_YRCLyric_Fail(QString error);
    void CloudAPI_Lyric_Ready(QJsonObject object);
    void CloudAPI_Lyric_Fail(QString error);
protected:
};

