#include "ofApp.h"
#include <ETC.hpp>
#include <CCC.hpp>
#include <tuple>
#include <algorithm>


void ofApp::reCalcETCParams() {
    ETCHopSize = ETCRange * ETCRelativeHop;
    ETCStepCount=0;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    
    
    
    ofSoundStreamSettings settings;
    int bufferSize        = 64;
    int sampleRate        = 44100;
    settings.setInListener(this);
    settings.setOutListener(this);
    settings.sampleRate = sampleRate;
    settings.numOutputChannels = 2;
    settings.bufferSize = bufferSize;


    
    auto devices = soundStream.getDeviceList();
    int audioInterfaceIndex = 0;
    int devIdx=0;
    for(auto i : devices) {
        cout << devIdx << ", " << i.name << endl;
        //Apple Inc.: Built-in-out
        if (i.name == "MOTU: MOTU UltraLite"){
            audioInterfaceIndex=devIdx;
            settings.numInputChannels = 4;
            break;
        }else if (i.name == "Apple Inc.: Built-in-out"){
            audioInterfaceIndex=devIdx;
            settings.numInputChannels = 1;
            break;
        }
        devIdx++;
    }
    cout << devices.at(audioInterfaceIndex).name << endl;
    cout << devices.at(audioInterfaceIndex).inputChannels << endl;
    cout << devices.at(audioInterfaceIndex).outputChannels << endl;
    
    
    settings.setOutDevice(devices[audioInterfaceIndex]);
    soundStream.setup(settings);
    
    audioInBuffer.resize(bufferSize);
    

    
    //gui
    
    ofxDatGui* gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setWidth(400);
    gui->setOpacity(0.9);

    gui->addLabel(">> Recording");
    
    recordNameField = gui->addTextInput("Name", "rec");
    auto recordToggle = gui->addToggle("Record", isRecording);
    recordToggle->onToggleEvent([&](ofxDatGuiToggleEvent e) {
        if (e.checked) {
            isRecording=1;
            sfinfo.samplerate = 44100;
            sfinfo.channels = 3;
            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            stringstream s;
            s << "/tmp/";
            s << recordNameField->getText();
            s << "_";
            s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            s << ".wav";
            wavfile = sf_open(s.str().c_str(), SFM_WRITE, &sfinfo);
            
            ofstream paramsFile;
            s << ".params";
            paramsFile.open(s.str().c_str());
            paramsFile << "{";
            paramsFile << "'maxheadroom':" << maxHeadroom << ",";
            paramsFile << "'rmsSize':" << rmsSize << ",";
            paramsFile << "'rmsRelativeHop':" << rmsRelativeHop << ",";
            paramsFile << "'ETCSymbolCount':" << ETCSymbolCount << ",";
            paramsFile << "'ETCRange':" << ETCRange << ",";
            paramsFile << "'ETCRelativeHop':" << ETCRelativeHop << ",";
            paramsFile << "'channelGains0':" << channelGains[0] << ",";
            paramsFile << "'channelGains1':" << channelGains[1] << ",";
            paramsFile << "'channelGains2':" << channelGains[2] << ",";
            paramsFile << "'channelGains3':" << channelGains[3] << ",";
            paramsFile << "'dampingCurve':" << dampingCurve << ",";
            paramsFile << "'damping':" << damping << ",";
            paramsFile << "'dampingResponseFrequency':" << dampingResponseFrequency << ",";
            paramsFile << "'dynCCWindowSize':" << dynCCWindowSize << ",";
            paramsFile << "'dynCCSize':" << dynCCSize << ",";
            paramsFile << "'dynCCStep':" << dynCCStep << ",";
            paramsFile << "'dynCCPastSize':" << dynCCPastSize << ",";
            paramsFile << "'dynCCPastSize':" << dynCCPastSize << ",";
            paramsFile << "'verbMix':" << verbMix << ",";
//            paramsFile << "'verbAbsorbtion':" << verbAbsorbtion << ",";
//            paramsFile << "'verbRoomSize':" << verbRoomSize << ",";
            paramsFile << "}";
            paramsFile.close();
            
        }else{
            isRecording=0;
            wwcv.notify_one();

//            sf_close(wavfile);
        };
    });
    gui->addLabel(">> Analysis");
    auto rmsModeToggle = gui->addToggle("RMS rel/abs", rmsMode);
    rmsModeToggle->onToggleEvent([&](ofxDatGuiToggleEvent e) {
        rmsMode = e.checked;
    });

    auto headroomSlider = gui->addSlider("Headroom size: ", 0.01, 1.5, maxHeadroom);
    headroomSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        maxHeadroom = e.value;
    });
    
    auto setupRMS = [&]() {
        rmsHop = (int)(rmsSize * rmsRelativeHop);
        rmsCounter=0;
    };

    auto rmsSizeSlider = gui->addSlider("RMS window size: ", 8, 512, rmsSize);
    rmsSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        rmsSize = e.value;
