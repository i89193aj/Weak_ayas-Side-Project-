-- 選擇使用的資料庫
USE leetcode_tracker;

-- 找到對應區間
/*CREATE TEMPORARY TABLE tempProblemInfo AS
WITH problemInfo AS (
    SELECT p.id AS ExamNumber, p.title AS Title,
           GROUP_CONCAT(mp.problem_type ORDER BY mp.problem_type SEPARATOR ', ') AS types
    FROM problems p
    LEFT JOIN problem_type_map mp ON mp.problem_id = p.id
    GROUP BY p.id, p.title
)
SELECT * FROM problemInfo;*/

-- 查找所有單元的成功率
SELECT
    pt.problem_type AS unit,
    COUNT(DISTINCT p.id) AS 'problems solved',
	COUNT(p.id) AS 'totalsubmit',
	SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 20 THEN 1 ELSE 0 END) AS solved_20min,
	SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 30 THEN 1 ELSE 0 END) AS solved_30min,
    SUM(CASE WHEN s.status = 0 OR s.time_spent_min > 30 THEN 1 ELSE 0 END) AS solved_over_30min,
	CONCAT(ROUND(SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 20 THEN 1 ELSE 0 END) / COUNT(*) * 100, 2), '%') AS rate_20min,
	CONCAT(ROUND(SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 30 THEN 1 ELSE 0 END) / COUNT(*) * 100, 2), '%') AS rate_30min,
    CONCAT(ROUND(SUM(CASE WHEN s.status = 0 OR s.time_spent_min > 30 THEN 1 ELSE 0 END) / COUNT(*) * 100, 2), '%') AS rate_over_30min
FROM problem_type_map pt
JOIN problems p ON p.id = pt.problem_id
JOIN submissions s ON s.problem_id = p.id
WHERE s.users_id = 1
GROUP BY pt.problem_type
ORDER BY solved_over_30min DESC, solved_30min DESC, solved_20min DESC;


-- 查找各別單元的解題情況
SELECT
    p.id AS Exam_No,
    p.title AS Name,
    COUNT(s.id) AS total_submit,  -- 每題總提交次數
    SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 20 THEN 1 ELSE 0 END) AS solved_20min,
    SUM(CASE WHEN s.status = 1 AND s.time_spent_min <= 30 THEN 1 ELSE 0 END) AS solved_30min,
    SUM(CASE WHEN s.status = 0 OR s.time_spent_min > 30 THEN 1 ELSE 0 END) AS solved_over_30min,
    e.submit_date AS last_solved_date,
    e.time_spent AS last_solved_time,
    e.status AS last_status
FROM problems p
JOIN problem_type_map pt ON p.id = pt.problem_id
JOIN submissions s ON s.problem_id = p.id AND s.users_id = 1
JOIN (
    -- 取每個題目最新的一筆提交
    SELECT s1.problem_id, s1.time_spent, s1.status, s1.submit_date
    FROM submissions s1
    JOIN (
        SELECT problem_id, MAX(id) AS max_id
        FROM submissions
        WHERE users_id = 1
        GROUP BY problem_id
    ) s2 ON s1.id = s2.max_id
    WHERE s1.users_id = 1
) e ON e.problem_id = p.id
WHERE pt.problem_type = 'Array'
GROUP BY p.id, p.title, e.submit_date, e.time_spent, e.status
ORDER BY last_solved_date DESC, solved_over_30min DESC, solved_30min DESC, solved_20min DESC;


