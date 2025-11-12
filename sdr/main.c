#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>

#define BLOCK_SAMPLES 4096
#define TX_FREQUENCY  915e6
#define RX_FREQUENCY  915e6
#define SAMPLE_RATE   2e6  // Поддерживаемая PlutoSDR
#define GAIN          40

// Чтение PCM в int16
int16_t* read_pcm(const char *filename, size_t *sample_count)
{
    FILE *file = fopen(filename, "rb");
    if (!file) { perror("Ошибка открытия файла"); return NULL; }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *sample_count = file_size / sizeof(int16_t);
    

    int16_t *samples = (int16_t*)malloc(file_size);
    if (!samples) { fprintf(stderr,"Ошибка памяти!\n"); fclose(file); return NULL; }

    fread(samples, sizeof(int16_t), *sample_count, file);
    fclose(file);
    printf("[OK] Загружено %zu сэмплов из %s\n", *sample_count, filename);
    return samples;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Использование: %s <tx.pcm> <rx_output.pcm>\n", argv[0]);
        return -1;
    }

    // Чтение TX PCM
    size_t sample_count = 0;
    int16_t *tx_data = read_pcm(argv[1], &sample_count);
    if (!tx_data) return -1;

    // Инициализация SDR
    SoapySDRDevice *sdr = SoapySDRDevice_makeStrArgs("driver=plutosdr");
    if (!sdr) { fprintf(stderr,"Не удалось создать устройство SDR!\n"); free(tx_data); return -1; }

    // Настройка TX/RX
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, SAMPLE_RATE);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, TX_FREQUENCY, NULL);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, GAIN);

    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, SAMPLE_RATE);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, RX_FREQUENCY, NULL);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, GAIN);

    // Создание потоков
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CF32, NULL, 0, NULL);
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CF32, NULL, 0, NULL);
    if (!txStream || !rxStream) { fprintf(stderr,"Ошибка создания потоков!\n"); SoapySDRDevice_unmake(sdr); free(tx_data); return -1; }

    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0);
    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0);

    printf("[INFO] Передача и приём начаты...\n");

    FILE *rx_file = fopen(argv[2], "wb");
    if (!rx_file) { perror("Ошибка открытия RX файла"); free(tx_data); SoapySDRDevice_unmake(sdr); return -1; }

    float *tx_cf32 = (float*)malloc(BLOCK_SAMPLES*2*sizeof(float)); // I/Q
    float *rx_cf32 = (float*)malloc(BLOCK_SAMPLES*2*sizeof(float));
    int16_t *rx_int16 = (int16_t*)malloc(BLOCK_SAMPLES*sizeof(int16_t));

    int flags_tx=0, flags_rx=0;
    long long timeNs=0;

    for (size_t i=0; i<sample_count; i+=BLOCK_SAMPLES)
    {
        size_t block_size = (i+BLOCK_SAMPLES <= sample_count) ? BLOCK_SAMPLES : (sample_count-i);

        // Конвертация int16 -> CF32
        for (size_t n=0; n<block_size; n++)
        {
            tx_cf32[2*n]   = tx_data[i+n] / 32768.0f; // I
            tx_cf32[2*n+1] = 0.0f;                     // Q
        }

        const void *tx_buffs[] = { tx_cf32 };
        void *rx_buffs[] = { rx_cf32 };

        int written = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, block_size, &flags_tx, 0, 100000);
        int readed  = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, block_size, &flags_rx, &timeNs, 100000);

        if (written < 0) fprintf(stderr,"Ошибка передачи блока %zu: %d\n", i, written);
        if (readed < 0) fprintf(stderr,"Ошибка приёма блока %zu: %d\n", i, readed);

        // Конвертация CF32 -> int16 и запись
        for (int n=0; n<readed; n++)
        {
            rx_int16[n] = (int16_t)(rx_cf32[2*n] * 32767.0f); // только I
        }
        if (readed>0) fwrite(rx_int16, sizeof(int16_t), readed, rx_file);
    }

    fclose(rx_file);
    free(tx_cf32); free(rx_cf32); free(rx_int16); free(tx_data);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_unmake(sdr);

    printf("[OK] Завершено успешно.\n");
    return 0;
}