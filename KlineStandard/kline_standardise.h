#pragma once
#ifndef KLINE_STANDARDISE_H
#define KLINE_STANDARDISE_H
#include <vector>

struct Kline {
	float high;	// 最高值
	float low;  // 最低值
	int valid; // 是否有效
};

void KlineStandardise(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetHighSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetLowSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetValidSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineTestHigh(int lineNum, float* output, float* high, float* low, float* date);
void KlineTestLow(int lineNum, float* output, float* high, float* low, float* date);
std::vector<Kline>& GetStdKline();
void ReseveStdKlineSize(int kline_num);
void ClearStdKlineSize();
#endif
