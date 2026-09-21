#include "audio.h"

static DFRobotDFPlayerMini myDFPlayer;

void initAudio(void *pvParameters) {
    Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

    if (!myDFPlayer.begin(Serial2)) {
        Serial.println(F("Erro no DFPlayer Mini"));
        while (true) vTaskDelay(pdMS_TO_TICKS(100));
    }

    myDFPlayer.setTimeOut(500);
    myDFPlayer.volume(5);
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

    myDFPlayer.play(1);
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_AUDIO);
    vTaskDelete(NULL); // Libera memória
}

void TaskAudio(void *pvParameters) {
    ComandoAudio cmd;

    for (;;) {
        if (xQueueReceive(filaAudio, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.acao) {
                case AUDIO_PLAY_FOLDER:
                    myDFPlayer.playFolder(cmd.pasta, cmd.faixa);
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