#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include "sql_driver.hpp"
#include "protocol.hpp"


namespace sqlmanager {

class SQLManager {
private:
    std::unordered_map<int, int> userFdToId; // fd -> user_id
    sql_driver::SQL_Driver db;               // 資料庫驅動
	int special_id;							 // 給的用特殊操作	

    // ===== 封包處理 =====
    bool SendData(int fd, uint8_t type, const std::string& payload);

    // ===== Handlers =====
    void handle_register(int fd, const protocol::SocketPackage& pkg);
    void handle_login(int fd, const protocol::SocketPackage& pkg);
    void handle_unitsdata(int fd, const protocol::SocketPackage& pkg);
    void handle_unitdetail(int fd, const protocol::SocketPackage& pkg);
    void handle_problem_submissions(int fd, const protocol::SocketPackage& pkg);
    void handle_insert_submission(int fd, const protocol::SocketPackage& pkg);
    void handle_delete_submission(int fd, const protocol::SocketPackage& pkg);

    // ===== Helper =====
    bool check_login(int fd);
    bool string_to_bool(const std::string& s);
    std::vector<std::string> getdata(const protocol::SocketPackage& pkg);
    std::string get_units_table(const std::vector<sql_driver::Units>& units) const;
    std::string get_unit_stats_table(const std::vector<sql_driver::UnitStat>& unitStats) const;
    std::string get_problem_submit_table(const std::unordered_map<long long, sql_driver::ExamInfo>& user_exam_submits) const;
	
	//For Oscar(me)
	void handle_delete_user(int fd, const protocol::SocketPackage& pkg);
    void handle_insert_problem(int fd, const protocol::SocketPackage& pkg);
    void handle_delete_problem(int fd, const protocol::SocketPackage& pkg);
    void handle_insert_problem_type(int fd, const protocol::SocketPackage& pkg);
    void handle_delete_problem_type(int fd, const protocol::SocketPackage& pkg);

public:
    SQLManager() = default;
	std::string build_package(uint8_t type, const std::string& payload);
    void handle_request(int fd, const protocol::SocketPackage& pkg);
    void erase_fd(int fd);
};

} // namespace sqlmanager
