#ifndef SMTC_H
#define SMTC_H

// 对 Qt 暴露的纯 Qt 接口（不含任何 WinRT 类型，避免 WinRT 头与 Qt 头冲突）
#include <QObject>
#include <QString>
#include <memory>

// 播放状态，与 Windows::Media::Control 的枚举顺序一致
enum class SMTCStatus
{
    Closed = 0,   // 会话已关闭
    Changing = 1, // 正在切换
    Stopped = 2,
    Playing = 3,
    Paused = 4
};

struct MediaInfo
{
    QString title;      // 歌曲名
    QString artist;     // 艺术家
    QString album;      // 专辑
    QString sourceApp;  // 播放源（如网易云/QQ音乐/Spotify）
    SMTCStatus status = SMTCStatus::Closed;
    qint64 positionMs = 0;  // 当前进度（毫秒）
    qint64 durationMs = 0;  // 总时长（毫秒）

    bool isValid() const { return !title.isEmpty(); }
};

class SMTCWatcher : public QObject
{
    Q_OBJECT
public:
    explicit SMTCWatcher(QObject *parent = nullptr);
    ~SMTCWatcher() override;

    // 初始化 COM 单元并接管当前媒体会话，失败返回 false
    bool init();

    // 开始轮询（默认 300ms），会持续发出 positionUpdated
    void start(int intervalMs = 300);
    void stop();
    bool isActive() const;

    MediaInfo current() const;

public slots:
    void refreshNow();      // 手动刷新一次
    void play();
    void pause();
    void togglePlayPause();
    void next();
    void previous();

signals:
    void mediaChanged(const MediaInfo &info);      // 切歌 / 歌曲信息变化
    void statusChanged(SMTCStatus status);         // 播放 / 暂停 / 停止
    void positionUpdated(qint64 positionMs);       // 进度变化（高频）
    void sessionLost();                            // 没有任何播放器在播

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // SMTC_H
