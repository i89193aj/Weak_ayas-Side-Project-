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
#include <sstream>   // ostringstream

#include "sql_driver.hpp"  // 你的 SQL_Driver 類別

using namespace std;
using namespace sql_driver;

int main() {
    SQL_Driver db;

    // ======== 1. User Operator ========
    std::string username;
    cout << "\n=== Register User ===" << endl;
    cout << "Enter username to register: ";
    getline(cin, username);
    db.registerUser(username);

    cout << "\n=== Delete User ===" << endl;
    cout << "Enter username to delete: ";
    getline(cin, username);
    db.deleteUser(username);
    cout << endl;
    // ======== End User Operator ========

    // ======== 2. Get UserID ========
    cout << "\n=== Login In ===" << endl;
    cout << "Enter username to get ID: ";
    getline(cin, username);

    int user_id = db.getUserId(username);
    if (!db.checkUserId(user_id)) return 1;
    if (user_id == -1) {
        cerr << "User not found!" << endl;
        return 1;
    }
    // ======== End Get UserID ========

    // ======== 3. Problem Operator ========
    cout << "\n=== Insert Problem ===" << endl;
    int problem_id;
    string title, difficulty;
    cout << "Problem ID: ";
    cin >> problem_id;
    cin.ignore();

    cout << "Title: ";
    getline(cin, title);

    cout << "Difficulty: ";
    getline(cin, difficulty);

    db.InsertProblem(problem_id, title, difficulty);

    cout << "\n=== Delete Problem ===" << endl;
    int delete_problemid;
    cout << "Delete Problem ID: ";
    cin >> delete_problemid;
    cin.ignore();
    db.DeleteProblem(delete_problemid);
    cout << endl;
    // ======== End Problem Operator ========

    // ======== 4. Problem Unit ========
    string unit;
    cout << "\n=== Insert This Problem Unit ===" << endl;
    cout << "Unit name: ";
    getline(cin, unit);
    db.InsertProblemType(problem_id, unit);

    cout << "\n=== Delete Problem Unit ===" << endl;
    int delete_unit_problemid;
    cout << "Delete Unit Problem ID: ";
    cin >> delete_unit_problemid;
    cin.ignore();

    string delete_type;
    cout << "Delete Unit: ";
    getline(cin, delete_type);
    db.DeleteProblemType(delete_unit_problemid, delete_type);
    cout << endl;
    // ======== End Problem Unit ========

    // ======== 5. Submission Operator ========
    cout << "\n=== Insert This Problem submission ===" << endl;
    bool status;
    string time_spent;
    cout << "Status (1=solved,0=unsolved): ";
    cin >> status;
    cin.ignore();

    cout << "Time spent (HH:MM:SS): ";
    getline(cin, time_spent);
    db.InsertSubmitted(user_id, problem_id, status, time_spent);

    cout << "\n=== Delete The Problem submission ===" << endl;
    int delete_submit_problem_id;
    cout << "Delete Problem ID For Submits: ";
    cin >> delete_submit_problem_id;
    cin.ignore();

    auto exams = db.getSubmissions(user_id, delete_submit_problem_id);
    db.displaySubmissions(exams);

    long long delete_submit_id;
    cout << "Delete Submits ID: ";
    cin >> delete_submit_id;
    cin.ignore();
    db.DeleteSubmission(delete_submit_id, user_id);
    // ======== End Submission Operator ========

    // ======== 6. Query Units Data ========
    cout << "\n=== Units Data ===" << endl;
    auto units = db.Select_UnitsData(user_id);

    cout << left
         << setw(15) << "Unit"
         << setw(10) << "Solved"
         << setw(15) << "Total Submit"
         << setw(15) << "20min(%)"
         << setw(15) << "30min(%)"
         << setw(15) << ">30min(%)"
         << endl;

    cout << string(82, '-') << endl;

    for (const auto &u : units) {
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

    // ======== 7. Query Unit Detail ========
    cout << "\n=== Unit Detail ===" << endl;
    cout << "Select Unit: ";
    string selectunit;
    getline(cin, selectunit);

    auto unitStats = db.Select_Unitdetail(user_id, selectunit);

    cout << left
         << setw(8)  << "Exam_No"
         << setw(40) << "Name"
         << setw(8)  << "Total"
         << setw(8)  << "20min"
         << setw(8)  << "30min"
         << setw(8)  << ">30min"
         << setw(15) << "Last Date"
         << setw(12) << "Last Time"
         << setw(10) << "State"
         << endl;

    cout << string(113, '-') << endl;

    for (const auto& us : unitStats) {
        cout << setw(8)  << us.exam_No
             << setw(40) << us.exam_name
             << setw(8)  << us.TotalExam
             << setw(8)  << us.solved_20min
             << setw(8)  << us.solved_30min
             << setw(8)  << us.solved_over_30min
             << setw(15) << us.last_date
             << setw(12) << us.last_date_time
             << setw(10) << us.last_date_state
             << endl;
    }
    // ======== End Query Unit Detail ========

    return 0;
}
