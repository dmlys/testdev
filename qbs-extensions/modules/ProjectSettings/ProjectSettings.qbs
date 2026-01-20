import qbs
import qbs.Environment

Module
{
	Depends { name: "cpp" }
	Depends { name: "LocalSettings"; required: false }

	qbs.debugInformation: true
	cpp.minimumWindowsVersion: "6.1" // windows 7
	cpp.defines: ["BOOST_ALLOW_DEPRECATED_HEADERS", "_CRT_NONSTDC_NO_WARNINGS"]
	//cpp.runtimeLibrary: "static"
}
