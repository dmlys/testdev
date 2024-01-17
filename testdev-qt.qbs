import qbs
import qbs.Environment

Project
{
	references: [
		"QtTools/QtTools.qbs",
		"QtTools/QtTools-tests.qbs",
		"QtTools/examples/viewed-examples.qbs",
	]
			
	CppApplication
	{
		name: "Qt-testdev"
		
		Depends { name: "netlib" }
		Depends { name: "extlib" }
		Depends { name: "wincrypt-utils"; condition: qbs.targetOS.contains("windows") }
		Depends { name: "xercesc-utils" }

		Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }
		Depends { name: "QtTools" }
		
		Depends { name: "dmlys.qbs-common"; required: false }
		Depends { name: "ProjectSettings"; required: false }

		qbs.debugInformation: true
		cpp.cxxLanguageVersion : "c++23"
		
		cpp.dynamicLibraries:
		{
			if (qbs.toolchain.contains("msvc"))
			{
				var libs = [
					//"boost_system", "boost_thread", // on msvc boost is autolinked
					"xercesc",
					"openssl-crypto", "openssl-ssl",
					"zlib", "libfmt",
				];
				
				libs = BuildUtils.make_winlibs(qbs, cpp, libs)
				libs = libs.concat(["ws2_32", "crypt32", "user32", "advapi32", "Iphlpapi"])
				
				return libs
			}
			
			if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			{
				var libs = [
					"boost_system", "boost_thread",
					"xerces-c",
					"ssl", "crypto", "z", "fmt", "stdc++fs",
				]
				
				return libs
			}
			
			return undefined
		}

		files: [
			"qt-main.cpp"
		]
	}
}
 
