import qbs
import qbs.Environment


Project
{
	qbsSearchPaths: ["qbs-extensions"]
	
	SubProject
	{
		filePath: "extlib/extlib.qbs"
		Properties {
			name: "extlib"
			with_zlib: true
		}
	}

	SubProject
	{
		filePath: "netlib/netlib.qbs"
		Properties {
			name: "netlib"
			with_openssl: true
		}
	}

	SubProject
	{
		filePath: "xercesc_utils/xercesc_utils.qbs"
		Properties {
			name: "xercesc_utils"
		}
	}

	SubProject
	{
		filePath: "testdev-tests.qbs"
		Properties {
			name: "tests"
		}
	}

	CppApplication
	{
		Depends { name: "netlib" }
		Depends { name: "extlib" }
		Depends { name: "xercesc_utils" }
		Depends { name: "ProjectSettings"; required: false }

		qbs.debugInformation: true
		cpp.cxxLanguageVersion : "c++17"
		cpp.defines: project.additionalDefines
		cpp.cxxFlags: project.additionalCxxFlags
		cpp.driverFlags: project.additionalDriverFlags

		cpp.includePaths: project.additionalIncludePaths
		cpp.systemIncludePaths: project.additionalSystemIncludePaths
		cpp.libraryPaths: project.additionalLibraryPaths

		cpp.dynamicLibraries: {
			libs = []
			if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			{
				//libs = libs.concat(["log4cplus"])
				libs = libs.concat(["boost_context", "boost_fiber", "boost_timer", "boost_filesystem", "boost_system", "boost_thread"])
				libs = libs.concat(["xerces-c"])
				libs = libs.concat(["ssl", "crypto", "z", "fmt", "stdc++fs"])
			}

			if (qbs.toolchain.contains("mingw"))
			{
				libs.push("ws2_32")
				libs.push("crypt32")
				libs.push("ssp") // for mingw(gcc) stack protector, _FORTIFY_SOURCE stuff
			}

			//if (qbs.targetOS.contains("windows"))
			//{
			//	if (qbs.buildVariant == "release")
			//		return ["libfmt-mt", "openssl-crypto-mt", "openssl-ssl-mt", "zlib-mt"]
			//	else
			//		return ["libfmt-mt-gd", "openssl-crypto-mt-gd", "openssl-ssl-mt-gd", "zlib-mt-gd"]
			//}

			return libs
		}

		files: [
			"main.cpp",
			"future-fiber.*",
			//"unix-domain-socket.cpp",
			//"socket-rest-subscriber-main.cpp",
		]
	}
	
	AutotestRunner {}
}
