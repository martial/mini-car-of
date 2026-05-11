#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofJson.h"

#include <filesystem>
#include <system_error>

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

// Serial byte protocol: firmware sends one byte at a time at 9600 baud.
static constexpr uint8_t SERIAL_COUNTRY_OFF = 3;
static constexpr uint8_t SERIAL_COUNTRY_ON  = 4;
static constexpr uint8_t SERIAL_NIGHT_OFF   = 5;
static constexpr uint8_t SERIAL_NIGHT_ON    = 6;
static constexpr uint8_t SERIAL_SPEED_BASE  = 7;    // speed byte = raw - base
static constexpr int     SERIAL_SPEED_RANGE = 240;  // 0..240 maps to the speed output range

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
    void exit() override;

    void computeSpeed(uint8_t byte);
    void processByte(uint8_t byte);
    void loadConfig();
    void saveConfig();
    void openSerial();
    void drawScaledVideo(bool fill);
    VideoMode stringToVideoMode(const std::string& modeStr);

    ofVideoPlayer videos[4];
    std::string videoFiles[4];

    ofSoundPlayer sounds[4];
    std::string soundFiles[4];

    bool night = false;
    bool country = false;
    float speed = 1.0f;
    float targetSpeed = 1.0f;
    int currentVideoIndex = 0;
    int speedByte = 0;

    ofSerial serial;
    bool serialOk = false;
    std::string serialStatus = "serial: not initialized";
    float lastTime = 0.0f;
    float lastSaveTime = 0.0f;
    bool configDirty = false;
    // True iff the on-disk config.json was last seen in a valid state — either
    // loaded successfully at startup or written successfully by saveConfig().
    // When false, saveConfig skips the rotate-to-.bak step so a corrupted
    // config.json cannot clobber the last known-good .bak.
    bool currentConfigIsValid = false;

    // simulated-serial input (keyboard + GUI slider, bypasses real serial)
    int simSpeedByte = 128;
    int lastSimByte = -1;
    bool simMode = false;

    VideoMode videoMode = FILL_SCREEN;
    float speedScale = 1.0f;
    float smoothScale = 0.25f;
    std::string usbAddress;

    ofxPanel gui;
    ofParameter<float> guiSpeedScale;
    ofParameter<float> guiSmoothScale;
    ofParameter<string> guiUsbAddress;
    ofParameter<int> guiVideoMode;
    ofParameter<bool> guiInvert;
    ofParameter<bool> guiSimMode;
    ofParameter<int> guiSimByte;

    bool guiVisible = false;
    bool isFullScreen = true;
    bool invert = true;

    std::string previousUsbAddress;
};

void ofApp::setup() {
    ofLogToFile("logs/mini-car-of.log", true);

    ofSetFullscreen(true);
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);
    ofHideCursor();

    loadConfig();

    // Pick the starting video from the persisted night/country flags so the
    // first frame matches the last saved state.
    currentVideoIndex = (country ? 2 : 0) + (night ? 1 : 0);
    lastTime = ofGetElapsedTimef();
    lastSaveTime = ofGetElapsedTimef();

    for (int i = 0; i < 4; ++i) {
        videos[i].load(videoFiles[i]);
        videos[i].setLoopState(OF_LOOP_NORMAL);
        sounds[i].load(soundFiles[i]);
        sounds[i].setLoop(true);
        sounds[i].stop();
    }
    videos[currentVideoIndex].play();
    sounds[currentVideoIndex].play();

    // Leave initial speed at 1.0; avoid setSpeed(0.0) here — some DirectShow
    // filters treat put_Rate(0) as a stop they can't recover from.

    openSerial();

    gui.setup();
    gui.add(guiSpeedScale.set("Speed Scale", speedScale, 0.0, 3.0));
    gui.add(guiSmoothScale.set("Inertia", smoothScale, 0.0, 0.99));
    gui.add(guiUsbAddress.set("USB Address", usbAddress));
    gui.add(guiVideoMode.set("Video Mode", videoMode, 0, 2));
    gui.add(guiInvert.set("Invert", invert, 0, 1));
    gui.add(guiSimMode.set("Sim Mode", false));
    gui.add(guiSimByte.set("Sim Byte", 128, 0, SERIAL_SPEED_RANGE + 8));

    previousUsbAddress = usbAddress;
}

