#include "../../DsscPpt/DsscPpt.hh"

#include <boost/shared_ptr.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <utility>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/PluginLoader.hh"


#define DEVICE_SERVER_ID "testDeviceSrvCpp"
#define TEST_DEVICE_ID   "testDsscPpt"
#define LOG_PRIORITY     "DEBUG"  // Can also be "FATAL", "INFO" or "ERROR"

#define DEV_CLI_TIMEOUT_SEC 2


/**
 * @brief Test fixture for the DsscDevices device class.
 */
class DsscPptFixture: public testing::Test {
protected:

    DsscPptFixture() = default;

    void SetUp( ) {
        m_eventLoopThread = std::thread(&karabo::net::EventLoop::work);

        // Load the library dynamically
        const karabo::util::Hash& pluginConfig = karabo::util::Hash("pluginDirectory", ".");
        karabo::util::PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate C++ Device Server.
        karabo::util::Hash config("serverId", DEVICE_SERVER_ID,
                                  "scanPlugins", true,
                                  "Logger.priority", LOG_PRIORITY);
        m_deviceSrv = karabo::core::DeviceServer::create("DeviceServer", config);
        m_deviceSrv->finalizeInternalInitialization();
        // Instantiate Device Client.
        m_deviceCli = boost::make_shared<karabo::core::DeviceClient>();
    }

    void TearDown( ) {
        m_deviceCli.reset();
        m_deviceSrv.reset();
        karabo::net::EventLoop::stop();
        m_eventLoopThread.join();
    }

    void instantiateTestDevice(const karabo::util::Hash& devConfig) {

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "DsscPpt",
                                     devConfig, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID << "':\n"
            << success.second;
    }

    void deinstantiateTestDevice() {
        ASSERT_NO_THROW(
            m_deviceCli->killDevice(TEST_DEVICE_ID, DEV_CLI_TIMEOUT_SEC))
        << "Failed to deinstantiate device '" << TEST_DEVICE_ID << "'";
    }

    std::thread m_eventLoopThread;

    karabo::core::DeviceServer::Pointer m_deviceSrv;
    karabo::core::DeviceClient::Pointer m_deviceCli;
};


TEST_F(DsscPptFixture, testDeviceInstantiation){
    using namespace karabo::util;
    // Make use of the default config files saved with the device sources.
    std::stringstream fullConfigFileName;
    const std::string filename = __FILE__;
    std::size_t parent_dir_limit = filename.find_last_of("/", filename.find_last_of("/", filename.find_last_of("/")-1)-1);
    fullConfigFileName << filename.substr(0, parent_dir_limit)  << "/ConfigFiles/F2Init.conf";

    const auto hash = Hash(
        "deviceId", TEST_DEVICE_ID,
        "quadrantId", "FENICE",
        "fullConfigFileName", fullConfigFileName.str());
    instantiateTestDevice(hash);

    State state(State::INIT);
    // test for schema injection
    do {
        state = m_deviceCli->get<State>(TEST_DEVICE_ID, "state");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (state == State::INIT);

    // test for autoconnect
    ASSERT_TRUE(state == State::OPENING);
    do {
        state = m_deviceCli->get<State>(TEST_DEVICE_ID, "state");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (state == State::OPENING);

    // test autoconnect failed
    ASSERT_TRUE(state == State::UNKNOWN);

    deinstantiateTestDevice();
}
