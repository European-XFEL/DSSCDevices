"""
Author: kirchgessner
Creation date: April, 2017, 01:41 PM
Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
"""

from karabo.middlelayer import (
    AccessMode, Device, DeviceNode, Float, getDevice,
    Bool, Integer, Slot, State, String, Unit)

import subprocess
import re
import random
import time
import h5py
import os
import numpy
import datetime


def moveMeasurementInfo(dirName,outputDir,deleteSource = False) :
    outputDir = outputDir.replace(" ","\ ")
    dirName = dirName.replace(" ","\ ")
    print("Copy " + dirName + " to " + outputDir + " on max-exfl")
    callString = "cp ";
    if deleteSource :
        callString = "mv ";
    callString += dirName + "/Measurement* " + outputDir + "; "
    os.system(callString)
    print("Measurement info moved")


def transferDirectory(dirName,distDir, removeOrig = False, toMAXXFEL = False) :
    dirName = dirName.replace(" ","\ ")
    distDir = distDir.replace(" ","\ ")

    if toMAXXFEL :
      print("Copy " + dirName + " to " + distDir + " on max-exfl")
    else :
      #print("Copy " + dirName + " to " + distDir + " on dsscbigdata01_1")
      print("Copy " + dirName + " to " + distDir + " on fecdsscdata01")

    callString  = "rsync -avz"

    if removeOrig :
      callString += " --remove-source-files "

    if toMAXXFEL :
      callString += " -e \"ssh -i /home/dssc/.ssh/gweiden-dsscmtca02-rsa-nokey\" "
      callString += dirName
      callString += " gweiden@max-exfl.desy.de:/gpfs/exfel/data/scratch/gweiden/Measurements/"
    else :
      callString += " -e \"ssh -i /home/dssc/.ssh/gweiden-dsscmtca02-rsa-nokey\" "
      callString += dirName
      #callString += " dssc@dsscbigdata01_1:/dataFast1/PetraIII_2018/"
      callString += " dssc@fecdsscdata01:/dataFastRemovable/PetraIII_2018/"

    callString += distDir + " && echo \"data transfer done\" & "

    print(callString)
    os.system(callString)

    print("Measurement "+ dirName +" transferred")

# scp -i ~/.ssh/gweiden-fecdsscdata01-rsa-nokey -r StartTest gweiden@max-exfl.desy.de:/gpfs/exfel/data/scratch/gweiden/Measurements/ && rm -rf StartTest &
# rsync -avz --remove-source-files -e "ssh -i /home/dssc/.ssh/gweiden-fecdsscdata01-rsa-nokey" StartTest gweiden@max-exfl.desy.de:/gpfs/exfel/data/scratch/gweiden/Measurements/ && rm -rf StartTest/ && echo "data transfer done" &

def compressDirectory(dirName,outputDir,deleteSource = False) :
    dirName = dirName.replace(" ","\ ")
    outputDir = outputDir.replace(" ","\ ")
    baseDir = os.path.dirname(dirName)
    compDir = os.path.basename(dirName)
    outputFile = outputDir + "/" + compDir + ".tar"
    print("Compressing Measurement: " + dirName)
    callString = "function compress() { "\
               "   cd " + baseDir + " ; "\
               "   tar -czf " + outputFile + " " + compDir + "; "\
               "   echo ++++Compression of " + dirName + " done ;"
    if deleteSource :
        callString += " rm -rf " + dirName + " ; "

    callString += " } ; compress &"
    print (callString)
    os.system(callString)
    print("Compressing Started")


