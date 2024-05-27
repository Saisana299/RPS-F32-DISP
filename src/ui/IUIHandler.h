#ifndef IUIHANDLER_H
#define IUIHANDLER_H

#define DISPST_IDLE        0x00
#define DISPST_TITLE       0x01
#define DISPST_PRESETS     0x02
#define DISPST_ADSR        0x03
#define DISPST_DEBUG       0x04
#define DISPST_MENU        0x05
#define DISPST_FILEMAN     0x06
#define DISPST_PRESET_EDIT 0x07
#define DISPST_UNISON      0x08

struct Files {
    bool type;
    String name;
    String path;
};

struct Preset {
    String name; // 最大30文字
    String path;
};

class IUIHandler {
public:
    virtual ~IUIHandler() = default;
    virtual void refreshUI() = 0;
    virtual void handleButtonUp(bool longPush = false) = 0;
    virtual void handleButtonDown(bool longPush = false) = 0;
    virtual void handleButtonLeft(bool longPush = false) = 0;
    virtual void handleButtonRight(bool longPush = false) = 0;
    virtual void handleButtonEnter(bool longPush = false) = 0;
    virtual void handleButtonCancel(bool longPush = false) = 0;
};

#endif // IUIHANDLER_H
