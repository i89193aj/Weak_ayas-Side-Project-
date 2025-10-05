#pragma once
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
namespace sql_driver{
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
	int user_id;				//使用者ID
	int problem_id;				//題目ID
	bool state;					//是否解提成功
	std::string s_solvedtime;   //解題時間(展示用)
	int i_solvedtime;           //解題時間(計算用)
	std::string date;           //提交日期
};      

class SQL_Driver {    
    sql::Driver* driver; 
    std::string url;
    std::string user;
    std::string password;
    //std::shared_ptr<sql::Connection> con; 
    bool isConnected();
	std::unique_ptr<sql::Connection> getConnection();

public:
    SQL_Driver();
	~SQL_Driver();
	
	int getUserId(const std::string& username); //得用戶ID
	bool checkUserId(int users_id); //檢查用戶 ID
    bool registerUser(const std::string& username); //新增用戶
    bool deleteUser(const std::string& username);   //刪除用戶
	bool InsertProblem(int id, const std::string& title, const std::string& difficulty);//插入題目
	bool DeleteProblem(int problem_id);//刪除題目
	bool InsertProblemType(int problem_id, const std::string& problem_type);//插入題目類型
	bool DeleteProblemType(int problem_id, const std::string& problem_type);//刪除題目類型
	bool InsertSubmitted(int users_id, int problem_id, bool status, const std::string& time_spent);//新增提交解題結果
	bool DeleteSubmission(long long submission_id, int users_id);//刪除解題結果
    std::unordered_map<long long, ExamInfo> getSubmissions(int users_id, int problem_id);// 查詢 submissions 並裝到 unordered_map (僅回傳資料)
    void displaySubmissions(const std::unordered_map<long long, ExamInfo>& user_exam_submits) const;// 顯示 unordered_map 資料
	std::vector<Units> Select_UnitsData(int user_id);//查找所有單元的解題成功率
	std::vector<UnitStat> Select_Unitdetail(int user_id,const std::string& unit);//查找各別單元的解題情況
	
	int getMinuteFromTime(const std::string& t); //轉換時間To數字
};
}


