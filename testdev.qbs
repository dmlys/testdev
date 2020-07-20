import qbs
import qbs.Environment


Project
{
	property bool portable: false
	
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
		cpp.driverFlags: project.additionalDriverFlags
		cpp.cxxFlags: project.additionalCxxFlags

		cpp.includePaths: project.additionalIncludePaths
		cpp.systemIncludePaths: project.additionalSystemIncludePaths
		cpp.libraryPaths: project.additionalLibraryPaths

		cpp.driverLinkerFlags:
		{
			var flags = project.additionalDriverLinkerFlags || [];
			
			if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			{
				if (project.portable)
					flags.push("-Wl,-Bstatic")
				
				flags = flags.concat([
					//"-lboost_timer", "-lboost_filesystem",
					"-lboost_system", "-lboost_thread",
					//"-lboost_context", "-lboost_fiber",
					"-lxerces-c",
					"-lssl", "-lcrypto", "-lz", "-lfmt", "-lstdc++fs",
				]);
				
				if (project.portable && qbs.toolchain.contains("mingw"))
					flags = flags.concat([
						 "-lstdc++", "-lpthread", // for libpthread static linking mingw somehow needs explicit linking of libstdc++ and libpthread
						 "-lssp",                 // for mingw(gcc) stack protector, _FORTIFY_SOURCE stuff
					]);
				
				if (project.portable)
					flags.push("-Wl,-Bdynamic")
				
				if (qbs.toolchain.contains("mingw"))
				{
					flags.push("-lws2_32")
					flags.push("-lcrypt32")
				}
				
				if (project.portable)
					flags = flags.concat(["-static-libstdc++", "-static-libgcc"]);
				
				return flags
			}
		}

		files: [
			"main.cpp",
			//"future-fiber.*",
			//"unix-domain-socket.cpp",
			//"socket-rest-subscriber-main.cpp",
		]
	}
	
	AutotestRunner {}
}