//        setupRMS();
        rmsHop = (int)(rmsSize * rmsRelativeHop);
        rmsCounter=0;
    });

    auto rmsHopSizeSlider = gui->addSlider("RMS hop size: ", 0.05, 1.0, rmsRelativeHop);
    rmsHopSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        rmsRelativeHop = e.value;
//        setupRMS();
        rmsHop = (int)(rmsSize * rmsRelativeHop);
        rmsCounter=0;
    });

    auto symbolSlider = gui->addSlider("Symbol count: ", 2, 64, ETCSymbolCount);
    
//    symbolSlider->bind(ETCSymbolCount);
    symbolSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        ETCSymbolCount = e.value;
    });
    
    auto etcRangeSlider = gui->addSlider("ETC size: ", 10, 200, ETCRange);
    etcRangeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        ETCRange = e.value;
        reCalcETCParams();
    });

    auto etcHopSlider = gui->addSlider("ETC hop size: ", 0, 1.0, ETCRelativeHop);
    etcHopSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        ETCRelativeHop = e.value;
        reCalcETCParams();
    });

    gui->addLabel(">> Sound");
    gainSliders[0] = gui->addSlider("Gain ch 1", 0, 4.0, channelGains[0]);
    gainSliders[0]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        channelGains[0] = e.value;
    });
    gainSliders[1] = gui->addSlider("Gain ch 2", 0, 4.0, channelGains[1]);
    gainSliders[1]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        channelGains[1] = e.value;
    });
    gainSliders[2] = gui->addSlider("Gain ch 3", 0, 40.0, channelGains[2]);
    gainSliders[2]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        channelGains[2] = e.value;
    });
    gainSliders[3] = gui->addSlider("Gain ch 4", 0, 40.0, channelGains[3]);
    gainSliders[3]->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        channelGains[3] = e.value;
    });
    
    auto dampingCurveSlider = gui->addSlider("Damping Curve", 0.5, 3.0, dampingCurve);
    dampingCurveSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dampingCurve = e.value;
    });
    
    auto dampingSlider = gui->addSlider("Damping", 0, 10.0, damping);
    dampingSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        damping = e.value;
    });

    auto dampingResponseSlider = gui->addSlider("Damping Response", 0.001, 300.0, dampingResponseFrequency);
    dampingResponseSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dampingResponseFrequency = e.value;
        dampingResponse.set(maxiBiquad::LOWPASS, dampingResponseFrequency, 0.1, 1);
    });

    
    auto dynCCWindowSlider = gui->addSlider("DynCC Window", 10, 200, dynCCWindowSize);
    dynCCWindowSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dynCCWindowSize = e.value;
    });
    auto dynCCSizeSlider = gui->addSlider("DynCC Size", 0.0, 1.0, dynCCSize);
    dynCCSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dynCCSize = e.value;
    });
    auto dynCCStepSlider = gui->addSlider("DynCC Step", 0.0, 1.0, dynCCStep);
    dynCCStepSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dynCCStep = e.value;
    });
    auto dynCCPastSizeSlider = gui->addSlider("DynCC Past", 0.0, 1.0, dynCCPastSize);
    dynCCPastSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dynCCPastSize = e.value;
    });
    

    auto eq1FreqSlider = gui->addSlider("EQ 1 Freq", 0.0, 1.0, dynCCPastSize);
    dynCCPastSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        dynCCPastSize = e.value;
    });

    auto verbMixSlider = gui->addSlider("Verb Mix", 0.0, 1.0, verbMix);
    verbMixSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
        verbMix = e.value;
    });
