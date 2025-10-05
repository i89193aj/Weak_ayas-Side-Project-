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
#include "sql_driver.hpp"  // SQL_Diver.h

using namespace std;
namespace sql_driver{
//判定是否連線

bool SQL_Driver::isConnected() {
    auto con = getConnection();
    return con != nullptr;


	/*int attempts = 0;
	while (!con && attempts < 5) {
		try {
			con.reset(driver->connect("tcp://127.0.0.1:3306", "oscar", "oscar841004"),
					  [](sql::Connection* ptr){ delete ptr; });

			// <<< 改動1：設定 schema 和 transaction 隔離級別
			con->setSchema("leetcode_tracker");                      // <<< 改動2
			con->setAutoCommit(true);                                 // <<< 改動3，自動提交
			std::unique_ptr<sql::PreparedStatement> iso(
				con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
			);
			iso->execute();                                           // <<< 改動4

		} catch (sql::SQLException& e) {
			std::cerr << "連接失敗或初始化失敗，3秒後重試..." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			attempts++;
		}
	}
	return con != nullptr;*/
}

// <<< 新增方法，每次呼叫都建立新的 connection
std::unique_ptr<sql::Connection> SQL_Driver::getConnection() {
    /*return std::unique_ptr<sql::Connection>(
        driver->connect(url, user, password)
    );*/
	try { 
		auto conn = std::unique_ptr<sql::Connection>( 
			driver->connect(url, user, password)
		); 
		
		conn->setSchema("leetcode_tracker"); 
		conn->setAutoCommit(true); 
		std::unique_ptr<sql::Statement> stmt(conn->createStatement()); 
		stmt->execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ"); 
		return conn; 
	} catch (sql::SQLException& e) { 
		std::cerr << "Failed to create connection: " << e.what() << std::endl; 
		return nullptr; 
	}
}


//初始化，加入 transaction、commit、rollback
SQL_Driver::SQL_Driver(): url("tcp://127.0.0.1:3306"), user("oscar"), password("oscar841004")  {
	try {
		driver = get_driver_instance();
		/*if (!isConnected()) {
			std::cerr << "連接失敗，請確認SQL相關連線!" << std::endl;
		} else {
			try { 
				con->setSchema("leetcode_tracker"); 
				con->setAutoCommit(true);                     // <<< 改動1，預設自動提交
				std::unique_ptr<sql::PreparedStatement> iso(
					con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ") // <<< 改動2
				);
				iso->execute();
			} catch (sql::SQLException &e) {
				std::cerr << "設定 schema 或隔離級別失敗: " << e.what() << std::endl;
				con.reset(); // 標記連線失敗
			}
		}*/
	} catch (sql::SQLException &e) {
		std::cerr << "Error connecting to DB：" << e.what() << std::endl;
	}
}

SQL_Driver::~SQL_Driver() = default;

//取使用者ID，加入 transaction、commit、rollback
int SQL_Driver::getUserId(const std::string& username) {
	auto con = getConnection();
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return -1;
	}

	int user_id = -1;

	try {
		con->setAutoCommit(false); // <<< 改動1
		// 設定隔離級別，保證查詢快照一致
		std::unique_ptr<sql::PreparedStatement> iso(
			con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
		);
		iso->execute(); // <<< 改動2

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("SELECT id FROM users WHERE username = ?")
		);
		pstmt->setString(1, username);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next())
			user_id = res->getInt("id");

		con->commit(); // <<< 改動3
		con->setAutoCommit(true);

	} catch (sql::SQLException& e) {
		std::cerr << "SQL error: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動4
		con->setAutoCommit(true);
	}

	return user_id;    
}

bool SQL_Driver::checkUserId(int users_id) {
    if(users_id == -1) {
		std::cerr << "User not found!" << std::endl;
        std::cerr << "Invalid user_id\n";
        return false;
    }
    return true;
}

