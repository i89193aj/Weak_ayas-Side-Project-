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
using namespace sql_display;
//測試
int main() {
	try {
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
	} catch (sql::SQLException& e) {
		std::cerr << "SQL Error: " << e.what() << std::endl;
	}
	return 0;
}




