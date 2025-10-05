-- 選擇使用的資料庫
CREATE USER IF NOT EXISTS 'oscar'@'localhost' IDENTIFIED BY 'oscar841004';
GRANT ALL PRIVILEGES ON leetcode_tracker.* TO 'oscar'@'localhost';
FLUSH PRIVILEGES;