//使用者註冊，加入 transaction、commit、rollback
bool SQL_Driver::registerUser(const std::string& username) {
	auto con = getConnection();
    if (!con) {
        std::cerr << "DB connection not established!\n";
        return false;
    }

	try {
		con->setAutoCommit(false);  // <<< 改動1：開始 transaction
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("INSERT INTO users (username, created_at) VALUES (?, CURDATE())")
		);
		pstmt->setString(1, username);
		int rows = pstmt->executeUpdate();
		if (rows > 0) {
			std::cout << "User '" << username << "' registered successfully!" << std::endl;
		} else {
			std::cout << "User '" << username << "' registered fail!" << std::endl;
			con->rollback();  // <<< 改動2：失敗 rollback
			con->setAutoCommit(true);
			return false;
		}
		con->commit();  // <<< 改動3：成功 commit
		con->setAutoCommit(true);
	} catch (sql::SQLException &e) {
		std::cerr << "Error registering user: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){}  // <<< 改動4：錯誤 rollback
		con->setAutoCommit(true);
		return false;
	}
	return true;
}

//刪除使用者，加入 transaction、commit、rollback
bool SQL_Driver::deleteUser(const std::string& username) {
	auto con = getConnection();
    if (!con) {
        std::cerr << "DB connection not established!\n";
        return false;
    }

	try {
		con->setAutoCommit(false);  // <<< 改動1
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("DELETE FROM users WHERE username = ?")
		);
		pstmt->setString(1, username);
		int rows = pstmt->executeUpdate();
		if (rows > 0) {
			std::cout << "User '" << username << "' deleted successfully!" << std::endl;
		} else {
			std::cout << "User '" << username << "' deleted fail!" << std::endl;
			con->rollback();  // <<< 改動2
			con->setAutoCommit(true);
			return false;
		}
		con->commit();  // <<< 改動3
		con->setAutoCommit(true);
	} catch (sql::SQLException &e) {
		std::cerr << "Error deleting user: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){}  // <<< 改動4
		con->setAutoCommit(true);
		return false;
	}
	return true;
}

//插入題目， 加入 transaction、commit、rollback
bool SQL_Driver::InsertProblem(int id, const std::string& title, const std::string& difficulty) {
	auto con = getConnection();
    if (!con) {
        std::cerr << "DB connection not established!\n";
        return false;
    }
	
	try {
		con->setAutoCommit(false);  // <<< 改動1
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("INSERT INTO problems (id, title, difficulty) VALUES (?, ?, ?)")
		);
		pstmt->setInt(1, id);
		pstmt->setString(2, title);
		pstmt->setString(3, difficulty);
		int affected = pstmt->executeUpdate();
		if (affected > 0) {
			std::cout << "Problem '" << title << "' inserted successfully!" << std::endl;
		} else {
			std::cout << "Problem =" << title << " insertion failed" << std::endl;
			con->rollback();  // <<< 改動2
			con->setAutoCommit(true);
			return false;
		}
		con->commit();  // <<< 改動3
		con->setAutoCommit(true);
	} catch (sql::SQLException &e) {
		std::cerr << "Error inserting problem: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){}  // <<< 改動4
		con->setAutoCommit(true);
		return false;
	}
	return true;
}

//刪除題目， 加入 transaction、commit、rollback
bool SQL_Driver::DeleteProblem(int problem_id) {
	auto con = getConnection();
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return false;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1

		// 先鎖定該 problem 行
		std::unique_ptr<sql::PreparedStatement> pstmt_lock(
			con->prepareStatement("SELECT id FROM problems WHERE id = ? FOR UPDATE") // <<< 改動2
		);
		pstmt_lock->setInt(1, problem_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_lock->executeQuery());
		if (!res->next()) {
			std::cout << "Problem id = " << problem_id << " not found." << std::endl;
			con->rollback(); // <<< 改動3
			con->setAutoCommit(true);
			return false;
		}

		// 刪除 problem
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("DELETE FROM problems WHERE id = ?")
		);
		pstmt->setInt(1, problem_id);
		int affected = pstmt->executeUpdate();

		if (affected > 0) {
			std::cout << "Problem id = " << problem_id << " deleted successfully!" << std::endl;
		} else {
			std::cout << "Problem id = " << problem_id << " deletion failed." << std::endl;
			con->rollback(); // <<< 改動4
			con->setAutoCommit(true);
			return false;
		}

		con->commit(); // <<< 改動5
		con->setAutoCommit(true);

	} catch (sql::SQLException &e) {
		std::cerr << "Error deleting problem: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動6
		con->setAutoCommit(true);
		return false;
	}

	return true;
}


