/**\file
   \brief Miscellaneous utilities
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
 */
#include "utils.hh"
#include <iostream>

/// Produce an escaped string that, when double quoted, produces the original
std::string esc_str(std::string const & s)
{
	std::string t;
	bool f = false; // flag for previous extensible escape
	for (auto i:s) {
		if (f&&i>='0'&&i<'8') { // escaped with hex or oct
			t += "\"\""; // escape terminating concatenation
			t += i;
			f = false;
			continue;
		}
		f = false;
		if (i=='\\') {t += "\\\\"; continue;}
		if (i=='"') {t += "\\\""; continue;}
		if (isprint(i)) {t += i; continue;}
		if (i=='\n') {t += "\\n"; continue;}
		if (i=='\r') {t += "\\r"; continue;}
		if (i=='\t') {t += "\\t"; continue;}
		int r = (unsigned char) i;
		t += '\\';
		if (r>64) t += char('0'+r/64);
		if (r>8) t += char('0'+r%64/8);
		t += char('0'+r%8);
		f = true;
	}
	return t;
}

/// Message levels
enum MsgLevel {
	MSGL_ERROR = 0,
	MSGL_WARN = 1,
	MSGL_INFO = 2,
	MSGL_DEBUG = 9,
	MSGL_ALL = 10
};

int msg_level = MSGL_INFO; ///<message level to display
/// message output stream
std::ostream & msg(int l)
{
	static std::ostream null_stream(0);
	return msg_level>=l ? std::cerr : null_stream;
}
std::ostream & debug = msg(MSGL_DEBUG); ///<stream for debug messages
std::ostream & info = msg(MSGL_INFO); ///<stream for informative messages
std::ostream & warn = msg(MSGL_WARN); ///<stream for warning messages

/// throw an Error with given message string
void error(std::string const & m)
{
	msg(0) << "Error: " << m << '\n';
	throw Error(m);
}
