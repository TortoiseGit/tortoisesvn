#pragma once

class CUtils
{
public:
	CUtils(void);
	~CUtils(void);
	static void StringExtend(LPTSTR str);
	static void StringCollapse(LPTSTR str);
	static void Error();
};
