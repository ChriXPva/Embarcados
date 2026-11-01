#include "audio.h"

HardwareSerial SerialDFPlayer(1);
DFRobotDFPlayerMini myDFPlayer;

void initAudio(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(1500));

    SerialDFPlayer.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);

    vTaskDelay(pdMS_TO_TICKS(500));

    if (!myDFPlayer.begin(SerialDFPlayer,true,false)) {
        Serial.println("ERRO NO AUDIO!");
    }
    myDFPlayer.setTimeOut(500);
    myDFPlayer.volume(30);

    myDFPlayer.playMp3Folder(1); 
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_AUDIO);
    vTaskDelete(NULL); // Libera memória
}

void TaskAudio(void *pvParameters) {
    ComandoAudio cmd;

    for (;;) {
        if (xQueueReceive(filaAudio, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.acao) {
                case AUDIO_PLAY:
                    myDFPlayer.play(cmd.faixa);
                    break;
                case AUDIO_PAUSE:
                    myDFPlayer.pause();
                    break;
                case AUDIO_START:
                    myDFPlayer.start();
                    break;
                case AUDIO_STOP:
                    myDFPlayer.stop();
                    break;
            }
        }
    }
}