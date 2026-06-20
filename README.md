# boostApp (nvcomm)

面向 **RK3588 嵌入式 Linux** 的标准后台服务：接收 **nginx 反向代理**转发的 HTTP 请求，**多线程并发**处理网络数据包，可选事件持久化。

项目使用**自编译 Boost**（不依赖系统 Boost），源码与编译产物 vendored 在仓库中，clone 后可直接构建。

## 架构

```
nginx (proxy_pass)
    │
    ├─ TCP  0.0.0.0:8080
    └─ Unix run/nvcomm.sock
         │
         ▼
    nvcommd (Boost.Beast HTTP)
         │
         ├─ IoContextPool (N 个 io_context 线程，异步 accept/read/write)
         └─ thread_pool (M 个 worker 线程，并发处理数据包)
                  │
                  └─ data/events/events.log (可选持久化)
```

| 组件 | 说明 |
|------|------|
| `nvcommd` | 唯一守护进程：HTTP 监听、并发包处理、事件持久化、看门狗、systemd 集成 |
| nginx | 前端反向代理，示例配置见 `config/nginx/nvcomm.conf` |

技术栈：Boost.Asio、Boost.Beast、分层 conf 配置、RK3588 平台抽象（绑核/看门狗/systemd）。

### 并发模型

1. **IO 线程池**（`server.threads`）：多个 `io_context` 各自跑独立线程，负责连接接入与 HTTP 读写。
2. **Worker 线程池**（`server.worker_threads`）：`boost::asio::thread_pool` 异步执行业务逻辑；处理完成后通过 `asio::post` 回到连接 strand 写回响应。
3. nginx 可通过 upstream 连接复用（`keepalive`）提高吞吐。

## HTTP API

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查，返回 `ok` |
| POST | `/api/...` | 提交数据包 body，返回 JSON |

POST 响应示例：

```json
{"status":"ok","path":"/api/v1/packet","bytes":16,"client":"127.0.0.1:58142"}
```

## 目录结构

```
boostApp/
├── build.sh                      # 构建脚本（推荐）
├── CMakeLists.txt
├── config/
│   ├── nvcomm.conf               # 主配置（section 风格）
│   ├── nginx/nvcomm.conf         # nginx upstream 示例
│   └── systemd/nvcommd.service   # systemd 单元
│
├── deps/
│   └── boost_1_84_0/             # Boost 源码（已纳入 git）
│
├── lib/
│   └── boost/                    # Boost 编译安装目录（已纳入 git）
│
├── build/                        # 编译输出（git 忽略）
├── log/                          # 运行日志
├── data/events/                  # 事件持久化（git 忽略）
├── run/                          # Unix socket 运行时目录（git 忽略）
│
└── src/
    ├── namespace.hpp             # NV_NAMESPACE 宏定义
    ├── app/nvcommd/main.cpp      # 进程入口
    ├── core/                     # 配置、日志、Application 生命周期
    ├── platform/                 # RK3588 平台：绑核、看门狗、systemd、路径
    ├── domain/                   # NetworkPacket、IEventStore
    ├── infra/net/                # IoContextPool、Beast HTTP 服务
    ├── infra/persistence/        # 文件事件存储
    └── service/                  # NetworkService、PacketProcessor
```

> **说明**：`*.tar.bz2` 因 GitHub 100MB 限制不纳入 git。若缺少 `lib/boost/`，运行 `./build.sh boost` 从 `deps/boost_1_84_0/` 编译。

## 分层设计

```
app  →  service  →  domain  →  infra  →  platform
```

| 层级 | 职责 | 关键类型 |
|------|------|---------|
| **app** | 进程入口 | `main.cpp` |
| **service** | 业务编排 | `NetworkService`、`PacketProcessor` |
| **domain** | 领域模型与接口 | `NetworkPacket`、`IEventStore` |
| **infra** | 网络与持久化 | `HttpServer`、`IoContextPool`、`FileEventStore` |
| **platform** | RK3588 平台能力 | `Watchdog`、`cpu_affinity`、`systemd_notify` |
| **core** | 通用框架 | `Application`、`AppConfig`、logger |

## 依赖

- CMake >= 3.16
- C++20 编译器（GCC 11+ / Clang 14+）
- build-essential、tar、curl 或 wget

```bash
sudo apt install build-essential cmake curl
```

**不需要**安装系统 Boost，项目使用 `lib/boost/` 自编译版本。

## 构建

```bash
# 构建（若 lib/boost/ 不存在会自动编译 Boost）
./build.sh

# 仅编译 Boost（使用 deps/boost_1_84_0/ 源码，不下载）
./build.sh boost

# 清理后全量重编
./build.sh rebuild

# Debug 构建
./build.sh rebuild -t Debug

# 冒烟测试（启动 nvcommd，curl /health 与 POST /api/v1/packet）
./build.sh run
```

### build.sh 命令

