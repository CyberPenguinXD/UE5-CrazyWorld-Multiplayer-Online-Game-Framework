# Coop Adventure 游戏工程

此分支包含 Coop Adventure 的 Unreal Engine **5.7** 游戏工程：C++ 源码、配置、蓝图、地图、角色与其他游戏资源，以及 VRM4U 插件源码和必要的第三方库。

工程文件位于仓库根目录：`CoopAdventure.uproject`。原 `main` 分支保持不变；本次上传仅位于 `coop-adventure` 分支。

## 下载与打开

先安装 Git、Git LFS、Unreal Engine 5.7，以及支持 UE 5.7 的 Visual Studio C++ 游戏开发工具链。在终端中运行：

```powershell
git lfs install
git clone --branch coop-adventure --single-branch https://github.com/CyberPenguinXD/UE5-CrazyWorld-Multiplayer-Online-Game-Framework.git CoopAdventure
cd CoopAdventure
git lfs pull
```

请通过 Git LFS 下载资源文件；仅下载 GitHub ZIP 时，资源可能只是 LFS 指针。

右键 `CoopAdventure.uproject`，生成 Visual Studio 项目文件。打开生成的解决方案，选择 `Development Editor` / `Win64` 并编译，再打开 `CoopAdventure.uproject`。项目和 VRM4U 插件的编辑器模块会在本机重新编译。

当前配置使用 `OnlineSubsystem NULL` 进行局域网联机测试，并启用了 Steam 子系统插件。玩法、操作和联机说明见原 [README.md](README.md)。

## 上传范围

- 包含 `Source`、`Config`、`Content`、`Plugins`、`CoopAdventure.uproject` 和 `.vsconfig`。
- Unreal 资源、模型和第三方二进制依赖使用 Git LFS。
- 排除 `Binaries`、`Intermediate`、`Saved`、`DerivedDataCache`、IDE 缓存和生成的解决方案，可由本机重新生成。
- 本机专用的 `Test.bat` 与 Android File Server 调试连接令牌未上传。原始本地游戏工程未修改。

本次操作仅上传和核对文件，未进行游戏编译或运行验证。
