#include "SQLManager.hpp"
#include <unistd.h>
#include <errno.h>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace protocol;
using namespace protocol::sql_protocol;

namespace sqlmanager {

// ===== 封包處理 =====
std::string SQLManager::build_package(uint8_t type, const std::string& payload) {
    size_t len = payload.size();
    if (len > protocol::MAX_BUFFERSIZE - 7) {
        return "";
    }

    std::string msg;
    msg.reserve(1 + 1 + 4 + len + 1);
    msg.push_back(protocol::PACKAGE_START_BYTE);
    msg.push_back(type);

    int32_t paylen = static_cast<int32_t>(len);
    msg.push_back((paylen >> 24) & 0xFF);
    msg.push_back((paylen >> 16) & 0xFF);
    msg.push_back((paylen >> 8) & 0xFF);
    msg.push_back(paylen & 0xFF);

    msg += payload;
    msg.push_back(protocol::PACKAGE_END_BYTE);

    return msg;
}


bool SQLManager::SendData(int fd, uint8_t type, const std::string& payload) {
    std::string msg = build_package(type, payload);
    if (msg.empty()) {
        std::cerr << "[Packet] Payload too large, fd = " << fd << std::endl;
        return false;
    }

    ssize_t total = 0;
    while (total < (ssize_t)msg.size()) {
        ssize_t n = write(fd, msg.data() + total, msg.size() - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("write failed");
            return false;
        }
        total += n;
    }
    #if defined(onetry)
    std::cerr << "[SendData] fd = " << fd
              << ", type = " << static_cast<int>(type)
              << ", len = " << msg.size() << std::endl;
    #endif
    return true;
}


// ===== Public =====
void SQLManager::handle_request(int fd, const protocol::SocketPackage& pkg) {
    switch (pkg.cType) {
        case REGISTER_USER: 
			handle_register(fd, pkg); 
			break;
        case LOGIN: 
			handle_login(fd, pkg); 
			break;
        case SELECT_UNITS_DATA: 
			handle_unitsdata(fd, pkg); 
			break;
        case SELECT_UNIT_DETAIL: 
			handle_unitdetail(fd, pkg); 
			break;
        case GET_PROBLEM_SUBMISSIONS: 
			handle_problem_submissions(fd, pkg); 
			break;
        case INSERT_SUBMITTED: 
			handle_insert_submission(fd, pkg); 
			break;
        case DELETE_SUBMITTED: 
			handle_delete_submission(fd, pkg); 
			break;
		// === For Oscar ===
		case DELETE_USER:
			handle_delete_user(fd, pkg);
			break;
		case INSERT_PROBLEM:
			handle_insert_problem(fd, pkg);
			break;
		case DELETE_PROBLEM:
			handle_delete_problem(fd, pkg);
			break;
		case INSERT_PROBLEM_TYPE:
			handle_insert_problem_type(fd, pkg);
			break;
		case DELETE_PROBLEM_TYPE:
			handle_delete_problem_type(fd, pkg);
			break;
        default:
            std::cerr << "[SQLManager] Unknown request type: "
                      << static_cast<int>(pkg.cType) << std::endl;
            break;
    }
}


void SQLManager::erase_fd(int fd) {
	if(!userFdToId.count(fd)) return;
	if(userFdToId[fd] == special_id) special_id = -1;
    userFdToId.erase(fd);
    std::cout << "[SQLManager] erased fd = " << fd << std::endl;
}


// ===== Helper =====
bool SQLManager::check_login(int fd) {
    if (userFdToId.count(fd)) return true;

    std::string msg = "Please login or register first!";
    if (!SendData(fd, LOGIN, msg)) {
        std::cerr << "[check_login] fd = " << fd << " not logged in!" << std::endl;
    }
    return false;
}


bool SQLManager::string_to_bool(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "true" || lower == "1";
}


std::vector<std::string> SQLManager::getdata(const protocol::SocketPackage& pkg) {
    std::vector<std::string> fields;
    std::string data(reinterpret_cast<const char*>(pkg.pDataBuf.data()), pkg.nLen);

    std::stringstream ss(data);
    std::string item;

    while (std::getline(ss, item, '#')) {
        if (!item.empty())
            fields.push_back(item);
    }
    return fields;
}


// ===== Handlers =====
void SQLManager::handle_register(int fd, const protocol::SocketPackage& pkg) {
    std::vector<std::string> data = getdata(pkg);
    if (data.empty()) {
        std::cerr << "[handle_register] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[Register] Please set your account!");
        return;
    }

