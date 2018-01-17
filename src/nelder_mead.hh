/**\file
   \brief Nelder--Mead method for function minimization
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
   \details This implementation follows roughly the details as described at
   <a href="http://www.scholarpedia.org/article/Nelder-Mead_algorithm">Scholarpedia</a>,
   with a small deviation in the condition to perform shrink of simplex.
*/
#pragma once

/// Function type with a number of parameters
typedef double (func_t)(double const *);

/// Perform minimization with Nelder--Mead method
extern double nelder_mead(
	func_t f, ///<[in] function to be minimized
	int n, ///<[in] number of dimensions of parameter space
	double * x, ///<[in,out] initial guess, returns position of found minimum
	double const * steps, ///<[in] step sizes in all directions
	double mxrngy = 0.00001, ///<[in] required function value accuracy
	double mxrngx = 0.00001, ///<[in] required maximum position accuracy
	int mxiter = 1000 ///<[in] maximum number of iterations
); ///< \return the minimum value of the function found
