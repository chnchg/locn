/**\file
   \brief Miscellaneous utilities
 */
#pragma once
#include <array>
#include <iostream>
///print a C++ array of doubles
template<size_t N>
std::ostream & operator<<(std::ostream & s,std::array<double,N> a)
{
	std::string p = "[";
	for (auto v:a) {
		s << p << v;
		p = ",";
	}
	s << "]";
	return s;
}
