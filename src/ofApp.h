#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofJson.h"

enum VideoMode {
    ORIGINAL,
    FIT_SCREEN,
    FILL_SCREEN
};

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);

    void computeSpeed(uint8_t byte);
    void processByte(uint8_t byte);
    void loadConfig();
    void saveConfig();
    void openSerial();
    VideoMode stringToVideoMode(const std::string& modeStr);

    ofVideoPlayer videos[4];
    std::string videoFiles[4];
    
    ofSoundPlayer sounds[4];;
    std::string soundFiles[4];

    
    bool night;
    bool country;
    float speed;
    float targetSpeed;  // for smooth speed adjustment
    int currentVideoIndex;
    
    int speedByte;

    ofSerial serial;
    bool serialOk = false;
    std::string serialStatus = "serial: not initialized";
    float simulatedTime;
    float lastTime;
    float lastSaveTime; // for periodic saving
    float lastAppliedSpeed = -1.0f;
    float lastSetSpeedTime = 0.0f; // rate-limit setSpeed calls; DirectShow stalls on rapid changes
    bool pendingSetSpeed = false;

    // simulated-serial input (keyboard + GUI slider, bypasses real serial)
    int simSpeedByte = 128;
    int lastSimByte = -1;
    bool simMode = false;

    VideoMode videoMode;
    float speedScale;
    float smoothScale;
    std::string usbAddress;

    ofxPanel gui;
    ofParameter<float> guiSpeedScale;
    ofParameter<float> guiSmoothScale;
    ofParameter<string> guiUsbAddress;
    ofParameter<int> guiVideoMode;
    ofParameter<bool> guiInvert;
    ofParameter<bool> guiSimMode;
    ofParameter<int> guiSimByte;

    bool guiVisible;

    bool isFullScreen = true;
    bool invert = true;
    
    std::string previousUsbAddress;
};

void ofApp::setup() {
    ofSetFullscreen(true);
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);
    ofHideCursor();

    loadConfig();

    // Load videos
    for (int i = 0; i < 4; ++i) {
        videos[i].load(videoFiles[i]);
        videos[i].setLoopState(OF_LOOP_NORMAL);
        
        sounds[i].load(soundFiles[i]);
        sounds[i].setLoop(true);
        sounds[i].stop();
        
    }
    videos[currentVideoIndex].play();
    sounds[currentVideoIndex].play();

    // Don't setSpeed(0.0) at startup — some DirectShow filters (e.g. MJPEG)
    // treat put_Rate(0) as a stop they can't recover from. Let play() start
    // at rate 1.0 naturally; the inertia lerp will take over next frame.
    speed = 1.0f;
    targetSpeed = 1.0f;
    currentVideoIndex = 0;
    simulatedTime = 0;
    lastTime = ofGetElapsedTimef();
    lastSaveTime = ofGetElapsedTimef();

    // Serial setup
    openSerial();

    // GUI setup
    gui.setup();
    gui.add(guiSpeedScale.set("Speed Scale", speedScale, 0.0, 3.0));
    gui.add(guiSmoothScale.set("Inertia", smoothScale, 0.0, 0.99));
    gui.add(guiUsbAddress.set("USB Address", usbAddress));
    gui.add(guiVideoMode.set("Video Mode", videoMode, 0, 2));
    gui.add(guiInvert.set("Invert", invert, 0, 1));
    gui.add(guiSimMode.set("Sim Mode", false));
    gui.add(guiSimByte.set("Sim Byte", 128, 0, 248));

    
    guiVisible = false;
    previousUsbAddress = usbAddress;
    
}

void ofApp::update() {
    float currentTime = ofGetElapsedTimef();
    float elapsed = currentTime - lastTime;
    lastTime = currentTime;

    // Smoothly adjust speed
    float currSmoothScale = smoothScale;
    

    
    speed += (targetSpeed - speed) * ofMap((1.0 - currSmoothScale), 0.0, 1.0, 0.0, 0.25);

    float currSpeed = speed;
    if (currSpeed > 3.0)
        currSpeed = 3.0;

   if (currSpeed <0.05)
      currSpeed = 0.0;

    videos[currentVideoIndex].update();
    videos[currentVideoIndex].setSpeed(currSpeed);
    // sounds[currentVideoIndex].setSpeed(currSpeed);  // temporarily disabled to rule out empty-sound interference
    
    //sound.update();
    //sound.setSpeed(speed);

    simMode = guiSimMode;
    if (simMode) {
        int b = guiSimByte;
        if (b != lastSimByte) {
            processByte((uint8_t)(b + 7));
            simSpeedByte = b;
            lastSimByte = b;
        }
    } else if (serialOk && serial.available() > 0) {
        uint8_t byte;
        serial.readBytes(&byte, 1);
        processByte(byte);
    }

    // Save config every 5 seconds
    if (currentTime - lastSaveTime >= 5.0f) {
        saveConfig();
        lastSaveTime = currentTime;
    }

    // Update variables from GUI
    speedScale = guiSpeedScale;
    smoothScale = guiSmoothScale;
    usbAddress = guiUsbAddress;
    videoMode = static_cast<VideoMode>(guiVideoMode.get());
    invert = guiInvert;
    
    if (usbAddress != previousUsbAddress) {
        openSerial();
        previousUsbAddress = usbAddress;
    }
}

