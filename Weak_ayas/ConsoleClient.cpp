#include "TCPClient.hpp"
#include "protocol.hpp"
#include "SQLManager.hpp"
#include "ConsoleClient.hpp"
#include <iostream>
#include <sstream>

using namespace tcpclient;
using namespace sqlmanager;
using namespace protocol;
using namespace protocol::sql_protocol;
using namespace std;

namespace console_client {
    ConsoleClient::ConsoleClient(ClientTCP& client)
        : client(client) {}

    void ConsoleClient::print_help() {
        std::cout << "\n=== 可用指令 ===" << std::endl;
        std::cout << "login <username>" << std::endl;
        std::cout << "register <username>" << std::endl;
        std::cout << "delete_user <username>" << std::endl;
        std::cout << "insert_problem <problem_id> <title> <difficulty>" << std::endl;
        std::cout << "delete_problem <problem_id>" << std::endl;
        std::cout << "insert_type <problem_id> <type>" << std::endl;
        std::cout << "delete_type <problem_id> <type>" << std::endl;
        std::cout << "query_units_data" << std::endl;
        std::cout << "query_unit_detail <unit>" << std::endl;
        std::cout << "get_submission <problem_id>" << std::endl;
        std::cout << "insert_submission <problem_id> <status> <hh:mm:ss>" << std::endl;
        std::cout << "delete_submission <submission_id> <user_id>" << std::endl;
        std::cout << "@back or press \"Enter\" go back to the initial page" << std::endl;
        std::cout << "exit" << std::endl;
    }

    ConsoleClient::InputResult ConsoleClient::get_valid_input(const std::string& prompt, std::string& out,
                            bool(*validator)(const std::string&),
                            bool check_positive_int) 
    {
        while (true) {
            std::cout << prompt;
            if (!std::getline(std::cin, out)) return InputResult::EXIT;   // 外層 loop 結束

            if (out.empty() || check_global_command(out)) {       
                size_t start = prompt.find('[',0);
                size_t end = prompt.find(']',start);
                std::string func_name = (start != std::string::npos && end != std::string::npos)
                                        ? prompt.substr(start, end - start + 1)
                                        : "";
                if (!func_name.empty()) std::cout << func_name << " Go back." << std::endl;
                return InputResult::BACK; 
            } 

            // 驗證流程
            bool valid = true;
            if (check_positive_int) valid = is_positive_integer(out);
            if (validator) valid = valid && validator(out);

            if (valid) return InputResult::SUCCESS;

            std::cout << "Invalid input. Please try again.\n";
        }
    }

