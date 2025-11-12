#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>

#ifndef M_PI
#define M_PI 3.1415
#endif

#define NUM_SAMPLES 4096
#define AMPLITUDE 0.8f  // амплитуда [-1.0; 1.0]

int main(void)
{
    // Инициализация SDR
    SoapySDRKwargs args = {0};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", "usb:");
    SoapySDRKwargs_set(&args, "direct", "1");

    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    if (!sdr)
    {
        fprintf(stderr, "Ошибка: не удалось инициализировать SDR\n");
        return EXIT_FAILURE;
    }

    double sample_rate = 1e6;
    double freq = 100e6;

    // Настройка TX
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, freq, NULL);

    // Буфер I/Q
    float complex *buffer = malloc(NUM_SAMPLES * sizeof(float complex));
    if (!buffer)
    {
        fprintf(stderr, "Ошибка выделения памяти\n");
        SoapySDRDevice_unmake(sdr);
        return EXIT_FAILURE;
    }

    // Генерация треугольного сигнала
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        double t = (double)i / NUM_SAMPLES;
        double tri = (2.0 / M_PI) * asin(sin(2 * M_PI * t * 4)); // треугольная форма
        float val = (float)(tri * AMPLITUDE);
        buffer[i] = val + val * I; // одинаковая форма для I и Q
    }

    // Сохранение в файл (для анализа)
    FILE *f = fopen("tx_buff.pcm", "wb");
    if (f)
    {
        fwrite(buffer, sizeof(float complex), NUM_SAMPLES, f);
        fclose(f);
    }

    // Создание TX стрима
    SoapySDRStream *txStream = NULL;
    size_t channels[] = {0};
    txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CF32, channels, 1, NULL);
    if (!txStream)
    {
        fprintf(stderr, "Ошибка: не удалось создать TX стрим\n");
        free(buffer);
        SoapySDRDevice_unmake(sdr);
        return EXIT_FAILURE;
    }

    // Передача данных
    const void *buffs[] = {buffer};
    int flags = 0;
    long long timeNs = 0;
    int ret = SoapySDRDevice_writeStream(sdr, txStream, buffs, NUM_SAMPLES, &flags, timeNs, 1000000);
    printf("Передано %d сэмплов\n", ret);

    // Очистка
    SoapySDRDevice_closeStream(sdr, txStream);
    free(buffer);
    SoapySDRDevice_unmake(sdr);

    return 0;
}