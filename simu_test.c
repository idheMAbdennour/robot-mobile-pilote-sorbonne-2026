#include <stdio.h>
int main() {
    float offset = 30.0f;
    float avg1 = 0.1045 / 3.3 * 4095;
    float avg2 = 0.1045 / 3.3 * 4095;
    float avg3 = 0.0330 / 3.3 * 4095;
    
    float val1 = (avg1 > offset) ? (avg1 - offset) : 1.0f;
    float val2 = (avg2 > offset) ? (avg2 - offset) : 1.0f;
    float val3 = (avg3 > offset) ? (avg3 - offset) : 1.0f;

    float IND_DIST_GAIN = 150.0f;
    float dist_av = (val1 / val3) * IND_DIST_GAIN;
    float dist_ar = (val2 / val3) * IND_DIST_GAIN;
    
    float DIST_AV_CENTRE_MM = 460;
    float DIST_AR_CENTRE_MM = 690;
    float dist_mil = (DIST_AR_CENTRE_MM * dist_av + DIST_AV_CENTRE_MM * dist_ar) / (DIST_AV_CENTRE_MM + DIST_AR_CENTRE_MM);

    float y_mes = dist_mil / 1000.0f;
    printf("X%dmm\r\n", (int)(y_mes * 1000));
    return 0;
}
