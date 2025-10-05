# 第一步：打Cmake (注意：名稱只能是 → CMakeLists.txt)

# ==========================
# 設定 CMAKE 最少要求版本
# ==========================
cmake_minimum_required(VERSION 3.10) # 至少要求CMake 3.10 版本
project(Weak_ayas)					 # 定義專案名稱為 Weak_ayas

# ==========================
# 設定 C++ 的版本(這裡是C++17)
# ==========================
set(CMAKE_CXX_STANDARD 17)			 # 設定c++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # CMAKE_CXX_STANDARD_REQUIRED ON：強制編譯器必須使用 C++17，不會退回舊版本。

# ==========================
# Debug 模式自動加 -g 
# 注意：這邊一定要是debug，不然斷點進不去!
# 預設 Build Type (如果使用者沒有指定 CMAKE_BUILD_TYPE，就預設為 Debug。 可以改 Release，那預設就是 Release)
# Debug → 加上除錯資訊 (-g)，不啟用最佳化 (-O0)。
# Release → 啟用最佳化 (-O2 或 -O3)，去掉除錯資訊。
# RelWithDebInfo → 最佳化 + 部分除錯資訊。
# MinSizeRel → 最小化可執行檔大小。
# ==========================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ==========================
# CMake 選項 (options)
# 相當於在程式裡寫#define AUTO_SINGLE_CONNECT
# USE_ 前綴是 CMake 社群習慣
# 換define，要重新編譯
# 語法：option(cmake參數, 說明文字, 是否要定義 #define)
# ==========================
option(USE_AUTO_SINGLE_CONNECT "Enable Auto Single Connect" ON)
option(USE_MULTI_CONNECT "Enable Multi Connect" OFF)
# option(USE_DEBUG_MODE "Enable Debug Mode" ON) 這個不用打，上面那邊會判斷你給的是不是debug模式
# (這時候還沒有定義#define)，到target_compile_definitions(final_main PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>) 會真正判斷你有沒有需要定義 #define DEBUG_MODE 
# option(USE_ENABLE_LOGGING "Enable Logging" OFF)  # 可選

# ==========================
# 建立列表(SOURCES)
# 其實不用他也沒差，只是說明此專案有什麼cpp (下面那個set才有用!)
# ==========================
set(SOURCES
    TCPServer.cpp
	TCPClient.cpp
    EpollWorker.cpp
    ThreadPool.cpp
    SQLManager.cpp
    sql_driver.cpp
	ConsoleClient.cpp
	Weak_ayas.cpp
)

# ==========================
# Include / Libraries 
# (MySQL Connector/C++）
# ==========================
# 把 /usr/include/cppconn 加入搜尋頭文件的目錄。
include_directories(/usr/include/cppconn) # 不然編譯時 #include <mysql_connection.h> 不會找不到。(不然你程式內就要有完整路徑)
find_package(Threads REQUIRED) 			  # 找"系統上" pthread (尋找系統 pthread（POSIX Thread）庫。)

#############################################################################################################################
# ================================
# 第一個執行檔：final_main(最終測試)
# ================================
set(SOURCES_FINAL
    TCPServer.cpp
    EpollWorker.cpp
    ThreadPool.cpp
    TCPClient.cpp   # final_main 特有
    SQLManager.cpp
    sql_driver.cpp
	ConsoleClient.cpp
    final_main.cpp
)

# 執行程式
add_executable(final_main ${SOURCES_FINAL})
# 加入外部 lib
target_link_libraries(final_main
    Threads::Threads
    mysqlcppconn
)

# 根據選項定義宏
if(USE_AUTO_SINGLE_CONNECT)
    target_compile_definitions(final_main PRIVATE AUTO_SINGLE_CONNECT)
endif()

if(USE_MULTI_CONNECT)
    target_compile_definitions(final_main PRIVATE MULTI_CONNECT)
endif()

# 可以選擇 Debug/Release 自動切換
target_compile_definitions(final_main PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)
# ================================
# 第二個執行檔：Weak_ayas_server
# ================================
# 建立一個變數 SOURCES_WEAK_AYAS_SERVER，存放所有要編譯成 Weak_ayas_server 的 .cpp 檔案。
set(SOURCES_WEAK_AYAS_SERVER
    TCPServer.cpp
    EpollWorker.cpp
    ThreadPool.cpp
    SQLManager.cpp
    sql_driver.cpp
    Weak_ayas_server.cpp
)

