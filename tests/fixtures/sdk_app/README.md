# SDK 应用消费契约 fixture

这是框架仓库的最小消费者，不是项目生成模板。模板唯一归属 NandinaCLI，
由 NandinaCLI 按兼容的 SDK 版本发布并负责 `new/build/run` 端到端验收。

`tests/sdk_vendored_consumer_test.py` 把本 fixture 放入临时下游项目，使用当前源码打包的
SDK 离线构建，验证应用、窗口、组件与共享图片解码器能够链接，并启动到创建窗口之前。
静态与共享 SDL3 分别检查运行期依赖；真实窗口生命周期另由 `window-lifecycle` 覆盖。

本 fixture 防止框架的消费契约漂移，不能证明另一个仓库里的模板始终兼容。
NandinaCLI 发布时还须用真实生成模板对其固定的 SDK 版本构建运行；跨仓库兼容测试
应携带模板提交号与 SDK 版本，不能把这里的通过替代那里的验收。
