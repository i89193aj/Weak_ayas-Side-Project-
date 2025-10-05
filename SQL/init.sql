CREATE DATABASE IF NOT EXISTS leetcode_tracker;
USE leetcode_tracker;
CREATE TABLE IF NOT EXISTS users(
	id INT AUTO_INCREMENT PRIMARY KEY,
	username VARCHAR(50) NOT NULL UNIQUE,
	created_at DATE
);

CREATE TABLE IF NOT EXISTS problems(
	id INT PRIMARY KEY,
	title VARCHAR(100) NOT NULL UNIQUE,
	difficulty VARCHAR(20)
);

CREATE TABLE IF NOT EXISTS problem_type_map(
	problem_id INT,
	problem_type VARCHAR(50),
	PRIMARY  KEY (problem_id, problem_type),
	FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE
);


CREATE TABLE IF NOT EXISTS submissions (
	id INT AUTO_INCREMENT PRIMARY KEY,
	problem_id INT,		
	users_id INT,		
	status BOOLEAN,	
	time_spent TIME, 	
	time_spent_min INT, 
	submit_date DATE,	
	FOREIGN KEY (users_id) REFERENCES users(id) ON DELETE CASCADE,
	FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE
	/*
	id INT AUTO_INCREMENT PRIMARY KEY,	-- 每筆提交唯一編號(AUTO_INCREMENT自動遞增)
	problem_id INT,		-- 題目id
	users_id INT,		-- 誰提交答案
	status VARCHAR(20),	-- 是否有解決此題目
	time_spent TIME, 	-- 方便看每次提交耗時
	time_spent_min INT, -- 方便統計分析
	submit_date DATE,	-- 提交日期
	*/
);