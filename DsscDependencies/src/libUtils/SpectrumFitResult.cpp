#include <iostream>

#include "utils.h"

#include "SpectrumFitResult.h"


using namespace std;

namespace utils{

const SpectrumFitResult::TargetType SpectrumFitResult::g_FE55      = {5900,6.49/5.9,0.1168,"FE55"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_NBTarget  = {16521.28,16615.16/16521.28,0.1168,"NB"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_MoTarget	= {17374.29,17479.37/17374.29,0.1168,"Mo"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_TcTarget	= {18250.90,18367.20/18250.90,0.1168,"Tc"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_RuTarget	= {19150.49,19279.16/19150.49,0.1168,"Ru"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_RhTarget	= {20073.67,20216.12/20073.67,0.1168,"Rh"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_PdTarget	= {21020.15,21177.08/21020.15,0.1168,"Pd"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_AgTarget	= {21990.30,22162.92/21990.30,0.1168,"Ag"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_AgAdvTarget = {22103.10,25003.7/22103.1,0.13872,"AgAdv"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_CdTarget	= {22984.05,23173.98/22984.05,0.1168,"Cd"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_InTarget	= {24002.03,24209.75/24002.03,0.1168,"In"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_SnTarget	= {25044.04,25271.36/25044.04,0.1168,"Sn"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_SbTarget	= {26110.78,26358.86/26110.78,0.1168,"Sb"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_TeTarget	= {27201.99,27472.57/27201.99,0.1168,"Te"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_ITarget   = {28317.52,28612.32/28317.52,0.1168,"IT"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_XeTarget	= {29458.25,29778.78/29458.25,0.1168,"Xe"};
const SpectrumFitResult::TargetType SpectrumFitResult::g_CsTarget	= {30625.40,30973.13/30625.40,0.1168,"Cs"};

//Active target Configuration
SpectrumFitResult::TargetType SpectrumFitResult::s_currentTarget = SpectrumFitResult::g_AgAdvTarget;

//Target List for handling in software
SpectrumFitResult::TargetMap SpectrumFitResult::s_targetMap { {"FE55",SpectrumFitResult::g_FE55  },
                                                              {"NB",SpectrumFitResult::g_NBTarget},
                                                              {"Mo",SpectrumFitResult::g_MoTarget},
                                                              {"Tc",SpectrumFitResult::g_TcTarget},
                                                              {"Ru",SpectrumFitResult::g_RuTarget},
                                                              {"Rh",SpectrumFitResult::g_RhTarget},
                                                              {"Pd",SpectrumFitResult::g_PdTarget},
                                                              {"Ag",SpectrumFitResult::g_AgTarget},
                                                              {"AgAdv",SpectrumFitResult::g_AgAdvTarget},
                                                              {"Cd",SpectrumFitResult::g_CdTarget},
                                                              {"In",SpectrumFitResult::g_InTarget},
                                                              {"Sn",SpectrumFitResult::g_SnTarget},
                                                              {"Sb",SpectrumFitResult::g_SbTarget},
                                                              {"Te",SpectrumFitResult::g_TeTarget},
                                                              {"IT",SpectrumFitResult::g_ITarget },
                                                              {"Xe",SpectrumFitResult::g_XeTarget},
                                                              {"Cs",SpectrumFitResult::g_CsTarget}};


void SpectrumFitResult::TargetType::print() const
{
  cout << "Target Type  = " << name << endl;
  cout << "PhotonEnergy = " << photonEnergy << endl;
  cout << "KAB ERatio   = " << kab_Eratio << endl;
  cout << "KAB IRatio   = " << kab_Iratio << endl;
}


bool SpectrumFitResult::checkCurrentTarget(const std::string & targetName)
{
   std::string currTargetNameStr = s_currentTarget.name;
   bool same = currTargetNameStr == targetName;
   if(same){
     cout << " Current Target is equal to " << targetName << endl;
   }else{
     cout << " Current Target differs from " << targetName << endl;
   }
   return same;
}


void SpectrumFitResult::setCurrentTarget(const std::string & targetName)
{
  const auto it = s_targetMap.find(targetName);
  if(it != s_targetMap.end()){
    s_currentTarget = it->second;
    cout << "Current XRay Target set to:" << endl;
    s_currentTarget.print();
  }else{
    cout << "Error: Target " << targetName << " not found!" << endl;
  }
}


SpectrumFitResult::TargetType SpectrumFitResult::TargetType::getCurrentTarget(double targetGain)
{
  if(g_FE55.photonEnergy == targetGain){
    return g_FE55;
  }else if(g_NBTarget.photonEnergy == targetGain){
    return g_NBTarget;
  }else if(g_MoTarget.photonEnergy == targetGain){
    return g_MoTarget;
  }else if(g_TcTarget.photonEnergy == targetGain){
    return g_TcTarget;
  }else if(g_RuTarget.photonEnergy == targetGain){
    return g_RuTarget;
  }else if(g_RhTarget.photonEnergy == targetGain){
    return g_RhTarget;
  }else if(g_PdTarget.photonEnergy == targetGain){
    return g_PdTarget;
  }else if(g_AgTarget.photonEnergy == targetGain){
    return g_AgTarget;
  }else if(g_AgAdvTarget.photonEnergy == targetGain){
    return g_AgAdvTarget;
  }else if(g_CdTarget.photonEnergy == targetGain){
    return g_CdTarget;
  }else if(g_InTarget.photonEnergy == targetGain){
    return g_InTarget;
  }else if(g_SnTarget.photonEnergy == targetGain){
    return g_SnTarget;
  }else if(g_SbTarget.photonEnergy == targetGain){
    return g_SbTarget;
  }else if(g_TeTarget.photonEnergy == targetGain){
    return g_TeTarget;
  }else if(g_ITarget.photonEnergy == targetGain){
    return g_ITarget;
  }else if(g_XeTarget.photonEnergy == targetGain){
    return g_XeTarget;
  }else if(g_CsTarget.photonEnergy == targetGain){
    return g_CsTarget;
  }
  return g_AgAdvTarget;
}


void SpectrumFitResult::printHeader(std::ofstream & out, const std::string & pixelsStr)
{
  out << "#Specrtum FitResult:\n";
  out << "#TimeStamp :\t" << utils::getLocalTimeStr() << "\n";
  out << "#Contained Pixels :\t" << pixelsStr << "\n";
  if(pixelsStr == "0-4095"){
    out << "#### Columns\t64\t\n";
  }else{
    out << "#### Columns\t512\t\n";
  }
  out << "#Photon Energy: " << SpectrumFitResult::s_currentTarget.photonEnergy << "\n";
  out << "#Pixel\t"; // both export functions add the pixel info in front of the parameters
  out <<  "Gain\t";
  out <<  "KaDistance\t";
  out <<  "ENC\t";
  out <<  "roiNoiseStart\t";
  out <<  "roiNoiseEnd\t";
  out <<  "roiSignalStart\t";
  out <<  "roiSignalEnd\t";
  out <<  "noiseAmp\t";
  out <<  "noiseAmpErr\t";
  out <<  "noiseMean\t";
  out <<  "noiseMeanErr\t";
  out <<  "noiseSigma\t";
  out <<  "noiseSigmaErr\t";
  out <<  "kaPeakAmp\t";
  out <<  "kaPeakAmpErr\t";
  out <<  "kaPeakMean\t";
  out <<  "kaPeakMeanErr\t";
  out <<  "troughrat\t";
  out <<  "troughratErr\t";
  out << "\n";
}


double SpectrumFitResult::getGain() const
{
  double distance = getKaDistance();
  if(distance == 0.0) return 0.0;
  return s_currentTarget.photonEnergy/distance;
}


double SpectrumFitResult::getGainError() const
{
  double distance = getKaDistance();
  double gain = getGain();
  double distanceErr = sqrt(pow(kaPeakMeanErr,2) + pow(noiseMeanErr,2));
  return gain * distanceErr/distance;
}


void SpectrumFitResult::print(std::ostream & out) const
{
  out << getGain()       << "\t";
  out << getKaDistance() << "\t";
  out << getENC()        << "\t";
  out << roiNoiseStart   << "\t";
  out << roiNoiseEnd     << "\t";
  out << roiSignalStart  << "\t";
  out << roiSignalEnd    << "\t";
  out << noiseAmp        << "\t";
  out << noiseAmpErr     << "\t";
  out << noiseMean       << "\t";
  out << noiseMeanErr    << "\t";
  out << noiseSigma      << "\t";
  out << noiseSigmaErr   << "\t";
  out << kaPeakAmp       << "\t";
  out << kaPeakAmpErr    << "\t";
  out << kaPeakMean      << "\t";
  out << kaPeakMeanErr   << "\t";
  out << troughrat       << "\t";
  out << troughratErr    << "\t";
}

bool SpectrumFitResult::isValid() const
{
  if(getENC() < 0.01) return false;
  if(getGain() < 0) return false;
  return true;
}


void SpectrumGainMapFitResult::print(std::ostream & out) const
{
    out << pixel << "\t";
    out << gainSetting << "\t";
    SpectrumFitResult::print(out);
}


void SpectrumGainMapFitResult::printHeader(std::ofstream & out, const std::string & pixelsStr) const
{
    SpectrumFitResult::printHeader(out,pixelsStr);
    out << "#Pixel\t";
    out <<  "GainSetting\t";
    out <<  "Gain\t";
    out <<  "KaDistance\t";
    out <<  "ENC\t";
    out <<  "roiNoiseStart\t";
    out <<  "roiNoiseEnd\t";
    out <<  "roiSignalStart\t";
    out <<  "roiSignalEnd\t";
    out <<  "noiseAmp\t";
    out <<  "noiseMean\t";
    out <<  "noiseSigma\t";
    out <<  "kaPeakAmp\t";
    out <<  "kaPeakAmpErr\t";
    out <<  "kaPeakMean\t";
    out <<  "kaPeakMeanErr\t";
    out <<  "troughrat\t";
    out <<  "troughratErr\t";
    out << "\n";
}

SpectrumGainMapFitResult::SpectrumGainMapFitResult(const SpectrumFitResult& fitResult)
{
    operator=(fitResult);
}

void SpectrumGainMapFitResult::operator=(const SpectrumFitResult& fitResult)
{
  roiNoiseStart  = fitResult.roiNoiseStart;
  roiNoiseEnd    = fitResult.roiNoiseEnd;
  roiSignalStart = fitResult.roiSignalStart;
  roiSignalEnd   = fitResult.roiSignalEnd;
  noiseAmp       = fitResult.noiseAmp;
  noiseAmpErr    = fitResult.noiseAmpErr;
  noiseMean      = fitResult.noiseMean;
  noiseMeanErr   = fitResult.noiseMeanErr;
  noiseSigma     = fitResult.noiseSigma;
  noiseSigmaErr  = fitResult.noiseSigmaErr;
  kaPeakAmp      = fitResult.kaPeakAmp;
  kaPeakAmpErr   = fitResult.kaPeakAmpErr;
  kaPeakMean     = fitResult.kaPeakMean;
  kaPeakMeanErr  = fitResult.kaPeakMeanErr;
  troughrat      = fitResult.troughrat;
  troughratErr   = fitResult.troughratErr;
}


std::vector<double> SpectrumGainMapFitResult::getDataVec() const
{
    return {pixel,gainSetting,getGain(),getKaDistance(),getENC(),
            roiNoiseStart,roiNoiseEnd,roiSignalStart,roiSignalEnd,
            noiseAmp,noiseAmpErr,
            noiseMean,noiseMeanErr,
            noiseSigma,noiseSigmaErr,
            kaPeakAmp,kaPeakAmpErr,
            kaPeakMean,kaPeakMeanErr,
            troughrat,troughratErr};
}

std::vector<double> SpectrumFitResult::getDataVec() const
{
    return {getGain(),getKaDistance(),getENC(),
            roiNoiseStart,roiNoiseEnd,roiSignalStart,roiSignalEnd,
            noiseAmp,noiseAmpErr,
            noiseMean,noiseMeanErr,
            noiseSigma,noiseSigmaErr,
            kaPeakAmp,kaPeakAmpErr,
            kaPeakMean,kaPeakMeanErr,
            troughrat,troughratErr};
}

void SpectrumFitResult::fill(const std::vector<double> & values)
{
  if(values.size() != getDataNames().size()){
    cout << "SpectrumFitResult ERROR: can not fill from vector of size = " << values.size() << endl;
    return;
  }
  size_t idx = 3;
  roiNoiseStart  = values[idx++];
  roiNoiseEnd    = values[idx++];
  roiSignalStart = values[idx++];
  roiSignalEnd   = values[idx++];
  noiseAmp       = values[idx++];
  noiseAmpErr    = values[idx++];
  noiseMean      = values[idx++];
  noiseMeanErr   = values[idx++];
  noiseSigma     = values[idx++];
  noiseSigmaErr  = values[idx++];
  kaPeakAmp      = values[idx++];
  kaPeakAmpErr   = values[idx++];
  kaPeakMean     = values[idx++];
  kaPeakMeanErr  = values[idx++];
  troughrat      = values[idx++];
  troughratErr   = values[idx++];
}


void SpectrumGainMapFitResult::fill(const std::vector<double> & values)
{
  if(values.size() != getDataNames().size()){
    cout << "SpectrumFitResult ERROR: can not fill from vector of size = " << values.size() << endl;
    return;
  }
  size_t idx = 0;
  pixel       = values[idx++];
  gainSetting = values[idx++];
  idx = 5;
  roiNoiseStart  = values[idx++];
  roiNoiseEnd    = values[idx++];
  roiSignalStart = values[idx++];
  roiSignalEnd   = values[idx++];
  noiseAmp       = values[idx++];
  noiseAmpErr    = values[idx++];
  noiseMean      = values[idx++];
  noiseMeanErr   = values[idx++];
  noiseSigma     = values[idx++];
  noiseSigmaErr  = values[idx++];
  kaPeakAmp      = values[idx++];
  kaPeakAmpErr   = values[idx++];
  kaPeakMean     = values[idx++];
  kaPeakMeanErr  = values[idx++];
  troughrat      = values[idx++];
  troughratErr   = values[idx++];
}

std::vector<std::string> SpectrumGainMapFitResult::getDataNames()
{
  return {"Pixel","GainSetting","Gain keV","Separation","ENC",
    "roiNoiseStart","roiNoiseEnd","roiSignalStart","roiSignalEnd",
    "noiseAmp","noiseAmpErr",
    "noiseMean","noiseMeanErr",
    "noiseSigma","noiseSigmaErr",
    "kaPeakAmp","kaPeakAmpErr",
    "kaPeakMean","kaPeakMeanErr",
    "troughrat","troughratErr"};
}

std::vector<std::string> SpectrumFitResult::getDataNames()
{
  return {"Gain keV","Separation","ENC",
    "roiNoiseStart","roiNoiseEnd","roiSignalStart","roiSignalEnd",
    "noiseAmp","noiseAmpErr",
    "noiseMean","noiseMeanErr",
    "noiseSigma","noiseSigmaErr",
    "kaPeakAmp","kaPeakAmpErr",
    "kaPeakMean","kaPeakMeanErr",
    "troughrat","troughratErr"};
}


void dumpFitResultsToASCII(const string & fileName, const std::vector<uint32_t> & pixels, const FitResultsVec & fitResults)
{
  string fullFileName = fileName;
  if(fileName.rfind(".txt") == std::string::npos){
    fullFileName += ".txt";
  }

  ofstream out(fullFileName);

  SpectrumFitResult::printHeader(out,positionVectorToList(pixels));

  int idx = 0;
  for(auto && res : fitResults){
    out << pixels[idx++] << "\t";
    res.print(out);
    out << "\n";
  }
  out.close();

  cout << "FitResults saved to: " << fullFileName << endl;
}


void dumpFitResultsToASCII(const std::string & fileName, const FitResultsMap &fitResults)
{
  string fullFileName = fileName;
  if(fileName.rfind(".txt") == std::string::npos){
    fullFileName += ".txt";
  }

  ofstream out(fullFileName);

  SpectrumFitResult::printHeader(out,positionVectorToList(utils::getKeys(fitResults)));

  for(auto && item : fitResults){
    out << item.first << "\t";
    item.second.print(out);
    out << "\n";
  }
  out.close();

  cout << "FitResults saved to: " << fullFileName << endl;
}


FitResultsMap importFitResultsFromASCII(const std::string & fileName)
{
  FitResultsMap resultMap;
  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open FitResults file: " << fileName << "\n";
    return resultMap;
  }


  // not member in SpectrumFitResult
  uint32_t pixel;
  double gain;
  double kadistance;
  double enc;

  string line;

  bool header = true;
  while (getline(in, line)) {
    if(line.empty()) continue;

    if(header && line[0] == '#'){
      //TODO: test gain
      size_t pos = line.find("#Gain:");
      if(pos != string::npos){
        gain = std::stod(line.substr(pos));
        if(gain != SpectrumFitResult::s_currentTarget.photonEnergy){
          cout << "WARNING: imported file is for different gain than specified in data histo:" << gain << "/" << SpectrumFitResult::s_currentTarget.photonEnergy << endl;
        }
      }
      continue;
    }else{
      header = false;
    }

    SpectrumFitResult result;

    istringstream iss(line);
    iss >> pixel;
    iss >> gain;
    iss >> kadistance;
    iss >> enc;
    iss >> result.roiNoiseStart;    // double to be able to insert lower binBoundary
    iss >> result.roiNoiseEnd;
    iss >> result.roiSignalStart;
    iss >> result.roiSignalEnd;
    iss >> result.noiseAmp;
    iss >> result.noiseAmpErr;
    iss >> result.noiseMean;
    iss >> result.noiseMeanErr;
    iss >> result.noiseSigma;
    iss >> result.noiseSigmaErr;
    iss >> result.kaPeakAmp;
    iss >> result.kaPeakAmpErr;
    iss >> result.kaPeakMean;
    iss >> result.kaPeakMeanErr;
    iss >> result.troughrat;
    iss >> result.troughratErr;
    resultMap[pixel] = result;
  }
  return resultMap;
}


void dumpSpectrumGainMapFitResultsToASCII(const std::string & fileName, const SpectrumGainMapFitResultVec &fitResults)
{
  ofstream out(fileName);

  if(fitResults.empty()) return;

  fitResults.front().printHeader(out,"all");

  for(auto && result : fitResults){
    result.print(out);
    out << "\n";
  }
  out.close();
}


SpectrumGainMapFitResultVec importSpectrumGainMapFitResultsFromASCII(const std::string & fileName)
{
  SpectrumGainMapFitResultVec resultVec;
  ifstream in(fileName);

  if(!in.is_open()){
      cout << "Could not open FitResults file: " << fileName << "\n";
      return resultVec;
  }

  // not member in SpectrumFitResult
  double gain;
  double kadistance;
  double enc;

  string line;

  bool header = true;
  while (getline(in, line)) {

      if(line.empty()) continue;

      if(header && line[0] == '#'){
          //TODO: test gain
          size_t pos = line.find("#Gain:");
          if(pos != string::npos){
              gain = std::stod(line.substr(pos));
              if(gain != SpectrumFitResult::s_currentTarget.photonEnergy){
                cout << "WARNING: imported file is for different gain than specified in data histo:" << gain << "/" << SpectrumFitResult::s_currentTarget.photonEnergy << endl;

              }
          }
          continue;
      }else{
          header = false;
      }

      SpectrumGainMapFitResult result;

      istringstream iss(line);
      iss >> result.pixel;
      iss >> result.gainSetting;
      iss >> gain;
      iss >> kadistance;
      iss >> enc;
      iss >> result.roiNoiseStart;    // double to be able to insert lower binBoundary
      iss >> result.roiNoiseEnd;
      iss >> result.roiSignalStart;
      iss >> result.roiSignalEnd;
      iss >> result.noiseAmp;
      iss >> result.noiseAmpErr;
      iss >> result.noiseMean;
      iss >> result.noiseMeanErr;
      iss >> result.noiseSigma;
      iss >> result.noiseSigmaErr;
      iss >> result.kaPeakAmp;
      iss >> result.kaPeakAmpErr;
      iss >> result.kaPeakMean;
      iss >> result.kaPeakMeanErr;
      iss >> result.troughrat;
      iss >> result.troughratErr;
      resultVec.push_back(result);
  }

  return resultVec;
}

}
