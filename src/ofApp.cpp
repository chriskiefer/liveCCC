#include "ofApp.h"
#include <ETC.hpp>
#include <CCC.hpp>
#include <tuple>
#include <algorithm>

class myCustomTheme : public ofxDatGuiTheme{
    public:
        myCustomTheme(){
            font.size = 14;
            init();
        }
};

void ofApp::reCalcETCParams() {
    ETCHopSize = pETCRange * pETCRelativeHop;
    ETCStepCount=0;
}

void ofApp::pHeadroomChanged(double &v) {
    cout << v << endl;

}

void ofApp::pRecordingToggle(bool &v) {
    if (v) {
        sfinfo.samplerate = sampleRate;
        sfinfo.channels = 3;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        stringstream s;
        s << "/tmp/record_";
        s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        s << ".wav";
        wavfile = sf_open(s.str().c_str(), SFM_WRITE, &sfinfo);
    }else{
        wavWriter = std::thread([&](){
            cout<<"Wav Writer thread\n";
            sf_count_t n = sf_write_float(wavfile, audioInRecBuffer.data(), audioInRecBuffer.size());
            audioInRecBuffer.clear();
            sf_close(wavfile);
            cout << n << endl;
        });
        wavWriter.detach();
    };

//    if (v) {
////        isRecording=1;
//        sfinfo.samplerate = sampleRate;
//        sfinfo.channels = 3;
//        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
//        stringstream s;
//        s << "/tmp/record_";
//        s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//        s << ".wav";
//        wavfile = sf_open(s.str().c_str(), SFM_WRITE, &sfinfo);

//    }else{
////        isRecording=0;
//        sf_close(wavfile);
//    };
}


void ofApp::pRmsSizeChanged(int &v) {
    rmsHop = (int)(v * pRmsRelativeHop.get());
    rmsCounter=0;
}

void ofApp::pRmsRelHopChanged(double &v) {
    rmsHop = (int)(pRmsSize.get() * v);
    rmsCounter=0;
}

void ofApp::pETCSizeChanged(int &v) {
    cout << pETCRange << endl;
    reCalcETCParams();
}

void ofApp::pETCRelHopChanged(double &v) {
    reCalcETCParams();
}

void ofApp::pDampingResponseFrequencyChanged(double &v) {
    dampingResponse.set(maxiBiquad::LOWPASS, v, 0.1, 1);
}


//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    ofSetWindowShape(ofGetScreenWidth() * 0.7,     ofGetScreenHeight() * 0.5);
    ofSetWindowPosition(10, 10);
    
    for(size_t i = 0; i < 4; i++) {
        stringRingBuf[i].setSize(512);
        stringRMSRingBuf[i].setSize(512);
    }
    


    auto devices = soundStream.getDeviceList();
    int audioInterfaceIndex = 0;
    int devIdx=0;
    for(auto i : devices) {
        cout << devIdx << ", " << i.name << endl;
        //Apple Inc.: Built-in-out
//        if (i.name == "MOTU: MOTU UltraLite"){
//        if (i.name == "Apple Inc.: Built-in Output"){
//        if (i.name == "system"){
//        if (i.name == "default"){
          if (i.name == "hw:Komplete Audio 6,0"){

            audioInterfaceIndex=devIdx;
            break;
        }
        devIdx++;
    }
    cout << devices.at(audioInterfaceIndex).name << endl;
    cout << devices.at(audioInterfaceIndex).inputChannels << endl;
    cout << devices.at(audioInterfaceIndex).outputChannels << endl;
    
    

    
    //gui
