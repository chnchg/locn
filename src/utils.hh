/**\file
   \brief Miscellaneous utilities
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
 */
#pragma once
#include <string>

/// escape a string in a way that when double quoted, recovers original
extern std::string esc_str(
	std::string const & s ///< string to be escaped
); ///< \return escaped string

/// Error exception
struct Error
{
	std::string msg; ///<description of the error
	/// Create exception with message
	Error(std::string const & msg ///<message text
	) : msg(msg) {}
};

extern int msg_level; ///<level below which messages are shown
extern std::ostream & msg(int level); ///<message stream with given level
extern std::ostream & debug; ///<ostream for debugging message
extern std::ostream & info; ///<ostream informative message
extern std::ostream & warn; ///<ostream warning message
void error(std::string const & m); ///<throw error after error message