//插入 problem_type_map， 加入 transaction、commit、rollback
bool SQL_Driver::InsertProblemType(int problem_id, const std::string& problem_type) {
	auto con = getConnection();
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return false;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1

		// 先鎖定該 problem_type_map 行（如果已存在會被鎖定，避免同時插入重複）
		std::unique_ptr<sql::PreparedStatement> pstmt_lock(
			con->prepareStatement(
				"SELECT problem_id FROM problem_type_map WHERE problem_id = ? AND problem_type = ? FOR UPDATE" // <<< 改動2
			)
		);
		pstmt_lock->setInt(1, problem_id);
		pstmt_lock->setString(2, problem_type);
		std::unique_ptr<sql::ResultSet> res(pstmt_lock->executeQuery());
		if(res->next()) {
			std::cout << "ProblemType = " << problem_type 
					  << " of problem number = " << problem_id 
					  << " already exists!" << std::endl;
			con->rollback(); // <<< 改動3
			con->setAutoCommit(true);
			return false;
		}

		// 插入 mapping
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("INSERT INTO problem_type_map (problem_id, problem_type) VALUES (?, ?)")
		);
		pstmt->setInt(1, problem_id);
		pstmt->setString(2, problem_type);
		int affected = pstmt->executeUpdate();

		if(affected > 0){
			std::cout << "ProblemType = " << problem_type 
					  << " of problem number = " << problem_id 
					  << " inserted successfully!" << std::endl;
		} else {
			std::cout << "ProblemType = " << problem_type 
					  << " of problem number = " << problem_id 
					  << " insertion failed!" << std::endl;
			con->rollback(); // <<< 改動4
			con->setAutoCommit(true);
			return false;
		}

		con->commit(); // <<< 改動5
		con->setAutoCommit(true);

	} catch(sql::SQLException &e) {
		std::cerr << "Error inserting problem_type_map: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動6
		con->setAutoCommit(true);
		return false;
	}

	return true;
}


//刪除 problem_type_map， 加入 transaction、commit、rollback
bool SQL_Driver::DeleteProblemType(int problem_id, const std::string& problem_type) {
	auto con = getConnection();
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return false;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1

		// 先鎖定該 problem_type_map 行
		std::unique_ptr<sql::PreparedStatement> pstmt_lock(
			con->prepareStatement(
				"SELECT problem_id FROM problem_type_map WHERE problem_id = ? AND problem_type = ? FOR UPDATE" // <<< 改動2
			)
		);
		pstmt_lock->setInt(1, problem_id);
		pstmt_lock->setString(2, problem_type);
		std::unique_ptr<sql::ResultSet> res(pstmt_lock->executeQuery());
		if(!res->next()) {
			std::cout << "Delete failed! Mapping not found." << std::endl;
			con->rollback(); // <<< 改動3
			con->setAutoCommit(true);
			return false;
		}

		// 刪除 mapping
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("DELETE FROM problem_type_map WHERE problem_id = ? AND problem_type = ?")
		);
		pstmt->setInt(1, problem_id);
		pstmt->setString(2, problem_type);
		int affected = pstmt->executeUpdate();

		if (affected > 0) {
			std::cout << "ProblemType '" << problem_type 
					  << "' for problem_id = " << problem_id 
					  << " deleted successfully!" << std::endl;
		} else {
			std::cout << "Delete failed! Mapping not found." << std::endl;
			con->rollback(); // <<< 改動4
			con->setAutoCommit(true);
			return false;
		}

		con->commit(); // <<< 改動5
		con->setAutoCommit(true);

	} catch(sql::SQLException &e) {
		std::cerr << "Error deleting problem_type_map: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動6
		con->setAutoCommit(true);
		return false;
	}

	return true;
}