| 命令 | 说明 |
|------|------|
| `build` | 构建项目（默认） |
| `rebuild` | 清理 `build/` 后重编 |
| `boost` | 仅编译 Boost 到 `lib/boost/` |
| `clean` | 删除 `build/` |
| `clean-boost` | 删除 `lib/boost/` |
| `clean-all` | 删除 `build/` 和 `lib/` |
| `install` | 安装到 `out/` |
| `run` | 构建并运行 HTTP 冒烟测试 |

环境变量：`BUILD_TYPE`、`JOBS`、`BOOST_VERSION`、`THIRD_PARTY_LIB_DIR`。

手动 CMake：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 运行

直接启动（开发环境）：

```bash
mkdir -p run log data/events
./build/nvcommd -c config/nvcomm.conf
```

测试 API：

```bash
curl http://127.0.0.1:8080/health
curl -X POST http://127.0.0.1:8080/api/v1/packet -d 'hello from client'
```

### nginx 集成

```bash
sudo cp config/nginx/nvcomm.conf /etc/nginx/conf.d/
sudo nginx -t && sudo systemctl reload nginx
```

nginx 将 `/health` 与 `/api/` 转发到 `127.0.0.1:8080` 上的 `nvcommd`。

## 配置

默认配置：`config/nvcomm.conf`（开发环境）或 `/etc/nvcomm/nvcomm.conf`（板端）。

```bash
./build/nvcommd -c config/nvcomm.conf
```

conf 格式（`[section]` + `key = value`，`#` 为注释）：

```conf
[server]
listen = 0.0.0.0
port = 8080
threads = 4              # io_context 线程数
worker_threads = 4       # 数据包处理 worker 数
max_body_kb = 1024
unix_socket = run/nvcomm.sock   # 留空则仅 TCP

[log]
dir = log
level = info
max_size_mb = 32

[runtime]
watchdog_sec = 0          # >0 启用 /dev/watchdog
cpu_affinity = -1         # >=0 绑定到指定 CPU 核

[data]
event_dir = data/events
persist = true            # 处理请求时写入 events.log
```

| Section | 字段 | 说明 | 默认值 |
|---------|------|------|--------|
| server | listen / port | HTTP 监听地址与端口 | 0.0.0.0 / 8080 |
| server | threads | IO 线程数 | 4 |
| server | worker_threads | 包处理 worker 数 | 4 |
| server | max_body_kb | 请求 body 上限（KB） | 1024 |
| server | unix_socket | Unix domain socket 路径 | run/nvcomm.sock |
| log | dir / level | 日志目录与级别 | log / info |
| runtime | watchdog_sec | 硬件看门狗超时（秒，0=禁用） | 0 |
| runtime | cpu_affinity | 线程绑核（-1=不绑定） | -1 |
| data | event_dir | 事件持久化目录 | data/events |
| data | persist | 是否持久化 | true |

## RK3588 板端部署

安装：

```bash
./build.sh install -p /tmp/staging
# 或 cmake --install build --prefix /usr
```

路径约定（`platform/paths` 自动检测）：

| 用途 | 开发环境 | 板端 |
|------|---------|------|
| 配置 | `config/nvcomm.conf` | `/etc/nvcomm/nvcomm.conf` |
| 日志 | `log/nvcomm.log` | `/var/log/nvcomm/nvcomm.log` |
| 事件 | `data/events/` | `/data/nvcomm/events/` |
| 运行时 | `run/` | `/run/nvcomm/` |

systemd 服务：`config/systemd/nvcommd.service`

```bash
sudo cp config/nvcomm.conf /etc/nvcomm/
sudo cp config/systemd/nvcommd.service /lib/systemd/system/
sudo systemctl enable --now nvcommd
```

## 命名空间

通过 `src/namespace.hpp` 宏统一管理，默认 `NV_NAMESPACE=nvcomm`：

```cpp
NV_NS_CORE_BEGIN
class Application { ... };
NV_NS_CORE_END

// 命名空间外
NV_NS_CORE::Application app(...);
```

CMake 覆盖：

```bash
cmake -S . -B build -DNV_NAMESPACE=mycompany
```

## Boost

| 项 | 路径 |
|----|------|
| 源码 | `deps/boost_1_84_0/` |
| 安装 | `lib/boost/` |
| CMake | `Boost_NO_SYSTEM_PATHS=ON` |

切换版本：

```bash
BOOST_VERSION=1.85.0 ./build.sh boost
```

需同步更新 `CMakeLists.txt` 中 `find_package(Boost 1.84 ...)` 并将源码放入 `deps/`。

## 事件持久化

`nvcommd` 处理 POST 请求时，若 `data.persist=true`，写入 TSV 格式：

```
timestamp_ns  priority  source  topic  payload
434437170641047  0  nvcommd  /api/v1/packet  hello from client
```

文件路径：`data/events/events.log`
