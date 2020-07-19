import qbs
import qbs.Environment

Module
{
	Depends { name: "cpp" }

	//qbs.debugInformation: true

	cpp.driverFlags:
	{
		if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			return ["-pthread"];
		//if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
		//	return ["-pthread", "-march=native"]
	}

	cpp.driverLinkerFlags:
	{
		if (qbs.toolchain.contains("mingw"))
			// this will add linking for libssp under mingw, through not static(maybe depends on -Bstatic state)
			return ["-fstack-protector"];
	}

	cpp.defines:
	{
		var defs =[]

		var envDefines = Environment.getEnv("QBS_EXTRA_DEFINES")
		console.log(envDefines)
		if (envDefines)
		{
			var splitter = new RegExp(" " + "|" + qbs.pathListSeparator)
			envDefines = envDefines.split(splitter).filter(Boolean)
			defs = defs.uniqueConcat(envDefines)
		}

		if (qbs.toolchain.contains("msvc"))
			defs = defs.uniqueConcat(["_SCL_SECURE_NO_WARNINGS", "UNICODE", "_UNICODE"])

		if (qbs.toolchain.contains("mingw") || qbs.toolchain.contains("msvc"))
			defs = defs.uniqueConcat(["_WIN32_WINNT=0x0600"])

		if (qbs.toolchain.contains("gcc") && cpp.optimization == 'fast')
			defs.push("_FORTIFY_SOURCE=2")

		return defs
	}

	cpp.cxxFlags:
	{
		var flags = []

		var envFlags = Environment.getEnv("QBS_EXTRA_FLAGS")
		if (envFlags)
		{
			envFlags = envFlags.split(" ").filter(Boolean)
			flags = flags.concat(envFlags);
		}

		if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
		{
			//flags.push("-Wsuggest-override")
			flags.push("-Wno-extra");
			flags.push("-Wno-unused-parameter")
			flags.push("-Wno-unused-function")
			//flags.push("-Wno-unused-local-typedefs")
			//flags.push("-Wno-sign-compare")
			flags.push("-Wno-implicit-fallthrough")
			flags.push("-Wno-deprecated-declarations")
		}

		if (qbs.toolchain.contains("msvc"))
		{

		}

		return flags
	}

	cpp.includePaths:
	{
		var includes = []
		var envIncludes = Environment.getEnv("QBS_EXTRA_INCLUDES")
		if (envIncludes)
		{
			envIncludes = envIncludes.split(qbs.pathListSeparator).filter(Boolean)
			includes = includes.uniqueConcat(envIncludes)
		}

		return includes;
	}

	cpp.libraryPaths:
	{
		var libPaths = []
		var envLibPaths = Environment.getEnv("QBS_EXTRA_LIBPATH")
		if (envLibPaths)
		{
			envLibPaths = envLibPaths.split(qbs.pathListSeparator).filter(Boolean)
			libPaths = libPaths.uniqueConcat(envLibPaths)
		}

		return libPaths
	}
}
