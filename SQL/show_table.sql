-- 選擇使用的資料庫
USE leetcode_tracker;

-- 列出資料庫內的資料表(TABLE)
SHOW TABLES;

-- 查看每個表的資料
-- 查看 users
SELECT * FROM users ORDER BY id ASC;

-- 查看 problems
SELECT * FROM problems ORDER BY id ASC;

-- 查看 problem_type_map
SELECT * FROM problem_type_map ORDER BY problem_type ASC, problem_id ASC;

-- 查看 submissions
SELECT * FROM submissions ORDER BY users_id ASC, problem_id ASC, id DESC;

-- 檢查資料筆數
SELECT COUNT(*) FROM users;
SELECT COUNT(*) FROM problems;
SELECT COUNT(*) FROM problem_type_map;
SELECT COUNT(*) FROM submissions;
