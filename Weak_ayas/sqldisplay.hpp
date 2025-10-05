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

namespace sql_display {
class SQL_Display {    
private:
    sql::Driver* driver; 
    std::shared_ptr<sql::Connection> con; 
    bool isConnected();

public:
    SQL_Display();
	~SQL_Display();
	
	void displayUsers();
	void displayProblems();
	void displayProblemTypeMap();
	void displaySubmissions(int userid = 1);
};
}