def getSweepVector( inputStr , randomize = False) :

    doubleEx   = "\d+[\.\d+]*"
    range1Ex    = "(^"+doubleEx+"\-"+doubleEx+"(;|:)("+doubleEx+")$)"
    range2Ex    = "(^"+doubleEx+"\-"+doubleEx+"$)"
    rangeEx    = range1Ex + "|" + range2Ex
    stepSizeEx = "(^.*;.*$)"
    numStepsEx = "(^.*:\d+$)"
    list1 = "((.*)("+doubleEx+";){2,}(.*))"
    list2 = "(^"+doubleEx+";"+doubleEx+"$)"
    list3 = "(^"+doubleEx+"$)"
    listEx = list1 + "|" + list2 + "|" + list3

    print(str(inputStr))

    settingsStr = re.findall(doubleEx,inputStr)
    settings = []
    if re.match(rangeEx,inputStr) :
        print("Range found")

        settingStep  = 1
        startSetting = int(settingsStr[0])
        stopSetting  = int(settingsStr[1])

        if len(settingsStr) == 3 :

            if re.match(numStepsEx,inputStr) :
     #           print("Num Steps found")
                totalSteps = int(settingsStr[2])

                if totalSteps <= 1 :
                    stopSetting = startSetting
                else :
                    settingStep = (stopSetting-startSetting) / (totalSteps-1)

            elif re.match(stepSizeEx,inputStr) :
     #           print("Stepsize found")
                settingStep = int(settingsStr[2])
            else :
                print ("No format found")

            if settingStep == 0 :
                print("warning setting step must be different from 0. Otherwise measurement will take forever..." )
                return settings

     #   print("startSetting is " + str(startSetting))
     #   print("stopSetting is " + str(stopSetting))
     #   print("settingStep is " + str(settingStep))


        while True :
            settings.append(startSetting)
            if startSetting >= stopSetting :
                break
            startSetting += settingStep

    elif re.match(listEx,inputStr) :
     #   print("found settings list")

        settings = [int(value) for value in settingsStr]
    else :
        print("Error getting sweep settings from " + str )

    if randomize :
      random.shuffle(settings)

    return settings


class MeasurementConfig:
  configFileName = ''
  numIterations = 0
  numPreBurstVetos = 0
  ladderMode = 0
  activeASIC = 0
  columnSelection = 'none'


def writeMeasurementConfiguration(measConfig,
                                  settings1,name1,enSecondParam = False,settings2 = [1],name2 = "") :
        measName = ""
        numSettings = 1
        numMeasurements = len(settings1)
        if enSecondParam :
            numMeasurements = len(settings1) * len(settings2)
            numSettings = 2

        #Measurement Name
        measName = str(name1)
        if measName != "BurstMeasurement" :
            measName += " Sweep"

        # FileVersion 1 : Version Information and ColSelection String added

        #open file
        h5File = h5py.File(str(measConfig.configFileName), "w")

        strType = h5py.special_dtype(vlen=str)
        dset = h5File.create_dataset("MeasurementName", (1,), dtype=strType)
        dset[...] = measName
        #Measurement Information
        dset = h5File.create_dataset("fileVersion", (1,), dtype='i')
        dset[...] = 1

        dset = h5File.create_dataset("columnSelection", (1,), dtype=strType)
        dset[...] = measConfig.columnSelection

        dset = h5File.create_dataset("numIterations", (1,), dtype='i')
        dset[...] = measConfig.numIterations

        dset = h5File.create_dataset("numMeasurements", (1,), dtype='i')
        dset[...] = numMeasurements

        dset = h5File.create_dataset("numPreBurstVetos", (1,), dtype='i')
        dset[...] = measConfig.numPreBurstVetos

        dset = h5File.create_dataset("ladderMode", (1,), dtype='i')
        dset[...] = measConfig.ladderMode

        availableASICs = [measConfig.activeASIC]
        if measConfig.ladderMode != 0 :
          availableASICs = range(16)

        dset = h5File.create_dataset("availableASICs", (len(availableASICs),), dtype='i')
        dset[...] = availableASICs

        dset = h5File.create_dataset("numMeasurementParams", (1,), dtype='i')
        dset[...] = numSettings

        timeStamp = str(datetime.datetime.utcnow())
        dset = h5File.create_dataset("timeStamp", (1,), dtype=strType)
        dset[...] = timeStamp

        if enSecondParam :
            dset = h5File.create_dataset("measParamNames", (2,), dtype=strType)
            dset[...] = [str(name1),str(name2)]
        else :
            dset = h5File.create_dataset("measParamNames", (1,), dtype=strType)
            dset[...] = [str(name1)]

        mSetGrp = h5File.create_group("MeasurementSettings")
        dset = mSetGrp.create_dataset("settings1", (len(settings1),), dtype='i')
        dset[...] = settings1
        dset.attrs["numSettings"] = len(settings1)
        dset.attrs["settingName"] = str(name1)

        if enSecondParam :
            dset = mSetGrp.create_dataset("settings2", (len(settings2),), dtype='i')
            dset[...] = settings2
            dset.attrs["numSettings"] = len(settings2)
            dset.attrs["settingName"] = str(name2)

        #add all directory names for later analysis
        measurementDirectories = []
        for set2 in settings2 :
            measDir = ""
            if enSecondParam :
                measDir = str(name2) + str(set2) + "_"


            for set1 in settings1 :
                measurementDirectories.append(measDir + str(name1) + str(set1))

        dset = h5File.create_dataset("Measurements", (numMeasurements,), dtype=strType)
        dset[...] = measurementDirectories

        dset.attrs["timeStamp"] = str(timeStamp)
        dset.attrs["name"]      = str(measName)
        dset.attrs["numMeasurements"] = numMeasurements
        dset.attrs["numIterations"]   = measConfig.numIterations

        print("HDF5 Measurement Config Written")
        #file is written at end
        h5File.close()



