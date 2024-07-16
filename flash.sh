#!/bin/bash

# 设置相关变量
ELF_FILE="./build/bootloader.elf"
GDB_SERVER_IP="localhost"
GDB_SERVER_PORT="2331"

# 使用 GDB 连接到 JLink GDB Server 并烧写 ELF 文件
arm-none-eabi-gdb -q --batch \
                  -ex "set pagination off" \
                  -ex "target remote $GDB_SERVER_IP:$GDB_SERVER_PORT" \
                  -ex "monitor reset" \
                  -ex "load $ELF_FILE" \
                  -ex "monitor reset" \
                  -ex "monitor go" \
                  -ex "disconnect" \
                  -ex "quit"

echo "Flashing completed successfully."
