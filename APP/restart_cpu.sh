#!/bin/bash

# 设置 GDB 路径（根据你的安装路径调整）
GDB_PATH="/usr/local/bin/arm-none-eabi-gdb"

# 设置 J-Link GDB Server 端口（确保与你的配置一致）
GDB_SERVER_PORT="2331"

# 创建临时 GDB 脚本文件
GDB_SCRIPT=$(mktemp)

# 写入 GDB 脚本内容
cat <<EOF > $GDB_SCRIPT
# 连接到 J-Link GDB Server
target remote localhost:$GDB_SERVER_PORT

# 执行重启命令
monitor reset

# 退出 GDB
quit
EOF

# 执行 GDB 脚本文件
{
  # 等待两秒
  sleep 2
  
  # 执行 GDB 脚本
  $GDB_PATH -x $GDB_SCRIPT
} &

# 打印消息
echo "MCU reset command will be sent after a delay."
