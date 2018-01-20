///\file
#include "tiff.hh"
#include "nelder_mead.hh"
#include "utils.hh"
#include <iostream>
#include <cmath>

/// Point-spread function with integrated Gaussian
double psf_ig2(double x, double y, double const * p)
{
	double s2s = sqrt(2)*p[2]*p[2];
	double ex = (erf((x-p[0]+.5)/s2s)-erf((x-p[0]-.5)/s2s))*.5;
	double ey = (erf((y-p[1]+.5)/s2s)-erf((y-p[1]-.5)/s2s))*.5;
	return ex*ey*p[3]*p[3]+p[4]*p[4];
}

/// negative Likelihood calculation for given parameter
double likelihood(
	double const * i, ///< image
	int l, ///< lateral size of subimage
	int w, ///< original image width
	double const * p ///< parameters
)
{
	double tl = 0;
	for (int y = 0; y<l; y++) for (int x = 0; x<l; x++) {
		auto psf = psf_ig2(x,y,p);
		tl += i[y*w+x]*log(psf)-psf;
	}
	return -tl;
}

// additional parameters for function
double const * fn_im; ///<cropped square image data
int fn_w; ///<image width
unsigned fn_cnt; ///<count of function calls

/// Function to be minimized
double fn(double const * p)
{
	fn_cnt ++;
	return likelihood(fn_im, 9, fn_w, p);
}

/// Process a single 2D image
void process_image(
	double const * data, ///<image data
	int w, ///<image width
	int h ///<image height
)
{
	// convolution kernels
	double const wk1[] = {1./16,1./4,3./8,1./4,1./16};
	double const wk2[] = {1./16,0,1./4,0,3./8,0,1./4,0,1./16};
	auto sz = w*h;
	// workspace
	std::vector<double> bf(sz);

	// calculate v1,f1
	std::vector<double> v1(sz);
	for (auto i = 0; i<sz; i++) {
		auto x = i%w;
		double s = 0;
		auto r1 = x+2<w?5:w+2-x;
		for (auto j = x<2?2-x:0; j<r1; j++) s += data[i+j-2]*wk1[j];
		bf[i] = s;
	}
	double f1a = 0;
	double f1a2 = 0;
	for (auto i = 0; i<sz; i++) {
		auto y = i/w;
		double s = 0;
		auto r1 = y+2<h?5:h+2-y;
		for (auto j = y<2?2-y:0; j<r1; j++) s += bf[i+j*w-2*w]*wk1[j];
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
	for (auto i = 0; i<sz; i++) {
		auto x = i%w;
		double s = 0;
		auto r1 = x+4<w?9:w+4-x;
		for (auto j = x<4?4-x:0; j<r1; j++) s += v1[i+j-4]*wk2[j];
		bf[i] = s;
	}
	for (auto i = 0; i<sz; i++) {
		auto y = i/w;
		double s = 0;
		auto r1 = y+4<h?9:h+4-y;
		for (auto j = y<4?4-y:0; j<r1; j++) s += bf[i+j*w-4*w]*wk2[j];
		f2[i] = v1[i]-s;
	}

	// find 8-connected local maximum by forward elimination
	std::vector<int> nd{1,w+1,w,w-1};
	std::vector<bool> n8(sz,true);
	double stps[] = {1,1,0.2,1,1}; // step size
	auto ne = sz-w-1;
	for (int i = 0; i<ne; i++) {
		for (auto d: nd) {
			if (f2[i]>f2[i+d]) n8[i+d] = false;
			else n8[i] = false;
		}
	    int x = i%w;
		int y = i/w;
		if (n8[i]&&x>=4&&x<w-4&&y>=4&&y<h-4&&f2[i]>threshold) {
			// a local maximum, perform fitting to PSF
			double const * sq = data+i-(w+1)*4; // keeping starting corner of square
			// initial guess
			double mx = sq[0];
			double mn = sq[0];
			for (int y = 0; y<9; y++) for (int x = 0; x<9; x++) {
				auto vv = sq[y*w+x];
				if (vv>mx) mx = vv;
				else if (vv<mn) mn = vv;
			}
			double p[] = {4,4,sqrt(1.6),sqrt(mx-mn),sqrt(mn)};
			fn_im = sq;
			fn_w = w;
			fn_cnt = 0;
			nelder_mead(&fn, 5, p, stps);
			std::string s = "[";
			for (int i = 0; i<5; i++) {
				std::cout << s << p[i];
				s = ", ";
			}
			std::cout << "]\n";
		}
	}
}





/// Main function for localization
int main(int argc, char ** argv)
{
	if (argc<2) {
		std::cerr << "Missing expected filename!\nUsage:\n";
		std::cerr << '\t' << argv[0] << " <filename of TIFF>\n\n";
		return EXIT_FAILURE;
	}

	Tiff tf(std::make_shared<std::ifstream>(argv[1]));
	tf.start();

	uint32_t nxt = 0;
	unsigned icnt = 0;
	do {
		nxt = tf.parse_ifd(nxt);
		auto w = tf.image_width;
		auto h = tf.image_length;
		auto sz = w*h;
		auto imd = tf.read_image();
		uint16_t * v = reinterpret_cast<uint16_t *>(imd.data());
		// convert to double
		std::vector<double> im(v,v+sz);
		std::cout << "\n\n\nicnt = " << icnt << "\n===================\n";
		process_image(im.data(), w, h);
		icnt ++;
	} while (nxt);
	return 0;
}
