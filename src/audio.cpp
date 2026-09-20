#include "audio.h"

DFRobotDFPlayerMini myDFPlayer;

void initAudio(void *pvParameters) {
    // 1. PRIMEIRO inicia a comunicação serial no ESP32
    Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

    // 2. DEPOIS inicializa o módulo DFPlayer Mini usando a Serial2
    if (!myDFPlayer.begin(Serial2)) {
        Serial.println(F("Não inicializou :("));
        Serial.println(F("1. Checa as conexões do DFPlayer Mini"));
        Serial.println(F("2. Insere um cartão SD com formato FAT32"));
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        return; 
    }

    // 3. Configurações do áudio após a conexão ser bem-sucedida
    Serial.println(F("DFPlayer Mini inicializado com sucesso!"));
    myDFPlayer.setTimeOut(500);  // Serial timeout 500ms
    myDFPlayer.volume(5);        // Define o volume (0 a 30)
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL); // Define equalização Normal

    myDFPlayer.play(1); // Toca a primeira música do cartão SD
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_AUDIO);
}
