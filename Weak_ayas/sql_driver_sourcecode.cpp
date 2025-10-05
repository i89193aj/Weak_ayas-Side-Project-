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
//#include "SQL_Diver.h"  // 假設 SQL_Diver 類別放在這裡

using namespace std;
struct Units {
    std::string unit; 		// 單元名稱
    int problems_solved;	//單元題數(不重複)
    int total_submit;		//提交次數(可重複題目)
    int solved_20min;		//20分鐘內解完
    int solved_30min;		//30分鐘內解完
    int solved_over_30min;	//超過30分鐘 or 解不出來
    double rate_20min;		//20分鐘內解完成功率
    double rate_30min;      //30分鐘內解完成功率
    double rate_over_30min; //超過30分鐘 or 解不出來比例
};

struct UnitStat{
	int exam_No;					//題號
	std::string exam_name;			//題目名稱
	int TotalExam;					//題目提交次數
	int solved_20min;				//20分鐘內解完
	int solved_30min;				//30分鐘內解完
	int solved_over_30min;			//超過30分鐘 or 解不出來
	std::string last_date;			//最後一次解這題的時間
	std::string last_date_time;		//最後一次解這題的用時
	std::string last_date_state;	//最後一次是否解得出來
};

struct ExamInfo{
	int user_id;
	int problem_id;
	bool state;
	std::string s_solvedtime;
	int i_solvedtime;
	std::string date;
};
	

class SQL_Diver {    
    sql::Driver* driver; 
    std::shared_ptr<sql::Connection> con; 

    bool isConnected() {
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

public:
    SQL_Diver() {
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
	
	int getUserId(const std::string& username) {
		if (!con) {
			std::cerr << "DB connection not established!\n";
			return -1;
		}
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("SELECT id FROM users WHERE username = ?")
			);
			pstmt->setString(1, username);
			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			if (res->next())
				return res->getInt("id");
		} catch (sql::SQLException& e) {
			std::cerr << "SQL error: " << e.what() << std::endl;
		}
		return -1;    
	}


