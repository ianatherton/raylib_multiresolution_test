#include "utils.h"

Color MyColorBrightness(Color color, float factor) {
    Color result;
    
    result.r = (unsigned char)MyClamp((float)color.r * factor, 0, 255);
    result.g = (unsigned char)MyClamp((float)color.g * factor, 0, 255);
    result.b = (unsigned char)MyClamp((float)color.b * factor, 0, 255);
    result.a = color.a;
    
    return result;
}

Color MyColorLerp(Color a, Color b, float t) {
    Color result;
    
    t = MyClamp(t, 0.0f, 1.0f);
    
    result.r = (unsigned char)(a.r + (b.r - a.r) * t);
    result.g = (unsigned char)(a.g + (b.g - a.g) * t);
    result.b = (unsigned char)(a.b + (b.b - a.b) * t);
    result.a = (unsigned char)(a.a + (b.a - a.a) * t);
    
    return result;
}

float MyClamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
