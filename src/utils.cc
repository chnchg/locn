/**\file
 */
#include "utils.hh"
#include <iostream>

// produce an escaped string that, when double quoted, produces the original
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

int msg_level = 10;
std::ostream & msg(int l)
{
	static std::ostream null_stream(0);
	return msg_level>l ? std::cerr : null_stream;
}

void error(std::string const & m)
{
	msg(0) << "Error: " << m << '\n';
	throw Error(m);
}

void warn(std::string const & m)
{
	msg(1) << "Warning: " << m << '\n';
}
