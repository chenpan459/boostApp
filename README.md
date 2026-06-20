# boostApp

基于 Boost 的 Linux 进程间通信示例：守护进程 + 发布/订阅客户端。

项目使用**自编译 Boost**（不依赖系统 Boost），源码与编译产物均 vendored 在仓库中，clone 后可直接构建。

## 架构

```
boardcomm_pub  --[inbound MQ]-->  boardcommd  --[outbound MQ]-->  boardcomm_sub
```

| 组件 | 说明 |
|------|------|
| `boardcommd` | 守护进程，从 inbound 队列读消息并转发到 outbound 队列 |
| `boardcomm_pub` | 发布端，向 inbound 队列写入消息 |
| `boardcomm_sub` | 订阅端，从 outbound 队列读取消息，支持按 topic 过滤 |

技术栈：Boost.Asio（信号/异步 IO）、Boost.Interprocess（消息队列）、Boost.PropertyTree（JSON 配置）。

## 目录结构

```
boostApp/
├── build.sh                 # 构建脚本（推荐）
├── CMakeLists.txt
├── config/boardcomm.json    # 默认配置
│
├── deps/
│   └── boost_1_84_0/        # Boost 源码（已纳入 git）
│
├── lib/
│   └── boost/               # Boost 编译安装目录（已纳入 git）
│       ├── include/
│       └── lib/
│
├── build/                   # boostApp 编译输出（git 忽略）
├── log/                     # 运行日志
│
└── src/
    ├── app/                 # 可执行入口（boardcommd / pub / sub）
    ├── core/                # 应用框架（配置、日志、生命周期）
    ├── domain/              # 领域接口（IMessageBus）
    ├── infra/               # Boost IPC 实现
    └── service/             # 业务编排
```

> **说明**：`*.tar.bz2` 压缩包因超过 GitHub 100MB 限制不纳入 git；若本地缺少 `lib/boost/`，可运行 `./build.sh boost` 从 `deps/boost_1_84_0/` 重新编译。

## 依赖

- CMake >= 3.16
- C++20 编译器（GCC 11+ / Clang 14+）
- build-essential、tar、curl 或 wget

Ubuntu/Debian：

```bash
sudo apt install build-essential cmake curl
```

**不需要**安装系统 Boost（`libboost-all-dev`），项目使用 `lib/boost/` 下的自编译版本。

## 构建

推荐使用 `build.sh`：

```bash
# 首次：若 lib/boost/ 不存在，自动编译 Boost 并构建 boostApp
./build.sh

# 仅重新编译 Boost（直接使用 deps/boost_1_84_0/ 源码，不下载）
./build.sh boost

# 清理后全量重编
./build.sh rebuild

# Debug 构建
./build.sh rebuild -t Debug

# 快速冒烟测试
./build.sh run
```

也可手动使用 CMake（需确保 `lib/boost/` 已存在）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### build.sh 命令一览

| 命令 | 说明 |
|------|------|
| `build` | 构建 boostApp（默认） |
| `rebuild` | 清理 `build/` 后重编 |
| `boost` | 仅编译 Boost 到 `lib/boost/` |
| `clean` | 删除 `build/` |
| `clean-boost` | 删除 `lib/boost/` |
| `clean-lib` | 删除整个 `lib/` |
| `clean-all` | 删除 `build/` 和 `lib/` |
| `install` | 安装到 `out/`（可通过 `-p` 指定路径） |
| `run` | 构建并运行 pub/sub 冒烟测试 |

环境变量：`BUILD_TYPE`、`JOBS`、`BOOST_VERSION`、`THIRD_PARTY_LIB_DIR`。

## 运行

终端 1 — 启动守护进程：

```bash
./build/boardcommd
```

终端 2 — 启动订阅端：

```bash
./build/boardcomm_sub
# 或按 topic 过滤
./build/boardcomm_sub sensor/temp
```

终端 3 — 发布消息：

```bash
./build/boardcomm_pub sensor/temp 25.3
./build/boardcomm_pub sensor/humidity 60
```

## 配置

默认配置文件：`config/boardcomm.json`，可通过 `-c` / `--config` 指定：

```bash
./build/boardcommd -c config/boardcomm.json
```

| 字段 | 说明 | 默认值 |
|------|------|--------|
| `mq_in_name` | 发布端写入的队列名 | `boardcomm_in` |
| `mq_out_name` | 订阅端读取的队列名 | `boardcomm_out` |
| `max_messages` | 队列最大消息数 | `128` |
| `max_message_size` | 单条消息最大字节 | `4096` |
| `log_dir` | 日志目录 | `log` |
| `log_level` | trace / debug / info / warn / error / fatal | `info` |
| `poll_interval_ms` | 轮询间隔（毫秒） | `50` |

## Boost 版本与路径

| 项 | 路径 |
|----|------|
| 源码 | `deps/boost_1_84_0/` |
| 安装前缀 | `lib/boost/` |
| CMake 变量 | `BOOSTAPP_BOOST_ROOT`、`Boost_NO_SYSTEM_PATHS=ON` |

切换 Boost 版本：

```bash
BOOST_VERSION=1.85.0 ./build.sh boost
```

需同步更新 `CMakeLists.txt` 中的 `find_package(Boost 1.84 ...)` 版本号，并将对应源码放入 `deps/`。
