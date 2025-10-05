-- 刪除leetcode_tracker資料庫
USE leetcode_tracker;

-- 將類型從 INT 改成 BIGINT
ALTER TABLE submissions 
MODIFY COLUMN id BIGINT UNSIGNED AUTO_INCREMENT;

-- 查看table (\G → 結果會用「縱向格式」輸出，每一筆資料直著列出來，比較好看。)
SHOW CREATE TABLE submissions\G