//    auto verbAbsorptionSlider = gui->addSlider("Verb Absorb", 0.0, 1.0, verbAbsorbtion);
//    verbAbsorptionSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        verbAbsorbtion = e.value;
//    });
//    auto verbRoomSizeSlider = gui->addSlider("Verb Room Size", 0.0, 1.0, verbRoomSize);
//    verbRoomSizeSlider->onSliderEvent([&](ofxDatGuiSliderEvent e) {
//        verbRoomSize = e.value;
//    });

    dampingResponse.set(maxiBiquad::LOWPASS, dampingResponseFrequency, 0.1, 1);
    
    
    audioInRecBuffer.reserve(3 * 44100 * 60 * 15);
    wavWriter = std::thread([&](){
        cout<<"Wav Writer thread waiting";
        while(true) {
            std::unique_lock<std::mutex> lck(wwmtx);
            wwcv.wait(lck);
            sf_count_t n = sf_write_float(wavfile, audioInRecBuffer.data(), audioInRecBuffer.size());
            audioInRecBuffer.clear();
//            if(isRecording) {
//                std::unique_lock<std::mutex> lck(wwmtx);
//                diskBuffer = audioInRecBuffer;
//                wwcv.notify_one();
//                audioInRecBuffer.clear();
//            }
            sf_close(wavfile);

            cout << n << endl;
        }
    });
    wavWriter.detach();

    ofSoundStreamStart();

}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(255,255,255);
    ofSetColor(100,100,100);
    ofFill();
    ofDrawRectangle(10,ofGetHeight()*0.9,50, -ofGetHeight()*0.8*inputETC);
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
        ofDrawLine(xoff+(i*step)-1, ybase - (ofGetHeight()*0.9*(rmsbuf[i-1] / maxHeadroom)), xoff+(i*step), ybase - (ofGetHeight()*0.2*(rmsbuf[i]/maxHeadroom)));
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
}


