#pragma once
#ifndef KLINE_STANDARDISE_H
#define KLINE_STANDARDISE_H

void KlineStandardise(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetHighSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetLowSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineGetValidSet(int lineNum, float* output, float* high, float* low, float* date);
void KlineTestHigh(int lineNum, float* output, float* high, float* low, float* date);
void KlineTestLow(int lineNum, float* output, float* high, float* low, float* date);

#endif