void ofApp::draw() {
    float videoWidth = videos[currentVideoIndex].getWidth();
    float videoHeight = videos[currentVideoIndex].getHeight();
    float screenWidth = ofGetWidth();
    float screenHeight = ofGetHeight();
    float videoAspect = videoWidth / videoHeight;
    float screenAspect = screenWidth / screenHeight;

    if (videoMode == FIT_SCREEN) {
        if (videoAspect > screenAspect) {
            float newHeight = screenWidth / videoAspect;
            videos[currentVideoIndex].draw(0, (screenHeight - newHeight) / 2, screenWidth, newHeight);
        } else {
            float newWidth = screenHeight * videoAspect;
            videos[currentVideoIndex].draw((screenWidth - newWidth) / 2, 0, newWidth, screenHeight);
        }
    } else if (videoMode == FILL_SCREEN) {
        if (videoAspect > screenAspect) {
            float newWidth = screenHeight * videoAspect;
            videos[currentVideoIndex].draw((screenWidth - newWidth) / 2, 0, newWidth, screenHeight);
        } else {
            float newHeight = screenWidth / videoAspect;
            videos[currentVideoIndex].draw(0, (screenHeight - newHeight) / 2, screenWidth, newHeight);
        }
    } else { // ORIGINAL
        int x = (screenWidth - videoWidth) / 2;
        int y = (screenHeight - videoHeight) / 2;
        videos[currentVideoIndex].draw(x, y, videoWidth, videoHeight);
    }

    //ofDrawBitmapStringHighlight("Speed: " + ofToString(speed), 10, 20);

    if (guiVisible) {
        gui.draw();
        ofDrawBitmapStringHighlight("Potentiometer: " + ofToString(speedByte), 225, 20);
        ofDrawBitmapStringHighlight("FPS: " + ofToString(ceil(ofGetFrameRate())), 225, 40);
       // ofDrawBitmapStringHighlight("Video Index: " + ofToString(currentVideoIndex), 225, 60);
        ofDrawBitmapStringHighlight("Video URL: " + ofToString(videoFiles[currentVideoIndex]), 225, 60);
        ofDrawBitmapStringHighlight("Video Size: " + ofToString(videos[currentVideoIndex].getWidth()) + "x" + ofToString(videos[currentVideoIndex].getHeight()), 225, 80);

        ofDrawBitmapStringHighlight("Switches: A:" + ofToString(night) + " B:" + ofToString(country), 225, 100);
        ofDrawBitmapStringHighlight("Speed: " + ofToString(speed, 3) + "  target: " + ofToString(targetSpeed, 3) + "  simByte: " + ofToString((int)guiSimByte) + "  simMode: " + ofToString((bool)guiSimMode), 225, 120);

        ofColor serialColor = serialOk ? ofColor(0, 200, 0) : ofColor(220, 0, 0);
        ofDrawBitmapStringHighlight(serialStatus, 225, 140, serialColor, ofColor::white);
        if (simMode) {
            ofDrawBitmapStringHighlight("SIM MODE (keys: up/down, left/right, pgup/pgdn, 0/9)", 225, 160, ofColor(200, 180, 0), ofColor::black);
        }

        //ofDrawBitmapStringHighlight("speedByte: " + ofToString(speedByte), 10, 20);

    }
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_ESC) {
        saveConfig();
        ofExit();
    } else if (key == 'g' || key == 'G') {
        guiVisible = !guiVisible;
        if (guiVisible) {
            ofShowCursor();
        }
        else {
            ofHideCursor();
        }
    }
    else if (key == 'f' || key == 'F') {
        ofToggleFullscreen();
        isFullScreen = !isFullScreen;
        if (isFullScreen) {
            ofHideCursor();
        }
        else {
            ofShowCursor();
        }
    }
    else if (simMode) {
        // Sim-mode input: fabricate the same bytes the firmware would send
        // so processByte() — the real decode path — stays exercised.
        if (key == OF_KEY_UP || key == OF_KEY_DOWN) {
            int step = (key == OF_KEY_UP) ? 8 : -8;
            simSpeedByte = ofClamp(simSpeedByte + step, 0, 248);
            guiSimByte = simSpeedByte;
            processByte((uint8_t)(simSpeedByte + 7));
            lastSimByte = simSpeedByte;
        } else if (key == OF_KEY_LEFT) {
            processByte(3); // country off
        } else if (key == OF_KEY_RIGHT) {
            processByte(4); // country on
        } else if (key == OF_KEY_PAGE_UP) {
            processByte(5); // night off
        } else if (key == OF_KEY_PAGE_DOWN) {
            processByte(6); // night on
        } else if (key == '0') {
            simSpeedByte = 0;
            guiSimByte = 0;
            processByte(7);
            lastSimByte = 0;
        } else if (key == '9') {
            simSpeedByte = 248;
            guiSimByte = 248;
            processByte((uint8_t)(248 + 7));
            lastSimByte = 248;
        }
    }
}

