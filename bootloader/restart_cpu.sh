#!/bin/bash
GDB_SERVER_IP="localhost"
GDB_SERVER_PORT="2331"

# 使用 GDB 连接到 JLink GDB Server 并烧写 ELF 文件
arm-none-eabi-gdb \
                  -ex "target remote $GDB_SERVER_IP:$GDB_SERVER_PORT"

# 手动输入 monitor reset