    void registerUser() {
		if (!con) {
			std::cerr << "DB connection not established!\n";
			return;
		}
        std::string username;
        std::cout << "Enter username to register: ";
        std::getline(std::cin,username);
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("INSERT INTO users (username, created_at) VALUES (?, CURDATE())")
            );
            pstmt->setString(1, username);
            pstmt->executeUpdate();
            std::cout << "User '" << username << "' registered successfully!" << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "Error registering user: " << e.what() << std::endl;
        }
    }

    void deleteUser() {
		if (!con) {
			std::cerr << "DB connection not established!\n";
			return;
		}
        std::string username;
        std::cout << "Enter username to delete: ";
        std::getline(std::cin,username);
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("DELETE FROM users WHERE username = ?")
            );
            pstmt->setString(1, username);
            int rows = pstmt->executeUpdate();
            if (rows > 0)
                std::cout << "User '" << username << "' deleted successfully!" << std::endl;
            else
                std::cout << "User '" << username << "' not found." << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "Error deleting user: " << e.what() << std::endl;
        }
    }
	
	//插入題目
	void InsertProblem(int id, const std::string& title, const std::string& difficulty) {
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("INSERT INTO problems (id, title, difficulty) VALUES (?, ?, ?)")
			);
			pstmt->setInt(1, id);
			pstmt->setString(2, title);
			pstmt->setString(3, difficulty);
			pstmt->executeUpdate();

			std::cout << "Problem '" << title << "' inserted successfully!" << std::endl;
		} catch (sql::SQLException &e) {
			std::cerr << "Error inserting problem: " << e.what() << std::endl;
		}
	}
	
	//刪除題目
	void DeleteProblem(int problem_id) {
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("DELETE FROM problems WHERE id = ?")
			);
			pstmt->setInt(1, problem_id);
			int affected = pstmt->executeUpdate();

			if (affected > 0)
				std::cout << "Problem id=" << problem_id << " deleted successfully!" << std::endl;
			else
				std::cout << "Problem id=" << problem_id << " not found." << std::endl;
		} catch (sql::SQLException &e) {
			std::cerr << "Error deleting problem: " << e.what() << std::endl;
		}
	}
	
	//插入 problem_type_map
	void InsertProblemType(int problem_id, const std::string& problem_type) {
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("INSERT INTO problem_type_map (problem_id, problem_type) VALUES (?, ?)")
			);
			pstmt->setInt(1, problem_id);
			pstmt->setString(2, problem_type);
			pstmt->executeUpdate();

			std::cout << "ProblemType '" << problem_type << "' mapped to problem_id=" << problem_id << " successfully!" << std::endl;
		} catch (sql::SQLException &e) {
			std::cerr << "Error inserting problem_type_map: " << e.what() << std::endl;
		}
	}
	
	//刪除 problem_type_map
	void DeleteProblemType(int problem_id, const std::string& problem_type) {
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("DELETE FROM problem_type_map WHERE problem_id = ? AND problem_type = ?")
			);
			pstmt->setInt(1, problem_id);
			pstmt->setString(2, problem_type);
			int affected = pstmt->executeUpdate();

			if (affected > 0)
				std::cout << "ProblemType '" << problem_type << "' for problem_id=" << problem_id << " deleted successfully!" << std::endl;
			else
				std::cout << "Mapping not found." << std::endl;
		} catch (sql::SQLException &e) {
			std::cerr << "Error deleting problem_type_map: " << e.what() << std::endl;
		}
	}


	//插入提交
	void InsertSubmitted(int users_id, int problem_id, bool status, const std::string& time_spent) {
		if(users_id == -1) return;
		try {
			// 插入 submission
			{
				std::unique_ptr<sql::PreparedStatement> pstmt(
					con->prepareStatement(
						"INSERT INTO submissions (problem_id, users_id, status, time_spent, time_spent_min, submit_date)"
						"VALUES (?, ?, ?, ?, ?, CURDATE())"
				));
				pstmt->setInt(1, problem_id);
				pstmt->setInt(2, users_id);
				pstmt->setBoolean(3, status);
				pstmt->setString(4, time_spent);

				std::string time_spent_min = time_spent.substr(3, 2);//HH:MM:SS
				pstmt->setInt(5, std::stoi(time_spent_min));

				pstmt->executeUpdate();
			}

			// 查詢使用者名稱
			{
				std::unique_ptr<sql::PreparedStatement> pstmt(
					con->prepareStatement("SELECT username FROM users WHERE id = ?")
				);
				pstmt->setInt(1, users_id);
				std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
				if (res->next()) {
					std::cout << "User '" << res->getString("username") 
							  << "' Insert Submitted successfully!" << std::endl;
				} else {
					std::cout << "False!" << std::endl;
				}
			}
		} catch (sql::SQLException &e) {
			std::cerr << "Error inserting submission: " << e.what() << std::endl;
		}
	}
	
	//刪除題交
	void DeleteSubmission(int submission_id, int users_id) {
		if(users_id == -1) return;
		try {
			// 刪除指定 submission_id 的紀錄
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement("DELETE FROM submissions WHERE id = ? AND users_id = ?")
			);
			pstmt->setInt(1, submission_id);
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
			}
		} catch (sql::SQLException &e) {
			std::cerr << "Error deleting submission: " << e.what() << std::endl;
		}
	}
	
	// 查詢 submissions 並裝到 unordered_map (僅回傳資料)
    std::unordered_map<int, ExamInfo> getSubmissions(int users_id, int problem_id) {
        std::unordered_map<int, ExamInfo> examsubmitdata;

        if (!con) {
            std::cerr << "DB connection not established!\n";
            return examsubmitdata;
        }

        try {
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
                int submit_id = res->getInt("id");
                info.user_id = res->getInt("users_id");
                info.problem_id = res->getInt("problem_id");
                info.state = res->getBoolean("status");
                info.s_solvedtime = res->getString("time_spent");
                info.i_solvedtime = res->getInt("time_spent_min");
                info.date = res->getString("submit_date");

                examsubmitdata[submit_id] = info;
            }

        } catch (sql::SQLException &e) {
            std::cerr << "Error fetching submissions: " << e.what() << std::endl;
        }

        return examsubmitdata;
    }
	
	// 顯示 unordered_map 資料
    void displaySubmissions(const std::unordered_map<int, ExamInfo>& user_exam_submits) const {
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


	
	//找到各單元的解題成功率
	std::vector<Units> Select_UnitsData(int user_id) {
		if(user_id == -1) return {};
		std::vector<Units> stats;

		try {
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
		} catch (sql::SQLException &e) {
			std::cerr << "Error selecting stats: " << e.what() << std::endl;
		}

		return stats;
	}
	
	std::vector<UnitStat> Select_Unitdetail(int user_id,const string& unit) {
		if(user_id == -1) return {};
		std::vector<UnitStat> stats;

		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(
				con->prepareStatement(
					R"(
						SELECT
							p.id AS Exam_No,
							p.title AS Name,
							COUNT(s.id) AS total_submit,  -- 每題總提交次數
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
							-- 取每個題目最新的一筆提交
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
		} catch (sql::SQLException &e) {
			std::cerr << "Error selecting stats: " << e.what() << std::endl;
		}

		return stats;
	}

};

//測試
int main() {
    SQL_Diver db;
	
	// ======== 1. User Operator ========
    // 註冊使用者
    std::cout << "\n=== Register User ===" << std::endl;
    db.registerUser();  // 輸入 username
	
	// 刪除使用者
	std::cout << "\n=== Delete User ===" << std::endl;
    db.deleteUser();   // 手動輸入 username
    std::cout << std::endl;
	// ======== End User Operator  ========
	
	
	// ======== 2. Get UserID ========
	std::cout << "\n=== Login In ===" << std::endl;
	std::string username;
	
    std::cout << "Enter username to get ID: ";
    std::getline(std::cin, username);
	
    int user_id = db.getUserId(username);
    if (user_id == -1) {
        std::cerr << "User not found!" << std::endl;
        return 1;
    }
	// ======== End Get UserID ========
	
	
	// ======== 3. Problem Operator ========
    // 插入題目
	std::cout << "\n=== Insert Problem ===" << std::endl;
    int problem_id;
    std::string title, difficulty, unit;
    //std::cin.ignore();
	
    std::cout << "Problem ID: ";
    std::cin >> problem_id;
    std::cin.ignore();
	
    std::cout << "Title: ";
    std::getline(std::cin, title);
	
    std::cout << "Difficulty: ";
    std::getline(std::cin, difficulty);
	
    db.InsertProblem(problem_id, title, difficulty);
	
	// 刪除題目
	std::cout << "\n=== Delete Problem ===" << std::endl;
	int delete_problemid;
	
	std::cout << "Delete Problem ID: ";
	std::cin >> delete_problemid;
	std::cin.ignore();
	
    db.DeleteProblem(delete_problemid);
    std::cout << std::endl;
	// ======== End Problem Operator ========
	
	
	// ======== 4. Problem Unit ========
    // 插入映射單元
	std::cout << "\n=== This Problem Unit ===" << std::endl;
    std::cout << "Unit name: ";
    std::getline(std::cin, unit);
	
    db.InsertProblemType(problem_id, unit);
	
	// 刪除映射單元
	std::cout << "\n=== Delete Problem Unit ===" << std::endl;
	int delete_unit_problemid;
	
	std::cout << "Delete Unit Problem ID: ";
	std::cin >> delete_unit_problemid;
	std::cin.ignore();
	
	std::cout << "Delete Unit: ";
	string delele_type;
	std::getline(std::cin, delele_type);
	
    db.DeleteProblemType(delete_unit_problemid, delele_type);
    std::cout << std::endl;
	// ======== End Problem Unit ========
	
	
	// ======== 5. Submission Operator ========
    // 新增提交紀錄(submission)
	std::cout << "\n=== Insert This Problem submission ===" << std::endl;
    bool status;
    std::string time_spent;
	
    std::cout << "Status (1=solved,0=unsolved): ";
    std::cin >> status;
    std::cin.ignore();
	
    std::cout << "Time spent (HH:MM:SS): ";
    std::getline(std::cin, time_spent);
	
    db.InsertSubmitted(user_id, problem_id, status, time_spent);

	// 刪除提交紀錄(submission)
	std::cout << "\n=== Delete The Problem submission ===" << std::endl;
	
    std::cout << "Delete Problem ID For Submits: ";
	int delete_submit_problem_id;
	std::cin >> delete_submit_problem_id;
	std::cin.ignore();
	
	auto exams = db.getSubmissions(user_id, delete_submit_problem_id);
    db.displaySubmissions(exams);
	
    std::cout << "Delete Submits ID: ";
	int delete_submit_id;
	std::cin >> delete_submit_id;
	std::cin.ignore();
	
    db.DeleteSubmission(delete_submit_id, user_id);
	// ======== End Submission Operator ========
	
	
	// ======== 6. Query Units Data ========
    // 查詢各單元統計
    std::cout << "\n=== Units Data ===" << std::endl;
    auto units = db.Select_UnitsData(user_id);
    // 標題
    cout << left 
         << setw(15) << "Unit"
         << setw(10) << "Solved"
         << setw(15) << "Total Submit"
         << setw(15) << "20min(%)"
         << setw(15) << "30min(%)"
         << setw(15) << ">30min(%)"
         << endl;

    // 分隔線
    cout << string(145, '-') << endl;

    // 資料列
    for (const auto &u : units) {
		// 格式化百分比
        ostringstream r20, r30, r30plus;
        r20 << u.solved_20min << "(" << fixed << setprecision(2) << u.rate_20min << "%)";
        r30 << u.solved_30min << "(" << fixed << setprecision(2) << u.rate_30min << "%)";
        r30plus << u.solved_over_30min << "(" << fixed << setprecision(2) << u.rate_over_30min << "%)";
		
        cout << left 
             << setw(15) << u.unit
             << setw(10) << u.problems_solved
             << setw(15) << u.total_submit
             << setw(15) << r20.str()
             << setw(15) << r30.str()
             << setw(15) << r30plus.str()
             << endl;
    }
	// ======== End Query Units Data ========
	
	
	// ======== Query Unit Detail ========
    // 查詢單元詳細 (UnitStat)
    std::cout << "\n=== Unit Detail ===" << std::endl;
	std::cout << "Select Unit: ";
	string selectunit;
	std::getline(std::cin, selectunit);
    auto unitStats = db.Select_Unitdetail(user_id, selectunit);
    // 假設 unitStats 是 vector<UnitStat>
	std::cout << std::left; // 左對齊
	std::cout << std::setw(8)  << "Exam_No"
			  << std::setw(25) << "Name"
			  << std::setw(8)  << "Total"
			  << std::setw(8)  << "20min"
			  << std::setw(8)  << "30min"
			  << std::setw(8)  << ">30min"
			  << std::setw(15) << "Last Date"
			  << std::setw(12) << "Last Time"
			  << std::setw(10) << "State"
			  << std::endl;

	// 分隔線
	std::cout << std::string(94, '-') << std::endl;

	for (const auto& us : unitStats) {
		std::cout << std::setw(8)  << us.exam_No
				  << std::setw(25) << us.exam_name
				  << std::setw(8) << us.TotalExam
				  << std::setw(8)  << us.solved_20min
				  << std::setw(8)  << us.solved_30min
				  << std::setw(8)  << us.solved_over_30min
				  << std::setw(15) << us.last_date
				  << std::setw(12) << us.last_date_time
				  << std::setw(10) << us.last_date_state
				  << std::endl;
	}
	// ======== End Query Unit Detail ========
	
    return 0;
}




