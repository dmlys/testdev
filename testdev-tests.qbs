import qbs
import qbs.Environment

Project
{	
	references: [
		"extlib/extlib-tests.qbs",
		"netlib/netlib-tests.qbs",
		"wincrypt-utils/wincrypt-utils-tests.qbs",
	]
}
