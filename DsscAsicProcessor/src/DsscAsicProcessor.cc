/*
 * Author: <kirchgessner>
 *
 * Created on February, 2018, 01:04 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DsscAsicProcessor.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscAsicProcessor)

    void DsscAsicProcessor::expectedParameters(Schema& expected) {
        
        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::OFF, State::ACQUIRING, State::STOPPED)
                .commit();

        SLOT_ELEMENT(expected).key("accumulate")
                .displayedName("Run")
                .description("Accumulate data for several iterations")
                .allowedStates(State::OFF, State::STOPPED)
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Abort current acquisition")
                .allowedStates(State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected).key("resend")
                .displayedName("Resend")
                .description("Send again data to output if done, and data has been lost")
                .allowedStates(State::STOPPED)
                .commit();

        UINT32_ELEMENT(expected).key("minValidTrainId")
                .displayedName("Min Train ID")
                .description("Device processes data only with a train id higher than specified")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();


        UINT32_ELEMENT(expected).key("numIterations")
                .displayedName("Iterations")
                .description("Number of iterations to accumulate")
                .assignmentOptional().defaultValue(20).reconfigurable()
                .commit();


        UINT16_ELEMENT(expected).key("minSram")
                .displayedName("Min Sram")
                .description("Minimum SRAM address for mean value computation")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        UINT16_ELEMENT(expected).key("maxSram")
                .displayedName("Max Sram")
                .description("Maximum (included) SRAM address for mean value computation")
                .assignmentOptional().defaultValue(799).reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("iterationCnt")
                .displayedName("IterationCnt")
                .readOnly()
                .commit();

        BOOL_ELEMENT(expected).key("done")
                .displayedName("Done")
                .description("Signals if device has finished its work")
                .readOnly().initialValue(false)
                .commit();

        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .commit();


        Schema outSchema;

        NDARRAY_ELEMENT(outSchema).key("asicMeanData")
                .shape("64,64")
                .readOnly()
                .commit();

        NDARRAY_ELEMENT(outSchema).key("asicRMSData")
                .shape("64,64")
                .readOnly()
                .commit();

        NDARRAY_ELEMENT(outSchema).key("asicMinData")
                .shape("64,64")
                .readOnly()
                .commit();

        NDARRAY_ELEMENT(outSchema).key("asicMaxData")
                .shape("64,64")
                .readOnly()
                .commit();

        NDARRAY_ELEMENT(outSchema).key("trainIds")
                .shape("-1")
                .readOnly()
                .commit();

        UINT64_ELEMENT(outSchema).key("valueCnt")
                .readOnly()
                .commit();

        OUTPUT_CHANNEL(expected).key("meanPixelValuesOutput")
                .displayedName("Mean Values")
                .dataSchema(outSchema)
                .commit();
    }


    DsscAsicProcessor::DsscAsicProcessor(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_INITIAL_FUNCTION(initialization)

        KARABO_SLOT(resend)
        KARABO_SLOT(accumulate)
        KARABO_SLOT(stop)
    }


    DsscAsicProcessor::~DsscAsicProcessor() {
        stop();
    }


    void DsscAsicProcessor::initialization() {
        KARABO_ON_DATA("input", onData);
        m_iterationCnt = 0;
        m_pixelHistos.resize(s_NUMPX);
        updateState(State::OFF);
        resetCounters();
    }


    void DsscAsicProcessor::resend() {
        if (get<bool>("done")) {
            cout << get<string>("deviceId") << " : Resend mean data" << endl;

            writeChannel("meanPixelValuesOutput", m_lastSendHash);
        } else {
            cout << get<string>("deviceId") << " : Nothing to send" << endl;
        }
        signalEndOfStream("meanPixelValuesOutput");
    }


    void DsscAsicProcessor::accumulate() {
        resetCounters();
        m_run = true;
        updateState(State::ACQUIRING);
        cout << get<string>("deviceId") << ":Start Acquiring" << endl;
    }


    void DsscAsicProcessor::stop() {
        if (m_run) {
            KARABO_LOG_INFO << "User abort after " << m_iterationCnt << "/" << m_numIterations << " iterations";
        }
        m_run = false;
        updateState(State::STOPPED);
    }


    void DsscAsicProcessor::resetCounters() {
        m_trainIds.clear();
        m_iterationCnt = 0;
        m_numIterations = get<unsigned int>("numIterations");
        m_run = false;

        set<bool>("done", false);
        set<unsigned int>("iterationCnt", m_iterationCnt);

        m_minSram = get<unsigned short>("minSram");
        m_maxSram = get<unsigned short>("maxSram");

        clearHistos();
    }


    void DsscAsicProcessor::onData(const karabo::util::Hash& data,
                                   const karabo::xms::InputChannel::MetaData& meta) {
        if (!data.has("asicData")) {
            cout << "No asicData Found in input data " << endl;
            return;
        }

        if (m_run) {
            fillPixelHistos(data.get<util::NDArray>("asicData"));
            const auto currentTrainId = data.get<unsigned long long>("trainId");

            if (currentTrainId <= get<unsigned int>("minValidTrainId")) {
                cout << get<string>("deviceId") << ":Invalid Train received: " << currentTrainId << endl;
                return; // throw data away
            }

            m_trainIds.push_back(currentTrainId);

            if (m_iterationCnt >= m_numIterations) {
                cout << get<string>("deviceId") << " : All Trains acquired: " << m_iterationCnt << endl;

                m_lastSendHash = fillOutputSchema(data.get<int>("asicNum"));
                writeChannel("meanPixelValuesOutput", m_lastSendHash);
                signalEndOfStream("meanPixelValuesOutput");
                m_run = false;
                set<bool>("done", true);
                updateState(State::STOPPED);
            }
        }
    }


    void DsscAsicProcessor::fillPixelHistos(const karabo::util::NDArray& data) {
        const unsigned short* data_ptr = data.getData<unsigned short>();
        size_t data_size = data.size();

        size_t numSram = m_maxSram - m_minSram + 1;
        for (size_t px = 0; px < s_NUMPX; px++) {
            const unsigned short * pxPtr = data_ptr + 800 * px;
            m_pixelHistos[px].add(pxPtr, 800);
        }
        m_iterationCnt++;
        set<unsigned int>("iterationCnt", m_iterationCnt);
    }


    util::Hash DsscAsicProcessor::fillOutputSchema(int asicIdx) {
        static const Dims dims(s_NUMPX);
        static std::vector<double> meanValues(s_NUMPX);
        static std::vector<double> rmsValues(s_NUMPX);
        static std::vector<double> maxValues(s_NUMPX);
        static std::vector<double> minValues(s_NUMPX);

        int idx = 0;
        for (auto && histo : m_pixelHistos) {
            const auto stats = histo.getStats();
            const auto range = histo.getBinRange();
            meanValues[idx] = stats.mean;
            rmsValues[idx] = stats.rms;
            minValues[idx] = range.first;
            maxValues[idx] = range.second;
            idx++;
        }

        util::Hash outData;

        NDArray meanData(meanValues.data(), s_NUMPX, dims);
        outData.set("asicMeanData", meanData);

        NDArray rmsData(rmsValues.data(), s_NUMPX, dims);
        outData.set("asicRMSData", rmsData);

        NDArray minData(minValues.data(), s_NUMPX, dims);
        outData.set("asicMinData", minData);

        NDArray maxData(maxValues.data(), s_NUMPX, dims);
        outData.set("asicMaxData", maxData);

        NDArray trainIds(m_trainIds.data(), m_trainIds.size(), Dims(m_trainIds.size()));

        outData.set("trainIds", trainIds);
        outData.set("valueCnt", m_maxSram - m_minSram + 1);
        outData.set("dataType", "AsicMeanData");
        outData.set("asicNum", asicIdx);

        return outData;
    }


    void DsscAsicProcessor::clearHistos() {
        const int numHistos = m_pixelHistos.size();

#pragma omp parallel for      
        for (int i = 0; i < numHistos; i++) {
            m_pixelHistos[i].clear();
        }
    }


    void DsscAsicProcessor::preReconfigure(karabo::util::Hash& incomingReconfiguration) {

    }


    void DsscAsicProcessor::postReconfigure() {
    }
}
