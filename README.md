## 参照limbus company的歌词显示效果的歌词显示器

基于C++QT和SMTC的歌词显示器，目前暂时仅支持网易云音乐<br>
### 使用说明
网易云音乐：<br>
为了获取完整的SMTC信息，**请在使用前安装**<br>
[网易云BCM插件](https://github.com/std-microblock/BetterNCM-Installer/releases/tag/1.2.0)<br>
装入以下插件<br>
[InfLink-rs](https://github.com/apoint123/inflink-rs)<br>
同时在网易云设置内，**开启SMTC**<br>
确认插件加载成功后，打开.exe文件，播放歌曲即可自动同步目前进度

### 功能介绍
在桌面随机位置投放目前歌词<br>
支持客制化颜色、字体字号设置、文字淡入、阴影、抖动效果<br>
**推荐开启文字淡入、阴影、抖动，同时字体设置选择加粗字体**<br>
歌词搜索采用网易云歌词api，歌词版本较老，**可能存在歌词错误，时间戳错误等问题**，后续会更换搜索途径，加入QQ音乐和酷狗音乐的支持和逐字时间戳<br>

### AI使用注意
在Qt学习和少数Debug、逻辑优化方面使用了AI
