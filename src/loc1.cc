/**\file
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
*/
#include "tiff.hh"
#include "nelder_mead.hxx"
#include "utils.hh"
#include <iostream>
#include <cmath>

double i2p = 3.6; ///<intensity to photon scale
double plsz = 80; ///<pixel size in nm
int fwr = 4; ///<fitting window range

/// Type for fitting parameters
typedef std::array<double,5> param_t;

/// Point-spread function with integrated Gaussian
double psf_ig2(double x, double y, param_t const & p)
{
	double s2s = sqrt(2)*p[2]*p[2];
	double ex = (erf((x-p[0]+.5)/s2s)-erf((x-p[0]-.5)/s2s))*.5;
	double ey = (erf((y-p[1]+.5)/s2s)-erf((y-p[1]-.5)/s2s))*.5;
	return ex*ey*p[3]*p[3]+p[4]*p[4];
}

/// Functor for likelihood calculation
class Likelihood
{
	double const * i; ///<subimage pointer
	int l; ///<lateral size of subimage
	int w; ///<original image width (for row skip)
public:
	int cnt; ///<evaluaton count for the function
	Likelihood(
		int l, ///<lateral size
		int w ///<original image width
	) : l(l), w(w) {}
	void set_image(double const * im) {i = im;}
	double operator()(param_t const & p)
	{
		double tl = 0;
		for (int y = 0; y<l; y++) for (int x = 0; x<l; x++) {
			auto psf = psf_ig2(x,y,p);
			tl += i[y*w+x]*log(psf)-psf;
		}
		return -tl;
	}
};

struct Particle
{
	int x;
	int y;
	param_t p;
};

/// Process a single 2D image
std::vector<Particle> process_image(
	double const * data, ///<image data
	int w, ///<image width
	int h ///<image height
)
{
	std::vector<Particle> res;
	// convolution kernels
	double const wk1[] = {1./16,1./4,3./8,1./4,1./16};
	double const wk2[] = {1./16,0,1./4,0,3./8,0,1./4,0,1./16};
	int sz = w*h;
	int l = 2*fwr+1;
	// utilities
	std::vector<double> bf(sz); // workspace
	NelderMead<5> nm; // Nelder--Mead minimizer
	Likelihood fn(l,w); // likelihood function

	// calculate v1,f1
	std::vector<double> v1(sz);
	for (int i = 0; i<sz; i++) {
		int x = i%w;
		double s = 0;
		int r1 = x+2<w ? 5 : w+2-x;
		for (int j = x<2 ? 2-x : 0; j<r1; j++) s += data[i+j-2]*wk1[j];
		bf[i] = s;
	}
	double f1a = 0;
	double f1a2 = 0;
	for (int i = 0; i<sz; i++) {
		int y = i/w;
		double s = 0;
		int r1 = y+2<h ? 5 : h+2-y;
		for (int j = y<2 ? 2-y : 0; j<r1; j++) s += bf[i+j*w-2*w]*wk1[j];
		v1[i] = s;
		double f1 = data[i]-s;
		f1a += f1;
		f1a2 += f1*f1;
	}
	f1a /= sz;
	f1a2 /= sz;
	double threshold = 1.5*sqrt(f1a2-f1a*f1a);
	debug << "threshold = " << threshold << '\n';

	// calculate f2
	std::vector<double> f2(sz);
	for (int i = 0; i<sz; i++) {
		int x = i%w;
		double s = 0;
		int r1 = x+4<w ? 9 : w+4-x;
		for (int j = x<4 ? 4-x : 0; j<r1; j++) s += v1[i+j-4]*wk2[j];
		bf[i] = s;
	}
	for (int i = 0; i<sz; i++) {
		int y = i/w;
		double s = 0;
		int r1 = y+4<h ? 9 : h+4-y;
		for (int j = y<4 ? 4-y : 0; j<r1; j++) s += bf[i+j*w-4*w]*wk2[j];
		f2[i] = v1[i]-s;
	}
   	// convert intensity to photon count
	for (int i = 0; i<sz; i++) bf[i] = data[i]*i2p;

	// find 8-connected local maximum by forward elimination
	std::vector<int> nd{1,w+1,w,w-1};
	std::vector<bool> n8(sz,true);
	param_t stps = {1,1,0.2,1,1}; // step size
	int ne = sz-w-1;
	for (int i = 0; i<ne; i++) {
		for (int d: nd) {
			if (f2[i]>f2[i+d]) n8[i+d] = false;
			else n8[i] = false;
		}
	    int x = i%w;
		int y = i/w;
		if (n8[i] && x>=fwr && x<w-fwr && y>=fwr && y<h-fwr && f2[i]>threshold) {
			// a local maximum, perform fitting to PSF
			double const * sq = bf.data()+i-(w+1)*fwr; // keeping starting corner of square
			// initial guess
			double mx = sq[0];
			double mn = sq[0];
			for (int y = 0; y<l; y++) for (int x = 0; x<l; x++) {
				double vv = sq[y*w+x];
				if (vv>mx) mx = vv;
				else if (vv<mn) mn = vv;
			}
			param_t p = {double(fwr),double(fwr),sqrt(1.6),sqrt(mx-mn),sqrt(mn)};
			fn.set_image(sq);
			fn.cnt = 0;
			res.push_back({x,y,nm.minimize(std::ref(fn),p,stps)});
		}
	}
	return res;
}

/// Main function for localization
int main(int argc, char ** argv)
{
	if (argc<2) {
		msg(0) << "Missing expected filename!\nUsage:\n";
		msg(0) << '\t' << argv[0] << " <filename of TIFF>\n\n";
		return EXIT_FAILURE;
	}

	Tiff tf(std::make_shared<std::ifstream>(argv[1]));
	tf.start();

	uint32_t nxt = 0;
	unsigned icnt = 0;
	do {
		nxt = tf.parse_ifd(nxt);
		int w = tf.image_width;
		int h = tf.image_length;
		int sz = w*h;
		auto imd = tf.read_image();
		uint16_t * v = reinterpret_cast<uint16_t *>(imd.data());
		// convert to double
		std::vector<double> im(v,v+sz);
		for (auto r: process_image(im.data(), w, h)) {
			// throw out outliers
			if (r.p[0]<fwr-fwr/2||r.p[0]>fwr+fwr/2) continue;
			if (r.p[1]<fwr-fwr/2||r.p[1]>fwr+fwr/2) continue;
			if (r.p[2]<0.5||r.p[2]>fwr/2) continue;
			if (abs(r.p[3])>1000) continue;
			std::cout << icnt+1 << ",\t"; // layer
			std::cout << plsz*(r.x+r.p[0]) << ",\t";
			std::cout << plsz*(r.y+r.p[1]) << ",\t";
			std::cout << plsz*(r.p[2]*r.p[2]) << ",\t";
			std::cout << r.p[3]*r.p[3] << ",\t";
			std::cout << r.p[4]*r.p[4] << '\n';
		}
		icnt ++;
	} while (nxt);
	return 0;
}
