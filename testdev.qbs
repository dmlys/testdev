import qbs
import qbs.Environment


Project
{
	property pathList additionalIncludePaths: {
		var includes = []
		return includes
	}

	property pathList additionalSystemIncludePaths: {
		var includes = [];
		var envIncludes = Environment.getEnv("QBS_THIRDPARTY_INCLUDES")
		if (envIncludes)
		{
			envIncludes = envIncludes.split(qbs.pathListSeparator)
			includes = includes.uniqueConcat(envIncludes)
		}

		return includes;
	}

	property pathList additionalLibraryPaths: {
		var libPaths = []
		var envLibPaths = Environment.getEnv("QBS_THIRDPARTY_LIBPATH")
		if (envLibPaths)
		{
			envLibPaths = envLibPaths.split(qbs.pathListSeparator)
			libPaths = libPaths.uniqueConcat(envLibPaths)
		}

		return libPaths
	}

	property stringList additionalDefines: {
		var defs = []

        if (qbs.toolchain.contains("mingw") || qbs.toolchain.contains("msvc"))
		{
            defs.push("_SCL_SECURE_NO_WARNINGS")
			defs.push("_WIN32_WINNT=0x0600")
			defs.push("UNICODE")
			defs.push("_UNICODE")
		}

		return defs
	}

	property stringList additionalCxxFlags: {
		var flags = []
		if (qbs.toolchain.contains("msvc"))
		{

		}
		else if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
		{
			//flags.push("-Wsuggest-override")
			flags.push("-Wno-unused-parameter")
			flags.push("-Wno-unused-function")
			flags.push("-Wno-implicit-fallthrough")
		}

		//flags.push("-fdump-ipa-inline")
		//flags.push("-ggdb3")

		return flags
	}

	property stringList additionalDriverFlags: {
		var flags = ["-pthread"]
		return flags
	}

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
				libs = libs.concat(["ssl", "crypto", "z", "fmt", "stdc++fs"])
			}

			if (qbs.toolchain.contains("mingw"))
			{
				libs.push("ws2_32")
				libs.push("crypt32")
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
