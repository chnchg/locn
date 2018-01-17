/** \file
	\brief Implementation of Nelder--Mead method for minimization
*/
#include "nelder_mead.hh"
#include <vector>

double const alpha = 1; ///< reflection coefficient
double const beta = 0.5; ///< expansion coefficient
double const gamma = 2; ///< contraction coefficient
double const delta = 0.5; ///< shrinking coefficient

double nelder_mead(func_t f, int n,	double * x,	double const * steps, double mxrngy, double mxrngx, int mxiter)
{
	/// create initial simplex, initial x comes first
	auto s = new double [n*(n+1)];
	for (int i = 0; i<n; i++) s[i] = x[i];
	for (int i = 0; i<n; i++) {
		auto d = s+(i+1)*n;
		for (int j = 0; j<n; j++) d[j] = x[j];
		d[i] += steps[i];
	}

	auto y = new double [n+1];
	for (int i = 0; i<=n; i ++) y[i] = (*f)(s+i*n);
	
	// workspace
	auto xc = new double [n]; // reflection center
	auto xn = new double [n]; // next point
	auto x2 = new double [n]; // next point
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
		for (int i = 2; i<=n; i++) {
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
			for (int i = 0; i<n; i++) {
				auto d = s+i;
				double mnx = * d;
				double mxx = mnx;
				for (int j = 0; j<=n; j++) {
					double xx = s[j*n+i];
					if (xx<mnx) mnx = xx;
					if (xx>mxx) mxx = xx;
				}
				double r = mxx-mnx;
				if (r>mx) mx = r;
			}
			if (mx<mxrngx) break; // x-range small enough, done!
		}
		auto sh = s+hi*n; // highest position
		// find reflection center and reflected position
		for (int i = 0; i<n; i++) {
			xc[i] = 0;
			for (int j = 0; j<=n; j++) xc[i] += s[j*n+i];
			xc[i] = (xc[i]-sh[i])/n;
			xn[i] = xc[i]+(xc[i]-sh[i])*alpha;
		}
		double yn = (*f)(xn);
		if (yn<y[ni]) { // reflection ok?
			if (yn<y[li]) { // reflection best?
				// expand
				for (int i = 0; i<n; i++) x2[i] = xc[i]+(xc[i]-sh[i])*gamma;
				double y2 = (*f)(x2);
				if (y2<yn) { // expansion good?
					y[hi] = y2;
					for (int i = 0; i<n; i++) sh[i] = x2[i];
				}
				else { // scrap expansion
					y[hi] = yn;
					for (int i = 0; i<n; i++) sh[i] = xn[i];
				}
			}
			else { // just use reflection
				y[hi] = yn;
				for (int i = 0; i<n; i++) sh[i] = xn[i];
			}
		}
		else { // reflection bad
			if (yn<y[hi]) { // not worst
				// contract the reflection
				for (int i = 0; i<n; i++) x2[i] = xc[i]+(xn[i]-xc[i])*beta;
				double y2 = (*f)(x2);
				if (y2<yn) { // contraction better?
					y[hi] = y2;
					for (int i = 0; i<n; i++) sh[i] = x2[i];
				}
				else { // use relection (this differs from Scholarpedia, which shrinks)
					y[hi] = yn;
					for (int i = 0; i<n; i++) sh[i] = xn[i];
				}
			}
			else { // worst
				// contract the original position
				for (int i = 0; i<n; i++) xn[i] = xc[i]+(sh[i]-xc[i])*beta;
				yn = (*f)(xn);
				if (yn<y[hi]) { // improves a bit
					y[hi] = yn;
					for (int i = 0; i<n; i++) sh[i] = xn[i];
				}
				else { // not working, shink the simplex
					auto sl = s+li*n;
					for (int i = 0; i<=n; i++) {
						if (i==li) continue;
						auto d = s+i*n;
						for (int j = 0; j<n; j++) d[j] = sl[j]+(d[j]-sl[j])*delta;
						y[i] = (*f)(d);
					}
				}
			}
		}
		iter ++;
	} while (iter<mxiter);
	for (int i = 0; i<n; i++) x[i] = s[li*n+i];
	delete [] xc;
	delete [] xn;
	delete [] x2;
	return y[li];
}