//    pHeadroom.set(0.1);
    pHeadroom.addListener(this, &ofApp::pHeadroomChanged);
    pRecording.addListener(this, &ofApp::pRecordingToggle);
    pRmsSize.addListener(this, &ofApp::pRmsSizeChanged);
    pRmsRelativeHop.addListener(this, &ofApp::pRmsRelHopChanged);
    pETCRange.addListener(this, &ofApp::pETCSizeChanged);
    pETCRelativeHop.addListener(this, &ofApp::pETCRelHopChanged);
    pRmsRelativeHop.set(0.5);
    pETCRelativeHop.set(0.5);
    pDampingResponseFrequency.addListener(this, &ofApp::pDampingResponseFrequencyChanged);

    ofgui.setup("");
    ofgui.loadFont("verdana.ttf", 20);
    ofgui.setDefaultHeight(ofGetHeight() * 0.03);
    ofgui.setDefaultWidth(ofGetWidth() * 0.39);
    ofgui.setShape(ofGetWidth() * 0.6, 10,ofGetWidth() * 0.39, ofGetHeight() * 0.8);
    ofgui.add(recLabel.setup(">> Recording", ""));
    ofgui.add(pRecording.set("Record", false));
    ofgui.add(analysisLabel.setup(">> Analysis", ""));
    ofgui.add(pRmsMode.set("RMS rel(on)/abs(off)", true));
    ofgui.add(pHeadroom.set("Max headroom", 0.1, 0.01, 1.5));
    ofgui.add(pRmsSize.set("RMS Window Size", 64, 8, 512));
    ofgui.add(pRmsRelativeHop.set("RMS Hop Size", 0.5, 0.05, 1));
    ofgui.add(pETCSymbolCount.set("Symbol count", 8, 2, 64));
    ofgui.add(pETCRange.set("ETC Size", 50, 10, 200));
    ofgui.add(pETCRelativeHop.set("ETC Hop Size", 0.5, 0.05, 1));

    ofgui.add(soundLabel.setup(">> Sound", ""));
    ofgui.add(pChannelGains[0].set("Channel 1 Gain", 1, 0, 4.0));
    ofgui.add(pChannelGains[1].set("Channel 2 Gain", 1, 0, 4.0));

    //these go higher because of line level inputs on the Komplete 6
    ofgui.add(pChannelGains[2].set("Channel 3 Gain", 3, 0, 40.0));
    ofgui.add(pChannelGains[3].set("Channel 4 Gain", 3, 0, 40.0));

    ofgui.add(pDampingCurve.set("Damping Curve", 1.0, 0.5, 3.0));
    ofgui.add(pDamping.set("Damping", 0, 0, 10.0));
    ofgui.add(pDampingResponseFrequency.set("Damping Response", 50, 0.001, 20.0));

    ofgui.add(pVerbMix.set("Reverb Mix", 0,0,1));
    ofgui.add(pVerbAbsorbtion.set("Reverb Absorbtion", 0,0,1));
    ofgui.add(pVerbRoomSize.set("Reverb Room Size", 0,0,1));

    ofgui.add(CCCLabel.setup(">> CCC", ""));



//    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
//    gui->setOpacity(0.9);
//    gui->setTheme(new myCustomTheme());
//    gui->setWidth(ofGetWidth() * 0.4);

//    gui->addLabel(">> Recording");
//    recordToggle = gui->addToggle("Record", isRecording);
//    recordToggle->onToggleEvent([&](ofxDatGuiToggleEvent e) {
//        if (e.checked) {
//            isRecording=1;
//            sfinfo.samplerate = sampleRate;
//            sfinfo.channels = 3;
//            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
//            stringstream s;
//            s << "/tmp/record_";
//            s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//            s << ".wav";
//            wavfile = sf_open(s.str().c_str(), SFM_WRITE, &sfinfo);
            
//        }else{
//            isRecording=0;
//            sf_close(wavfile);
//        };
//    });
//    gui->addLabel(">> Analysis");
//    auto rmsModeToggle = gui->addToggle("RMS rel/abs", rmsMode);
//    rmsModeToggle->onToggleEvent([&](ofxDatGuiToggleEvent e) {
//        rmsMode = e.checked;
//    });

//    auto headroomSlider = gui->addSlider("Headroom size: ", 0.01, 1.5, maxHeadroom);
//    headroomSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        maxHeadroom = e.value;
//    });

