#pragma once
#include "TCPClient.hpp"
#include "protocol.hpp"
#include "SQLManager.hpp"
#include <string>
#include <iostream>
#include <cctype>

 namespace tcpclient{
    class ClientTCP;
 }
 namespace sqlmanager{
    class SQLManager;
 }

namespace console_client {
class ConsoleClient {

    enum class InputResult {
        SUCCESS,    // 驗證通過
        BACK,       // 空行，返回 service order
        EXIT        // getline 失敗，整個 console loop 結束
    };

    tcpclient::ClientTCP& client;
    sqlmanager::SQLManager mgr_SQL;

    void print_help();

    InputResult get_valid_input(const std::string& prompt, std::string& out,
                                bool(*validator)(const std::string&) = nullptr,
                                bool check_positive_int = false);

    // ========== inline 小函數 ==========
    static inline bool validate_difficulty(const std::string& diff) {
        return diff == "Easy" || diff == "Med." || diff == "Hard";
    }

    static inline bool validate_solve_status(const std::string& status) {
        return status == "0" || status == "1";
    }

    static inline bool validate_time_spent(const std::string& time_str) {
        if (time_str.size() != 8) return false;
        if (time_str[2] != ':' || time_str[5] != ':') return false;
        for (size_t i = 0; i < time_str.size(); ++i) {
            if (i == 2 || i == 5) continue;
            if (!std::isdigit(time_str[i])) return false;
        }

        int hh = std::stoi(time_str.substr(0, 2));
        int mm = std::stoi(time_str.substr(3, 2));
        int ss = std::stoi(time_str.substr(6, 2));

        if (hh < 0 || hh > 23) return false;
        if (mm < 0 || mm > 59) return false;
        if (ss < 0 || ss > 59) return false;

        return true;
    }

    inline bool check_global_command(std::string input) {
        input.erase(0, input.find_first_not_of(" \t\r\n"));
        input.erase(input.find_last_not_of(" \t\r\n") + 1);
        return input == "@back";
    }


    inline bool is_positive_integer(const std::string& s) {
        try {
            if (s.empty()) return false;
            if (s.find_first_not_of("0123456789") != std::string::npos)
                return false;
            long long n = std::stoll(s);
            return n > 0;
        } catch (...) {
            return false;
        }
    }
    public:
    ConsoleClient(tcpclient::ClientTCP& client);
    void run();
};
}
