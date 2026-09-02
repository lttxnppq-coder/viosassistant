#include <Arduino.h>
#include "../../utils/Filter.h"
#include "../../utils/Filter.cpp"

// TEST 10 - Filter (MovingAverage / Exponential / Median)
// Test unit-style, BAO GOM production source (utils/Filter.cpp) de kiem tra logic that.
// Dua vao huong dan test: duoc phep dung -I root de include production.
//
// NOTE: chay tren bo demo/upload lai moi co ket qua thuc; day chi la build/static;
//       tuy nhien cac assert trong setup do tim logic (khong can ngoai vi).
//
// PRODUCTION ISSUE phia ben (bao cao, khong sua):
//   MedianFilter update()/getValue() dung mang co dinh `float sorted[15]`:
//   neu window_size > 15 hoac count_ == window_size_ > 15 -> buffer overflow.
//   Nen debug cap nho window > 15.

void assertNear(const char* name, float got, float expect, float eps) {
    bool ok = (fabsf(got - expect) <= eps);
    Serial.printf("[FILTER] %-24s got=%.4f expect=%.4f -> %s\r\n",
                  name, got, expect, ok ? "PASS" : "FAIL");
}

// Chuoi co dinh da tinh tay de kiem tra
// moving[1,2,3,4,5] voi window=3:
//   step1: avg=1   (count=1)
//   step2: avg=1.5 (count=2)
//   step3: avg=2.0 (count=3)
//   step4: (2+3+4)/3 = 3.0
//   step5: (3+4+5)/3 = 4.0
void testMovingAverage() {
    utils::MovingAverageFilter f(3);
    float expected[5] = {1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
    float vals[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    for (int i = 0; i < 5; i++) {
        float got = f.update(vals[i]);
        char name[32]; snprintf(name, 32, "MovingAvg step%d", i + 1);
        assertNear(name, got, expected[i], 1e-4f);
    }
}

void testExponential() {
    utils::ExponentialFilter f(0.5f);
    // update1 -> gan truc tiep value=10 (chua init)
    float v1 = f.update(10.0f);
    assertNear("Exp update1 (init)", v1, 10.0f, 1e-4f);
    // update2 = 0.5*20 + 0.5*10 = 15
    float v2 = f.update(20.0f);
    assertNear("Exp update2", v2, 15.0f, 1e-4f);
    // update3 = 0.5*0 + 0.5*15 = 7.5
    float v3 = f.update(0.0f);
    assertNear("Exp update3", v3, 7.5f, 1e-4f);
}

void testMedian() {
    utils::MedianFilter f(5);
    // them 5 gia tri: 5,1,3,2,4 -> median cua {1,2,3,4,5} = 3
    float vals[5] = {5.0f, 1.0f, 3.0f, 2.0f, 4.0f};
    float med = 0.0f;
    for (int i = 0; i < 5; i++) med = f.update(vals[i]);
    assertNear("Median(5) sorted", med, 3.0f, 1e-4f);
    // tiet diem: dem 1 lai giu nguyen
    assertNear("Median isReady", f.isReady() ? 1.0f : 0.0f, 1.0f, 0.0f);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 10 : FILTER (unit) ===");
    testMovingAverage();
    testExponential();
    testMedian();
    Serial.println("[FILTER] done (build/static)");
    Serial.println("[PROD-NOTE] MedianFilter fixed sorted[15] - window>15 overflow");
}

void loop() {
    delay(1000);
}