//            ofstream paramsFile;
//            s << ".params";
//            paramsFile.open(s.str().c_str());
//            paramsFile << "{";
//            paramsFile << "'maxheadroom':" << maxHeadroom << ",";
//            paramsFile << "'rmsSize':" << rmsSize << ",";
//            paramsFile << "'rmsRelativeHop':" << rmsRelativeHop << ",";
//            paramsFile << "'ETCSymbolCount':" << ETCSymbolCount << ",";
//            paramsFile << "'ETCRange':" << ETCRange << ",";
//            paramsFile << "'ETCRelativeHop':" << ETCRelativeHop << ",";
//            paramsFile << "'channelGains0':" << channelGains[0] << ",";
//            paramsFile << "'channelGains1':" << channelGains[1] << ",";
//            paramsFile << "'channelGains2':" << channelGains[2] << ",";
//            paramsFile << "'channelGains3':" << channelGains[3] << ",";
//            paramsFile << "'dampingCurve':" << dampingCurve << ",";
//            paramsFile << "'damping':" << damping << ",";
//            paramsFile << "'dampingResponseFrequency':" << dampingResponseFrequency << ",";
//            paramsFile << "'dynCCWindowSize':" << dynCCWindowSize << ",";
//            paramsFile << "'dynCCSize':" << dynCCSize << ",";
//            paramsFile << "'dynCCStep':" << dynCCStep << ",";
//            paramsFile << "'dynCCPastSize':" << dynCCPastSize << ",";
//            paramsFile << "'dynCCPastSize':" << dynCCPastSize << ",";
//            paramsFile << "'verbMix':" << verbMix << ",";
////            paramsFile << "'verbAbsorbtion':" << verbAbsorbtion << ",";
////            paramsFile << "'verbRoomSize':" << verbRoomSize << ",";
//            paramsFile << "}";
//            paramsFile.close();
            
//            wwcv.notify_one();

////    auto setupRMS = [&]() {
////        rmsHop = (int)(rmsSize * rmsRelativeHop);
////        rmsCounter=0;
////    };

//    auto rmsSizeSlider = gui->addSlider("RMS window size: ", 8, 512, rmsSize);
//    rmsSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        rmsSize = e.value;
////        setupRMS();
//        rmsHop = (int)(rmsSize * rmsRelativeHop);
//        rmsCounter=0;
//    });

//    auto rmsHopSizeSlider = gui->addSlider("RMS hop size: ", 0.05, 1.0, rmsRelativeHop);
//    rmsHopSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        rmsRelativeHop = e.value;
////        setupRMS();
//        rmsHop = (int)(rmsSize * rmsRelativeHop);
//        rmsCounter=0;
//    });

//    auto symbolSlider = gui->addSlider("Symbol count: ", 2, 64, ETCSymbolCount);
    
////    symbolSlider->bind(ETCSymbolCount);
//    symbolSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        ETCSymbolCount = e.value;
//    });
    
//    auto etcRangeSlider = gui->addSlider("ETC size: ", 10, 200, ETCRange);
//    etcRangeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        ETCRange = e.value;
//        reCalcETCParams();
//    });

//    auto etcHopSlider = gui->addSlider("ETC hop size: ", 0, 1.0, ETCRelativeHop);
//    etcHopSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        ETCRelativeHop = e.value;
//        reCalcETCParams();
//    });

//    gui->addLabel(">> Sound");
//    gainSliders[0] = gui->addSlider("Gain ch 1", 0, 4.0, channelGains[0]);
//    gainSliders[0]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        channelGains[0] = e.value;
//    });
//    gainSliders[1] = gui->addSlider("Gain ch 2", 0, 4.0, channelGains[1]);
//    gainSliders[1]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        channelGains[1] = e.value;
//    });
//    gainSliders[2] = gui->addSlider("Gain ch 3", 0, 40.0, channelGains[2]);
//    gainSliders[2]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        channelGains[2] = e.value;
//    });
//    gainSliders[3] = gui->addSlider("Gain ch 4", 0, 40.0, channelGains[3]);
//    gainSliders[3]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        channelGains[3] = e.value;
//    });
    
//    auto dampingCurveSlider = gui->addSlider("Damping Curve", 0.5, 3.0, dampingCurve);
//    dampingCurveSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        dampingCurve = e.value;
//    });
    
//    auto dampingSlider = gui->addSlider("Damping", 0, 10.0, damping);
//    dampingSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        damping = e.value;
//    });


    
//    auto dynCCWindowSlider = gui->addSlider("DynCC Window", 10, 200, dynCCWindowSize);
//    dynCCWindowSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        dynCCWindowSize = e.value;
//    });
//    auto dynCCSizeSlider = gui->addSlider("DynCC Size", 0.0, 1.0, dynCCSize);
//    dynCCSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        dynCCSize = e.value;
//    });
//    auto dynCCStepSlider = gui->addSlider("DynCC Step", 0.0, 1.0, dynCCStep);
//    dynCCStepSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        dynCCStep = e.value;
//    });
//    auto dynCCPastSizeSlider = gui->addSlider("DynCC Past", 0.0, 1.0, dynCCPastSize);
//    dynCCPastSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        dynCCPastSize = e.value;
//    });
    

