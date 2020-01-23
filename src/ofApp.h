#pragma once

#include "ofMain.h"
#include <armadillo>
#include "ofxDatGui.h"
#include "armaRingBuf.hpp"
#include "maximilian.h"
#include "maxiReverb.h"
#include <sndfile.h>
#include "ofxGui.h"


using namespace arma;


class ofApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    void audioIn(ofSoundBuffer & buffer);
    void audioOut(ofSoundBuffer & buffer);
    void exit();

    ofSoundStream soundStream;
    int sampleRate, bufferSize;
    std::vector<float> audioInBuffer;
    float inputRMS=0;
    double etcAllStrings=0;
    double dynCC=0;
    int ETCStepCount=0;
    double meanRMS=0;
//    double meanRMSLong=0;
    double recentMaxRMS = 0;
    double ETCDiff=0;
//    int ETCSymbolCount=8;
//    int ETCRange = 50;
    int ETCHopSize = 25;
//    float ETCRelativeHop = 0.5;
    void reCalcETCParams();

    int dynCCWindowSize = 50;
    double dynCCStep = 0.2;
    double dynCCSize = 0.6;
    double dynCCPastSize = 0.5;
    
    double maxHeadroom = 0.1;
//    bool rmsMode = 1;
    //sound
    float masterGain = 1.0;
    armaRingBuf<float> rmsRingBuf{500};
    armaRingBuf<float> dynccRingBuf{500};
    armaRingBuf<float> cccRingBuf{500};
    armaRingBuf<float> etcDiffRingBuf{500};
    armaRingBuf<float> mainMixSignalRingBuf{512};
    armaRingBuf<float> stringRingBuf[4];
    armaRingBuf<float> stringRMSRingBuf[4];
    armaRingBuf<float> masterGainRingBuf{500};
    armaRingBuf<float> cccIVToMainRingBuf{500};


    double cccStringIV=0.0;
//    int rmsSize=64;
    int rmsHop=32;
    int rmsCounter=0;
    float rmsRelativeHop = 0.5;
//    float damping=0;
    maxiBiquad dampingResponse;
//    float dampingResponseFrequency=10;
//    float dampingCurve = 1.0;
    
    maxiBiquad eq1, eq2;
    float eq1Freq=1000, eq1Q=1, eq1Gain=0;

    maxiFreeVerb verb;
//    float verbMix=0.2, verbAbsorbtion=0.4, verbRoomSize=0.4;

    //CCC
    armaRingBuf<float> string1RingBuf{500};
    armaRingBuf<float> string2RingBuf{500};
    armaRingBuf<float> allStringsRingBuf{500};
    maxiDattaroReverb dverb;

    
    //gui
//    ofxDatGui* gui;
//    ofxDatGuiSlider* gainSliders[4];
    ofxDatGuiToggle* recordToggle;

    //ofxGui
    ofxPanel ofgui;
    ofxLabel recLabel;
    ofParameter<bool> pRecording;
    void pRecordingToggle(bool &v);
    ofxLabel analysisLabel;
    ofParameter<double> pHeadroom;
    void pHeadroomChanged(double &v);
    ofParameter<bool> pRmsMode;
    ofParameter<int> pRmsSize;
    ofParameter<double> pRmsRelativeHop;
    void pRmsSizeChanged(int &v);
    void pRmsRelHopChanged(double &v);
    ofParameter<int> pETCSymbolCount;
    ofParameter<float> pChannelGains[4];
    ofxLabel soundLabel;
    void pETCSizeChanged(int &v);
    void pETCRelHopChanged(double &v);
    ofParameter<int> pETCRange;
    ofParameter<double> pETCRelativeHop;
    ofParameter<double> pDamping;
    ofParameter<double> pDampingCurve;
    ofParameter<double> pDampingResponseFrequency;
    void pDampingResponseFrequencyChanged(double &v);
    ofParameter<double> pVerbMix, pVerbAbsorbtion, pVerbRoomSize;

    ofxLabel CCCLabel;

    ofParameter<bool> pCalcCCC = false;


    //recording
    SNDFILE*  wavfile;
    SF_INFO sfinfo;
//    bool isRecording = 0;
    
    std::vector<float> audioInRecBuffer;
    std::vector<float> diskBuffer;
    std::thread wavWriter;
    std::mutex wwmtx;
    std::condition_variable wwcv;
    
};