void ofApp::update() {
    float currentTime = ofGetElapsedTimef();
    lastTime = currentTime;

    speed += (targetSpeed - speed) * ofMap((1.0 - smoothScale), 0.0, 1.0, 0.0, 0.25);

    float currSpeed = ofClamp(speed, 0.0f, 3.0f);
    if (currSpeed < 0.05f) currSpeed = 0.0f;

    videos[currentVideoIndex].update();
    videos[currentVideoIndex].setSpeed(currSpeed);
    sounds[currentVideoIndex].setSpeed(currSpeed);

    simMode = guiSimMode;
    if (simMode) {
        int b = guiSimByte;
        if (b != lastSimByte) {
            processByte((uint8_t)(b + SERIAL_SPEED_BASE));
            simSpeedByte = b;
            lastSimByte = b;
        }
    } else if (serialOk && serial.available() > 0) {
        uint8_t byte;
        serial.readBytes(&byte, 1);
        processByte(byte);
    }

    if (configDirty && currentTime - lastSaveTime >= 5.0f) {
        saveConfig();
        lastSaveTime = currentTime;
        configDirty = false;
    }

    if (speedScale  != guiSpeedScale.get())                    { speedScale  = guiSpeedScale;                          configDirty = true; }
    if (smoothScale != guiSmoothScale.get())                   { smoothScale = guiSmoothScale;                         configDirty = true; }
    if (usbAddress  != guiUsbAddress.get())                    { usbAddress  = guiUsbAddress;                          configDirty = true; }
    if (videoMode   != static_cast<VideoMode>(guiVideoMode.get())) { videoMode = static_cast<VideoMode>(guiVideoMode.get()); configDirty = true; }
    if (invert      != guiInvert.get())                        { invert      = guiInvert;                              configDirty = true; }

    if (usbAddress != previousUsbAddress) {
        openSerial();
        previousUsbAddress = usbAddress;
    }
}

void ofApp::drawScaledVideo(bool fill) {
    float vw = videos[currentVideoIndex].getWidth();
    float vh = videos[currentVideoIndex].getHeight();
    if (vw <= 0.0f || vh <= 0.0f) return;

    float sw = ofGetWidth();
    float sh = ofGetHeight();
    float videoAspect = vw / vh;
    float screenAspect = sw / sh;

    bool useWidth = fill ? (videoAspect < screenAspect) : (videoAspect > screenAspect);
    float w = useWidth ? sw : sh * videoAspect;
    float h = useWidth ? sw / videoAspect : sh;
    videos[currentVideoIndex].draw((sw - w) / 2.0f, (sh - h) / 2.0f, w, h);
}

