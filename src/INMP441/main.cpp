// INMP441 MEMS High Precision Omnidirectional Microphone Module I2S

#include <Arduino.h>
#include <driver/i2s.h>

// ── I2S Pin Config ──────────────────────────────────────────────
#define I2S_WS GPIO_NUM_4  // Word Select (LRCLK)
#define I2S_SCK GPIO_NUM_3 // Bit Clock (BCLK)
#define I2S_SD GPIO_NUM_2  // Serial Data (DOUT of mic)

// ── I2S Audio Config ────────────────────────────────────────────
#define SAMPLE_RATE 16000                     // 16 kHz
#define SAMPLE_BITS I2S_BITS_PER_SAMPLE_32BIT // INMP441 outputs 24-bit in 32-bit frame
#define BUFFER_LEN 1024                       // samples per read

// ── I2S Port ────────────────────────────────────────────────────
#define I2S_PORT I2S_NUM_0

// ── Stats interval ──────────────────────────────────────────────
#define STATS_EVERY_MS 500

void i2s_init()
{
  i2s_config_t config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = SAMPLE_BITS,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // L/R tied to GND
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE, // TX not used
      .data_in_num = I2S_SD};

  ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &config, 0, NULL));
  ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pins));
  ESP_ERROR_CHECK(i2s_zero_dma_buffer(I2S_PORT));

  Serial.println("[OK] I2S driver initialized.");
}

// Convert raw 32-bit I2S frame to actual 24-bit signed value
// INMP441 left-justifies 24-bit data in a 32-bit slot
inline int32_t extractSample(int32_t raw)
{
  return raw >> 8; // shift down to get 24-bit range
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== INMP441 I2S Mic Test ===");
  Serial.printf("Sample Rate : %d Hz\n", SAMPLE_RATE);
  Serial.printf("Buffer Size : %d samples\n\n", BUFFER_LEN);

  i2s_init();
}

void loop()
{
  static int32_t samples[BUFFER_LEN];
  static uint32_t lastStatsMs = 0;

  size_t bytesRead = 0;
  esp_err_t result = i2s_read(
      I2S_PORT,
      samples,
      sizeof(samples),
      &bytesRead,
      pdMS_TO_TICKS(100) // 100ms timeout
  );

  if (result != ESP_OK)
  {
    Serial.printf("[ERROR] i2s_read failed: %d\n", result);
    return;
  }

  int samplesRead = bytesRead / sizeof(int32_t);

  if (samplesRead == 0)
  {
    Serial.println("[WARN] No samples read.");
    return;
  }

  // ── Compute stats ─────────────────────────────────────────────
  int32_t minVal = INT32_MAX;
  int32_t maxVal = INT32_MIN;
  int64_t sum = 0;
  int64_t sumSq = 0;

  for (int i = 0; i < samplesRead; i++)
  {
    int32_t s = extractSample(samples[i]);
    if (s < minVal)
      minVal = s;
    if (s > maxVal)
      maxVal = s;
    sum += s;
    sumSq += (int64_t)s * s;
  }

  int32_t peakToPeak = maxVal - minVal;
  float mean = (float)sum / samplesRead;
  float rms = sqrt((float)sumSq / samplesRead);

  // ── dBFS (dB relative to full scale of 24-bit = 8388607) ──────
  float dbfs = (rms > 0.0f)
                   ? 20.0f * log10f(rms / 8388607.0f)
                   : -999.0f;

  // ── Print stats at interval ───────────────────────────────────
  if (millis() - lastStatsMs >= STATS_EVERY_MS)
  {
    lastStatsMs = millis();

    Serial.printf(
        "Samples: %4d  |  Min: %8ld  Max: %8ld  |  P2P: %8ld  |  RMS: %8.1f  |  dBFS: %6.1f\n",
        samplesRead,
        (long)minVal, (long)maxVal,
        (long)peakToPeak,
        rms,
        dbfs);

    // ── Simple ASCII level meter ──────────────────────────────
    int bars = (int)((dbfs + 80.0f) / 80.0f * 40.0f); // map -80..0 dBFS → 0..40 bars
    bars = constrain(bars, 0, 40);

    Serial.print("Level: [");
    for (int i = 0; i < 40; i++)
      Serial.print(i < bars ? '=' : ' ');
    Serial.printf("] %.1f dBFS\n\n", dbfs);
  }
}