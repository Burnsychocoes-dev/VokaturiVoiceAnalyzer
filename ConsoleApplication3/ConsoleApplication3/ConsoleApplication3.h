#pragma once

#define TESTDLLSORT_API __declspec(dllexport) 

extern "C" {
	TESTDLLSORT_API int AnalyzeSamplesWithoutBuffer(double samples[], int nbOfSample, double emotions[]);
}