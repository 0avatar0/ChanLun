#include <vector>
#include <algorithm>
#include <afxwin.h>  
#include "../TdxApi/PluginTCalcFunc.h"
#include "kline_standardise.h"

enum KlineDir {
	KlineDirContain = 0,
	KlineDirUp,
	KlineDirDown,
};

struct Kline {
	float high;	// 最高值
	float low;  // 最低值
	int valid; // 是否有效
};

std::vector<Kline> g_std_kline;
typedef void (*pKlineStandardiseHandle) (const Kline& standardLeft, const Kline& srcRight);

struct KlineStandardiseBranch {
	KlineDir penDir;
	KlineDir tempDir;
	pKlineStandardiseHandle handle;
	KlineDir nextPenDir;
};

static inline bool KlineIsUp(const Kline& standardLeft, const Kline& srcRight)
{
	if (standardLeft.high < srcRight.high && standardLeft.low < srcRight.low) {
		return true;
	}
	return false;
}

static inline bool KlineIsDown(const Kline& standardLeft, const Kline& srcRight)
{
	if (standardLeft.high > srcRight.high && standardLeft.low > srcRight.low) {
		return true;
	}
	return false;
}

static inline bool KlineIsContain(const Kline& standardLeft, const Kline& srcRight)
{
	return ((!KlineIsUp(standardLeft, srcRight)) && (!KlineIsDown(standardLeft, srcRight)));
}

static KlineDir KlineGetDir(const Kline& standardLeft, const Kline& srcRight)
{
	if (KlineIsUp(standardLeft, srcRight)) {
		return KlineDirUp;
	}
	else if (KlineIsDown(standardLeft, srcRight)) {
		return KlineDirDown;
	} else {
		return KlineDirContain;
	}
}

static void EliminateInvalidKline(const Kline& standardLeft, const Kline& srcRight)
{
	g_std_kline.back().valid = false;
	g_std_kline.emplace_back(srcRight);
}

static void AddIndependentKline(const Kline& standardLeft, const Kline& srcRight)
{
	g_std_kline.emplace_back(srcRight);
}

static void CombineUpKlines(const Kline& standardLeft, const Kline& srcRight)
{
	Kline tempLine = {
		.high = max(standardLeft.high, srcRight.high),
		.low = max(standardLeft.low, srcRight.low),
		.valid = srcRight.valid,
	};
	g_std_kline.back().valid = false;

	g_std_kline.emplace_back(tempLine);
}

static void CombineDownKlines(const Kline& standardLeft, const Kline& srcRight)
{
	Kline tempLine = {
		.high = min(standardLeft.high, srcRight.high),
		.low = min(standardLeft.low, srcRight.low),
		.valid = srcRight.valid,
	};
	g_std_kline.back().valid = false;

	g_std_kline.emplace_back(tempLine);
}

KlineStandardiseBranch gKlineStandardiseTable[] = {
	{KlineDirContain, KlineDirContain, EliminateInvalidKline, KlineDirContain},
	{KlineDirContain, KlineDirUp, AddIndependentKline, KlineDirUp},
	{KlineDirContain, KlineDirDown, AddIndependentKline, KlineDirDown},

	{KlineDirUp, KlineDirContain, CombineUpKlines, KlineDirUp},
	{KlineDirUp, KlineDirUp, AddIndependentKline, KlineDirUp},
	{KlineDirUp, KlineDirDown, AddIndependentKline, KlineDirDown},

	{KlineDirDown, KlineDirContain, CombineDownKlines, KlineDirDown},
	{KlineDirDown, KlineDirUp, AddIndependentKline, KlineDirUp},
	{KlineDirDown, KlineDirDown, AddIndependentKline, KlineDirDown},
};

void RunKlineStandardiseMachine(KlineDir& penDir, KlineDir tempDir, Kline &standardLeft, Kline &srcRight)
{
	for (auto it : gKlineStandardiseTable) {
		if (it.penDir == penDir && it.tempDir == tempDir) {
			if (it.handle == nullptr) {
				break;
			}
			it.handle(standardLeft, srcRight);
			penDir = it.nextPenDir;
			break;
		}
	}
}

static void KlineStandardise(int lineNum, float* output, float* high, float* low, float* date)
{
	KlineDir penDir = KlineDirContain;
	for (int i = 0; i < lineNum; i++) {
		if (i == 0) {
			Kline temp_line = {
				.high = high[i],
				.low = low[i],
				.valid = true,
			};
			g_std_kline.emplace_back(temp_line);
			continue;
		}
		Kline leftKline = g_std_kline.back();
		Kline rightKline = {
			.high = high[i],
			.low = low[i],
			.valid = true,
		};
		KlineDir tempDir = KlineGetDir(leftKline, rightKline);
		RunKlineStandardiseMachine(penDir, tempDir, leftKline, rightKline);
	}
}

void KlineGetHighSet(int lineNum, float* output, float* high, float* low, float* date)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_std_kline.reserve(lineNum);
	KlineStandardise(lineNum, output, high, low, date);
	for (auto i = 0; i < lineNum; i++) {
		output[i] = g_std_kline[i].high;
	}
	g_std_kline.clear();
}

void KlineGetLowSet(int lineNum, float* output, float* high, float* low, float* date)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_std_kline.reserve(lineNum);
	KlineStandardise(lineNum, output, high, low, date);
	for (auto i = 0; i < lineNum; i++) {
		output[i] = g_std_kline[i].low;
	}
	g_std_kline.clear();
}

void KlineGetValidSet(int lineNum, float* output, float* high, float* low, float* date)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_std_kline.reserve(lineNum);
	KlineStandardise(lineNum, output, high, low, date);
	for (auto i = 0; i < lineNum; i++) {
		output[i] = g_std_kline[i].valid;
	}
	g_std_kline.clear();
}

void KlineTestHigh(int lineNum, float* output, float* high, float* low, float* date)
{
	for (auto i = 0; i < lineNum; i++) {
		output[i] = high[i];
	}
}

void KlineTestLow(int lineNum, float* output, float* high, float* low, float* date)
{
	for (auto i = 0; i < lineNum; i++) {
		output[i] = low[i];
	}
}