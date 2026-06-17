/* Example of using the DSSC Library in a standalone script, but in the Karabo environment


The following file gets created in the DSSC Library entry point. Delete it to run again:
rm ./DsscModuleInfo.ini

Compile this file as follows:

g++ libexample.cpp  \
-I $KARABO/extern/include/dssc/utils \
-L $KARABO/extern/lib/dssc \
-l Utils \
-Wl,-rpath,$KARABO/extern/lib \
-o example

Run it by specifying the DSSC Library path:

LD_LIBRARY_PATH=/scratch/xctrl/karabo3/karabo/extern/lib/dssc  ./example

 */

#include <iostream>
#include "DsscModuleInfo.h"

int main(void){
    std::cout << "Entries: " << utils::DsscModuleInfo::getQuadrantIdList() << std::endl;
    return 0;
}