# 建執行檔 (告訴 CMake 建立一個可執行檔 Weak_ayas_server，源碼檔案為 ${SOURCES_WEAK_AYAS_SERVER}。)
add_executable(Weak_ayas_server ${SOURCES_WEAK_AYAS_SERVER})

# 將 pthread 和 mysqlcppconn 連結到 Weak_ayas_server。
target_link_libraries(Weak_ayas_server
    Threads::Threads
    mysqlcppconn
)

# Debug mode 定義 ($<CONFIG:Debug> 條件判定式 → 如果 Build Type 是 Debug，就在編譯時定義 DEBUG_MODE，DEBUG_MODE 自己定義的宏)
# #ifdef DEBUG_MODE 可以用他來切換要跑的位置
target_compile_definitions(Weak_ayas_server PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)
# ================================
# 第三個執行檔：Weak_ayas_client
# ================================
# 建立一個變數 SOURCES_WEAK_AYAS_CLIENT，存放所有要編譯成 Weak_ayas_client 的 .cpp 檔案。
set(SOURCES_WEAK_AYAS_CLIENT
	TCPClient.cpp
	sql_driver.cpp
    SQLManager.cpp
	ConsoleClient.cpp
    Weak_ayas_client.cpp
)

# 建執行檔 (告訴 CMake 建立一個可執行檔 Weak_ayas_client，源碼檔案為 ${SOURCES_WEAK_AYAS_CLIENT}。)
add_executable(Weak_ayas_client ${SOURCES_WEAK_AYAS_CLIENT})

# 將 pthread 和 mysqlcppconn 連結到 Weak_ayas_client。
target_link_libraries(Weak_ayas_client
    Threads::Threads
    mysqlcppconn # 沒用到(但是 SQLManager 連接到 sql_driver 就會連接到 mysqlcppconn)
)

# Debug mode 定義 ($<CONFIG:Debug> 條件判定式 → 如果 Build Type 是 Debug，就在編譯時定義 DEBUG_MODE，DEBUG_MODE 自己定義的宏)
# #ifdef DEBUG_MODE 可以用他來切換要跑的位置
target_compile_definitions(Weak_ayas_client PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)
# ================================
# 第四個執行檔：Weak_ayas
# ================================
# 建立一個變數 SOURCES_WEAK，存放所有要編譯成 Weak_ayas 的 .cpp 檔案。
set(SOURCES_WEAK
	Weak_ayas.cpp
)

# 建執行檔 (告訴 CMake 建立一個可執行檔 Weak_ayas，源碼檔案為 ${SOURCES_WEAK}。)
add_executable(Weak_ayas ${SOURCES_WEAK})

# 將 pthread 和 mysqlcppconn 連結到 Weak_ayas。
target_link_libraries(Weak_ayas
    # 無需額外連結 (沒有到 thread)
)

# Debug mode 定義 ($<CONFIG:Debug> 條件判定式 → 如果 Build Type 是 Debug，就在編譯時定義 DEBUG_MODE，DEBUG_MODE 自己定義的宏)
# #ifdef DEBUG_MODE 可以用他來切換要跑的位置
target_compile_definitions(Weak_ayas PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)



# 第二步 terminal code
# Debug
# cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug (這裡如果有切換 #define(option)，那就要執行這裡)
# cmake --build build_debug --target Weak_ayas	(如果只是編輯程式，那只需要跑這裡)
# cmake --build build_debug --target final_main

# Release
# cmake -B build_release -DCMAKE_BUILD_TYPE=Release
# cmake --build build_release --target Weak_ayas
# cmake --build build_release --target final_main

# 第三步(這步給vscode用)，編譯launch.json → 路徑：專案/.vs/launch.json 
#(詳細範例請看 launch.json)

# 第四步：按 F5，開始Debug

# 補充：
# 1.若更動太大，建議 rm -rf (build_release/build_debug) 在重新編譯
