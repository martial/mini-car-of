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
    VideoMode stringToVideoMode(const std::string& modeStr);

    ofVideoPlayer videos[4];
    std::string videoFiles[4];

    bool night;
    bool country;
    float speed;
    float targetSpeed;  // for smooth speed adjustment
    int currentVideoIndex;

    ofSerial serial;
    float simulatedTime;
    float lastTime;
    float lastSaveTime; // for periodic saving

    VideoMode videoMode;
    float speedScale;
    float smoothScale;
    std::string usbAddress;

    ofxPanel gui;
    ofParameter<float> guiSpeedScale;
    ofParameter<float> guiSmoothScale;
    ofParameter<string> guiUsbAddress;
    ofParameter<int> guiVideoMode;
    bool guiVisible;
};

void ofApp::setup() {
    ofSetFullscreen(true);
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);

    loadConfig();

    // Load videos
    for (int i = 0; i < 4; ++i) {
        videos[i].load(videoFiles[i]);
        videos[i].setLoopState(OF_LOOP_NORMAL);
        videos[i].play();
    }

    speed = 0.0f;
    targetSpeed = 0.0f;
    currentVideoIndex = 0;
    simulatedTime = 0;
    lastTime = ofGetElapsedTimef();
    lastSaveTime = ofGetElapsedTimef();

    // Serial setup
    serial.setup(usbAddress, 9600);

    // GUI setup
    gui.setup();
    gui.add(guiSpeedScale.set("Speed Scale", speedScale, 0.0, 3.0));
    gui.add(guiSmoothScale.set("Smooth Scale", smoothScale, 0.0, 1.0));
    gui.add(guiUsbAddress.set("USB Address", usbAddress));
    gui.add(guiVideoMode.set("Video Mode", videoMode, 0, 2));
    guiVisible = false;
}

void ofApp::update() {
    float currentTime = ofGetElapsedTimef();
    float elapsed = currentTime - lastTime;
    lastTime = currentTime;

    // Smoothly adjust speed
    float currSmoothScale = smoothScale;
    if (speed < 0.25) {
        speed = targetSpeed;
    } else {
        speed += (targetSpeed - speed) * currSmoothScale * elapsed;
    }

    if (speed > 3.0)
        speed = 3.0;

    videos[currentVideoIndex].update();
    videos[currentVideoIndex].setSpeed(speed);

    if (serial.available() > 0) {
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
    }
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_ESC) {
        saveConfig();
        ofExit();
    } else if (key == 'g' || key == 'G') {
        guiVisible = !guiVisible;
    }
}

void ofApp::computeSpeed(uint8_t byte) {
    targetSpeed = ofMap(byte, 0, 255, 0.0f, 1.8f * speedScale, true);
}

void ofApp::processByte(uint8_t byte) {
    if (byte == 3 || byte == 4) {
        night = (byte == 4);
    } else if (byte == 5 || byte == 6) {
        country = (byte == 6);
    } else if (byte >= 7) {
        computeSpeed(byte - 7);
        //ofLogNotice("byte Total ") << byte;

        ofLogNotice("byte ") << byte-7;
    }

    int newIndex = (country ? 2 : 0) + (night ? 1 : 0);

    if (newIndex != currentVideoIndex) {
        currentVideoIndex = newIndex;
    }
}

void ofApp::loadConfig() {
    ofJson config = ofLoadJson("config.json");

    // Read video file URLs
    for (int i = 0; i < 4; ++i) {
        videoFiles[i] = config["videoFiles"][i];
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
