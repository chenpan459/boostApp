# boostApp

基于 Boost 的 Linux 进程间通信示例：守护进程 + 发布/订阅客户端。

## 架构

```
boardcomm_pub  --[inbound MQ]-->  boardcommd  --[outbound MQ]-->  boardcomm_sub
```

- **boardcommd**：守护进程，从 inbound 队列读消息并转发到 outbound 队列
- **boardcomm_pub**：发布端，向 inbound 队列写入消息
- **boardcomm_sub**：订阅端，从 outbound 队列读取消息

## 依赖

- CMake >= 3.16
- C++20 编译器
- Boost >= 1.74（thread, filesystem, asio, interprocess, property_tree）

Ubuntu/Debian:

```bash
sudo apt install build-essential cmake libboost-all-dev
```

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

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

默认配置文件：`config/boardcomm.json`

| 字段 | 说明 |
|------|------|
| mq_in_name | 发布端写入的队列名 |
| mq_out_name | 订阅端读取的队列名 |
| max_messages | 队列最大消息数 |
| max_message_size | 单条消息最大字节 |
| log_dir | 日志目录 |
| log_level | trace/debug/info/warn/error/fatal |
| poll_interval_ms | 轮询间隔（毫秒） |

## 目录结构

```
src/
├── app/           # 可执行入口
├── core/          # 应用框架（配置、日志、生命周期）
├── domain/        # 领域接口（IMessageBus）
├── infra/         # Boost IPC 实现
└── service/       # 业务编排
```
