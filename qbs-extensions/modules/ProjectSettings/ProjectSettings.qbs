import qbs
import qbs.Environment

Module
{
	Depends { name: "cpp" }
	cpp.minimumWindowsVersion: "6.1" // windows 7
	
	cpp.defines: ["BOOST_ALLOW_DEPRECATED_HEADERS"]
}
