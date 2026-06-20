# boostApp (nvcomm)

面向 **RK3588 嵌入式 Linux** 的 **标准 API 网关**：接收 **nginx 反向代理**转发的 HTTP 请求，经 **中间件链 → 路由 → Handler** 处理，支持多线程并发与可选事件持久化。

项目使用**自编译 Boost**（不依赖系统 Boost），源码与编译产物 vendored 在仓库中，clone 后可直接构建。

## 网关架构

```
Client
  │
  ▼
nginx (L7 反向代理, proxy_pass)
  │
  ▼
HttpServer (Boost.Beast, TCP + Unix socket)
  │
  ▼
Gateway
  ├─ Middleware Pipeline
  │    ├─ AccessLogMiddleware      访问日志
  │    └─ BodyLimitMiddleware      请求体大小限制
  │
  ├─ Router (method + path 匹配)
  │    ├─ GET  /health        → HealthHandler   (同步)
  │    └─ POST /api/*         → PacketHandler   (异步 worker)
  │
  └─ thread_pool (worker 线程池, 并发执行业务 Handler)
         │
         └─ FileEventStore (可选持久化)
```

| 层级 | 组件 | 职责 |
|------|------|------|
| **infra/net** | `HttpServer` | 传输层：异步 accept/read/write |
| **service/gateway** | `Gateway` | 编排中间件链与路由分发 |
| **service/gateway** | `Router` | 按 method + path 匹配 Handler |
| **service/gateway/middleware** | 中间件 | 横切关注点（日志、限流等） |
| **service/gateway/handlers** | Handler | 业务逻辑（健康检查、数据包处理） |
| **domain/gateway** | Request/Response/Context | 网关领域模型 |
| **domain/ports** | IMiddleware / IHandler | 扩展接口 |

### 并发模型

1. **IO 线程池**（`server.threads`）：多个 `io_context` 线程负责连接与 HTTP 读写。
2. **Worker 线程池**（`server.worker_threads`）：标记 `async=true` 的路由在 worker 中执行 Handler。
3. 处理完成后通过 `asio::post` 回到连接 executor 写回响应。

## HTTP API

| 方法 | 路径 | Handler | 说明 |
|------|------|---------|------|
| GET | `/health` | HealthHandler | 健康检查，返回 `ok` |
| POST | `/api/...` | PacketHandler | 提交数据包，返回 JSON |

POST 响应示例：

```json
{"status":"ok","path":"/api/v1/packet","bytes":16,"client":"127.0.0.1:60282"}
```

## 目录结构

```
boostApp/
├── build.sh
├── CMakeLists.txt
├── config/
│   ├── nvcomm.conf
│   ├── nginx/nvcomm.conf         # nginx upstream 示例
│   └── systemd/nvcommd.service
│
├── deps/boost_1_84_0/
├── lib/boost/
│
└── src/
    ├── app/nvcommd/main.cpp
    ├── core/                     # Application、Config、Logger
    ├── platform/                 # RK3588：绑核、看门狗、systemd
    ├── domain/
    │   ├── gateway/                # GatewayRequest/Response/Context
    │   └── ports/                  # IMiddleware、IHandler、IEventStore
    ├── infra/net/                  # HttpServer、IoContextPool
    ├── infra/persistence/          # FileEventStore
    └── service/
        ├── gateway/                # Gateway、Router
        │   ├── middleware/         # 中间件实现
        │   └── handlers/           # 路由 Handler
        └── gateway_service.cpp     # 网关生命周期 + 路由注册
```

## 扩展网关

新增路由（在 `GatewayFactory::create` 中注册）：

```cpp
gateway->add_route({.method = "POST",
                    .path = "/api/v2/",
                    .prefix = true,
                    .async = true,
                    .handler = std::make_shared<MyHandler>(config)});
```

新增中间件：

```cpp
class MyMiddleware : public domain::IMiddleware {
    void process(domain::GatewayContext& ctx, domain::NextMiddleware next) override {
        // 前置逻辑
        next();
        // 后置逻辑
    }
};
gateway->use(std::make_shared<MyMiddleware>());
```

## 依赖

- CMake >= 3.16
- C++20 编译器（GCC 11+ / Clang 14+）
- build-essential、tar、curl 或 wget

```bash
sudo apt install build-essential cmake curl
```

## 构建

```bash
./build.sh              # 构建
./build.sh rebuild      # 清理重编
./build.sh run          # HTTP 冒烟测试
```

## 运行

```bash
mkdir -p run log data/events
./build/nvcommd -c config/nvcomm.conf

curl http://127.0.0.1:8080/health
curl -X POST http://127.0.0.1:8080/api/v1/packet -d 'hello'
```

### nginx 集成

```bash
sudo cp config/nginx/nvcomm.conf /etc/nginx/conf.d/
sudo nginx -t && sudo systemctl reload nginx
```

## 配置

```conf
[server]
listen = 0.0.0.0
port = 8080
threads = 4              # IO 线程数
worker_threads = 4       # 异步 Handler worker 数
max_body_kb = 1024
unix_socket = run/nvcomm.sock

[log]
dir = log
level = info

[runtime]
watchdog_sec = 0
cpu_affinity = -1

[data]
event_dir = data/events
persist = true
```

## RK3588 板端部署

```bash
./build.sh install -p /tmp/staging
sudo cp config/nvcomm.conf /etc/nvcomm/
sudo cp config/systemd/nvcommd.service /lib/systemd/system/
sudo systemctl enable --now nvcommd
```

## 命名空间

默认 `NV_NAMESPACE=nvcomm`，见 `src/namespace.hpp`。

## Boost

| 项 | 路径 |
|----|------|
| 源码 | `deps/boost_1_84_0/` |
| 安装 | `lib/boost/` |

## 事件持久化

POST 请求经 PacketHandler 处理时，若 `data.persist=true`，写入 TSV：

```
timestamp_ns  priority  source  topic  payload
```

文件路径：`data/events/events.log`
