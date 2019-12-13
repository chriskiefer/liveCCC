#pragma once

#include "ofMain.h"
#include <armadillo>
#include "ofxDatGui.h"
#include "armaRingBuf.hpp"
#include "maximilian.h"
#include "maxiReverb.h"
#include <sndfile.h>


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

    ofSoundStream soundStream;
    std::vector<float> audioInBuffer;
    float inputRMS=0;
    double inputETC=0;
    double dynCC=0;
    int ETCStepCount=0;
    double meanRMS=0;
//    double meanRMSLong=0;
    double recentMaxRMS = 0;
    double ETCDiff=0;
    int ETCSymbolCount=8;
    int ETCRange = 50;
    int ETCHopSize = 25;
    float ETCRelativeHop = 0.5;
    void reCalcETCParams();

    int dynCCWindowSize = 50;
    double dynCCStep = 0.2;
    double dynCCSize = 0.6;
    double dynCCPastSize = 0.5;
    
    double maxHeadroom = 0.1;
    bool rmsMode = 1;
    //sound
    float channelGains[4] = {1,1,3,3};
    float masterGain = 1.0;
    armaRingBuf<float> rmsRingBuf{500};
    armaRingBuf<float> dynccRingBuf{500};
    armaRingBuf<float> etcDiffRingBuf{500};
    armaRingBuf<float> sigRingBuf{512};
    armaRingBuf<float> masterGainRingBuf{500};
    int rmsSize=64;
    int rmsHop=32;
    int rmsCounter=0;
    float rmsRelativeHop = 0.5;
    float damping=0;
    maxiBiquad dampingResponse;
    float dampingResponseFrequency=10;
    float dampingCurve = 1.0;
    
    maxiBiquad eq1, eq2;
    float eq1Freq=1000, eq1Q=1, eq1Gain=0;

    maxiFreeVerb verb;
    float verbMix=0.2, verbAbsorbtion=0.4, verbRoomSize=0.4;

    
    //gui
    ofxDatGui* gui;
    ofxDatGuiSlider* gainSliders[4];
    
    //recording
    SNDFILE*  wavfile;
    SF_INFO sfinfo;
    bool isRecording = 0;
    
};
