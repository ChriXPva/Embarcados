/*
#include "audio.h"
#include "display_ui.h"

static DFRobotDFPlayerMini myDFPlayer;

void initAudio(void *pvParameters) {
    Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

    if (!myDFPlayer.begin(Serial2)) {
        ComandoDisplay cmdErro;
        cmdErro.tipo = DISPLAY_TEXTO;
        strncpy(cmdErro.textoLinha1, "ERRO NO AUDIO!", sizeof(cmdErro.textoLinha1));
        strncpy(cmdErro.textoLinha2, "DFPlayer Falhou", sizeof(cmdErro.textoLinha2));
        xQueueSend(filaDisplay, &cmdErro, 0);
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

*/

#include "audio.h"

static DFRobotDFPlayerMini myDFPlayer;

void initAudio(void *pvParameters) {
    Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

    if (!myDFPlayer.begin(Serial2)) {
        Serial.println("ERRO NO AUDIO!");
        while (true) vTaskDelay(pdMS_TO_TICKS(100));
    }
    myDFPlayer.start();
    myDFPlayer.setTimeOut(500);
    myDFPlayer.volume(5);
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

    myDFPlayer.play(1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    myDFPlayer.pause();
    vTaskDelay(pdMS_TO_TICKS(1000));
    myDFPlayer.play(2);
    vTaskDelay(pdMS_TO_TICKS(1000));
    myDFPlayer.pause();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    myDFPlayer.play(3);
    vTaskDelay(pdMS_TO_TICKS(1000));
    myDFPlayer.pause();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    myDFPlayer.play(4);
    vTaskDelay(pdMS_TO_TICKS(1000));
    myDFPlayer.pause();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
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