class DsscControl_Test(Device):
    __version__ = "2.1.13"

    status  = String(displayedName="Status",
                     accessMode=AccessMode.READONLY)

    ladderMode = Bool(displayedName="Ladder Mode",
                      defaultValue=True,
                      allowedStates=[State.STOPPED])

    abortMeasurement = Bool(displayedName="Abort Measurement",
                            defaultValue=False,
                            accessMode=AccessMode.RECONFIGURABLE,
                            allowedStates=[State.ACTIVE,State.ACQUIRING])
    baseDir = String()
    quarterDir = String()
    initConfigFile = String()
    measurementOutDir = String()

    changeSweepParamOnly = Bool(displayedName="Change Sweep Param Only",
                                defaultValue=True)

    selPixels = String(displayedName="Sel Pixels",
                       defaultValue="all")

    storeAllConfig = Bool(displayedName="Store All Configs",
                     defaultValue=True)

    enSending = Bool(displayedName="Enable Data Transfer",
                      defaultValue=False)

    removeOrig = Bool(displayedName="Remove Original Files",
                      defaultValue=False)

    compressDir   = String(displayedName="Compression OutputDir")

    xfelMode = Bool(displayedName="Run XFEL Mode",
                   defaultValue=False,
                   accessMode=AccessMode.RECONFIGURABLE)

    numIterations = Float(displayedName="Number of Iterations",
                            defaultValue=20,
                            accessMode=AccessMode.RECONFIGURABLE)

    activeModule = Float(displayedName="Active Module",
                           defaultValue=1,
                           accessMode=AccessMode.RECONFIGURABLE)

    sweepSecondPixelParameter = Bool(displayedName="En Second Parameter")

    firstPixelParameterName   = String(displayedName="First PixelParam Name",
                                      defaultValue="RmpFineTrm",
                                      options=["RmpFineTrm",
                                               "RmpCurrDouble",
                                               "RmpEnFineDelayCntrl",
                                               "RmpDelayCntrl",
                                               "CompCoarse",
                                               "FCF_EnCap",
                                               "D0_EnMimCap",
                                               "D0_BypComprResistor"])

    firstPixelParameterRange  = String(displayedName="First PixelParam Range")

    secondPixelParameterName  = String(displayedName="Second PixelParam Name",
                                      defaultValue="RmpFineTrm",
                                      options=["RmpFineTrm",
                                               "RmpCurrDouble",
                                               "RmpEnFineDelayCntrl",
                                               "RmpDelayCntrl",
                                               "CompCoarse",
                                               "FCF_EnCap",
                                               "D0_EnMimCap",
                                               "D0_BypComprResistor"])
    secondPixelParameterRange = String(displayedName="Second PixelParam Range")


    injectionSettingRange = String(displayedName="Injection Setting Range")

    burstParameterName  = String(displayedName="BurstParameter Name",
                                 defaultValue="start_wait_time",
                                 options=["start_wait_time",
                                          "start_wait_offs",
                                          "gdps_on_time",
                                          "clr_on_time",
                                          "clr_cycle",
                                          "iprog_clr_duty",
                                          "iprog_clr_offset",
                                          "SW_PWR_ON",
                                          "fet_on_time"])
    burstParameterRange = String(displayedName="BurstParameter Range")


    sequencerParameterRange = String(displayedName="SequencerParameter Range")

    receiver_device = DeviceNode(displayedName="Receiver Device",
                                 description="Select receiver device to control",
                                 properties=["currentTrainId",
                                             "numTrainsToStore",
                                             "numStoredTrains",
                                             "acquiring",
                                             "outputDir",
                                             "removeOrig",
                                             "transferDirectory",
                                             "transferDistantDirectory",
                                             "enHDF5Compression",
                                             "asicToShow"],
                                 commands=["start","stop","close","acquireTrains",
                                          "flushTrainStorage","startDataTransfer"])


    ppt_device = DeviceNode(displayedName="PPTDevice",
                            description="Select PPT device to control")