void ofApp::audioIn(ofSoundBuffer & buffer) {
    
    //mixdown to mono
//    masterGain = dampingResponse.play(1.0 - max(0.0,(ETCDiff * damping)));
    masterGain = dampingResponse.play(1.0 - min(1.0,pow(ETCDiff * damping, dampingCurve)));
    masterGainRingBuf.push(masterGain);
    for(size_t i=0; i < buffer.getNumFrames(); i++) {
        float mag=0;
        for (size_t j=0; j < buffer.getNumChannels(); j++) {
            mag += (buffer[(i*buffer.getNumChannels()) + j] * channelGains[j]);
        }
        mag = (mag / buffer.getNumChannels());
//        mag = mag + (verb.play(mag, verbRoomSize, verbAbsorbtion) * verbMix);
        mag = mag + (dverb.playStereo(mag)[0] * verbMix);
        audioInBuffer[i] = mag * masterGain;
        sigRingBuf.push(mag);
        if (rmsCounter++ == rmsHop) {
            auto rmsWindow = sigRingBuf.getBuffer(rmsSize);
            double rms=0;
            for(int j=0; j < rmsSize; j++) {
                rms += rmsWindow[j] * rmsWindow[j];
            }
            rms = sqrt(rms / rmsSize);
            inputRMS = rms;
            rmsRingBuf.push(inputRMS);
            rmsCounter=0;
            
            if (ETCStepCount==0) {
                auto rmsbuf = rmsRingBuf.getBuffer(ETCRange);
                ivec rmsSymBuf(rmsbuf.size());
                double rmsBufMax = rmsbuf.max();
//                double rmsMax = std::max(maxHeadroom, rmsBufMax);
                double rmsMax = rmsMode ? rmsbuf.max() : std::max(maxHeadroom, rmsBufMax);
                for(size_t i=0; i < rmsbuf.size(); i++) {
                    rmsSymBuf[i] = static_cast<sword>(rmsbuf[i]/rmsMax * ETCSymbolCount);
                }
                
                
                double rmsSum=0;
                for(int i=0; i < rmsbuf.size(); i++) {
                    rmsSum += rmsbuf[i];
                }
                meanRMS =  rmsSum / rmsbuf.size();
                
                inputETC = ETC::calc(rmsSymBuf);
                //        ETCDiff = (meanRMS/recentMaxRMS) - inputETC;
//                ETCDiff = (meanRMS/rmsMax) - inputETC;
                ETCDiff = (1.0-inputETC) * (meanRMS / rmsMax);
                etcDiffRingBuf.push(ETCDiff);
                
                //        auto rmsbufLong = rmsRingBuf.getBuffer(400);
                //        recentMaxRMS=rmsbufLong.max();
                
                //Dyn CC
                rmsbuf = rmsRingBuf.getBuffer(dynCCWindowSize);
                ivec rmsDynCCSymBuf(rmsbuf.size());
                rmsBufMax = rmsbuf.max();
                rmsMax = rmsMode ? rmsbuf.max() : std::max(maxHeadroom, rmsBufMax);
                for(size_t i=0; i < rmsbuf.size(); i++) {
                    rmsDynCCSymBuf[i] = static_cast<sword>(rmsbuf[i]/rmsMax * ETCSymbolCount);
                }
                
                double dynCCRange = dynCCWindowSize * dynCCSize;
                int dynCCxpast = dynCCRange * dynCCPastSize;
                if (dynCCxpast < 2) dynCCxpast = 2;
                int dynCCdx = dynCCRange - dynCCxpast;
                int dynCCStepSize = (dynCCxpast + dynCCdx) * dynCCStep;
                dynCCStepSize = max(2, dynCCStepSize);
                dynCC = CCC::dynamicCC(rmsDynCCSymBuf, dynCCdx, dynCCxpast, dynCCStepSize, CCC::SINGLETHREAD);
                dynccRingBuf.push(dynCC);
            }
            ETCStepCount++;
            if (ETCStepCount==ETCHopSize) ETCStepCount=0;

        }
//        sf_count_t n;
//        float rec[3];
//        rec[0] = mag;
//        rec[1] = inputETC;
//        rec[2] = ETCDiff;
        
        if (isRecording) {
//            n = sf_write_float(wavfile, &rec[0], 3);
            audioInRecBuffer.push_back(mag);
            audioInRecBuffer.push_back(inputETC);
            audioInRecBuffer.push_back(ETCDiff);
        }

    }
//    if(isRecording) {
//        std::unique_lock<std::mutex> lck(wwmtx);
//        diskBuffer = audioInRecBuffer;
//        wwcv.notify_one();
//        audioInRecBuffer.clear();
//    }
}

void ofApp::audioOut(ofSoundBuffer & buffer) {
    double amp=1;
    if (ETCDiff >0) {
//        amp -= (ETCDiff * 2);
//        cout << ETCDiff;
    }
    for(size_t i=0; i < buffer.getNumFrames(); i++) {
        //feedback!!!!
        buffer[buffer.getNumChannels() * i] =  audioInBuffer[i] * amp;
        buffer[(buffer.getNumChannels() * i) + 1] =  audioInBuffer[i] * amp;
    }
}

//armaRingBuf<sword> rbuf(5);

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
