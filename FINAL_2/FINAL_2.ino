/* Edge Impulse Arduino examples
   Copyright (c) 2021 EdgeImpulse Inc.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0

/**
   Define the number of slices per model window. E.g. a model window of 1000 ms
   with slices per model window set to 4. Results in a slice size of 250 ms.
   For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
*/
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 3

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>
#include <PI3_SR_V5_v1_inferencing.h>
#include "Switchable.h"
#include "VibrationMotor.h"

#define RED 22
#define BLUE 24
#define GREEN 23
#define BTN8 8
#define BTN7 7
#define VM1 3
#define VM2 4

VibrationMotor vibrationMotor1(VM1);
VibrationMotor vibrationMotor2(VM2);

/** Audio buffers, pointers and selectors */
typedef struct {
  signed short *buffers[2];
  unsigned char buf_select;
  unsigned char buf_ready;
  unsigned int buf_count;
  unsigned int n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;
static signed short *sampleBuffer;
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static float points[EI_CLASSIFIER_LABEL_COUNT] = {0, 0, 0};


/**
   @brief      Printf function uses vsnprintf and output using Arduino Serial

   @param[in]  format     Variable argument list
*/
void ei_printf(const char *format, ...) {
  static char print_buf[1024] = { 0 };

  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);

  if (r > 0) {
    Serial.write(print_buf);
  }
}

/**
   @brief      PDM buffer full callback
               Get data and call audio thread callback
*/
static void pdm_data_ready_inference_callback(void)
{
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

  if (record_ready == true) {
    for (int i = 0; i<bytesRead >> 1; i++) {
      inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

      if (inference.buf_count >= inference.n_samples) {
        inference.buf_select ^= 1;
        inference.buf_count = 0;
        inference.buf_ready = 1;
      }
    }
  }
}

/**
   @brief      Init inferencing struct and setup/start PDM

   @param[in]  n_samples  The n samples

   @return     { description_of_the_return_value }
*/
static bool microphone_inference_start(uint32_t n_samples)
{
  inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[0] == NULL) {
    return false;
  }

  inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[1] == NULL) {
    free(inference.buffers[0]);
    return false;
  }

  sampleBuffer = (signed short *)malloc((n_samples >> 1) * sizeof(signed short));

  if (sampleBuffer == NULL) {
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    return false;
  }

  inference.buf_select = 0;
  inference.buf_count = 0;
  inference.n_samples = n_samples;
  inference.buf_ready = 0;

  // configure the data receive callback
  PDM.onReceive(&pdm_data_ready_inference_callback);

  PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));

  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
  if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
    ei_printf("Failed to start PDM!");
  }

  // set the gain, defaults to 20
  PDM.setGain(127);

  record_ready = true;

  return true;
}

/**
   @brief      Wait on new data

   @return     True when finished
*/
static bool microphone_inference_record(void)
{
  bool ret = true;

  if (inference.buf_ready == 1) {
    ei_printf(
      "Error sample buffer overrun. Decrease the number of slices per model window "
      "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
    ret = false;
  }

  while (inference.buf_ready == 0) {
    delay(1);
  }

  inference.buf_ready = 0;

  return ret;
}

/**
   Get raw audio signal data
*/
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
  numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

  return 0;
}

/**
   @brief      Stop PDM and release buffers
*/
static void microphone_inference_end(void)
{
  PDM.end();
  free(inference.buffers[0]);
  free(inference.buffers[1]);
  free(sampleBuffer);
}

void recognition()
{

  bool m = microphone_inference_record();
  if (!m) {
    ei_printf("ERR: Failed to record audio...\n");
    return;
  }

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
  signal.get_data = &microphone_audio_signal_get_data;
  ei_impulse_result_t result = {0};

  EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
  if (r != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", r);
    return;
  }



  ei_printf("Instrument : %.5f \n", result.classification[0].value);
  points[0] = points[0] + result.classification[0].value;
  ei_printf("Noise : %.5f \n", result.classification[1].value);
  points[1] = points[1] + result.classification[1].value;
  ei_printf("Readaptation : %.5f \n", result.classification[2].value);
  points[2] = points[2] + result.classification[2].value;

  /*
      if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
          // print the predictions

          ei_printf("Predictions ");
          ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);
          ei_printf(": \n");
          for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: %.5f\n", result.classification[ix].label,
                        result.classification[ix].value);
          }
    #if EI_CLASSIFIER_HAS_ANOMALY == 1
          ei_printf("    anomaly score: %.3f\n", result.anomaly);
    #endif

          print_results = 0;



    } */



}

/**
   @brief      Arduino setup function
*/
void setup()
{
  // put your setup code here, to run once:
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(VM1, OUTPUT);

  digitalWrite(RED, HIGH);
  digitalWrite(BLUE, HIGH);
  digitalWrite(GREEN, HIGH);

  Serial.begin(115200);
  Serial.println("Edge Impulse Inferencing Demo");


  // summary of inferencing settings (from model_metadata.h)

  ei_printf("Inferencing settings:\n");
  ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
  ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
  ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
            sizeof(ei_classifier_inferencing_categories[0]));


  run_classifier_init();


  /*
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
      ei_printf("ERR: Failed to setup audio sampling\r\n");
      return;
    }
  */
}

/**
   @brief      Arduino main function. Runs the inferencing loop.
*/
void loop()
{
  if (digitalRead(BTN8) == HIGH) {
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == true) {
      ei_printf("Setup audio sampling\r\n");
    }
    // time loop for 3 second
    // Or loop for x number of time, because we know how long the recognition function is.
    uint32_t starttime = millis();
    uint32_t endtime = starttime;
    while ((endtime - starttime) <= 2000) { // do this loop for up to 3000mS
      // code here
      recognition();
      endtime = millis();

    }
    digitalWrite(BLUE, LOW);
  }

  if (digitalRead(BTN7) == HIGH) {
    digitalWrite(BLUE, HIGH);
    ei_printf("Recording finished \n");
    microphone_inference_end();
    ei_printf("Printing result \n");

    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      ei_printf("%.5f \n", points[i]);

    }

    if (points[0] > points[1] and points[0] > points[2] and points[0] > 5.5) {
      digitalWrite(GREEN, LOW);
      vibrationMotor1.on();
      delay(1000);
      /*
        vibrationMotor2.on();
        delay(500);
      */
      vibrationMotor1.off();
      // vibrationMotor2.off();
      digitalWrite(GREEN, HIGH);
      memset(points, 0, sizeof(points));

    }
    else if (points[2] > points[1] and points[2] > points[0] and points[2] > 5.5) {
      digitalWrite(GREEN, LOW);
      /*
        vibrationMotor1.on();
        delay(250);
        vibrationMotor1.off();
      */
      vibrationMotor2.on();
      delay(1000);
      /*
        vibrationMotor1.on();
        delay(500);
        vibrationMotor1.off();
      */
      vibrationMotor2.off();
      // vibrationMotor1.off();
      digitalWrite(GREEN, HIGH);
      memset(points, 0, sizeof(points));
    }
    else {
      digitalWrite(RED, LOW);
      delay(1000);
      digitalWrite(RED, HIGH);
      memset(points, 0, sizeof(points));
    }


  }

  //  INSTRUMENT IS ALWAYS 0.9... ON THE FIRST INFERENCE
}


#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
