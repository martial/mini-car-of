#pragma once

#include "ofMain.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    void computeSpeed(uint8_t byte);
    void processByte(uint8_t byte);

    ofVideoPlayer videos[4];
    bool night;
    bool country;
    float speed;
    int currentVideoIndex;

    ofSerial serial;
    float simulatedTime;
    float lastTime;

    bool adjustVideoSize;
};

void ofApp::setup() {
    ofSetFullscreen(true);
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);

    // Load videos
    videos[0].load("BigCar_CityDay.mp4");
    videos[1].load("BigCar_CityNight.mov");
    videos[2].load("BigCar_CountryDay.mov");
    videos[3].load("BigCar_CountryNight.mov");
    
    for (auto & video : videos) {
        video.setLoopState(OF_LOOP_NORMAL);
        video.play();
    }

    night = true;
    country = true;
    speed = 0.0f;
    currentVideoIndex = 0;
    simulatedTime = 0;
    lastTime = ofGetElapsedTimef();
    
    // Serial setup
    std::string serialPort = "/dev/tty.usbmodem21201";  // Update as per your requirements
    serial.setup(serialPort, 9600);

    adjustVideoSize = false;  // Set this based on your needs
}

void ofApp::update() {
    float currentTime = ofGetElapsedTimef();
    float elapsed = currentTime - lastTime;
    lastTime = currentTime;
    
    if (speed > 0) {
        simulatedTime += elapsed * speed;
    }
    
    ofLogNotice("speed ") << speed;
    
    //videos[currentVideoIndex].setPosition(fmod(simulatedTime, videos[currentVideoIndex].getDuration()) / videos[currentVideoIndex].getDuration());
    videos[currentVideoIndex].update();
    
    videos[currentVideoIndex].setSpeed(speed);

    if (serial.available() > 0) {
        uint8_t byte;
        serial.readBytes(&byte, 1);
        processByte(byte);
    }
}

void ofApp::draw() {
    if (adjustVideoSize) {
        videos[currentVideoIndex].draw(0, 0, ofGetWidth(), ofGetHeight());
    } else {
        videos[currentVideoIndex].draw(0, 0);
    }

    ofDrawBitmapStringHighlight("Speed: " + ofToString(speed), 10, 20);
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_ESC) {
        ofExit();
    }
}

void ofApp::computeSpeed(uint8_t byte) {
    float scale = 4.0f;
    speed = ofMap(byte, 0, 255, 0.0f, 1.8f * scale, true);
}

void ofApp::processByte(uint8_t byte) {
    if (byte == 3 || byte == 4) {
        night = (byte == 4);
    } else if (byte == 5 || byte == 6) {
        country = (byte == 6);
    } else if (byte >= 7) {
        computeSpeed(byte - 7);
    }
    int newIndex = 0;
    if (!country && !night) newIndex = 0;
    else if (!country && night) newIndex = 1;
    else if (country && !night) newIndex = 2;
    else if (country && night) newIndex = 3;

    if (newIndex != currentVideoIndex) {
        currentVideoIndex = newIndex;
    }
}