void ofApp::draw() {
    float vw = videos[currentVideoIndex].getWidth();
    float vh = videos[currentVideoIndex].getHeight();

    if (vw <= 0.0f || vh <= 0.0f) {
        // Video failed to load — show the missing filename prominently so
        // on-site operators see the fault without needing the GUI panel.
        ofDrawBitmapStringHighlight("Missing video: " + videoFiles[currentVideoIndex],
                                    20, 20, ofColor(200, 0, 0), ofColor::white);
    } else if (videoMode == FIT_SCREEN) {
        drawScaledVideo(false);
    } else if (videoMode == FILL_SCREEN) {
        drawScaledVideo(true);
    } else { // ORIGINAL
        videos[currentVideoIndex].draw((ofGetWidth() - vw) / 2, (ofGetHeight() - vh) / 2, vw, vh);
    }

    if (guiVisible) {
        gui.draw();
        ofDrawBitmapStringHighlight("Potentiometer: " + ofToString(speedByte), 225, 20);
        ofDrawBitmapStringHighlight("FPS: " + ofToString(ceil(ofGetFrameRate())), 225, 40);
        ofDrawBitmapStringHighlight("Video URL: " + ofToString(videoFiles[currentVideoIndex]), 225, 60);
        ofDrawBitmapStringHighlight("Video Size: " + ofToString(videos[currentVideoIndex].getWidth()) + "x" + ofToString(videos[currentVideoIndex].getHeight()), 225, 80);
        ofDrawBitmapStringHighlight("Switches: A:" + ofToString(night) + " B:" + ofToString(country), 225, 100);
        ofDrawBitmapStringHighlight("Speed: " + ofToString(speed, 3), 225, 120);

        ofColor serialColor = serialOk ? ofColor(0, 200, 0) : ofColor(220, 0, 0);
        ofDrawBitmapStringHighlight(serialStatus, 225, 140, serialColor, ofColor::white);
        if (simMode) {
            ofDrawBitmapStringHighlight("SIM MODE  target:" + ofToString(targetSpeed, 3) + "  simByte:" + ofToString((int)guiSimByte),
                                        225, 160, ofColor(200, 180, 0), ofColor::black);
            ofDrawBitmapStringHighlight("(keys: up/down, left/right, pgup/pgdn, 0/9)",
                                        225, 180, ofColor(200, 180, 0), ofColor::black);
        }
    }
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_ESC) {
        saveConfig();
        ofExit();
    } else if (key == 'g' || key == 'G') {
        guiVisible = !guiVisible;
        if (guiVisible) ofShowCursor();
        else ofHideCursor();
    } else if (key == 'f' || key == 'F') {
        ofToggleFullscreen();
        isFullScreen = !isFullScreen;
        if (isFullScreen) ofHideCursor();
        else ofShowCursor();
    } else if (simMode) {
        // Sim-mode input: fabricate the same bytes the firmware would send
        // so processByte() — the real decode path — stays exercised.
        if (key == OF_KEY_UP || key == OF_KEY_DOWN) {
            int step = (key == OF_KEY_UP) ? 8 : -8;
            simSpeedByte = ofClamp(simSpeedByte + step, 0, SERIAL_SPEED_RANGE);
            guiSimByte = simSpeedByte;
            processByte((uint8_t)(simSpeedByte + SERIAL_SPEED_BASE));
            lastSimByte = simSpeedByte;
        } else if (key == OF_KEY_LEFT)      processByte(SERIAL_COUNTRY_OFF);
        else if (key == OF_KEY_RIGHT)       processByte(SERIAL_COUNTRY_ON);
        else if (key == OF_KEY_PAGE_UP)     processByte(SERIAL_NIGHT_OFF);
        else if (key == OF_KEY_PAGE_DOWN)   processByte(SERIAL_NIGHT_ON);
        else if (key == '0') {
            simSpeedByte = 0;
            guiSimByte = 0;
            processByte(SERIAL_SPEED_BASE);
            lastSimByte = 0;
        } else if (key == '9') {
            simSpeedByte = SERIAL_SPEED_RANGE;
            guiSimByte = SERIAL_SPEED_RANGE;
            processByte((uint8_t)(SERIAL_SPEED_RANGE + SERIAL_SPEED_BASE));
            lastSimByte = SERIAL_SPEED_RANGE;
        }
    }
}

void ofApp::computeSpeed(uint8_t byte) {
    float top = 1.8f * speedScale;
    if (!invert) {
        targetSpeed = ofMap(byte, 0, 255, 0.0f, top, true);
    } else {
        targetSpeed = ofMap(byte, SERIAL_SPEED_RANGE, 0, 0.0f, top, true);
    }
}

