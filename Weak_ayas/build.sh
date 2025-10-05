#!/bin/bash

# ================================
# Build script for Weak_ayas project
# ================================

# 預設 build type
BUILD_TYPE=${1:-Debug}   # 第一個參數：Build type
TARGET=${2:-all}         # 第二個參數：Target
CLEAN=${3:-0}            # 第三個參數：是否刪除舊 build 目錄 (0 = 不刪, 1 = 刪除)

# build 目錄名稱
BUILD_DIR="build_${BUILD_TYPE,,}"  # 轉小寫，例如 Debug -> build_debug

# 選擇性刪除舊 build 目錄
if [ "$CLEAN" -eq 1 ]; then
    echo "Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "=== Building target ${TARGET} in ${BUILD_TYPE} mode ==="
#if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
#    OLD_TYPE=$(grep "CMAKE_BUILD_TYPE:STRING" "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2)
#    if [ "$OLD_TYPE" != "$BUILD_TYPE" ]; then
#        echo "Build type changed: $OLD_TYPE -> $BUILD_TYPE, removing old build directory..."
#        rm -rf "$BUILD_DIR"
#    fi
#fi
# 建立 build 目錄（如果不存在）
mkdir -p "$BUILD_DIR"

# 第一步：cmake configure
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
# 重新 configure 強制覆蓋舊設定 (-U 是 “unset variable”，清掉舊的 Cache，保證用新的設定)
# cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -U CMAKE_BUILD_TYPE

# 第二步：build 指定 target
cmake --build "$BUILD_DIR" --target "$TARGET"

echo "=== Build finished ==="


# 使用方式：
# ==========================
# 可執行
# chmod +x build.sh

# 只 build，保留舊目錄(0可以省略)
# ./build.sh Debug all 
# ./build.sh Release all 

# 刪除舊目錄再 build
# ./build.sh Debug all 1
# ./build.sh Release all 1

# 只編譯其中一個
# ./build.sh Debug Weak_ayas_client (0/1)
# ./build.sh Release Weak_ayas_server (0/1) 
# ==========================
