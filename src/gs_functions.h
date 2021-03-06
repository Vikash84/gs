/******************************************\
| Graph Splitting Method v2.4 (2019/02/12) |
|                                          |
| Copyright (c) 2015-2019 Motomu Matsui    |
|     Distributed under the GNU GPL        |
|                                          |
|     Matsui M and Iwasaki W (2019)        |
|     Systematic Biology, xx:xx-xx.        |
|                                          |
|     http://gs.bs.s.u-tokyo.ac.jp/        |
\******************************************/

#ifndef GS_FUNCTION_H
#define GS_FUNCTION_H

#include <algorithm>
#include <cmath> 
#include <vector>

using namespace std;

void sedMATRIX(int* (&step), vector<int>& vec, vector<int> const& pos, int const num, int const N);
void sedVECTOR(vector<int>& vec, vector<int> const& pos, int const num);
double simI(double* const (&W), vector<int> const& res, int const num);
int whichMIN(vector<double> const& vec);
double gev(double const& x, double const& mu);

#endif /*GS_FUNCTION_H*/
