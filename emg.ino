/*
 * Two-Channel EMG Signal Processing
 * ----------------------------------
 * Reads two EMG channels (A0, A1) at a fixed 500Hz sample rate using
 * a micros()-based timing loop for precise, jitter-free sampling.
 *
 * Per channel:
 *   1. 4th-order Butterworth bandpass filter (cascaded biquad stages)
 *      removes baseline drift and high-frequency noise.
 *   2. Signal is rectified (absolute value).
 *   3. A Kalman filter smooths the rectified signal to extract the
 *      muscle activation envelope, while also tracking estimation
 *      uncertainty (p).
 *
 * The two channel envelopes are then combined via confidence-weighted
 * fusion: each channel's contribution is weighted by the inverse of
 * its Kalman uncertainty, so the more "confident" channel dominates
 * the fused output.
 *
 * Output (Serial, 115200 baud, CSV):
 *   filtered_ch1, filtered_ch2, fused_envelope
 */

#include <Servo.h>

#define Fs 500
#define SAMPLE_PERIOD_US (1000000 / Fs)
#define inpin1 A0
#define inpin2 A1

Servo finger;

int pos = 0;

class kalman {
  private: 
    float x;
    float p;
    float q;
    float r;
    float k;

  public:
    kalman(float process_noise, float measurement_noise);
    float update(float measurement);
    void tune_r(float newr);
    float getUncertainty() { return p; }
};

kalman::kalman(float process_noise, float measurement_noise){
    q = process_noise;
    r = measurement_noise;
    x = 0.0;
    p = 1.0;
}

float kalman::update(float measurement){
    p = p + q;
    k = p / (p + r);
    x = x + k * (measurement - x);
    p = (1.0 - k) * p;
    return x;
}

void kalman::tune_r(float newr){
    r = newr;
}

class butterworth_filter {
  private:
    float z1_st1, z2_st1;
    float z1_st2, z2_st2;
    float z1_st3, z2_st3;
    float z1_st4, z2_st4;

  public:
    butterworth_filter();
    float filter(float sensor_value);
};

butterworth_filter::butterworth_filter(){
    z1_st1 = z2_st1 = z1_st2 = z2_st2 = 0.0;
    z1_st3 = z2_st3 = z1_st4 = z2_st4 = 0.0;
}

float butterworth_filter::filter(float sensor_value){
    float output = sensor_value;
    float x;

    // Stage 1
    x = output - 0.05159732 * z1_st1 - 0.36347401 * z2_st1;
    output = 0.01856301 * x + 0.03712602 * z1_st1 + 0.01856301 * z2_st1;
    z2_st1 = z1_st1; z1_st1 = x;

    // Stage 2
    x = output - -0.53945795 * z1_st2 - 0.39764934 * z2_st2;
    output = 1.00000000 * x + -2.00000000 * z1_st2 + 1.00000000 * z2_st2;
    z2_st2 = z1_st2; z1_st2 = x;

    // Stage 3
    x = output - 0.47319594 * z1_st3 - 0.70744137 * z2_st3;
    output = 1.00000000 * x + 2.00000000 * z1_st3 + 1.00000000 * z2_st3;
    z2_st3 = z1_st3; z1_st3 = x;

    // Stage 4
    x = output - -1.00211112 * z1_st4 - 0.74520226 * z2_st4;
    output = 1.00000000 * x + -2.00000000 * z1_st4 + 1.00000000 * z2_st4;
    z2_st4 = z1_st4; z1_st4 = x;

    return output;
}

kalman sensor1(0.5, 0.02);
kalman sensor2(0.5, 0.02);

butterworth_filter s1;
butterworth_filter s2;

void setup() {
  Serial.begin(115200);
  finger.attach(9);
}

void loop() {
  static unsigned long past = 0;
  unsigned long present = micros();

  if (present - past >= SAMPLE_PERIOD_US) {
    past = present;

    float sensor1_value = analogRead(inpin1);
    float sensor2_value = analogRead(inpin2);
    
    float filtered_value1 = s1.filter(sensor1_value);
    float filtered_value2 = s2.filter(sensor2_value);

    float x1 = sensor1.update(abs(filtered_value1));
    float x2 = sensor2.update(abs(filtered_value2));

    float p1 = sensor1.getUncertainty();
    float p2 = sensor2.getUncertainty();
    
    // Joint variance weighting
    float wave = (x1 * p2 + x2 * p1) / (p1 + p2);
    
    
    Serial.print(abs(filtered_value1));
    Serial.print(",");
    Serial.print(abs(filtered_value2));
    Serial.print(",");
    Serial.println(wave);


    if (wave > 10.0)
    finger.write(180);
    else 
      finger.write(0);
  }
}
}

