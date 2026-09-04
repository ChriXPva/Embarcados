#include "audio.h"

HardwareSerial mySoftwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;

void initAudio() {
    mySoftwareSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    if (myDFPlayer.begin(mySoftwareSerial)) {
        myDFPlayer.volume(20);
    }
}
