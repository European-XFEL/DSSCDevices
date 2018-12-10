/*
 * $Id: DsscPpt_App.cc 13002 2014-03-04 15:07:02Z esenov $
 *
 * Author: <serguei.essenov@xfel.eu>
 * 
 * Created on March, 2014, 04:06 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/karabo.hpp>

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;
using namespace karabo::core;


int main(int argc, char** argv) {

    // Activate the Logger (the threshold priority can be changed to INFO, WARN or ERROR)
    Logger::configure(Hash("priority", "DEBUG"));

    // Create an instance of the device
    BaseDevice::Pointer d = BaseDevice::create("DsscPpt", Hash("deviceId","Test_DsscPpt_0",
                                                               "connection.Jms.hostname","localhost",
                                                               "connection.Jms.port",7676) );

    // Run the device
    d->run(); // Will block

    return (EXIT_SUCCESS);
}