//    auto verbAbsorptionSlider = gui->addSlider("Verb Absorb", 0.0, 1.0, verbAbsorbtion);
//    verbAbsorptionSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        verbAbsorbtion = e.value;
//    });
//    auto verbRoomSizeSlider = gui->addSlider("Verb Room Size", 0.0, 1.0, verbRoomSize);
//    verbRoomSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        verbRoomSize = e.value;
//    });
    ofSoundStreamSettings settings;
//    settings.setApi(ofSoundDevice::ALSA);
    bufferSize        = 64;
    sampleRate        = 44100;
    settings.setInListener(this);
    settings.setOutListener(this);
    settings.sampleRate = sampleRate;
    settings.numInputChannels = 4;
//    settings.numInputChannels = 1;
    settings.numOutputChannels = 2;
    settings.numBuffers = 4;
    settings.bufferSize = bufferSize;
    settings.setInDevice(devices[audioInterfaceIndex]);
   settings.setOutDevice(devices[audioInterfaceIndex]);
//        settings.setDeviceID(devices[audioInterfaceIndex]);

    audioInBuffer.resize(bufferSize);
    soundStream.setup(settings);


    dampingResponse.set(maxiBiquad::LOWPASS, pDampingResponseFrequency.get(), 0.1, 1);

//    ofSoundStreamStart();    
    
    audioInRecBuffer.reserve(3 * 44100 * 60 * 15);
//    wavWriter.detach();


}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(255,255,255);
    ofSetColor(100,100,100);
    ofFill();
    ofDrawRectangle(10,ofGetHeight()*0.9,50, -ofGetHeight()*0.8*etcAllStrings);
    ofSetColor(0,0,0);
    double meanRMSHeight =  (ofGetHeight()*0.9)-(ofGetHeight()*0.8*meanRMS);
    ofSetLineWidth(3);
    ofDrawLine(10,meanRMSHeight,50,meanRMSHeight);

    ofSetColor(200,100,100);
    ofDrawRectangle(60,ofGetHeight()*0.5, 50, -ofGetHeight()*0.4*ETCDiff);

    ofSetColor(100,100, dynCC>0 ? 255: 0);
    ofDrawRectangle(110,ofGetHeight()*0.5, 50, -ofGetHeight()*0.8*dynCC);

    ofSetColor(0,0,0);
    ofSetLineWidth(1);
    auto rmsbuf = rmsRingBuf.getBuffer(rmsRingBuf.size());
    int xoff = 200;
    int ybase = ofGetHeight()*0.9;
    ofNoFill();
    int step=1;
    for(int i=1; i < rmsbuf.size(); i++) {
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.9*(rmsbuf[i-1] / pHeadroom)), xoff+(i*step), ybase - (ofGetHeight()*0.2*(rmsbuf[i]/pHeadroom)));
    }

    ofSetColor(0,150,0, 150);
    ofSetLineWidth(1);
    auto dynccbuf = dynccRingBuf.getBuffer(dynccRingBuf.size());
    ybase = ofGetHeight()*0.5;
    ofNoFill();
    for(int i=1; i < dynccbuf.size(); i++) {
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.45*(dynccbuf[i-1])), xoff+(i*step), ybase - (ofGetHeight()*0.45*(dynccbuf[i])));
    }

    ofSetColor(255,0,0, 150);
    ofSetLineWidth(1);
    auto etcdiffbuf = etcDiffRingBuf.getBuffer(etcDiffRingBuf.size());
    ybase = ofGetHeight()*0.9;
    ofNoFill();
    for(int i=1; i < etcdiffbuf.size(); i++) {
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.9*(etcdiffbuf[i-1])), xoff+(i*step), ybase - (ofGetHeight()*0.9*(etcdiffbuf[i])));
    }
    
    ofSetColor(0,0,255, 150);
    ofSetLineWidth(1);
    auto mgainBuf = masterGainRingBuf.getBuffer(masterGainRingBuf.size());
    ybase = ofGetHeight()*0.9;
    ofNoFill();
    for(int i=1; i < etcdiffbuf.size(); i++) {
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.9*(mgainBuf[i-1])), xoff+(i*step), ybase - (ofGetHeight()*0.9*(mgainBuf[i])));
    }

    ofSetColor(255,100,255, 150);
    ofSetLineWidth(1);
    auto cccIVBuf = cccIVToMainRingBuf.getBuffer(cccIVToMainRingBuf.size());
    ybase = ofGetHeight()*0.45;
    ofNoFill();
    for(int i=1; i < cccIVBuf.size(); i++) {
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.9*(cccIVBuf[i-1])), xoff+(i*step), ybase - (ofGetHeight()*0.9*(cccIVBuf[i])));
    }

    ofgui.draw();
}