void ofApp::processByte(uint8_t byte) {
    if (byte == SERIAL_COUNTRY_OFF || byte == SERIAL_COUNTRY_ON) {
        bool newCountry = (byte == SERIAL_COUNTRY_ON);
        if (newCountry != country) { country = newCountry; configDirty = true; }
    } else if (byte == SERIAL_NIGHT_OFF || byte == SERIAL_NIGHT_ON) {
        bool newNight = (byte == SERIAL_NIGHT_ON);
        if (newNight != night) { night = newNight; configDirty = true; }
    } else if (byte >= SERIAL_SPEED_BASE) {
        speedByte = byte - SERIAL_SPEED_BASE;
        computeSpeed(speedByte);
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
    // Try config.json, then config.json.bak (last known-good after a torn
    // save), then defaults. ofLoadJson on a 0-byte / missing file returns
    // an empty/null ofJson without throwing, so we treat both as a miss.
    auto tryLoad = [](const std::string & relPath, ofJson & out) -> bool {
        try {
            ofJson j = ofLoadJson(relPath);
            if (j.is_null() || j.empty()) return false;
            out = j;
            return true;
        } catch (const std::exception &) {
            return false;
        }
    };

    ofJson config;
    if (tryLoad("config.json", config)) {
        currentConfigIsValid = true;
    } else {
        currentConfigIsValid = false;
        ofLogWarning("loadConfig") << "config.json missing or unreadable, trying config.json.bak";
        if (!tryLoad("config.json.bak", config)) {
            ofLogError("loadConfig") << "no usable config; falling back to defaults";
            config = ofJson::object();
        } else {
            ofLogNotice("loadConfig") << "recovered values from config.json.bak";
        }
    }

    if (config.contains("videoFiles") && config["videoFiles"].is_array()) {
        auto& arr = config["videoFiles"];
        for (int i = 0; i < 4; ++i) {
            videoFiles[i] = (i < (int)arr.size() && arr[i].is_string())
                          ? arr[i].get<std::string>() : std::string("");
        }
    }
    if (config.contains("soundFiles") && config["soundFiles"].is_array()) {
        auto& arr = config["soundFiles"];
        for (int i = 0; i < 4; ++i) {
            soundFiles[i] = (i < (int)arr.size() && arr[i].is_string())
                          ? arr[i].get<std::string>() : std::string("");
        }
    }

    videoMode   = stringToVideoMode(config.value("videoMode",   std::string("fillScreen")));
    speedScale  = config.value("speedScale",  1.0f);
    smoothScale = config.value("smoothScale", 0.25f);
    usbAddress  = config.value("usbAddress",  std::string("COM3"));
    invert      = config.value("invert",      true);
    night       = config.value("nightMode",   false);
    country     = config.value("countryMode", false);
}

void ofApp::saveConfig() {
    ofJson config;

    for (int i = 0; i < 4; ++i) config["videoFiles"][i] = videoFiles[i];
    for (int i = 0; i < 4; ++i) config["soundFiles"][i] = soundFiles[i];

    switch (videoMode) {
        case ORIGINAL:    config["videoMode"] = "original";   break;
        case FIT_SCREEN:  config["videoMode"] = "fitScreen";  break;
        case FILL_SCREEN: config["videoMode"] = "fillScreen"; break;
    }

    config["speedScale"]  = speedScale;
    config["smoothScale"] = smoothScale;
    config["usbAddress"]  = usbAddress;
    config["invert"]      = invert;
    config["nightMode"]   = night;
    config["countryMode"] = country;

    // Durable save: write .tmp, force its contents to physical disk, rotate
    // the current config.json to .bak, then rename the tmp onto config.json.
    //
    //   ofSaveJson(tmp)  → bytes hit the OS page cache, file is closed
    //   FlushFileBuffers → those bytes are forced past the page cache and the
    //                      NTFS journal onto physical media. Without this,
    //                      std::filesystem::rename is metadata-atomic but a
    //                      power loss can leave config.json pointing at an
    //                      unwritten data extent (= a 0-byte file on reboot).
    //   rename → .bak    → keeps last good copy as a fallback for load.
    //   rename tmp → cfg → directory entry flip is metadata-atomic.
    const std::filesystem::path finalPath = ofToDataPath("config.json", true);
    const std::filesystem::path tmpPath   = finalPath.string() + ".tmp";
    const std::filesystem::path bakPath   = finalPath.string() + ".bak";

    if (!ofSaveJson(tmpPath.string(), config)) {
        ofLogError("saveConfig") << "failed to write " << tmpPath;
        return;
    }

#ifdef _WIN32
    HANDLE h = CreateFileW(tmpPath.wstring().c_str(),
                           GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        ofLogError("saveConfig") << "CreateFile on tmp failed: " << GetLastError();
    } else {
        if (!FlushFileBuffers(h)) {
            ofLogError("saveConfig") << "FlushFileBuffers failed: " << GetLastError();
        }
        CloseHandle(h);
    }
#endif

    std::error_code ec;
    // Only rotate to .bak if the existing config.json is known good. Skipping
    // this when currentConfigIsValid == false prevents a corrupted file (e.g.
    // a 0-byte config.json from a previous torn write) from overwriting the
    // last known-good .bak we just recovered from.
    if (currentConfigIsValid && std::filesystem::exists(finalPath, ec)) {
        ec.clear();
        std::filesystem::rename(finalPath, bakPath, ec);
        if (ec) ofLogError("saveConfig") << "rotate to .bak failed: " << ec.message();
        ec.clear();
    }

    std::filesystem::rename(tmpPath, finalPath, ec);
    if (ec) {
        ofLogError("saveConfig") << "rename failed: " << ec.message();
        std::filesystem::remove(tmpPath, ec);
        return;
    }
    currentConfigIsValid = true;
}

void ofApp::exit() {
    saveConfig();
}

VideoMode ofApp::stringToVideoMode(const std::string& modeStr) {
    if (modeStr == "fitScreen")  return FIT_SCREEN;
    if (modeStr == "fillScreen") return FILL_SCREEN;
    return ORIGINAL;
}

void ofApp::openSerial() {
    serial.close();
    serialOk = serial.setup(usbAddress, 9600);
    serialStatus = serialOk ? ("serial OK: " + usbAddress)
                            : ("serial ERROR: could not open " + usbAddress);
    ofLogNotice("openSerial") << serialStatus;
}