#                                        "selModule","selModuleSet",
#                                        "selSignal","fullConfigFileName","selValue",
#                                        "selRegName"],
#                            commands=[ "open","initSystem","runXFEL","runStandAlone",
#                                       "startAcquisition","stopAcquisition","stopStandalone"])
#                                       "preProgSelReg","progSelReg","readSelReg",
#                                       "storeFullConfigFile","storeFullConfigHDF5"])


    def onInitialization(self):
        self.state = State.STOPPED
        self.status = "Ready"
        self.abortMeasurement = False
        self.selPixels = "all"


    def setBurstParameter(self, paramName, value) :
        self.ppt_device.burstParameterName = paramName
        self.ppt_device.burstParameterValue = value
        self.ppt_device.setBurstParameter()

        if paramName == "start_wait_offs" :
          self.ppt_device.updateStartWaitOffset()
        else :
          self.ppt_device.updateSequenceCounters()

    def setSequencerParameter(self, paramName, value) :
        self.ppt_device.sequencerParameterName = paramName
        self.ppt_device.sequencerParameterValue = value
        self.ppt_device.setSequencerParameter()
        self.ppt_device.programSequencer()


    def setEPCRegister(self,moduleSet,signalName,value,program):
        self.ppt_device.selRegName   = "epc"
        self.ppt_device.selModuleSet = moduleSet
        self.ppt_device.selSignal    = signalName
        self.ppt_device.selValue     = value

        if program == True :
            self.ppt_device.progSelReg()
        else :
            self.ppt_device.preProgSelReg()


    def setIOBRegister(self,moduleSet,module,signalName,value,program):
        self.ppt_device.selRegName = "iob"
        self.ppt_device.selModule  = module
        self.ppt_device.selModuleSet = moduleSet
        self.ppt_device.selSignal = signalName
        self.ppt_device.selValue = value

        if program == True :
            self.ppt_device.progSelReg()
        else :
            self.ppt_device.preProgSelReg()


    def setGlobalJTAGRegister(self,module,asicsStr,signalName,value,program) :
        self.ppt_device.selRegName   = "jtag"
        self.ppt_device.selModule    = module
        self.ppt_device.selPixels    = asicsStr
        self.ppt_device.selSignal    = signalName
        self.ppt_device.selValue     = value

        if program == True :
            self.ppt_device.progSelReg()
        else :
            self.ppt_device.preProgSelReg()

    def setInjectionValue(self,setting) :
        self.ppt_device.injectionValue = setting
        self.ppt_device.setInjectionValue()
        print(self.ppt_device.state)


    def setPixelRegister(self,module,pixelStr,signalName,value,program) :
        self.ppt_device.selRegName   = "pixel"
        self.ppt_device.selModule    = module
        self.ppt_device.selPixels    = pixelStr
        self.ppt_device.selPixelSignal = signalName
        self.ppt_device.selValue     = value

        if program == True :
            self.ppt_device.progSelReg()
        else :
            self.ppt_device.preProgSelReg()


    def updateMeasurementDir(self,enSecond,set1,name1,set2 = 1, name2 = "") :
        self.measurementOutDir = self.baseDir + "/"

        if enSecond :
            self.measurementOutDir += name2 + str(set2) + "_"

        if name1 == "" :
            print("Name 1 must not be empty set to default value")
            name1 = "MeasurementParam"

        self.measurementOutDir += name1 + str(set1)

        if os.path.isdir(self.measurementOutDir) == False :
            os.makedirs(self.measurementOutDir)


    def storeConfiguration(self) :
        print("Store Configuration")
        self.ppt_device.fullConfigFileName = self.measurementOutDir + "/MeasurementConfig"
        self.ppt_device.storeFullConfigFile()
        self.ppt_device.storeFullConfigHDF5()

    def acquireData(self) :
        print("Acquire Data")
        self.state = State.ACQUIRING


        if not self.measurementOutDir :
            print("Error Measurement Dir is not set")
            return

        self.receiver_device.outputDir = self.measurementOutDir
        self.receiver_device.flushTrainStorage()
        time.sleep(0.1)
        self.receiver_device.acquireTrains()

        while self.receiver_device.acquiring == True or self.receiver_device.state == State.ACTIVE :
            print("Trains stored: " + str(self.receiver_device.numStoredTrains) + "/" + str(self.numIterations))
            time.sleep(0.3)

        print("Data Acquisition Done")
        self.state = State.ACTIVE


    def transferDirectory(self,srcDir,distDir,removeSource = False) :
        self.receiver_device.removeOrig = removeSource
        self.receiver_device.transferDirectory = srcDir
        self.receiver_device.transferDistantDirectory = distDir

        self.receiver_device.startDataTransfer()

        transferDirectory(srcDir,distDir, removeSource,False)


    def transferConfigDirectory(self) :
        distDir = os.path.basename(self.baseDir)
        transferDirectory(self.baseDir,"",False,self.enSending)


    def measureData(self,enSecond,set1,name1,set2,name2) :
        self.updateMeasurementDir(enSecond,set1,name1,set2,name2)

        if self.storeAllConfig :
            self.storeConfiguration()

        self.acquireData()

        if self.enSending :
            distDir = os.path.basename(self.baseDir)
            self.transferDirectory(self.measurementOutDir,distDir, self.removeOrig)



    def initMeasurement(self) :
        self.status = "Init Measurement"
        print(self.status)
        self.state = State.ACTIVE
        self.abortMeasurement = False

        self.baseDir = str(self.receiver_device.outputDir)


        self.initConfigFile = self.ppt_device.fullConfigFileName
        self.measurementOutDir = ""

        self.receiver_device.numTrainsToStore = self.numIterations

        self.startPPT()

        if self.ladderMode != self.receiver_device.ladderMode :
            self.receiver_device.ladderMode = self.ladderMode
        else :
            self.receiver_device.start()

        if self.receiver_device.state == State.ACQUIRING :
            self.receiver_device.stop()

        self.receiver_device.enableLadderPreview = False
        self.receiver_device.enableDAQOutput     = False
        self.receiver_device.enableASICPreview   = False
        self.receiver_device.enablePixelPreview  = False


    def finishMeasurement(self) :
        self.state = State.STOPPED
        if self.abortMeasurement :
            self.abortMeasurement = False
            self.status = "Measurement Aborted"
            print("Last directory: " + str(self.receiver_device.outputDir))
        else :
            self.status = "Measurement Finished"

        self.abortMeasurement = False

        self.receiver_device.outputDir = self.baseDir
        #self.ppt_device.fullConfigFileName = self.initConfigFile

        print(self.status)



    def updateStatus(self,enSecond,set1,name1,max1,set2,name2,max2) :
        if enSecond :
            self.status ="Measuring: " + str(name2) + " " + str(set2) + "/" + str(max2)
        else :
            self.status ="Measuring: "
        self.status+= str(name1) + " " + str(set1) + "/" + str(max1)
        print(self.status)


    def checkSecondPixelParameter(self) :
        if not self.secondPixelParameterRange :
            print("Second Pixel Parameter Range not set")
            return False

        if not self.secondPixelParameterName :
            print("Second Pixel Parameter Name not set")
            return False

        return True

    def checkFirstPixelParameter(self) :
        if not self.firstPixelParameterRange :
            print("First Pixel Parameter Range not set")
            return False

        if not self.firstPixelParameterName :
            print("First Pixel Parameter Name not set")
            return False

        return True

    def checkInjectionSettingRangeParameter(self) :
        if not self.injectionSettingRange :
            print("Injection Setting Range not set")
            return False


        return True

    def checkBurstParameter(self) :
        if not self.burstParameterRange :
            print("Burst Parameter Range not set")
            return False

        if not self.burstParameterName :
            print("Burst Parameter Name not set")
            return False

        return True


    def checkSequencerParameter(self) :
        if not self.sequencerParameterRange :
            print("Sequencer Parameter Range not set")
            return False

        if not self.ppt_device.sequencerParameterName :
            print("Sequencer Parameter Name not set")
            return False

        if self.ppt_device.sequencerParameterName == "cycle_length" :
            print("ERROR: Can not Sweep Cycle Length")
            return False

        return True


    def getMeasurementConfig(self, colsActive = False) :
      config = MeasurementConfig()
      config.configFileName = self.measurementOutDir + "/MeasurementInfo.h5"
      config.numIterations = int(self.numIterations)
      config.numPreBurstVetos = int(self.ppt_device.numPreBurstVetos)
      config.activeASIC = int(self.receiver_device.asicToShow)

      if self.ladderMode :
        config.ladderMode = 1
      else :
        config.ladderMode = 0

      if colsActive :
        skip = ""
        if self.ppt_device.colSelectMode == "SKIP" :
          skip = "skip"

        cols = self.ppt_device.columnSelect

        if self.ppt_device.numParallelCols == 0 :
          numRuns = "";
        else :
          numRuns = ":" +str(64/self.ppt_device.numParallelCols);

        config.columnSelection = "col" + skip + cols + numRuns + "#" + str(quarter)
      # endif

      return config

    #StartPPT connect,initSytem(incl program IOB FPGA) enable XFEL/Standalone+Data
    @Slot(displayedName="Start PPT",
          description="Connect and Initialize PPT",
          allowedStates=[State.STOPPED])
    def startPPT(self) :
        self.state = State.ACTIVE
        self.status = "Starting PPT..."
        print(self.status)
        self.status = self.ppt_device.state

        if(self.ppt_device.state == State.UNKNOWN) :
            print("open ppt")
            self.ppt_device.open()

        if(self.ppt_device.state == State.OFF) :
            self.ppt_device.xfelMode = self.xfelMode
            print("init ppt")
            self.ppt_device.numFramesToSendOut = 800
            self.ppt_device.initSystem()

        if(self.ppt_device.state == State.ON) :
            self.ppt_device.numFramesToSendOut = 800
            print("activate ppt")
            if self.xfelMode == True :
                self.ppt_device.runXFEL()
            else :
                self.ppt_device.runStandAlone()

        if(self.ppt_device.state == State.STARTED) :
            self.ppt_device.startAcquisition()

        if self.ppt_device.state == State.ACQUIRING :
            self.status = "DSSC_PPT: Started - ppt now sending data"
            self.state = State.STOPPED
        else :
            self.status = "DSSC_PPT: Could not be started - check device"
            self.state = State.ERROR

        print(self.status)


    #StartDataReceicer connect UDP and activate
    @Slot(displayedName="Start DataReceiver",
          description="Connect to UDP and start acquisition",
          allowedStates=[State.STOPPED])
    def startDataReceiver(self) :
        self.state = State.ACTIVE
        self.status = "Starting DataReceiver..."

        self.receiver_device.start()

        if self.receiver_device.state == State.ACQUIRING :
            self.status = "DsscDataReceiver: Sucessfully Startted - Now Acquiring"
        else :
            self.status = "DsscDataReceiver: Could not start receiver"

        self.state = State.STOPPED


    #StopPPT
    @Slot(displayedName="Stop PPT",
        description="Stop continuous mode to reinitialize PPT",
        allowedStates=[State.STOPPED])
    def stopPPT(self) :
        if self.ppt_device.state == State.ACQUIRING :
            self.ppt_device.stopAcquisition()

        if self.ppt_device.state == State.STARTED :
            self.ppt_device.stopStandalone()

    #ABORT
    @Slot(displayedName="ABORT",
        description="ABORT current measurement",
        allowedStates=[State.ACQUIRING,State.ACTIVE,State.CHANGING])
    def abortMeasurementSlot(self) :
        self.abortMeasurement = True

    #RESET
    @Slot(displayedName="Reset",
        description="Reset control device",
        allowedStates=[State.ACQUIRING,State.ACTIVE,State.CHANGING])
    def resetDeviceSlot(self) :
        self.state = State.STOPPED


    @Slot(displayedName="Run PixelParameterSweep",
          description="Start Pixel parameter sweep of one or two pixel parameters",
          allowedStates=[State.STOPPED])
    def runPixelParameterSweep(self) :
        print("Run Pixel Parameter Sweep")

        if not self.receiver_device.outputDir :
            self.status = "Output dir not specified"
            self.state  = State.ERROR
            return

        self.initMeasurement()

        enSecond = self.sweepSecondPixelParameter

        name2 = "none"
        settings2 = [1]
        if enSecond :
            if not self.checkSecondPixelParameter() :
                return

            name2 = self.secondPixelParameterName
            settings2 = getSweepVector(self.secondPixelParameterRange)
            print("Measure " + name2 + " Settings:" + str(settings2))


        if not self.checkFirstPixelParameter() :
            return

        name1 = self.firstPixelParameterName
        settings1 = getSweepVector(self.firstPixelParameterRange)

        self.measurementOutDir = self.baseDir
        self.storeConfiguration()
        writeMeasurementConfiguration(self.getMeasurementConfig(),
                                      settings1,name1,enSecond,settings2,name2)

        self.transferConfigDirectory()

        print("Measure " + name1 + " Settings:" + str(settings1))

        self.status = "Running Measurement ..."
        print(self.status)

        for set2 in settings2 :
            if enSecond :
                self.setPixelRegister(self.activeModule,self.selPixels,name2,set2,False)

            for set1 in settings1 :
                self.updateStatus(enSecond,set1,name1,settings1[-1],set2,name2,settings2[-1])
                self.setPixelRegister(self.activeModule,self.selPixels,name1,set1,True)
                self.measureData(enSecond,set1,name1,set2,name2)
                if self.abortMeasurement :
                    break
            if self.abortMeasurement :
                break

        self.finishMeasurement()


    @Slot(displayedName="Run Injection Sweep",
          description="Start Injection sweep if second a pixel parameter can be swept above",
          allowedStates=[State.STOPPED])
    def runInjectionSweep(self) :
        print("Run Injection Sweep")

        if not self.receiver_device.outputDir :
            self.status = "Output dir not specified"
            self.state  = State.ERROR
            return

        enSecond = self.sweepSecondPixelParameter

        if not self.checkInjectionSettingRangeParameter() :
            return

        name1 = self.ppt_device.injectionMode
        settings1 = getSweepVector(self.injectionSettingRange)

        name2 = "none"
        settings2 = [1]

        if enSecond :
            if not self.checkSecondPixelParameter() :
                return

            name2 = self.secondPixelParameterName
            settings2 = getSweepVector(self.secondPixelParameterRange)
            print("Measure " + name2 + " Settings:" + str(settings2))

        modeChange = 0
        if self.changeSweepParamOnly == False :
            self.ppt_device.setInjectionMode()
            if name1 == "CHARGE_BUSINJ" :
                modeChange = 1
            elif name1 == "ADJ_INJ" :
                modeChange = 2
                if self.ppt_device.numPreBurstVetos < 400 :
                    self.ppt_device.numPreBurstVetos = 600
                    self.status = "Num Pre Burst Vetos set to 600"
                    print(self.status)

        self.quarterDir = self.receiver_device.outputDir

        settingsStr = "0-" + str(int(64 / self.ppt_device.numParallelColumns - 1))

        quarters = getSweepVector(settingsStr)

        for quarter in quarters :
            print("\n##########################################\n")
            print("### Measure Quarter Q" + str(quarter) + "\n")
            print("### Measure Quarters " + settingsStr + "\n")
            print("##########################################\n")

            self.ppt_device.pixelsColSelect = quarter
            self.ppt_device.setCurrentColSkipOn()

            # Set output directories
            self.receiver_device.outputDir = str(self.quarterDir) + "/Quarter_" + str(quarter)

            self.initMeasurement()

            currentOutputDir = str(self.quarterDir) + "/Quarter_" + str(quarter)

            print("Store Data in " + currentOutputDir + "\n")
            self.measurementOutDir = currentOutputDir

            self.storeConfiguration()

            writeMeasurementConfiguration(self.getMeasurementConfig(True),
                                          settings1,name1,enSecond,settings2,name2)
            self.transferConfigDirectory()

            print("Measure " + name1 + " Settings:" + str(settings1))

            self.status = "Running Measurement ..."
            print(self.status)


            for set2 in settings2 :
                if enSecond :
                    self.setPixelRegister(self.activeModule,self.selPixels,name2,set2,True)

                for set1 in settings1 :
                    self.updateStatus(enSecond,set1,name1,settings1[-1],set2,name2,settings2[-1])

                    self.setInjectionValue(set1)

                    self.measureData(enSecond,set1,name1,set2,name2)
                    if self.abortMeasurement :
                        break

                if self.abortMeasurement :
                    break

            if self.abortMeasurement :
                break

            self.finishMeasurement()

        if self.abortMeasurement :
            self.finishMeasurement()

        self.receiver_device.outputDir = currentOutputDir


    @Slot(displayedName="Run BurstParameterSweep",
          description="Start Burst Parameter Sweep",
          allowedStates=[State.STOPPED])
    def runBurstParameterSweep(self) :
        print("Run Burst Parameter Sweep")
        if not self.receiver_device.outputDir :
            self.status = "Output dir not specified"
            self.state  = State.ERROR
            return

        self.initMeasurement()

        enSecond = False

        name2 = "none"
        settings2 = [1]

        if not self.checkBurstParameter() :
            return

        name1 = self.burstParameterName
        settings1 = getSweepVector(self.burstParameterRange)

        self.measurementOutDir = self.baseDir
        self.storeConfiguration()
        writeMeasurementConfiguration(self.getMeasurementConfig(),
                                      settings1,name1)

        self.transferConfigDirectory()

        print("Measure " + name1 + " Settings:" + str(settings1))

        self.status = "Running Measurement ..."
        print(self.status)


        for set1 in settings1 :
            self.updateStatus(enSecond,set1,name1,settings1[-1],1,name2,1)
            self.setBurstParameter(name1,set1)
            self.measureData(enSecond,set1,name1,1,name2)
            if self.abortMeasurement :
                break

        self.finishMeasurement()


    @Slot(displayedName="Run SequencerParameterSweep",
          description="Start Sequencer Parameter Sweep",
          allowedStates=[State.STOPPED])
    def runSequencerParameterSweep(self) :
        print("Run Sequencer Parameter Sweep")
        if not self.receiver_device.outputDir :
            self.status = "Output dir not specified"
            self.state  = State.ERROR
            return

        self.initMeasurement()

        enSecond = False

        name2 = "none"
        settings2 = [1]

        if not self.checkSequencerParameter() :
            return

        name1 = self.ppt_device.sequencerParameterName
        settings1 = getSweepVector(self.sequencerParameterRange)

        self.measurementOutDir = self.baseDir
        self.storeConfiguration()
        writeMeasurementConfiguration(self.getMeasurementConfig(),
                                      settings1,name1)

        self.transferConfigDirectory()

        print("Measure " + name1 + " Settings:" + str(settings1))

        self.status = "Running Measurement ..."
        print(self.status)

        for set1 in settings1 :
            self.updateStatus(enSecond,set1,name1,settings1[-1],1,name2,1)
            self.setSequencerParameter(name1,set1)
            self.measureData(enSecond,set1,name1,1,name2)
            if self.abortMeasurement :
                break

        self.finishMeasurement()


    @Slot(displayedName="Run Burst Acquisition",
          description="Start Burst Acquisition of selected number of trains",
          allowedStates=[State.STOPPED])
    def acquireBursts(self) :
        print("Run Burst Acquisition")
        if not self.receiver_device.outputDir :
            self.status = "Output dir not specified"
            self.state  = State.ERROR
            return

        self.initMeasurement()

        name1 = "BurstMeasurement"
        settings1 = [1]

        self.status = "Running " + name1 + " ..."
        print(self.status)

        self.measurementOutDir = self.baseDir
        self.storeConfiguration()
        writeMeasurementConfiguration(self.getMeasurementConfig(),
                                      settings1,name1)

        self.transferConfigDirectory()


        self.measureData(False,1,name1,"","")

        self.finishMeasurement()


    @Slot()
    def test(self) :
        print("transfer output Dir")
        if self.enSending :
          self.baseDir = str(self.receiver_device.outputDir)
          self.transferConfigDirectory()

          self.measurementOutDir = self.baseDir + "/test1"


          time.sleep(0.5)

          distDir = os.path.basename(self.baseDir)
          self.transferDirectory(self.measurementOutDir,distDir, self.removeOrig)
        else :
          print("en sending disabled")