    std::string username = data[0];
    bool success = db.registerUser(username);

    std::string msg = success ?
        "[Register] username = " + username + " , Successful!" :
        "[Register] username = " + username + " , Failed!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_register] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_login(int fd, const protocol::SocketPackage& pkg) {
    std::vector<std::string> data = getdata(pkg);
    if (data.empty()) {
        std::cerr << "[handle_login] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[Login] Please enter your account!");
        return;
    }

    std::string username = data[0];
    int user_id = db.getUserId(username);
	if(username == "Oscar") special_id = user_id;

    std::string msg;
    if (user_id != -1 && db.checkUserId(user_id)) {
        userFdToId[fd] = user_id;
        msg = "[Login] " + username + " login success (user_id = " + std::to_string(user_id) + ")";
    } else {
        msg = "[Login] Fail: " + username;
    }

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_login] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_unitsdata(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;

    int user_id = userFdToId[fd];
    auto units = db.Select_UnitsData(user_id);

    std::string msg = "[UnitsData] Send units stats for user_id = " + std::to_string(user_id) + "\n";
    msg += get_units_table(units);

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_unitsdata] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_unitdetail(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;

    int user_id = userFdToId[fd];
    std::vector<std::string> data = getdata(pkg);

    if (data.empty()) {
        std::cerr << "[handle_unitdetail] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[UnitDetail] Please set the problem type!");
        return;
    }

    std::string problem_type = data[0];
    auto stats = db.Select_Unitdetail(user_id, problem_type);

    std::string msg = "[UnitDetail] Unit = " + problem_type + " for user_id = " + std::to_string(user_id) + "\n";
    msg += get_unit_stats_table(stats);

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_unitdetail] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_problem_submissions(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;

    int user_id = userFdToId[fd];
    std::vector<std::string> data = getdata(pkg);

    if (data.empty()) {
        std::cerr << "[handle_problem_submissions] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[GetSubmissions] Please set the problem number!");
        return;
    }

    std::string problemIdStr = data[0];
    int problemId;

    try {
        problemId = std::stoi(problemIdStr);
    } catch (...) {
        SendData(fd, pkg.cType, "[GetSubmissions] Invalid problem ID format!");
        return;
    }

    auto subs = db.getSubmissions(user_id, problemId);

    std::string msg = "[GetSubmissions] problem = " + problemIdStr +
                      " for user_id = " + std::to_string(user_id) + "\n";
    msg += get_problem_submit_table(subs);

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_problem_submissions] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_insert_submission(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;

    int user_id = userFdToId[fd];
    std::vector<std::string> data = getdata(pkg);

    if (data.size() != 3) {
        std::cerr << "[handle_insert_submission] Error: incorrect submitted information." << std::endl;
        SendData(fd, pkg.cType, "[AddSubmissions] Please set the correct submitted information!");
        return;
    }

    std::string problemIdStr = data[0];
    std::string solve_statusStr = data[1];
    std::string solvetime = data[2];

    int problemId;
    try {
        problemId = std::stoi(problemIdStr);
    } catch (...) {
        SendData(fd, pkg.cType, "[AddSubmissions] Invalid problem ID format!");
        return;
    }

    bool bsolve_status = string_to_bool(solve_statusStr);

    std::string msg = "[AddSubmissions] problem = " + problemIdStr +
                      ", solve status = " + solve_statusStr +
                      ", solve time = " + solvetime +
                      " for user_id = " + std::to_string(user_id);

    if (db.InsertSubmitted(user_id, problemId, bsolve_status, solvetime)) {
        msg += " is successful!";
    } else {
        msg += " is fail!";
    }

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_insert_submission] Failed to send response to client." << std::endl;
    }
}


void SQLManager::handle_delete_submission(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;

    int user_id = userFdToId[fd];
    std::vector<std::string> data = getdata(pkg);

    if (data.empty()) {
        std::cerr << "[handle_delete_submission] Error: no submission ID provided." << std::endl;
        SendData(fd, pkg.cType, "[DeleteSubmissions] Please set the correct submission ID!");
        return;
    }

    std::string submissionIdStr = data[0];
    long long submissionId;

    try {
        submissionId = std::stoll(submissionIdStr);
    } catch (...) {
        SendData(fd, pkg.cType, "[DeleteSubmissions] Invalid submission ID format!");
        return;
    }

    std::string msg = "[DeleteSubmissions] submission ID = " + submissionIdStr +
                      " for user_id = " + std::to_string(user_id);

    if (db.DeleteSubmission(submissionId, user_id)) {
        msg += " is successful!";
    } else {
        msg += " is fail!";
    }

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_delete_submission] Failed to send response to client." << std::endl;
    }
}


