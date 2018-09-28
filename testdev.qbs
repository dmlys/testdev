import qbs
import qbs.Environment


Project
{
    //property pathList additionalIncludePaths: []
	//property pathList additionalLibraryPaths: []
    //property stringList additionalDefines: []
    property stringList additionalDriverFlags: ["-pthread"]
	
    property stringList additionalDefines: {
        var defs =[]

        // https://bugreports.qt.io/browse/QTCREATORBUG-20884
        // https://bugreports.qt.io/browse/QTCREATORBUG-19348
        // clang code model at least with creator 4.7.0 has some strange behaviour with __cplusplus define.
        // it is always defined as 201402L unless in project it explicitly defined via cpp.defines otherwise
        //defs.push("__cplusplus=201703L")

        if (qbs.toolchain.contains("msvc"))
            defs = defs.uniqueConcat(["_SCL_SECURE_NO_WARNINGS"])

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
        
        cpp.cxxLanguageVersion : "c++17"
        cpp.defines: project.additionalDefines
        cpp.cxxFlags: project.additionalCxxFlags

        cpp.includePaths: {
			var includes = []
			if (project.additionalIncludePaths)
				includes = includes.uniqueConcat(project.additionalIncludePaths)
			
			var envIncludes = Environment.getEnv("QBS_THIRDPARTY_INCLUDES")
			if (envIncludes)
			{
				envIncludes = envIncludes.split(qbs.pathListSeparator)
				includes = includes.uniqueConcat(envIncludes)
			}
			
			return includes
		}
		
		cpp.libraryPaths: {
			var paths = []
			if (project.additionalLibraryPaths)
				paths = paths.uniqueConcat(project.additionalLibraryPaths)
			
			var envPaths = Environment.getEnv("QBS_THIRDPARTY_LIBRARY_PATHS")
			if (envPaths)
			{
				envPaths = envPaths.split(qbs.pathListSeparator)
				paths = paths.uniqueConcat(envPaths)
			}
			
			return paths
		}
		
		cpp.dynamicLibraries: {
			if (!qbs.targetOS.contains("windows"))
				return ["z", "ssl", "crypto", "boost_system"]
			
			if (qbs.buildVariant == "release")
				return ["libfmt-mt", "openssl-crypto-mt", "openssl-ssl-mt", "zlib-mt"]
			else
                return ["libfmt-mt-gd", "openssl-crypto-mt-gd", "openssl-ssl-mt-gd", "zlib-mt-gd"]
		}

        files: [
            "main.cpp"
        ]
    }
    
}
