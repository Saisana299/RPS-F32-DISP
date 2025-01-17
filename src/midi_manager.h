#include <MD_MIDIFile.h>
#include <ctrl_manager.h>

#ifndef MIDIMANAGER_H
#define MIDIMANAGER_H

#define MIDI_IDLE     0x00
#define MIDI_PLAY     0x01
#define MIDI_PLAYING  0x02
#define MIDI_STOP     0x03
#define MIDI_PAUSE    0x04
#define MIDI_RESUME   0x05
#define MIDI_LOOP_ON  0x06
#define MIDI_LOOP_OFF 0x07
#define MIDI_PAUSE_FORCED  0x08
#define MIDI_RESUME_FORCED 0x09

#define SDFS_CS_PIN 5

class MidiManager {
private:
    // midi関連
    volatile uint8_t flag = MIDI_IDLE;
    volatile char midiFile[51] = ""; // 50まで /rp-ds16/midi/ も含む
    volatile bool forcedPause = false;

    SdFs SD;
    MD_MIDIFile SMF;

    CtrlManager* pCtrl;

    // Core1で実行
    static void midiCallback(midi_event *pev) {
        if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
            Serial1.write(pev->data[0] | pev->channel);
            Serial1.write(&pev->data[1], pev->size - 1);
        } else {
            Serial1.write(pev->data, pev->size);
        }
    }

    void resetSynth() {
        uint8_t data[] = {CTRL_RESET_SYNTH, 0xff};
        uint8_t received[1];
        pCtrl->ctrlTransmission(data, sizeof(data), received, 1);
    }

    void sendMidiMode(bool v) {
        uint8_t data[1] = {};
        if(v) data[0] = CTRL_MIDI_ON;
        else  data[0] = CTRL_MIDI_OFF;
        uint8_t received[1];
        pCtrl->ctrlTransmission(data, sizeof(data), received, 1);

        // シンセをリセットする
        resetSynth();
    }

public:
    // midi関連
    volatile bool isPlayMidi = false;
    volatile bool isLocking = false;

    MidiManager(CtrlManager* addr1) {
        pCtrl = addr1;
    }

    void init() {
        // SdFs初期化
        SD.begin(SDFS_CS_PIN, SPI_FULL_SPEED);

        // MIDIFileの初期化
        SMF.begin(&SD);
        SMF.setMidiHandler(midiCallback); // MIDIハンドラを設定
        SMF.looping(false); // ループ再生を無効に設定

        // SMFのcore0からの利用はここまで //
    }

    uint8_t getStatus() {
        return flag;
    }

    bool isMidiPaused() {
        return (flag == MIDI_PAUSE);
    }

    void playMidi(String path) {
        if(isLocking) return; // ロック中は再生できない
        if(path.length() > 50) return;
        strcpy((char*)midiFile, path.c_str());
        sendMidiMode(true);
        flag = MIDI_PLAY;
    }

    void pauseMidi() {
        flag = MIDI_PAUSE;
        while(isLocking);
        sendMidiMode(false);
    }

    void forcedPauseMidi() {
        flag = MIDI_PAUSE_FORCED;
        while(isLocking);
        sendMidiMode(false);
    }

    void resumeMidi() {
        if(forcedPause) {
            sendMidiMode(true);
            flag = MIDI_RESUME_FORCED;
            return;
        }
        sendMidiMode(true);
        flag = MIDI_RESUME;
    }

    void stopMidi() {
        flag = MIDI_STOP;
        while(isLocking);
        sendMidiMode(false);
    }

    void loopMidi(bool v) {
        if(v) flag = MIDI_LOOP_ON;
        else flag = MIDI_LOOP_OFF;
    }

    // Core1で実行
    void midiHandler() {
        if(flag == MIDI_PLAY) {
            flag = MIDI_PLAYING;
            SMF.load(const_cast<const char*>(midiFile));
            isLocking = true;
            isPlayMidi = true;
        }
        else if(flag == MIDI_PLAYING) {
            // 読込みを一時停止している場合
            if(forcedPause) return;

            if (!SMF.isEOF()) {
                SMF.getNextEvent();
            }
            else {
                isPlayMidi = false;
                SMF.close();
                isLocking = false;
            }
        }
        else if(flag == MIDI_STOP) {
            SMF.close();
            isLocking = false;
            isPlayMidi = false;
            flag = MIDI_IDLE;
        }
        else if(flag == MIDI_PAUSE) {
            SMF.pause(true);
            isLocking = false;
            isPlayMidi = false;
            flag = MIDI_PLAYING;
        }
        else if(flag == MIDI_PAUSE_FORCED) {
            forcedPause = true;
            isLocking = false;
            isPlayMidi = false;
            flag = MIDI_PLAYING;
        }
        else if(flag == MIDI_RESUME) {
            SMF.pause(false);
            isLocking = true;
            isPlayMidi = true;
            flag = MIDI_PLAYING;
        }
        else if(flag == MIDI_RESUME_FORCED) {
            // forcedPauseと逆の順番で変更
            isLocking = true;
            isPlayMidi = true;
            forcedPause = false;
            flag = MIDI_PLAYING;
        }
        else if(flag == MIDI_LOOP_ON) {
            SMF.looping(true);
            flag = MIDI_PLAYING;
        }
        else if(flag == MIDI_LOOP_OFF) {
            SMF.looping(false);
            flag = MIDI_PLAYING;
        }
    }
};

#endif // MIDIMANAGER_H