// ===== Table Helpers =====
std::string SQLManager::get_units_table(const std::vector<sql_driver::Units>& units) const {
    std::ostringstream oss;
	
	//標題
    oss << std::left
        << std::setw(15) << "Unit"
        << std::setw(10) << "Solved"
        << std::setw(15) << "Total Submit"
        << std::setw(15) << "20min(%)"
        << std::setw(15) << "30min(%)"
        << std::setw(15) << ">30min(%)"
        << "\n";
	
	//分隔線
    oss << std::string(82, '-') << "\n";
	
	//資料
    for (const auto &u : units) {
        std::ostringstream r20, r30, r30plus;
        r20 << u.solved_20min << "(" << std::fixed << std::setprecision(2) << u.rate_20min << "%)";
        r30 << u.solved_30min << "(" << std::fixed << std::setprecision(2) << u.rate_30min << "%)";
        r30plus << u.solved_over_30min << "(" << std::fixed << std::setprecision(2) << u.rate_over_30min << "%)";

        oss << std::left
            << std::setw(15) << u.unit
            << std::setw(10) << u.problems_solved
            << std::setw(15) << u.total_submit
            << std::setw(15) << r20.str()
            << std::setw(15) << r30.str()
            << std::setw(15) << r30plus.str()
            << "\n";
    }

    return oss.str();
}


std::string SQLManager::get_unit_stats_table(const std::vector<sql_driver::UnitStat>& unitStats) const {
    std::ostringstream oss;
	
	//標題
    oss << std::left
        << std::setw(8) << "Exam_No"
        << std::setw(40) << "Name"
        << std::setw(8) << "Total"
        << std::setw(8) << "20min"
        << std::setw(8) << "30min"
        << std::setw(8) << ">30min"
        << std::setw(15) << "Last Date"
        << std::setw(12) << "Last Time"
        << std::setw(10) << "State"
        << "\n";
		
	//分隔線
    oss << std::string(113, '-') << "\n";

	//資料
    for (const auto& us : unitStats) {
        oss << std::setw(8) << us.exam_No
            << std::setw(40) << us.exam_name
            << std::setw(8) << us.TotalExam
            << std::setw(8) << us.solved_20min
            << std::setw(8) << us.solved_30min
            << std::setw(8) << us.solved_over_30min
            << std::setw(15) << us.last_date
            << std::setw(12) << us.last_date_time
            << std::setw(10) << us.last_date_state
            << "\n";
    }

    return oss.str();
}


std::string SQLManager::get_problem_submit_table(const std::unordered_map<long long, sql_driver::ExamInfo>& user_exam_submits) const {
    std::ostringstream oss;
	
	//標題
    oss << std::left
        << std::setw(10) << "SubmitID"
        << std::setw(10) << "UserID"
        << std::setw(12) << "ProblemID"
        << std::setw(10) << "Status"
        << std::setw(12) << "Time(s)"
        << std::setw(10) << "Time(min)"
        << std::setw(12) << "Date"
        << "\n";

	//分隔線
    oss << std::string(76, '-') << "\n";
	
	
	//資料
    for (const auto& [submitId, info] : user_exam_submits) {
        oss << std::left
            << std::setw(10) << submitId
            << std::setw(10) << info.user_id
            << std::setw(12) << info.problem_id
            << std::setw(10) << (info.state ? "Solved" : "Unsolved")
            << std::setw(12) << info.s_solvedtime
            << std::setw(10) << info.i_solvedtime
            << std::setw(12) << info.date
            << "\n";
    }

    return oss.str();
}

//For Oscar(me)
void SQLManager::handle_delete_user(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;
	int user_id = userFdToId[fd];
	if(user_id != special_id) return;
	
    std::vector<std::string> data = getdata(pkg);
	if (data.empty()) {
        std::cerr << "[handle_delete_user] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[DeleteUser] Please enter the delete user!");
        return;
    }
	
	std::string deleteuser = data[0];

    bool success = db.deleteUser(deleteuser);

    std::string msg = "[DeleteUser] user_name = " + deleteuser + " for user_id = " + std::to_string(user_id);
    msg += success ? " deleted successfully!" : " deletion failed!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_delete_user] Failed to send response to client." << std::endl;
    }
}

