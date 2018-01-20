/**\file
   \brief Template implementation of Nelder--Mead method for minimization
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
*/
#pragma once
#include <functional>
#include <array>

/// Optimizer with Nelder--Mead method for functions with N arguments
template<int N>
class NelderMead
{
public:
	typedef std::array<double,N> vec_t; ///<parameter space
	typedef std::function<double(vec_t const &)> fun_t; ///<minimized function
private:
	double const alpha = 1; ///<reflection coefficient
	double const beta = 0.5; ///<expansion coefficient
	double const gamma = 2; ///<contraction coefficient
	double const delta = 0.5; ///<shrinking coefficient
	double mxrngy = 0.00001; ///<required function value accuracy
	double mxrngx = 0.00001; ///<required maximum position accuracy
	int mxiter = 1000; ///<maximum number of iterations
public:
	///<perform minimization
	vec_t minimize(
		fun_t fn, ///<function to be minimized
		vec_t const & x0, ///<starting point in parameter space
		vec_t const & step ///<step sizes for simplex construction
	){
		// initialize the simplex
		std::array<vec_t,N+1> s;
		for (int i = 0; i<N; i++) {
			s[i] = x0;
			s[i][i] += step[i];
		}
		s[N] = x0;

		std::array<double,N+1> y;
		for (int i = 0; i<=N; i ++) y[i] = fn(s[i]);

		// workspace
		vec_t xc; // reflection center
		vec_t xn; // test point
		vec_t x2; // next test point
		int li; //lowest
		int ni; //next-to-highest
		int hi; //highest

		// iterate
		int iter = 0;
		do {
			// find lowest, highest, and next-to-highest
			li = 0;
			ni = 0;
			hi = 1;
			if (y[1]<y[0]) {
				li = 1;
				ni = 1;
				hi = 0;
			}
			for (int i = 2; i<=N; i++) {
				if (y[i]<y[li]) li = i;
				else if (y[i]>y[hi]) {
					ni = hi;
					hi = i;
				}
				else if (y[i]>y[ni]) ni = i;
			}
			if (ni==li) ni = 2;
			// check for convergence
			if (y[hi]-y[li]<mxrngy) { // y-range satisfied, check x
				double mx = 0;
				for (int i = 0; i<N; i++) {
					double mnx = s[N][i];
					double mxx = mnx;
					for (int j = 0; j<N; j++) {
						double xx = s[j][i];
						if (xx<mnx) mnx = xx;
						if (xx>mxx) mxx = xx;
					}
					double r = mxx-mnx;
					if (r>mx) mx = r;
				}
				if (mx<mxrngx) break; // x-range small enough, done!
			}
			auto & sh = s[hi]; // highest position
			// find reflection center and reflected position
			for (int i = 0; i<N; i++) {
				xc[i] = 0;
				for (int j = 0; j<=N; j++) xc[i] += s[j][i];
				xc[i] = (xc[i]-sh[i])/N;
				xn[i] = xc[i]+(xc[i]-sh[i])*alpha;
			}
			double yn = fn(xn);
			if (yn<y[ni]) { // reflection ok?
				if (yn<y[li]) { // reflection best?
					// expand
					for (int i = 0; i<N; i++) x2[i] = xc[i]+(xc[i]-sh[i])*gamma;
					double y2 = fn(x2);
					if (y2<yn) { // expansion good?
						y[hi] = y2;
						sh = x2;
					}
					else { // scrap expansion
						y[hi] = yn;
						sh = xn;
					}
				}
				else { // just use reflection
					y[hi] = yn;
					sh = xn;
				}
			}
			else { // reflection bad
				if (yn<y[hi]) { // not worst
					// contract the reflection
					for (int i = 0; i<N; i++) x2[i] = xc[i]+(xn[i]-xc[i])*beta;
					double y2 = fn(x2);
					if (y2<yn) { // contraction better?
						y[hi] = y2;
						sh = x2;
					}
					else { // use relection (this differs from Scholarpedia, which shrinks)
						y[hi] = yn;
						sh = xn;
					}
				}
				else { // worst
					// contract the original position
					for (int i = 0; i<N; i++) xn[i] = xc[i]+(sh[i]-xc[i])*beta;
					yn = fn(xn);
					if (yn<y[hi]) { // improves a bit
						y[hi] = yn;
						sh = xn;
					}
					else { // not working, shink the simplex
						auto & sl = s[li];
						for (int i = 0; i<=N; i++) {
							if (i==li) continue;
							auto & d = s[i];
							for (int j = 0; j<N; j++) d[j] = sl[j]+(d[j]-sl[j])*delta;
							y[i] = fn(d);
						}
					}
				}
			}
			iter ++;
		} while (iter<mxiter);
		return s[li];
	}
};