void ofApp::computeSpeed(uint8_t byte) {

    if(!invert) {
        targetSpeed = ofMap(byte, 0, 255, 0.0f, 1.8f * speedScale, true);
    } else {
        targetSpeed = ofMap(byte, 240, 0.0, 0.0f, 1.8f * speedScale, true);

    }
}

void ofApp::processByte(uint8_t byte) {
    if (byte == 3 || byte == 4) {
        country = (byte == 4);
    } else if (byte == 5 || byte == 6) {
        night = (byte == 6);
    } else if (byte >= 7) {
        computeSpeed(byte - 7);
        speedByte = byte - 7;
    }

    int newIndex = (country ? 2 : 0) + (night ? 1 : 0);

    if (newIndex != currentVideoIndex) {
        currentVideoIndex = newIndex;
        
        for (int i = 0; i < 4; ++i) {
            videos[i].stop();
            sounds[i].stop();
            
            videos[i].setPosition(0.0);
            sounds[i].setPosition(0.0);
        }
        
        videos[currentVideoIndex].play();
        sounds[currentVideoIndex].play();
    }
}

void ofApp::loadConfig() {
    ofJson config = ofLoadJson("config.json");

    // Read video file URLs
    for (int i = 0; i < 4; ++i) {
        videoFiles[i] = config["videoFiles"][i];
    }
    
    for (int i = 0; i < 4; ++i) {
        soundFiles[i] = config["soundFiles"][i];
    }

    // Read video mode
    std::string modeStr = config["videoMode"];
    videoMode = stringToVideoMode(modeStr);

    // Read speed scale
    speedScale = config["speedScale"];

    // Read smooth scale
    smoothScale = config["smoothScale"];

    // Read USB address
    usbAddress = config["usbAddress"];
    
    invert = config["invert"];


    // Read current settings
    night = config["nightMode"];
    country = config["countryMode"];
}

void ofApp::saveConfig() {
    ofJson config;

    // Save video file URLs
    for (int i = 0; i < 4; ++i) {
        config["videoFiles"][i] = videoFiles[i];
    }
    
    for (int i = 0; i < 4; ++i) {
        config["soundFiles"][i] = soundFiles[i];
    }

    // Save video mode
    std::string modeStr;
    switch (videoMode) {
        case ORIGINAL:
            modeStr = "original";
            break;
        case FIT_SCREEN:
            modeStr = "fitScreen";
            break;
        case FILL_SCREEN:
            modeStr = "fillScreen";
            break;
    }
    config["videoMode"] = modeStr;

    // Save speed scale
    config["speedScale"] = speedScale;

    // Save smooth scale
    config["smoothScale"] = smoothScale;

    // Save USB address
    config["usbAddress"] = usbAddress;
    config["invert"] = invert;

    // Save current settings
    config["nightMode"] = night;
    config["countryMode"] = country;

    ofSaveJson("config.json", config);
}

VideoMode ofApp::stringToVideoMode(const std::string& modeStr) {
    if (modeStr == "fitScreen") return FIT_SCREEN;
    if (modeStr == "fillScreen") return FILL_SCREEN;
    return ORIGINAL;
}

void ofApp::openSerial() {
    serial.close();
    serialOk = serial.setup(usbAddress, 9600);
    if (serialOk) {
        serialStatus = "serial OK: " + usbAddress;
    } else {
        serialStatus = "serial ERROR: could not open " + usbAddress;
    }
    ofLogNotice("openSerial") << serialStatus;
}
