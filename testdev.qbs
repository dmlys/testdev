import qbs
import qbs.Environment
import "qbs-utils/imports/dmlys/BuildUtils" as BuildUtils

Project
{
	property bool portable: false
	
	qbsSearchPaths: ["qbs-utils", "qbs-extensions"]
	
	SubProject
	{
		filePath: "extlib/extlib.qbs"
		Properties {
			name: "extlib"
			with_zlib: true
			with_openssl: true
		}
	}

	SubProject
	{
		filePath: "netlib/netlib.qbs"
		Properties {
			name: "netlib"
		}
	}

	SubProject
	{
		filePath: "xercesc-utils/xercesc-utils.qbs"
		Properties {
			name: "xercesc-utils"
		}
	}

	SubProject
	{
		filePath: "wincrypt-utils/wincrypt-utils.qbs"
		Properties {
			name: "wincrypt-utils"
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
		Depends { name: "wincrypt-utils"; condition: qbs.targetOS.contains("windows") }
		Depends { name: "xercesc-utils" }
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
			
			return undefined
		}
		
		cpp.driverLinkerFlags:
		{
			var flags = project.additionalDriverLinkerFlags || [];
			
			if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			{
				//flags.push("-fcoroutines")
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
					flags.push("-lcryptui")
					flags.push("-liphlpapi")
				}
				
				if (project.portable)
					flags = flags.concat(["-static-libstdc++", "-static-libgcc"]);
				
				return flags
			}
		}

		files: [
			"main.cpp",
			"netstat/*",
			//"future-fiber.*",
			//"unix-domain-socket.cpp",
			//"socket-rest-subscriber-main.cpp",
		]
	}
	
	AutotestRunner {}
}
