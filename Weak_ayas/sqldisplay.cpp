#include <iostream>
#include <memory>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

#include <thread>
#include <chrono>
#include <iomanip>  // std::setw
#include <string>
#include <vector>
#include <unordered_map>
#include "sqldisplay.hpp"  // SQL_Display.h

using namespace std;

namespace sql_display {
bool SQL_Display::isConnected() {
	int attempts = 0;
	while (!con && attempts < 5) {
		try {
			con.reset(driver->connect("tcp://127.0.0.1:3306", "oscar", "oscar841004"),
					  [](sql::Connection* ptr){ delete ptr; });
		} catch (sql::SQLException& e) {
			std::cerr << "連接失敗，3秒後重試..." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			attempts++;
		}
	}
	return con != nullptr;
}


SQL_Display::SQL_Display() {
	try {
		driver = get_driver_instance();
		if (!isConnected()) 
			std::cerr << "連接失敗，請確認SQL相關連線!" << std::endl;
		else {
			try { 
				con->setSchema("leetcode_tracker"); 
			} catch (sql::SQLException &e) {
				std::cerr << "設定 schema 失敗: " << e.what() << std::endl;
				con.reset(); // 標記連線失敗
			}
		}
		
	} catch (sql::SQLException &e) {
		std::cerr << "Error connecting to DB：" << e.what() << std::endl;
	}
}

void SQL_Display::displayUsers() {
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return;
	}
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("SELECT * FROM users ORDER BY id ASC")
		);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		std::cout << std::left
				  << std::setw(10) << "ID"
				  << std::setw(20) << "Username"
				  << std::setw(12) << "CreatedAt"
				  << std::endl;
		std::cout << std::string(42, '-') << std::endl;

		while (res->next()) {
			std::cout << std::left
					  << std::setw(10) << res->getInt("id")
					  << std::setw(20) << res->getString("username")
					  << std::setw(12) << res->getString("created_at")
					  << std::endl;
		}
	} catch (sql::SQLException& e) {
		std::cerr << "Error displaying users: " << e.what() << std::endl;
	}
}

void SQL_Display::displayProblems() {
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return;
	}
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("SELECT * FROM problems ORDER BY id ASC")
		);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		std::cout << std::left
				  << std::setw(10) << "ID"
				  << std::setw(40) << "Title"
				  << std::setw(12) << "Difficulty"
				  << std::endl;
		std::cout << std::string(62, '-') << std::endl;

		while (res->next()) {
			std::cout << std::left
					  << std::setw(10) << res->getInt("id")
					  << std::setw(40) << res->getString("title")
					  << std::setw(12) << res->getString("difficulty")
					  << std::endl;
		}
	} catch (sql::SQLException& e) {
		std::cerr << "Error displaying problems: " << e.what() << std::endl;
	}
}

void SQL_Display::displayProblemTypeMap() {
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return;
	}
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("SELECT * FROM problem_type_map ORDER BY problem_type ASC, problem_id ASC")
		);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		std::cout << std::left
				  << std::setw(20) << "ProblemType"
				  << std::setw(12) << "ProblemID"
				  << std::endl;
		std::cout << std::string(32, '-') << std::endl;

		while (res->next()) {
			std::cout << std::left
					  << std::setw(20) << res->getString("problem_type")
					  << std::setw(12) << res->getInt("problem_id")
					  << std::endl;
		}
	} catch (sql::SQLException& e) {
		std::cerr << "Error displaying problem_type_map: " << e.what() << std::endl;
	}
}

void SQL_Display::displaySubmissions(int userid) {
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return;
	}
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement(
				"SELECT * FROM submissions WHERE users_id = ? ORDER BY problem_id ASC, id DESC"
			)
		);
		pstmt->setInt(1, userid);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		std::cout << std::left
				  << std::setw(10)  << "ID"
				  << std::setw(10) << "UserID"
				  << std::setw(12) << "ProblemID"
				  << std::setw(12)  << "Status"
				  << std::setw(12) << "TimeSpent"
				  << std::setw(12) << "TimeMin"
				  << std::setw(12) << "Date"
				  << std::endl;
		std::cout << std::string(80, '-') << std::endl;

		while (res->next()) {
			std::cout << std::left
					  << std::setw(10)  << res->getInt64("id")
					  << std::setw(10) << res->getInt("users_id")
					  << std::setw(12) << res->getInt("problem_id")
					  << std::setw(12)  << (res->getBoolean("status") ? "Solved" : "Unsolved")
					  << std::setw(12) << res->getString("time_spent")
					  << std::setw(12) << res->getInt("time_spent_min")
					  << std::setw(12) << res->getString("submit_date")
					  << std::endl;
		}
	} catch (sql::SQLException& e) {
		std::cerr << "Error displaying submissions: " << e.what() << std::endl;
	}
}

SQL_Display::~SQL_Display(){}
}

//測試
/*
int main() {
	SQL_Display sqlDisplay;

    std::cout << "\n=== Users ===\n";
    sqlDisplay.displayUsers();

    std::cout << "\n=== Problems ===\n";
    sqlDisplay.displayProblems();

    std::cout << "\n=== Problem Type Map ===\n";
    sqlDisplay.displayProblemTypeMap();

    std::cout << "\n=== Submissions for user 1 ===\n";
    sqlDisplay.displaySubmissions();
	
	std::cout << "\n=== Submissions for user 1 ===\n";
    sqlDisplay.displaySubmissions(2);
	
    return 0;
}*/




