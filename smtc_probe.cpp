#include<iostream>
#include<winrt/base.h>
#include<windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include<winrt/Windows.Foundation.h>
#include<winrt/Windows.Media.Control.h>
#include<chrono>
using SessionManager=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using Media=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;
using Playbackstatus=winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    bool init=false;
    try{
        winrt::init_apartment();
        std::cout<<"WinRT initialization succeeded\n";
        init=true;
        {
        auto manager=SessionManager::RequestAsync().get();//get获取管理器
        auto session=manager.GetCurrentSession();//获取最优会话
        if(!session)//非空检查
        {
            std::cout<<"NO current session\n";
        }
        else
        {
            auto appName=session.SourceAppUserModelId();//获取会话名
            std::cout<<winrt::to_string(appName)<<"\n";
            auto propertiesOperation=session.TryGetMediaPropertiesAsync();//异步请求会话属性
            auto properties=propertiesOperation.get();//获取会话属性
            auto title=properties.Title();//标题获取
            std::cout<<"Title:"<<winrt::to_string(title)<<"\n";
            auto artist=properties.Artist();//歌手信息获取
            std::cout<<"Artist:"<<winrt::to_string(artist)<<"\n";
            auto playbackinfo=session.GetPlaybackInfo();//播放状态获取
            if(playbackinfo)
            {
                auto status=playbackinfo.PlaybackStatus();
                if(status==Playbackstatus::Playing)
                {
                    std::cout<<"Status:Playing\n";
                }
                else if(status==Playbackstatus::Paused)
                {
                    std::cout<<"Status:Paused\n";
                }
                else if(status==Playbackstatus::Stopped)
                {
                    std::cout<<"Status:Stopped\n";
                }
                else std::cout<<"Status:Others\n";
            }
            auto timeline=session.GetTimelineProperties();//获取时间线
            if(timeline)
            {
                auto position=timeline.Position();
                auto endtime=timeline.EndTime();
                auto positionMS=std::chrono::duration_cast<std::chrono::milliseconds>(position);//毫秒转换
                auto endtimeMS=std::chrono::duration_cast<std::chrono::milliseconds>(endtime);
                std::cout<<"Position:"<<positionMS.count()<<"\n"<<"Endtime:"<<endtimeMS.count()<<"\n";
            }
        }
        }
        winrt::uninit_apartment();
        return 0;
    }
    catch(const winrt::hresult_error& error){
        if(init==true)winrt::uninit_apartment();
        std::cerr<<winrt::to_string(error.message())<<"\n";
        return 1;
    }
}