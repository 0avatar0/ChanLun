#include "fractal.h"

#include <vector>
#include <afxwin.h>  
#include <afxmt.h>

#include "../KlineStandard/kline_standardise.h"

#define IS_STROKE true
#define NOT_STROKE false
typedef void (*pFractalHandle) (size_t input_idx, size_t& cur_idx);

CCriticalSection g_fractal_cs;

enum FractalType {
	FractalTypeNone = 0,
	FractalTypeTop,
	FractalTypeBottom,
};

struct Fractal
{
	FractalType type;
	bool is_endpoint;
};

struct FindStrokeEndpointBranch
{
	FractalType cur_fractal;          // 当前分型
	FractalType input_fractal;        // 输入的分型
	bool is_stroke;			          // 是否成笔
	pFractalHandle handle;	          // 行为
	FractalType cur_fractal_will_be;  // 当前分型将要变成的分型
};

std::vector<Fractal> g_fractals;

std::vector<Fractal>& GetFractals()
{
	CSingleLock lock(&g_fractal_cs, TRUE);
	return g_fractals;
}

void ReserveFractals(size_t size)
{
	CSingleLock lock(&g_fractal_cs, TRUE);
	g_fractals.reserve(size);
}

void ClearFractals()
{
	CSingleLock lock(&g_fractal_cs, TRUE);
	g_fractals.clear();
}

static void MarkIndependentEndpoint(size_t input_idx, size_t& cur_idx)
{
	CSingleLock lock(&g_fractal_cs, TRUE);
	g_fractals[input_idx].is_endpoint = true;
	cur_idx = input_idx;
}

static void MarkHigherTopEndpoint(size_t input_idx, size_t& cur_idx)
{
	std::vector<Kline> &std_kline = GetStdKline();
	CSingleLock lock(&g_fractal_cs, TRUE);
	if (std_kline[input_idx].high >= std_kline[cur_idx].high) {
		g_fractals[input_idx].is_endpoint = true;
		cur_idx = input_idx;
	}
}

static void MarkLowerTopEndpoint(size_t input_idx, size_t& cur_idx)
{
	std::vector<Kline>& std_kline = GetStdKline();
	CSingleLock lock(&g_fractal_cs, TRUE);
	if (std_kline[input_idx].low <= std_kline[cur_idx].low) {
		g_fractals[input_idx].is_endpoint = true;
		cur_idx = input_idx;
	}
}

FindStrokeEndpointBranch g_find_stroke_endpoint_table[] = {
	{FractalTypeNone, FractalTypeNone, IS_STROKE, nullptr, FractalTypeNone},
	{FractalTypeNone, FractalTypeTop, IS_STROKE, MarkIndependentEndpoint, FractalTypeTop},
	{FractalTypeNone, FractalTypeBottom, IS_STROKE, MarkIndependentEndpoint, FractalTypeBottom},

	{FractalTypeNone, FractalTypeNone, NOT_STROKE, nullptr, FractalTypeNone},
	{FractalTypeNone, FractalTypeTop, NOT_STROKE, MarkIndependentEndpoint, FractalTypeTop},
	{FractalTypeNone, FractalTypeBottom, NOT_STROKE, MarkIndependentEndpoint, FractalTypeBottom},

	{FractalTypeTop, FractalTypeNone, IS_STROKE, nullptr, FractalTypeTop},
	{FractalTypeTop, FractalTypeTop, IS_STROKE, MarkHigherTopEndpoint, FractalTypeTop},
	{FractalTypeTop, FractalTypeBottom, IS_STROKE, MarkIndependentEndpoint, FractalTypeBottom},

	{FractalTypeTop, FractalTypeNone, NOT_STROKE, nullptr, FractalTypeTop},
	{FractalTypeTop, FractalTypeTop, NOT_STROKE, MarkHigherTopEndpoint, FractalTypeTop},
	{FractalTypeTop, FractalTypeBottom, NOT_STROKE, nullptr, FractalTypeTop},


	{FractalTypeBottom, FractalTypeNone, IS_STROKE, nullptr, FractalTypeBottom},
	{FractalTypeBottom, FractalTypeTop, IS_STROKE, MarkIndependentEndpoint, FractalTypeTop},
	{FractalTypeBottom, FractalTypeBottom, IS_STROKE, MarkLowerTopEndpoint, FractalTypeBottom},

	{FractalTypeBottom, FractalTypeNone, NOT_STROKE, nullptr, FractalTypeBottom},
	{FractalTypeBottom, FractalTypeTop, NOT_STROKE, nullptr, FractalTypeBottom},
	{FractalTypeBottom, FractalTypeBottom, NOT_STROKE, MarkLowerTopEndpoint, FractalTypeBottom},
};