void ofApp::audioIn(ofSoundBuffer & buffer) {
    
    //mixdown to mono

    for(size_t i=0; i < buffer.getNumFrames(); i++) {
        masterGain = dampingResponse.play(1.0 - min(1.0,pow(ETCDiff * pDamping, pDampingCurve.get())));
        float mag=0;
        for (size_t j=0; j < buffer.getNumChannels(); j++) {
            double chInput = (buffer[(i*buffer.getNumChannels()) + j] * pChannelGains[j].get());
            stringRingBuf[j].push(chInput);
            mag += chInput;
        }
        mag = (mag / buffer.getNumChannels());
//        mag = mag + (verb.play(mag, verbRoomSize, verbAbsorbtion) * verbMix);

        mag = mag + (dverb.playStereo(mag)[0] * pVerbMix);
        audioInBuffer[i] = mag * masterGain;
        mainMixSignalRingBuf.push(mag);
        if (rmsCounter++ == rmsHop) {
            auto calcRMS = [&](Col<float> &win) {
                double rms=0;
                for(int j=0; j < pRmsSize.get(); j++) {
                    rms += win[j] * win[j];
                }
                rms = sqrt(rms / pRmsSize.get());
                return rms;
            };
            //calc individual string RMS
            for (size_t j=0; j < buffer.getNumChannels(); j++) {
                auto stringRMSWindow = stringRingBuf[j].getBuffer(pRmsSize.get());
                double stringRMS = calcRMS(stringRMSWindow);
            }
            auto rmsWindow = mainMixSignalRingBuf.getBuffer(pRmsSize.get());
//            double rms=0;
//            for(int j=0; j < pRmsSize.get(); j++) {
//                rms += rmsWindow[j] * rmsWindow[j];
//            }
//            rms = sqrt(rms / pRmsSize.get());
            inputRMS = calcRMS(rmsWindow);
            rmsRingBuf.push(inputRMS);
            rmsCounter=0;
            
            if (ETCStepCount==0) {

                auto symbolize = [&](Col<float> &win) {
                    ivec rmsSymBuf(win.size());
                    double rmsBufMax = win.max();
                    double rmsMax = pRmsMode.get() ? rmsBufMax : std::max(pHeadroom.get(), rmsBufMax);
                    for(size_t i=0; i < win.size(); i++) {
                        rmsSymBuf[i] = static_cast<sword>(win[i]/rmsMax * pETCSymbolCount.get());
                    }
                    return rmsSymBuf;
                };

//                auto calcETCFromRMSBuf = [&](Col<float> &win) {
//                    ivec rmsSymBuf(win.size());
//                    double rmsBufMax = win.max();
//                    double rmsMax = pRmsMode.get() ? rmsBufMax : std::max(pHeadroom.get(), rmsBufMax);
//                    for(size_t i=0; i < win.size(); i++) {
//                        rmsSymBuf[i] = static_cast<sword>(win[i]/rmsMax * pETCSymbolCount.get());
//                    }
//                    return ETC::calc(rmsSymBuf);
//                };

                auto allStringsRMSBuf = rmsRingBuf.getBuffer(pETCRange.get());
                auto allStringsSymBuf = symbolize(allStringsRMSBuf);
//                double rmsSum=0;
//                for(int i=0; i < rmsbuf.size(); i++) {
//                    rmsSum += rmsbuf[i];
//                }
//                meanRMS =  rmsSum / rmsbuf.size();
                etcAllStrings = ETC::calc(allStringsSymBuf);


//                ivec rmsSymBuf(rmsbuf.size());
                double rmsBufMax = allStringsRMSBuf.max();
////                double rmsMax = std::max(pHeadroom, rmsBufMax);
                double rmsMax = pRmsMode.get() ? allStringsRMSBuf.max() : std::max(pHeadroom.get(), rmsBufMax);
//                for(size_t i=0; i < rmsbuf.size(); i++) {
//                    rmsSymBuf[i] = static_cast<sword>(rmsbuf[i]/rmsMax * pETCSymbolCount.get());
//                }
                
                
                double rmsSum=0;
                for(int i=0; i < allStringsRMSBuf.size(); i++) {
                    rmsSum += allStringsRMSBuf[i];
                }
                meanRMS =  rmsSum / allStringsRMSBuf.size();
                
//                inputETC = ETC::calc(rmsSymBuf);

//                //        ETCDiff = (meanRMS/recentMaxRMS) - inputETC;
////                ETCDiff = (meanRMS/rmsMax) - inputETC;
///
//                cout <<  (meanRMS / rmsMax) << endl;

//                ETCDiff = (1.0-inputETC) * (meanRMS / rmsMax);
                ETCDiff = (1.0-etcAllStrings);
                etcDiffRingBuf.push(ETCDiff);
                cccIVToMainRingBuf.push(rmsMax - 0.5 );




                
                //        auto rmsbufLong = rmsRingBuf.getBuffer(400);
                //        recentMaxRMS=rmsbufLong.max();
                
                //CCC
                if (pCalcCCC.get()) {
                    auto stringIVRMSBuf = stringRMSRingBuf[0].getBuffer(pETCRange.get());
                    auto stringIVSymBuf = symbolize(stringIVRMSBuf);
                    int ccc_dx = pETCRange.get() * 0.25;
                    int ccc_xpast = ccc_dx;
                    double cccIVToMain;
                    int cccIVToMainType;
                    tie(cccIVToMain, cccIVToMainType) = CCC::CCCausality(allStringsSymBuf, stringIVSymBuf, ccc_dx, ccc_xpast, 5);
                    cccIVToMainRingBuf.push(cccIVToMain);
                }



//                //Dyn CC
//                rmsbuf = rmsRingBuf.getBuffer(dynCCWindowSize);
//                ivec rmsDynCCSymBuf(rmsbuf.size());
//                rmsBufMax = rmsbuf.max();
//                rmsMax = rmsMode ? rmsBufMax : std::max(pHeadroom.get(), rmsBufMax);
//                //prevent div by zero
//                rmsMax = max(rmsMax, 1e-10);
//                for(size_t i=0; i < rmsbuf.size(); i++) {
//                    rmsDynCCSymBuf[i] = static_cast<sword>(rmsbuf[i]/rmsMax * pETCSymbolCount.get());
//                }
                
//                double dynCCRange = dynCCWindowSize * dynCCSize;
//                int dynCCxpast = dynCCRange * dynCCPastSize;
//                if (dynCCxpast < 2) dynCCxpast = 2;
//                int dynCCdx = dynCCRange - dynCCxpast;
//                int dynCCStepSize = (dynCCxpast + dynCCdx) * dynCCStep;
//                dynCCStepSize = max(2, dynCCStepSize);
//                dynCC = CCC::dynamicCC(rmsDynCCSymBuf, dynCCdx, dynCCxpast, dynCCStepSize, CCC::SINGLETHREAD);
//                dynccRingBuf.push(dynCC);

//                CCC



            }
            ETCStepCount++;
            if (ETCStepCount==ETCHopSize) ETCStepCount=0;

        }

        if (pRecording.get()) {
            audioInRecBuffer.push_back(mag);
            audioInRecBuffer.push_back(etcAllStrings);
            audioInRecBuffer.push_back(ETCDiff);
        }

    }
    cout << pRecording.get() << endl;
    masterGainRingBuf.push(masterGain);

}

void ofApp::audioOut(ofSoundBuffer & buffer) {
    double amp=1;
    for(size_t i=0; i < buffer.getNumFrames(); i++) {
        //feedback!!!!
        buffer[buffer.getNumChannels() * i] =  audioInBuffer[i] * amp;
        buffer[(buffer.getNumChannels() * i) + 1] =  audioInBuffer[i] * amp;
    }
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
//    rbuf.push(random() % 10);
//    auto cbuf = rbuf.getBuffer();
//    cout << cbuf << endl;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


void ofApp::exit() {
    soundStream.stop();
    soundStream.close();
}