int SQL_Driver::getMinuteFromTime(const std::string& t) {
    int hh, mm, ss;
    if(sscanf(t.c_str(), "%d:%d:%d", &hh, &mm, &ss) == 3) return hh * 60 + mm;;
    return -1;
}

//插入提交，加入 transaction、commit、rollback
bool SQL_Driver::InsertSubmitted(int users_id, int problem_id, bool status, const std::string& time_spent) {
	if(users_id == -1) return false;
	auto con = getConnection();
	if (!con) {
		std::cerr << "DB connection not established!\n";
		return false;
	}

	try {
		con->setAutoCommit(false);  // <<< 改動1

		// 查 username 並加行鎖
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("SELECT username FROM users WHERE id = ? FOR UPDATE")  // <<< 改動2
		);
		pstmt->setInt(1, users_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string username;
		if (res->next()) {
			username = res->getString("username");
		} else {
			std::cerr << "User not found!\n";
			con->rollback();  // <<< 改動3
			con->setAutoCommit(true);
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_insert(
			con->prepareStatement(
				"INSERT INTO submissions (problem_id, users_id, status, time_spent, time_spent_min, submit_date) "
				"VALUES (?, ?, ?, ?, ?, CURDATE())"
			)
		);
		pstmt_insert->setInt(1, problem_id);
		pstmt_insert->setInt(2, users_id);
		pstmt_insert->setBoolean(3, status);
		pstmt_insert->setString(4, time_spent);
		pstmt_insert->setInt(5, getMinuteFromTime(time_spent));

		pstmt_insert->executeUpdate();

		std::cout << "User '" << username << "' Insert Submitted successfully!" << std::endl;

		con->commit();  // <<< 改動4
		con->setAutoCommit(true);
	} catch (sql::SQLException &e) {
		std::cerr << "Error inserting submission: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){}  // <<< 改動5
		con->setAutoCommit(true);
		return false;
	}
	return true;
}

//刪除題交，加入 transaction、commit、rollback
bool SQL_Driver::DeleteSubmission(long long submission_id, int users_id) {
	if(users_id == -1) return false;
	auto con = getConnection();
	if(!con) {
		std::cerr << "DB connection not established!\n";
		return false;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1

		// 先鎖定該 submission 行
		std::unique_ptr<sql::PreparedStatement> pstmt_lock(
			con->prepareStatement(
				"SELECT id FROM submissions WHERE id = ? AND users_id = ? FOR UPDATE" // <<< 改動2
			)
		);
		pstmt_lock->setInt64(1, submission_id);
		pstmt_lock->setInt(2, users_id);
		std::unique_ptr<sql::ResultSet> res_lock(pstmt_lock->executeQuery());
		if(!res_lock->next()) {
			std::cout << "Delete failed! No matching submission found." << std::endl;
			con->rollback(); // <<< 改動3
			con->setAutoCommit(true);
			return false;
		}

		// 刪除 submission
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement("DELETE FROM submissions WHERE id = ? AND users_id = ?")
		);
		pstmt->setInt64(1, submission_id);
		pstmt->setInt(2, users_id);

		int affectedRows = pstmt->executeUpdate();

		if (affectedRows > 0) {
			// 查詢使用者名稱，方便輸出提示
			pstmt.reset(con->prepareStatement("SELECT username FROM users WHERE id = ?"));
			pstmt->setInt(1, users_id);

			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			if (res->next()) {
				std::cout << "User '" << res->getString("username")
						  << "' deleted submission (id=" << submission_id << ") successfully!" << std::endl;
			}
		} else {
			std::cout << "Delete failed! No matching submission found." << std::endl;
			con->rollback(); // <<< 改動4
			con->setAutoCommit(true);
			return false;
		}

		con->commit(); // <<< 改動5
		con->setAutoCommit(true);

	} catch (sql::SQLException &e) {
		std::cerr << "Error deleting submission: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動6
		con->setAutoCommit(true);
		return false;
	}

	return true;
}

