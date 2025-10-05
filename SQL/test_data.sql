-- 選擇使用的資料庫
USE leetcode_tracker;

SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE users;
TRUNCATE TABLE problems;
TRUNCATE TABLE problem_type_map;
TRUNCATE TABLE submissions;
SET FOREIGN_KEY_CHECKS = 1;
-- Users
INSERT INTO users (username, created_at) VALUES
('Oscar', CURDATE()),
('Alice', '2025-09-10'),
('Bob', '2025-09-15');

-- Problems
INSERT INTO problems (id, title, difficulty) VALUES
(1, 'Two Sum', 'Easy'),
(2, 'LRU Cache', 'Medium'),
(3, 'Merge Intervals', 'Medium'),
(4, 'Binary Tree Inorder Traversal', 'Easy');


-- Problem Type Map
INSERT INTO problem_type_map (problem_id, problem_type) VALUES
(1, 'Array'),
(2, 'Hash Table'),
(2, 'Design'),
(2,'Array'),
(3, 'Interval'),
(3,'Tree'),
(4, 'Tree'),
(4, 'DFS'),
(4,'Graph');

-- Submissions
INSERT INTO submissions (problem_id, users_id, status, time_spent, time_spent_min, submit_date) VALUES
(1, 1, TRUE, '00:15:30', 15, '2025-09-18'),
(2, 1, TRUE, '00:45:00', 45, '2025-09-19'),
(2, 2, FALSE, '00:30:00', 30, '2025-09-20'),
(3, 3, TRUE, '01:10:00', 70, '2025-09-20'),
(4, 1, FALSE, '00:20:00', 20, CURDATE()),
(1,1,0,'00:05:00',5,'2025-09-23'),   -- 失敗
(1,1,1,'00:12:00',12,'2025-09-23'),  -- 成功
(1,1,1,'00:18:00',18,'2025-09-24'),  -- 成功
-- Problem 2 (Array)
(2,1,1,'00:22:00',22,'2025-09-23'),  -- 成功
(2,1,0,'00:30:00',30,'2025-09-23'),  -- 失敗
(2,1,1,'00:28:00',28,'2025-09-24'),  -- 成功
-- Problem 3 (Tree)
(3,1,0,'00:50:00',50,'2025-09-23'),  -- 失敗
(3,1,1,'01:05:00',65,'2025-09-24'),  -- 成功
-- Problem 4 (Graph)
(4,1,1,'00:15:00',15,'2025-09-23'),  -- 成功
(4,1,1,'00:45:00',45,'2025-09-24'),  -- 成功
(4,1,0,'00:20:00',20,'2025-09-25');  -- 失敗


-- 列出資料庫內的資料表
-- 查看每個表的資料
-- 查看 users
SELECT * FROM users;

-- 查看 problems
SELECT * FROM problems;

-- 查看 problem_type_map
SELECT * FROM problem_type_map;

-- 查看 submissions
SELECT * FROM submissions;
