# LeetCode Tracker (Weak_ayas)
C++ Console Application for tracking personal LeetCode problem-solving performance.  
Helps you analyze your strengths and weaknesses across different problem categories.


## 📝 Features
- Runs on Linux with console-based C/C++ interface
- Track solved and attempted problems on LeetCode by the problem category (unit).
- Calculate success rate for each problem category.
- Identify problems attempted multiple times.
- Multi-threaded TCP client/server architecture (optional for future expansion).
- Organize and analyze personal weaknesses using SQL queries
- Supports Debug/Release build modes with CMake.


## 📁 Project Structure
Weak_ayas/

├─ src/ 	# Source code

      ├─ TCPServer.cpp   
      ├─ TCPClient.cpp   
      ├─ EpollWorker.cpp  
      ├─ ThreadPool.cpp   
      ├─ SQLManager.cpp
      ├─ sql_driver.cpp
      ├─ ConsoleClient.cpp
      ├─ final_main.cpp
      ├─ Weak_ayas_client.cpp
      ├─ Weak_ayas_server.cpp
      ├─ Weak_ayas.cpp
      
├─ include/	# Header files

      ├─ TCPServer.hpp
      ├─ TCPClient.hpp
      ├─ EpollWorker.hpp
      ├─ ThreadPool.hpp
      ├─ SQLManager.hpp
      ├─ sql_driver.hpp
      ├─ ConsoleClient.hpp
      
├─ build/ # Build directory (ignored in Git)

      ├─ ./Weak_ayas/
   
├─ CMakeLists.txt # CMake build configuration

      ├─ build Debug/Release(DEBUG_MODE/RELEASE)
   
├─ README.md

└─ data/ # SQL database files or config

      ├─ CreateDB Permissions.sql
      ├─ CreateTable init.sql
      ├─ TestData test_data.sql
      ├─ DisplayTable show_table.sql


## ⚙️ Build & Run
- Directions:

  shell is in the ./Weak_ayas/
- USAGE:

   chmod +x build.sh

- Build the Project  (0: can be omitted, 1: rm -rf rebuild)
  
   ./build.sh Debug all (0/1)
  
   ./build.sh Release all (0/1)

- Rebuild individual files (0: can be omitted, 1: rm -rf rebuild)
  
   ./build.sh Debug Weak_ayas_client (0/1)
  
   ./build.sh Release Weak_ayas_server (0/1) 


## 🚀 Usage
 "=== Available instructions ===" 
 
      "login <username>" 
      "register <username>" 
      "delete_user <username>" 
      "insert_problem <problem_id> <title> <difficulty>" 
      "delete_problem <problem_id>" 
      "insert_type <problem_id> <type>" 
      "delete_type <problem_id> <type>" 
      "query_units_data"
      "query_unit_detail <unit>" 
      "get_submission <problem_id>" << std::endl;
      "insert_submission <problem_id> <status> <hh:mm:ss>"
      "delete_submission <submission_id> <user_id>"
      "@back or press \"Enter\" go back to the initial page" 
      "exit"

Ex：

      ## cout << Enter service order:
      # input: login
      ## cout << [login] Enter your name:
      # input: Oscar
      ## cout << Received: [Login] Oscar login success (user_id = 1)
      ## cout << Enter service order:
      # input: query_units_data
      ## cout << Received: [UnitsData] Send units stats for user_id = 1
      ## Show:
      
      Unit           Solved    Total Submit   20min(%)       30min(%)       >30min(%)
      ----------------------------------------------------------------------------------
      Tree           3         6              1(16.67%)      1(16.67%)      5(83.33%)
      Segment Tree   2         2              0(0.00%)       0(0.00%)       2(100.00%)
      Array          2         5              4(80.00%)      4(80.00%)      1(20.00%)
      Interval       1         1              0(0.00%)       0(0.00%)       1(100.00%)
      Two Pointer    1         2              2(100.00%)     2(100.00%)     0(0.00%)
      Two Pointers   1         1              1(100.00%)     1(100.00%)     0(0.00%)
      Hash Table     1         1              0(0.00%)       1(100.00%)     0(0.00%)

      ## cout << Enter service order:
      input: exit