    void ConsoleClient::run() {
        std::string cmd;
        std::cout << "=== Console Client Started ===" << std::endl;
        std::cout << "輸入 'help' 查看可用指令, 'exit' 離開, '@back' or Enter 返回：" << std::endl;

        while (true) {
            if(client.client_get_server_stats() == false){
                std::cout << "Server is down, exiting console." << std::endl;
                break;
            }
            string Title_Description;
            InputResult result;
            std::cout << "Enter service order: \n";
            if (!std::getline(std::cin, cmd)) 
                break;
            if (cmd.empty()) continue;

            if (cmd == "exit") {
                std::cout << "[exit] Bye!" << std::endl;
                break;
            } 
            else if (cmd == "help") {
                print_help();
            }
            else if (cmd == "login") {
                std::string username;
                Title_Description = "[login] Enter your name: \n";
                result = get_valid_input(Title_Description, username, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                auto msg = mgr_SQL.build_package(LOGIN, "#" + username);
                client.send_msg(msg);
            } 
            else if (cmd == "register") {
                std::string username;
                Title_Description = "[register] Enter your name: \n";
                result = get_valid_input(Title_Description, username, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                auto msg = mgr_SQL.build_package(REGISTER_USER, "#" + username);
                client.send_msg(msg);
            } 
            else if (cmd == "delete_user") {
                std::string username;
                Title_Description = "[delete_user] Enter the delete name: \n";
                result = get_valid_input(Title_Description, username, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                auto msg = mgr_SQL.build_package(DELETE_USER, "#" + username);
                client.send_msg(msg);
            } 
            else if (cmd == "insert_problem") {
                std::string id, title, difficulty;
                //輸入題號
                Title_Description = "[insert_problem] Enter the insert exam No.: \n";
                result = get_valid_input(Title_Description, id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                //輸入題目標題
                Title_Description = "[insert_problem] Enter the title: \n";
                result = get_valid_input(Title_Description, title, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                //輸入題目難度
                Title_Description = "[insert_problem] Enter the difficulty(Med./Hard/Easy): \n";
                result = get_valid_input(Title_Description, difficulty, validate_difficulty);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + id + "#" + title + "#" + difficulty;
                auto msg = mgr_SQL.build_package(INSERT_PROBLEM, data);
                client.send_msg(msg);
            }
            else if (cmd == "delete_problem") {
                std::string problem_id;
                //輸入題號
                Title_Description = "[delete_problem] Enter the delete exam No.: \n";
                result = get_valid_input(Title_Description, problem_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + problem_id;
                auto msg = mgr_SQL.build_package(DELETE_PROBLEM, data);
                client.send_msg(msg);
            }
            else if (cmd == "insert_type") {
                std::string problem_id, type;
                //輸入題號
                Title_Description = "[insert_type] Enter the insert exam No.: \n";
                result = get_valid_input(Title_Description, problem_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                //輸入題目類型
                Title_Description = "[insert_type] Enter the type: \n";
                result = get_valid_input(Title_Description, type, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                std::string data = "#" + problem_id + "#" + type;
                auto msg = mgr_SQL.build_package(INSERT_PROBLEM_TYPE, data);
                client.send_msg(msg);
            }
            else if (cmd == "delete_type") {
                std::string problem_id, type;
                //輸入題號
                Title_Description = "[delete_type] Enter the delete exam No.: \n";
                result = get_valid_input(Title_Description, problem_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                //輸入題目類型
                Title_Description = "[delete_type] Enter the type: \n";
                result = get_valid_input(Title_Description, type, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + problem_id + "#" + type;
                auto msg = mgr_SQL.build_package(DELETE_PROBLEM_TYPE, data);
                client.send_msg(msg);
            }
            else if (cmd == "query_units_data") {
                //不用輸入直接傳送
                auto msg = mgr_SQL.build_package(SELECT_UNITS_DATA, "");
                client.send_msg(msg);
            }
            else if (cmd == "query_unit_detail") {
                std::string type;
                //輸入題目類型
                Title_Description = "[query_unit_detail] Enter the type: \n";
                result = get_valid_input(Title_Description, type, nullptr);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + type;
                auto msg = mgr_SQL.build_package(SELECT_UNIT_DETAIL, data);
                client.send_msg(msg);
            }
            else if (cmd == "get_submission") {
                //輸入題號
                std::string problem_id;
                Title_Description = "[get_submission] Enter the exam No.: \n";
                result = get_valid_input(Title_Description, problem_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + problem_id;
                auto msg = mgr_SQL.build_package(GET_PROBLEM_SUBMISSIONS, data);
                client.send_msg(msg);
            }
            else if (cmd == "insert_submission") {
                std::string problem_id, solve_status, time_spent;
                //輸入題號
                Title_Description = "[insert_submission] Enter the insert exam No.: \n";
                result = get_valid_input(Title_Description, problem_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                //輸入解題狀況
                Title_Description = "[insert_submission] Enter the solve status(0/1): \n";
                result = get_valid_input(Title_Description, solve_status, validate_solve_status);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;
                
                //輸入花費時間(hh::mm::ss)
                Title_Description = "[insert_submission] Enter the spent time(hh:mm:ss): \n";
                result = get_valid_input(Title_Description, time_spent, validate_time_spent);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue; 
                
                std::string data = "#" + problem_id + "#" + solve_status + "#" + time_spent;
                auto msg = mgr_SQL.build_package(INSERT_SUBMITTED, data);
                client.send_msg(msg);
            }
            else if (cmd == "delete_submission") {
                std::string submission_id;
                //輸入刪除提交號碼
                Title_Description  = "[delete_submission] Enter the delete submission ID: \n";
                result = get_valid_input(Title_Description, submission_id, nullptr, true);
                if (result == InputResult::EXIT) break;
                if (result == InputResult::BACK) continue;

                std::string data = "#" + submission_id;
                auto msg = mgr_SQL.build_package(DELETE_SUBMITTED, data);
                client.send_msg(msg);
            }
            else {
                std::cout << "[Error] Unknown command. Type 'help' for list.\n" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

