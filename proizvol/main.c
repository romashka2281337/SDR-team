#include <SoapySDR/Device.h>   // Инициализация устройства
#include <SoapySDR/Formats.h>  // Типы данных, используемых для записи сэмплов
#include <stdio.h>             // printf
#include <stdlib.h>            // free
#include <stdint.h>
#include <complex.h>

int main(void)
{
    // Настройка устройства
    SoapySDRKwargs args = {0};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", "usb:");
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");

    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    if (!sdr)
    {
        fprintf(stderr, "SoapySDRDevice_make failed\n");
        return EXIT_FAILURE;
    }

    // Параметры
    int sample_rate = 1000000;
    int carrier_freq = 800000000;

    // RX
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);

    // TX
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    // Каналы
    size_t channels[] = {0};
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 20.0);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, -20.0);
    size_t channel_count = sizeof(channels) / sizeof(channels[0]);

    // Потоки
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0);
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0);

    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);

    int16_t tx_buff[2 * tx_mtu];
    int16_t rx_buffer[2 * rx_mtu];

    const long timeoutUs = 400000;
    long long last_time = 0;
    size_t iteration_count = 10;

    // Заполнение tx_buff
    for (size_t i = 2; i < 2 * tx_mtu; i += 2)
    {
        tx_buff[i] = 1500 << 4;   // I
        tx_buff[i + 1] = 1500 << 4; // Q
    }
    for (size_t i = 0; i < 2; i++)
    {
        tx_buff[0 + i] = 0xffff;
        tx_buff[10 + i] = 0xffff;
    }

    FILE *fptr = fopen("rx_buff.pcm", "wb");
    if (!fptr)
    {
        fprintf(stderr, "Не удалось открыть файл для записи\n");
        return EXIT_FAILURE;
    }

    for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
    {
        void *rx_buffs[] = {rx_buffer};
        int flags;
        long long timeNs;

        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        fwrite(rx_buffer, 2 * rx_mtu * sizeof(int16_t), 1, fptr);

        printf("Buffer: %zu - Samples: %i, Flags: %i, Time: %lli, ΔTime: %lli\n",
               buffers_read, sr, flags, timeNs, timeNs - last_time);

        last_time = timeNs;

        long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 мс в будущее
        for (size_t i = 0; i < 8; i++)
        {
            uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
            tx_buff[2 + i] = tx_time_byte << 4;
        }

        void *tx_buffs[] = {tx_buff};
        flags = SOAPY_SDR_HAS_TIME;
        int st = SoapySDRDevice_writeStream(sdr, txStream, (const void *const *)tx_buffs, tx_mtu, &flags, tx_time, timeoutUs);

        if ((size_t)st != tx_mtu)
        {
            printf("TX Failed: %i\n", st);
        }

        printf("buffers_read: %zu\n", buffers_read);
    }

    fclose(fptr);


    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_unmake(sdr);

    printf("write done\n");
    return 0;
}