static FractalType GetFractalType(const Kline& left, const Kline& middle, const Kline& right)
{
	if (middle.high > left.high && middle.high > right.high) {
		return FractalTypeTop;
	}
	else if (middle.low < left.low && middle.low < right.low) {
		return FractalTypeBottom;
	}
	else {
		return FractalTypeNone;
	}
}

void MarkAllFractal()
{
	std::vector<Kline>& std_kline = GetStdKline();

	CSingleLock lock(&g_fractal_cs, TRUE);
	g_fractals.reserve(std_kline.size());
	for (auto i = 0; i < std_kline.size(); i++) {
		if (i < 2) {
			Fractal fractal = {
				.type = FractalTypeNone,
				.is_endpoint = false,
			};
			g_fractals.emplace_back(fractal);
			continue;
		}
		Fractal temp_fractal = {
			.type = GetFractalType(std_kline[i - 2], std_kline[i - 1], std_kline[i]),
			.is_endpoint = false,
		};
		g_fractals.emplace_back(temp_fractal);
	}
}

void RunFindStrokeEndpointMachine(FractalType cur_fractal, FractalType input_fractal, bool is_stroke, size_t input_idx, size_t& cur_idx)
{
	for (auto it : g_find_stroke_endpoint_table) {
		if (it.cur_fractal == cur_fractal && it.input_fractal == input_fractal && it.is_stroke == is_stroke) {
			if (it.handle != nullptr) {
				it.handle(input_idx, cur_idx);
			}
			break;
		}
	}
}

bool IsStroke(size_t input_idx, size_t cur_idx)
{
	if (cur_idx - input_idx >= 5) {
		return true;
	}
	return false;
}

void MarkStrokeEndpoint()
{
	MarkAllFractal();
	CSingleLock lock(&g_fractal_cs, TRUE);
	size_t cur_fractal_idx = 0;
	for (auto i = 0; i < g_fractals.size(); i++) {
		FractalType cur_fractal = g_fractals[cur_fractal_idx].type;
		FractalType input_fractal = g_fractals[i].type;
		bool is_stroke = IsStroke(i, cur_fractal_idx);
		RunFindStrokeEndpointMachine(cur_fractal, input_fractal, is_stroke, i, cur_fractal_idx);
	}
}

void TdxGetStrokeEndpoint(int lineNum, float* output, float* high, float* low, float* date)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ReseveStdKlineSize(lineNum);
	ReserveFractals(lineNum);
	KlineStandardise(lineNum, output, high, low, date);
	MarkStrokeEndpoint();
	std::vector<Fractal>& fractals = GetFractals();
	for (auto i = 0; i < lineNum; i++)
	{
		output[i] = fractals[i].is_endpoint;
	}
	ClearStdKlineSize(); 
	ClearFractals();
}

void TdxGetFractalType(int lineNum, float* output, float* high, float* low, float* date)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ReseveStdKlineSize(lineNum);
	ReserveFractals(lineNum);
	KlineStandardise(lineNum, output, high, low, date);
	MarkStrokeEndpoint();
	std::vector<Fractal>& fractals = GetFractals();
	for (auto i = 0; i < lineNum; i++)
	{
		output[i] = fractals[i].is_endpoint;
	}
	ClearStdKlineSize();
	ClearFractals();
}