void SQLManager::handle_insert_problem(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;
	int user_id = userFdToId[fd];
	if(user_id != special_id) return;

    std::vector<std::string> data = getdata(pkg);
    if (data.size() < 3) {
		std::cerr << "[handle_insert_problem] Error: no correct param is provided." << std::endl;
        SendData(fd, pkg.cType, "[InsertProblem] Please provide problem number, problem title, problem's difficulty!");
        return;
    }
	
	std::string problemIdStr = data[0];
    int problemId;
	try{
		problemId = std::stoi(problemIdStr);
	} catch (...) {
        SendData(fd, pkg.cType, "[InsertProblem] Invalid problem ID format!");
        return;
    }
    std::string problem_title = data[1];
    std::string problem_difficulty = data[2];

    bool success = db.InsertProblem(problemId, problem_title, problem_difficulty);

    std::string msg = "[InsertProblem] problemId = " + problemIdStr + ", problem title = " + problem_title + " for user_id = " + std::to_string(user_id);
    msg += success ? " is successful!" : " is fail!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_insert_problem] Failed to send response to client." << std::endl;
    }
}

void SQLManager::handle_delete_problem(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;
	int user_id = userFdToId[fd];
	if(user_id != special_id) return;

    std::vector<std::string> data = getdata(pkg);
    if (data.empty()) {
		std::cerr << "[handle_delete_problem] Error: no data provided." << std::endl;
        SendData(fd, pkg.cType, "[DeleteProblem] Please provide problem number!");
        return;
    }
	std::string problemIdStr = data[0];
    int problemId;
	try{
		problemId = std::stoi(problemIdStr);
	} catch (...) {
        SendData(fd, pkg.cType, "[InsertProblem] Invalid problem ID format!");
        return;
    }

    bool success = db.DeleteProblem(problemId);

    std::string msg = "[DeleteProblem] problemId = " + problemIdStr + " for user_id = " + std::to_string(user_id);
    msg += success ? " deleted successfully!" : " deletion failed!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_delete_problem] Failed to send response to client." << std::endl;
    }
}

void SQLManager::handle_insert_problem_type(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;
	int user_id = userFdToId[fd];
	if(user_id != special_id) return;

    std::vector<std::string> data = getdata(pkg);
    if (data.size() < 2) {
		std::cerr << "[handle_insert_problem_type] Error: no correct param is provided." << std::endl;
        SendData(fd, pkg.cType, "[InsertProblemType] Please provide problem_id and problem type!");
        return;
    }
	
	std::string problemIdStr = data[0];
    int problemId;
	try{
		problemId = std::stoi(problemIdStr);
	} catch (...) {
        SendData(fd, pkg.cType, "[InsertProblemType] Invalid problem ID format!");
        return;
    }
    std::string problem_type = data[1];

    bool success = db.InsertProblemType(problemId, problem_type);

    std::string msg = "[InsertProblemType] problemId = " + problemIdStr + ", problem type = " + problem_type + " for user_id = " + std::to_string(user_id);
    msg += success ? " inserted successfully!" : " insertion failed!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_insert_problem_type] Failed to send response to client." << std::endl;
    }
}

void SQLManager::handle_delete_problem_type(int fd, const protocol::SocketPackage& pkg) {
    if (!check_login(fd)) return;
	int user_id = userFdToId[fd];
	if(user_id != special_id) return;
	
    std::vector<std::string> data = getdata(pkg);
    if (data.size() < 2) {
		std::cerr << "[handle_delete_problem_type] Error: no correct param is provided." << std::endl;
        SendData(fd, pkg.cType, "[DeleteProblemType] Please provide problemId and problem type!");
        return;
    }
	
	std::string problemIdStr = data[0];
	int problemId;
	try{
		problemId = std::stoi(problemIdStr);
	} catch (...) {
        SendData(fd, pkg.cType, "[DeleteProblemType] Invalid problem ID format!");
        return;
    }

    std::string problem_type = data[1];

    bool success = db.DeleteProblemType(problemId, problem_type);

    std::string msg = "[DeleteProblemType] problemId = " + problemIdStr + ", problem type = " + problem_type + " for user_id = " + std::to_string(user_id);
    msg += success ? " deleted successfully!" : " deletion failed!";

    if (!SendData(fd, pkg.cType, msg)) {
        std::cerr << "[handle_delete_problem_type] Failed to send response to client." << std::endl;
    }
}



} // namespace sqlmanager