// 查詢 submissions 並裝到 unordered_map (僅回傳資料)，加入 transaction、commit、rollback
std::unordered_map<long long, ExamInfo> SQL_Driver::getSubmissions(int users_id, int problem_id) {
	std::unordered_map<long long, ExamInfo> examsubmitdata;
	auto con = getConnection();
    if (!con) {
        std::cerr << "DB connection not established!\n";
        return examsubmitdata;
    }

    try {
        con->setAutoCommit(false); // <<< 開啟 transaction
        // 可選：設置隔離級別，保證讀到 transaction 開始時的快照
        std::unique_ptr<sql::PreparedStatement> iso(
            con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        );
        iso->execute();

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, users_id, problem_id, status, time_spent, time_spent_min, submit_date "
                "FROM submissions WHERE users_id = ? AND problem_id = ? ORDER BY id ASC"
            )
        );
        pstmt->setInt(1, users_id);
        pstmt->setInt(2, problem_id);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            ExamInfo info;
            long long submit_id = res->getInt64("id");
            info.user_id = res->getInt("users_id");
            info.problem_id = res->getInt("problem_id");
            info.state = res->getBoolean("status");
            info.s_solvedtime = res->getString("time_spent");
            info.i_solvedtime = res->getInt("time_spent_min");
            info.date = res->getString("submit_date");

            examsubmitdata[submit_id] = info;
        }

        con->commit();   // <<< 結束 transaction
        con->setAutoCommit(true);

    } catch (sql::SQLException &e) {
        std::cerr << "Error fetching submissions: " << e.what() << std::endl;
        try { con->rollback(); } catch(...) {}
        con->setAutoCommit(true);
    }

    return examsubmitdata;
}

// 顯示 unordered_map 資料
void SQL_Driver::displaySubmissions(const std::unordered_map<long long, ExamInfo>& user_exam_submits) const {
	std::cout << std::left
			  << std::setw(10) << "SubmitID"
			  << std::setw(10) << "UserID"
			  << std::setw(12) << "ProblemID"
			  << std::setw(10) << "Status"
			  << std::setw(12) << "Time(s)"
			  << std::setw(10) << "Time(min)"
			  << std::setw(12) << "Date"
			  << std::endl;

	std::cout << std::string(76, '-') << std::endl;

	for (const auto& [submitId, info] : user_exam_submits) {
		std::cout << std::left
				  << std::setw(10) << submitId
				  << std::setw(10) << info.user_id
				  << std::setw(12) << info.problem_id
				  << std::setw(10) << (info.state ? "Solved" : "Unsolved")
				  << std::setw(12) << info.s_solvedtime
				  << std::setw(10) << info.i_solvedtime
				  << std::setw(12) << info.date
				  << std::endl;
	}
}


//找到各單元的解題成功率，加入 transaction、commit、rollback
std::vector<Units> SQL_Driver::Select_UnitsData(int user_id) {
	if(user_id == -1) return {};
	std::vector<Units> stats;
	auto con = getConnection();
	if(!con) {
		std::cerr << "DB connection not established!\n";
		return stats;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1
		// 可選：保證讀取一致性快照
		std::unique_ptr<sql::PreparedStatement> iso(
			con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
		);
		iso->execute(); // <<< 改動2

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement(
				"SELECT pt.problem_type AS unit,"
				" COUNT(DISTINCT p.id) AS problems_solved,"
				" COUNT(p.id) AS total_submit,"
				" SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 20 THEN 1 ELSE 0 END) AS solved_20min,"
				" SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 30 THEN 1 ELSE 0 END) AS solved_30min,"
				" SUM(CASE WHEN s.status = 0 OR s.time_spent_min > 30 THEN 1 ELSE 0 END) AS solved_over_30min"
				" FROM problem_type_map pt"
				" JOIN problems p ON p.id = pt.problem_id"
				" JOIN submissions s ON s.problem_id = p.id"
				" WHERE s.users_id = ?"
				" GROUP BY pt.problem_type"
				" ORDER BY solved_over_30min DESC, solved_30min DESC, solved_20min DESC;"
			)
		);
		pstmt->setInt(1, user_id);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			Units u;
			u.unit = res->getString("unit");
			u.problems_solved = res->getInt("problems_solved");
			u.total_submit = res->getInt("total_submit");
			u.solved_20min = res->getInt("solved_20min");
			u.solved_30min = res->getInt("solved_30min");
			u.solved_over_30min = res->getInt("solved_over_30min");
			//在這裡算比較快
			u.rate_20min = u.total_submit ? (double)u.solved_20min / u.total_submit * 100 : 0;				
			u.rate_30min = u.total_submit ? (double)u.solved_30min / u.total_submit * 100 : 0;
			u.rate_over_30min = u.total_submit ? (double)u.solved_over_30min / u.total_submit * 100 : 0;
			stats.push_back(u);
		}

		con->commit(); // <<< 改動3
		con->setAutoCommit(true);

	} catch (sql::SQLException &e) {
		std::cerr << "Error selecting stats: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動4
		con->setAutoCommit(true);
	}

	return stats;
}

