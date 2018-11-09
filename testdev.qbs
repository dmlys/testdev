import qbs
import qbs.Environment


Project
{
	property pathList additionalIncludePaths: {
		var includes = []
		var envIncludes = Environment.getEnv("QBS_THIRDPARTY_INCLUDES")
		if (envIncludes)
		{
			envIncludes = envIncludes.split(qbs.pathListSeparator)
			includes = includes.uniqueConcat(envIncludes)
		}

		return includes
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

		if (qbs.toolchain.contains("msvc"))
		{
			defs.push("_WIN32_WINNT=0x0600")
			defs = defs.uniqueConcat(["_SCL_SECURE_NO_WARNINGS"])
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

	CppApplication
	{
		Depends { name: "netlib" }
		Depends { name: "extlib" }

		//qbs.debugInformation: true
		cpp.cxxLanguageVersion : "c++17"
		cpp.defines: project.additionalDefines
		cpp.cxxFlags: project.additionalCxxFlags
		cpp.driverFlags: ["-pthread"]

		cpp.includePaths: project.additionalIncludePaths
		cpp.libraryPaths: project.additionalLibraryPaths

		cpp.dynamicLibraries: {
			if (!qbs.targetOS.contains("windows"))
				return ["z", "ssl", "fmt", "crypto", "boost_system", "boost_context", "boost_fiber", "boost_timer"]

			if (qbs.buildVariant == "release")
				return ["libfmt-mt", "openssl-crypto-mt", "openssl-ssl-mt", "zlib-mt"]
			else
				return ["libfmt-mt-gd", "openssl-crypto-mt-gd", "openssl-ssl-mt-gd", "zlib-mt-gd"]
		}

		files: [
			"main.cpp",
			"future-fiber.*",
		]
	}
}
