#pragma once
#include <cstdint>
#include <array>
namespace protocol {
	constexpr size_t MAX_BUFFERSIZE = 2048;

	// 傳送封包結構
	struct SocketPackage {
		uint8_t cStartByte;
		uint8_t cType;
		int32_t nLen;
		std::array<uint8_t, MAX_BUFFERSIZE> pDataBuf{};
		uint8_t cEndByte;
	};
	
    // SQL 協議類型
    namespace sql_protocol {
		constexpr uint8_t REGISTER_USER          = 0; // 註冊用戶
		constexpr uint8_t LOGIN                  = 1; // 登入
		constexpr uint8_t SELECT_UNITS_DATA      = 2; // 查找所有單元的解題成功率
		constexpr uint8_t SELECT_UNIT_DETAIL     = 3; // 查找各別單元的解題情況
		constexpr uint8_t GET_PROBLEM_SUBMISSIONS= 5; // 查詢某題的提交狀況
		constexpr uint8_t INSERT_SUBMITTED       = 6; // 插入提交狀況
		constexpr uint8_t DELETE_SUBMITTED       = 7; // 刪除提交狀況
		// === For Oscar ===
		constexpr uint8_t DELETE_USER             = 8;  // 刪除用戶
		constexpr uint8_t INSERT_PROBLEM          = 9;  // 插入題目
		constexpr uint8_t DELETE_PROBLEM          = 10; // 刪除題目
		constexpr uint8_t INSERT_PROBLEM_TYPE     = 11; // 插入題目類型
		constexpr uint8_t DELETE_PROBLEM_TYPE     = 12; // 刪除題目類型
	}

    // 封包起始與結束
    constexpr char PACKAGE_START_BYTE = '*';  //42
    constexpr char PACKAGE_END_BYTE   = '\r'; //13
};