//查找單元細節成功率，加入 transaction、commit、rollback
std::vector<UnitStat> SQL_Driver::Select_Unitdetail(int user_id,const std::string& unit) {
	if(user_id == -1) return {};
	std::vector<UnitStat> stats;
	auto con = getConnection();
	if(!con) {
		std::cerr << "DB connection not established!\n";
		return stats;
	}

	try {
		con->setAutoCommit(false); // <<< 改動1
		// 可選：設定隔離級別為 REPEATABLE READ 保證快照一致
		std::unique_ptr<sql::PreparedStatement> iso(
			con->prepareStatement("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
		);
		iso->execute(); // <<< 改動2

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->prepareStatement(
				R"(
					SELECT
						p.id AS Exam_No,
						p.title AS Name,
						COUNT(s.id) AS total_submit,
						SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 20 THEN 1 ELSE 0 END) AS solved_20min,
						SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 30 THEN 1 ELSE 0 END) AS solved_30min,
						SUM(CASE WHEN s.status = 0 OR s.time_spent_min > 30 THEN 1 ELSE 0 END) AS solved_over_30min,
						e.submit_date AS last_solved_date,
						e.time_spent AS last_solved_time,
						e.status AS last_status
					FROM problems p
					JOIN problem_type_map pt ON p.id = pt.problem_id
					JOIN submissions s ON s.problem_id = p.id AND s.users_id = ?
					JOIN (
						SELECT s1.problem_id, s1.time_spent, s1.status, s1.submit_date
						FROM submissions s1
						JOIN (
							SELECT problem_id, MAX(id) AS max_id
							FROM submissions
							WHERE users_id = ?
							GROUP BY problem_id
						) s2 ON s1.id = s2.max_id
						WHERE s1.users_id = ?
					) e ON e.problem_id = p.id
					WHERE pt.problem_type = ?
					GROUP BY p.id, p.title, e.submit_date, e.time_spent, e.status
					ORDER BY last_solved_date DESC, solved_over_30min DESC, solved_30min DESC, solved_20min DESC;
				)"
			)
		);
		pstmt->setInt(1, user_id);
		pstmt->setInt(2, user_id);
		pstmt->setInt(3, user_id);
		pstmt->setString(4, unit);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			UnitStat u;
			u.exam_No = res->getInt("Exam_No");
			u.exam_name = res->getString("Name");
			u.TotalExam = res->getInt("total_submit");
			u.solved_20min = res->getInt("solved_20min");
			u.solved_30min = res->getInt("solved_30min");
			u.solved_over_30min = res->getInt("solved_over_30min");
			u.last_date = res->getString("last_solved_date");
			u.last_date_time = res->getString("last_solved_time"); 
			u.last_date_state = res->getBoolean("last_status") ? "Solved" : "Unsolved";   	
			stats.push_back(u);
		}

		con->commit(); // <<< 改動3
		con->setAutoCommit(true);

	} catch (sql::SQLException &e) {
		std::cerr << "Error selecting stats: " << e.what() << std::endl;
		try { con->rollback(); } catch(...){} // <<< 改動4
		con->setAutoCommit(true);
	}

	return stats;
}
}




