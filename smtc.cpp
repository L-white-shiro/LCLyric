#include "smtc.h"

#include <QTimer>
#include <QDebug>
#include <QDateTime>

// ---- WinRT 与 Qt 头的隔离区 ------------------------------------------
// windows.h 把 GetCurrentTime 宏定义成 GetTickCount，会撞 WinRT 的同名方法，
// 必须在包含 winrt 头之前 undef（Qt 头内部已间接包含 windows.h）。
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
// ----------------------------------------------------------------------

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Media::Control;

namespace
{
// HRESULT: 0x80010106，表示当前线程 COM 单元已初始化（Qt 主线程就是这种情况）
constexpr int32_t kRpcChangedMode = static_cast<int32_t>(0x80010106);

// WinRT 的 TimeSpan 单位是 100ns
qint64 toMs(const TimeSpan &t)
{
    return t.count() / 10000;
}

SMTCStatus mapStatus(GlobalSystemMediaTransportControlsSessionPlaybackStatus s)
{
    switch (s)
    {
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:  return SMTCStatus::Playing;
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:   return SMTCStatus::Paused;
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:  return SMTCStatus::Stopped;
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Changing: return SMTCStatus::Changing;
    default: return SMTCStatus::Closed;
    }
}

QString fromHstring(hstring const &s)
{
    return QString::fromWCharArray(s.c_str(), static_cast<int>(s.size()));
}
}

struct SMTCWatcher::Impl
{
    SMTCWatcher *q = nullptr;
    QTimer *timer = nullptr;

    bool ready = false;
    MediaInfo info;

    GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
    GlobalSystemMediaTransportControlsSession session{nullptr};

    winrt::event_token propToken{};   // 歌曲信息变化
    winrt::event_token pbToken{};     // 播放状态变化
    winrt::event_token tlToken{};     // 进度变化
    winrt::event_token sessToken{};   // 会话列表变化
    winrt::event_token curToken{};    // 当前会话变化

    void unbindSession()
    {
        if (session)
        {
            try
            {
                session.MediaPropertiesChanged(propToken);
                session.PlaybackInfoChanged(pbToken);
                session.TimelinePropertiesChanged(tlToken);
            }
            catch (...) {}
            session = nullptr;
        }
    }

    void bindSession()
    {
        unbindSession();
        if (!manager) return;
        try
        {
            session = manager.GetCurrentSession();
        }
        catch (...) { session = nullptr; }
        if (!session) return;

        try
        {
            // 回调发生在 COM 单元线程上；主线程是 STA，即 Qt 主线程，可直接 emit
            propToken = session.MediaPropertiesChanged([this](auto &&, auto &&) { q->refreshNow(); });
            pbToken   = session.PlaybackInfoChanged([this](auto &&, auto &&) { q->refreshNow(); });
            tlToken   = session.TimelinePropertiesChanged([this](auto &&, auto &&) { q->refreshNow(); });
        }
        catch (...) {}
    }

    void refresh()
    {
        if (!ready) return;

        if (!session && manager) bindSession();

        MediaInfo next;
        if (session)
        {
            try
            {
                auto props = session.TryGetMediaPropertiesAsync().get();
                next.title  = fromHstring(props.Title());
                next.artist = fromHstring(props.Artist());
                if (next.artist.isEmpty()) next.artist = fromHstring(props.AlbumArtist());
                next.album = fromHstring(props.AlbumTitle());
            }
            catch (const winrt::hresult_error &e)
            {
                qWarning() << "[SMTC] 读取媒体属性失败:" << e.code();
            }

            try
            {
                next.sourceApp = fromHstring(session.SourceAppUserModelId());
            }
            catch (...) {}

            try
            {
                next.status = mapStatus(session.GetPlaybackInfo().PlaybackStatus());
            }
            catch (...) { next.status = SMTCStatus::Closed; }

            try
            {
                auto tl = session.GetTimelineProperties();
                next.positionMs = toMs(tl.Position());
                next.durationMs = toMs(tl.EndTime());
            }
            catch (...) {}
        }

        const bool trackChanged = (next.title != info.title) || (next.artist != info.artist)
                               || (next.album != info.album) || (next.sourceApp != info.sourceApp);
        const bool statusChanged = (next.status != info.status);
        const bool posChanged = (next.positionMs != info.positionMs);

        if (trackChanged || statusChanged)
        {
            // 切歌时顺带更新时长
            next.durationMs = next.durationMs ? next.durationMs : info.durationMs;
            info = next;
            if (trackChanged) emit q->mediaChanged(info);
            if (statusChanged) emit q->statusChanged(info.status);
        }
        else
        {
            info.positionMs = next.positionMs;
        }

        if (!info.isValid())
        {
            emit q->sessionLost();
            return;
        }
        if (posChanged) emit q->positionUpdated(info.positionMs);
    }
};

SMTCWatcher::SMTCWatcher(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->q = this;
}

SMTCWatcher::~SMTCWatcher() = default;

bool SMTCWatcher::init()
{
    // Qt 主线程已初始化过 COM(STA)，这里会抛 RPC_E_CHANGED_MODE，属正常情况
    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
    }
    catch (const winrt::hresult_error &e)
    {
        if (e.code() != kRpcChangedMode)
        {
            qWarning() << "[SMTC] init_apartment 失败:" << e.code();
            return false;
        }
    }

    try
    {
        d->manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    }
    catch (const winrt::hresult_error &e)
    {
        qWarning() << "[SMTC] 获取会话管理器失败:" << e.code()
                   << QString::fromWCharArray(e.message().c_str());
        return false;
    }

    try
    {
        d->sessToken = d->manager.SessionsChanged([this](auto &&, auto &&) { d->bindSession(); refreshNow(); });
        d->curToken  = d->manager.CurrentSessionChanged([this](auto &&, auto &&) { d->bindSession(); refreshNow(); });
    }
    catch (...) {}

    d->bindSession();
    d->ready = true;
    refreshNow();
    return true;
}

void SMTCWatcher::start(int intervalMs)
{
    if (!d->timer) d->timer = new QTimer(this);
    d->timer->setInterval(intervalMs);
    connect(d->timer, &QTimer::timeout, this, &SMTCWatcher::refreshNow);
    d->timer->start();
}

void SMTCWatcher::stop()
{
    if (d->timer) d->timer->stop();
}

bool SMTCWatcher::isActive() const
{
    return d->timer && d->timer->isActive();
}

MediaInfo SMTCWatcher::current() const
{
    return d->info;
}

void SMTCWatcher::refreshNow()
{
    d->refresh();
}

void SMTCWatcher::play()
{
    if (!d->session) return;
    try { d->session.TryPlayAsync().get(); } catch (...) {}
}

void SMTCWatcher::pause()
{
    if (!d->session) return;
    try { d->session.TryPauseAsync().get(); } catch (...) {}
}

void SMTCWatcher::togglePlayPause()
{
    if (!d->session) return;
    try { d->session.TryTogglePlayPauseAsync().get(); } catch (...) {}
}

void SMTCWatcher::next()
{
    if (!d->session) return;
    try { d->session.TrySkipNextAsync().get(); } catch (...) {}
}

void SMTCWatcher::previous()
{
    if (!d->session) return;
    try { d->session.TrySkipPreviousAsync().get(); } catch (...) {}
}
