/**
 * @file
 * @brief computes sRGB to and from from CIE xy and brightness
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @note Public domain algorithms from Philips and W3C
 * @date 2022
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "bacnet/basic/sys/color_rgb.h"

/**
 * @brief Clamp a double precision value between two limits
 * @param d - value to be clamped
 * @param min - minimum value to clamp within
 * @param max - maximum value to clamp within
 * @return value clamped between min and max inclusive
 */
double color_rgb_clamp(double d, double min, double max)
{
    if (isnan(d)) {
        return min;
    } else {
        const double t = d < min ? min : d;
        return t > max ? max : t;
    }
}

/**
 * @brief Convert sRGB to CIE xy
 * @param r - R value of sRGB 0..255
 * @param g - G value of sRGB 0..255
 * @param b - B value of sRGB 0..255
 * @param x_coordinate - return x of CIE xy 0.0..1.0
 * @param y_coordinate - return y of CIE xy 0.0..1.0
 * @param brightness - return brightness of the CIE xy color 0..255
 * @param gamma_correction - true if gamma correction is applied
 * @note http://en.wikipedia.org/wiki/Srgb
 */
static void color_rgb_to_xy_gamma_correction(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness,
    bool gamma_correction)
{
    float X, Y, Z;
    float x, y;
    /*  Get the RGB values from your color object
        and convert them to be between 0 and 1.
        So the RGB color (255, 0, 100) becomes (1.0, 0.0, 0.39) */
    float red = (float)r;
    float green = (float)g;
    float blue = (float)b;

    red /= 255.0f;
    green /= 255.0f;
    blue /= 255.0f;

    if (gamma_correction) {
        /* Apply a gamma correction to the RGB values,
           which makes the color more vivid and more the
           like the color displayed on the screen of your device.
           This gamma correction is also applied to the screen
           of your computer or phone, thus we need this to create
           the same color on the light as on screen. */
        red = (red > 0.04045f)
            ? (float)pow(((double)red + 0.055) / (1.0 + 0.055), 2.4)
            : (red / 12.92f);
        green = (green > 0.04045f)
            ? (float)pow(((double)green + 0.055) / (1.0 + 0.055), 2.4)
            : (green / 12.92f);
        blue = (blue > 0.04045f)
            ? (float)pow(((double)blue + 0.055) / (1.0 + 0.055), 2.4)
            : (blue / 12.92f);
    }

    /*  Convert the RGB values to XYZ using the
        Wide RGB D65 conversion formula */
    X = red * 0.649926f + green * 0.103455f + blue * 0.197109f;
    Y = red * 0.234327f + green * 0.743075f + blue * 0.022598f;
    Z = red * 0.0000000f + green * 0.053077f + blue * 1.035763f;

    /* Calculate the xy values from the XYZ values */
    x = X / (X + Y + Z);
    y = Y / (X + Y + Z);

    x = color_rgb_clamp(x, 0.0f, 1.0f);
    y = color_rgb_clamp(y, 0.0f, 1.0f);

    /* copy to return values if possible */
    if (x_coordinate) {
        *x_coordinate = x;
    }
    if (y_coordinate) {
        *y_coordinate = y;
    }

    /*  Use the Y value of XYZ as brightness
        The Y value indicates the brightness
        of the converted color. */
    Y = Y * 255.0f;
    Y = color_rgb_clamp(Y, 0.0f, 255.0f);
    if (brightness) {
        *brightness = (uint8_t)Y;
    }
}

/**
 * @brief Convert sRGB to CIE xy
 * @param r - R value of sRGB 0..255
 * @param g - G value of sRGB 0..255
 * @param b - B value of sRGB 0..255
 * @param x_coordinate - return x of CIE xy 0.0..1.0
 * @param y_coordinate - return y of CIE xy 0.0..1.0
 * @param brightness - return brightness of the CIE xy color 0..255
 * @note http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_to_xy(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness)
{
    color_rgb_to_xy_gamma_correction(
        r, g, b, x_coordinate, y_coordinate, brightness, false);
}

/**
 * @brief Convert sRGB to CIE xy, with gamma correction
 * @param r - R value of sRGB 0..255
 * @param g - G value of sRGB 0..255
 * @param b - B value of sRGB 0..255
 * @param x_coordinate - return x of CIE xy 0.0..1.0
 * @param y_coordinate - return y of CIE xy 0.0..1.0
 * @param brightness - return brightness of the CIE xy color 0..255
 * @note http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_to_xy_gamma(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness)
{
    color_rgb_to_xy_gamma_correction(
        r, g, b, x_coordinate, y_coordinate, brightness, true);
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param x_coordinate - x of CIE xy
 * @param y_coordinate - y of CIE xy
 * @param brightness - brightness of the CIE xy color
 * @param gamma_correction - true if gamma correction is needed
 * @note http://en.wikipedia.org/wiki/Srgb
 */
static void color_rgb_from_xy_gamma_correction(
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness,
    bool gamma_correction)
{
    float r, g, b;
    float x, y, z, X, Y, Z;

    /*  Calculate XYZ values */
    x = x_coordinate;
    y = y_coordinate;
    z = 1.0f - x - y;
    Y = (float)brightness;
    Y /= 255.0f;
    X = x * (Y / y);
    Z = z * (Y / y);

    /*  Convert to RGB using Wide RGB D65 conversion
       (THIS IS A D50 conversion currently) */
    r = X * 1.4628067f - Y * 0.1840623f - Z * 0.2743606f;
    g = -X * 0.5217933f + Y * 1.4472381f + Z * 0.0677227f;
    b = X * 0.0349342f - Y * 0.0968930f + Z * 1.2884099f;

    if (gamma_correction) {
        /*  Apply reverse gamma correction */
        r = r <= 0.0031308f
            ? 12.92f * r
            : (1.0f + 0.055f) * (float)pow((double)r, (1.0 / 2.4)) - 0.055f;
        g = g <= 0.0031308f
            ? 12.92f * g
            : (1.0f + 0.055f) * (float)pow((double)g, (1.0 / 2.4)) - 0.055f;
        b = b <= 0.0031308f
            ? 12.92f * b
            : (1.0f + 0.055f) * (float)pow((double)b, (1.0 / 2.4)) - 0.055f;
    }

    /*  Convert the RGB values to your color object
        The rgb values from the above formulas are
        between 0.0 and 1.0. */
    r = r * 255.0f;
    r = color_rgb_clamp(r, 0.0f, 255.0f);
    g = g * 255;
    g = color_rgb_clamp(g, 0.0f, 255.0f);
    b = b * 255;
    b = color_rgb_clamp(b, 0.0f, 255.0f);
    /* copy to return value if possible */
    if (red) {
        *red = (uint8_t)r;
    }
    if (green) {
        *green = (uint8_t)g;
    }
    if (blue) {
        *blue = (uint8_t)b;
    }
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param x_coordinate - x of CIE xy
 * @param y_coordinate - y of CIE xy
 * @param brightness - brightness of the CIE xy color
 * @note http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_from_xy(
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness)
{
    color_rgb_from_xy_gamma_correction(
        red, green, blue, x_coordinate, y_coordinate, brightness, false);
}

/**
 * @brief Convert sRGB from CIE xy and brightness, with gamma correction
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param x_coordinate - x of CIE xy
 * @param y_coordinate - y of CIE xy
 * @param brightness - brightness of the CIE xy color
 * @note http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_from_xy_gamma(
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness)
{
    color_rgb_from_xy_gamma_correction(
        red, green, blue, x_coordinate, y_coordinate, brightness, true);
}

/* table for converting RGB to and from ASCII color names */
struct css_color_rgb {
    const char *name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
static struct css_color_rgb CSS_Color_RGB_Table[] = {
    { "aliceblue", 240, 248, 255 },
    { "antiquewhite", 250, 235, 215 },
    { "aqua", 0, 255, 255 },
    { "aquamarine", 127, 255, 212 },
    { "azure", 240, 255, 255 },
    { "beige", 245, 245, 220 },
    { "bisque", 255, 228, 196 },
    { "black", 0, 0, 0 },
    { "blanchedalmond", 255, 235, 205 },
    { "blue", 0, 0, 255 },
    { "blueviolet", 138, 43, 226 },
    { "brown", 165, 42, 42 },
    { "burlywood", 222, 184, 135 },
    { "cadetblue", 95, 158, 160 },
    { "chartreuse", 127, 255, 0 },
    { "chocolate", 210, 105, 30 },
    { "coral", 255, 127, 80 },
    { "cornflowerblue", 100, 149, 237 },
    { "cornsilk", 255, 248, 220 },
    { "crimson", 220, 20, 60 },
    { "cyan", 0, 255, 255 },
    { "darkblue", 0, 0, 139 },
    { "darkcyan", 0, 139, 139 },
    { "darkgoldenrod", 184, 134, 11 },
    { "darkgray", 169, 169, 169 },
    { "darkgreen", 0, 100, 0 },
    { "darkgrey", 169, 169, 169 },
    { "darkkhaki", 189, 183, 107 },
    { "darkmagenta", 139, 0, 139 },
    { "darkolivegreen", 85, 107, 47 },
    { "darkorange", 255, 140, 0 },
    { "darkorchid", 153, 50, 204 },
    { "darkred", 139, 0, 0 },
    { "darksalmon", 233, 150, 122 },
    { "darkseagreen", 143, 188, 143 },
    { "darkslateblue", 72, 61, 139 },
    { "darkslategray", 47, 79, 79 },
    { "darkslategrey", 47, 79, 79 },
    { "darkturquoise", 0, 206, 209 },
    { "darkviolet", 148, 0, 211 },
    { "deeppink", 255, 20, 147 },
    { "deepskyblue", 0, 191, 255 },
    { "dimgray", 105, 105, 105 },
    { "dimgrey", 105, 105, 105 },
    { "dodgerblue", 30, 144, 255 },
    { "firebrick", 178, 34, 34 },
    { "floralwhite", 255, 250, 240 },
    { "forestgreen", 34, 139, 34 },
    { "fuchsia", 255, 0, 255 },
    { "gainsboro", 220, 220, 220 },
    { "ghostwhite", 248, 248, 255 },
    { "gold", 255, 215, 0 },
    { "goldenrod", 218, 165, 32 },
    { "gray", 128, 128, 128 },
    { "green", 0, 128, 0 },
    { "greenyellow", 173, 255, 47 },
    { "grey", 128, 128, 128 },
    { "honeydew", 240, 255, 240 },
    { "hotpink", 255, 105, 180 },
    { "indianred", 205, 92, 92 },
    { "indigo", 75, 0, 130 },
    { "ivory", 255, 255, 240 },
    { "khaki", 240, 230, 140 },
    { "lavender", 230, 230, 250 },
    { "lavenderblush", 255, 240, 245 },
    { "lawngreen", 124, 252, 0 },
    { "lemonchiffon", 255, 250, 205 },
    { "lightblue", 173, 216, 230 },
    { "lightcoral", 240, 128, 128 },
    { "lightcyan", 224, 255, 255 },
    { "lightgoldenrodyellow", 250, 250, 210 },
    { "lightgray", 211, 211, 211 },
    { "lightgreen", 144, 238, 144 },
    { "lightgrey", 211, 211, 211 },
    { "lightpink", 255, 182, 193 },
    { "lightsalmon", 255, 160, 122 },
    { "lightseagreen", 32, 178, 170 },
    { "lightskyblue", 135, 206, 250 },
    { "lightslategray", 119, 136, 153 },
    { "lightslategrey", 119, 136, 153 },
    { "lightsteelblue", 176, 196, 222 },
    { "lightyellow", 255, 255, 224 },
    { "lime", 0, 255, 0 },
    { "limegreen", 50, 205, 50 },
    { "linen", 250, 240, 230 },
    { "magenta", 255, 0, 255 },
    { "maroon", 128, 0, 0 },
    { "mediumaquamarine", 102, 205, 170 },
    { "mediumblue", 0, 0, 205 },
    { "mediumorchid", 186, 85, 211 },
    { "mediumpurple", 147, 112, 219 },
    { "mediumseagreen", 60, 179, 113 },
    { "mediumslateblue", 123, 104, 238 },
    { "mediumspringgreen", 0, 250, 154 },
    { "mediumturquoise", 72, 209, 204 },
    { "mediumvioletred", 199, 21, 133 },
    { "midnightblue", 25, 25, 112 },
    { "mintcream", 245, 255, 250 },
    { "mistyrose", 255, 228, 225 },
    { "moccasin", 255, 228, 181 },
    { "navajowhite", 255, 222, 173 },
    { "navy", 0, 0, 128 },
    { "navyblue", 0, 0, 128 },
    { "oldlace", 253, 245, 230 },
    { "olive", 128, 128, 0 },
    { "olivedrab", 107, 142, 35 },
    { "orange", 255, 165, 0 },
    { "orangered", 255, 69, 0 },
    { "orchid", 218, 112, 214 },
    { "palegoldenrod", 238, 232, 170 },
    { "palegreen", 152, 251, 152 },
    { "paleturquoise", 175, 238, 238 },
    { "palevioletred", 219, 112, 147 },
    { "papayawhip", 255, 239, 213 },
    { "peachpuff", 255, 218, 185 },
    { "peru", 205, 133, 63 },
    { "pink", 255, 192, 203 },
    { "plum", 221, 160, 221 },
    { "powderblue", 176, 224, 230 },
    { "purple", 128, 0, 128 },
    { "red", 255, 0, 0 },
    { "rosybrown", 188, 143, 143 },
    { "royalblue", 65, 105, 225 },
    { "saddlebrown", 139, 69, 19 },
    { "salmon", 250, 128, 114 },
    { "sandybrown", 244, 164, 96 },
    { "seagreen", 46, 139, 87 },
    { "seashell", 255, 245, 238 },
    { "sienna", 160, 82, 45 },
    { "silver", 192, 192, 192 },
    { "skyblue", 135, 206, 235 },
    { "slateblue", 106, 90, 205 },
    { "slategray", 112, 128, 144 },
    { "slategrey", 112, 128, 144 },
    { "snow", 255, 250, 250 },
    { "springgreen", 0, 255, 127 },
    { "steelblue", 70, 130, 180 },
    { "tan", 210, 180, 140 },
    { "teal", 0, 128, 128 },
    { "thistle", 216, 191, 216 },
    { "tomato", 255, 99, 71 },
    { "turquoise", 64, 224, 208 },
    { "violet", 238, 130, 238 },
    { "wheat", 245, 222, 179 },
    { "white", 255, 255, 255 },
    { "whitesmoke", 245, 245, 245 },
    { "yellow", 255, 255, 0 },
    { "yellowgreen", 154, 205, 50 },
    { NULL, 0, 0, 0 }
};

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @return name - CSS color name from W3C or "" if not found
 * @note Official CSS3 colors from w3.org:
 *  https://www.w3.org/TR/2010/PR-css3-color-20101028/#html4
 *  names do not have spaces
 */
const char *color_rgb_to_ascii(uint8_t red, uint8_t green, uint8_t blue)
{
    const char *name = "";
    unsigned index = 0;

    while (CSS_Color_RGB_Table[index].name) {
        if ((red == CSS_Color_RGB_Table[index].red) &&
            (green == CSS_Color_RGB_Table[index].green) &&
            (blue == CSS_Color_RGB_Table[index].blue)) {
            return CSS_Color_RGB_Table[index].name;
        }
        index++;
    };

    return name;
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param name - CSS color name from W3C
 * @return index 0..color_rgb_count(), where color_rgb_count() is not found.
 */
unsigned color_rgb_from_ascii(
    uint8_t *red, uint8_t *green, uint8_t *blue, const char *name)
{
    unsigned index = 0;

    while (CSS_Color_RGB_Table[index].name) {
        if (strcmp(CSS_Color_RGB_Table[index].name, name) == 0) {
            if (red) {
                *red = CSS_Color_RGB_Table[index].red;
            }
            if (green) {
                *green = CSS_Color_RGB_Table[index].green;
            }
            if (blue) {
                *blue = CSS_Color_RGB_Table[index].blue;
            }
            break;
        }
        index++;
    };

    return index;
}

/**
 * @brief Convert CSS color name to CIE xy coordinates and brightness
 * @param x_coordinate - return x of CIE xy 0.0..1.0
 * @param y_coordinate - return y of CIE xy 0.0..1.0
 * @param brightness - return brightness of the CIE xy color 0..255
 * @param name - CSS color name from W3C
 * @return true if color name was found, false if not found
 */
bool color_rgb_xy_from_ascii(
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness,
    const char *name)
{
    bool status = false;
    uint8_t r, g, b;
    unsigned index;

    index = color_rgb_from_ascii(&r, &g, &b, name);
    if (index < color_rgb_count()) {
        color_rgb_to_xy(r, g, b, x_coordinate, y_coordinate, brightness);
        status = true;
    }

    return status;
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @return CSS ASCII color name from W3C or NULL if invalid index
 */
const char *color_rgb_from_index(
    unsigned target_index, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    unsigned index = 0;

    while (CSS_Color_RGB_Table[index].name) {
        if (target_index == index) {
            if (red) {
                *red = CSS_Color_RGB_Table[index].red;
            }
            if (green) {
                *green = CSS_Color_RGB_Table[index].green;
            }
            if (blue) {
                *blue = CSS_Color_RGB_Table[index].blue;
            }
            return CSS_Color_RGB_Table[index].name;
        }
        index++;
    };

    return NULL;
}

/**
 * @brief Gets the number of sRGB names from CSS3 defines in W3C
 * @return the number of defined RGB names
 */
unsigned color_rgb_count(void)
{
    unsigned count = 0;

    while (CSS_Color_RGB_Table[count].name) {
        count++;
    }

    return count;
}

/**
 * @brief Return an RGB color from a color temperature in Kelvin.
 * @note This is a rough approximation based on the formula
 *  provided by T. Helland
 *  http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
 */
void color_rgb_from_temperature(
    uint16_t temperature_kelvin, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float red = 0, green = 0, blue = 0;

    if (temperature_kelvin < 1000) {
        temperature_kelvin = 1000;
    } else if (temperature_kelvin > 40000) {
        temperature_kelvin = 40000;
    }
    temperature_kelvin /= 100;

    /* Calculate Red */
    if (temperature_kelvin <= 66) {
        /* Red values below 6600 K are always 255 */
        red = 255.0;
    } else {
        red = (float)(temperature_kelvin - 60);
        red = 329.698727446 * pow(red, -0.1332047592);
        red = color_rgb_clamp(red, 0.0, 255.0);
    }
    /* Calculate Green */
    if (temperature_kelvin <= 66) {
        /* Green values below 6600 K */
        green = (float)temperature_kelvin;
        green = 99.4708025861 * log(green) - 161.1195681661;
    } else {
        green = (float)(temperature_kelvin - 60);
        green = 288.1221695283 * pow(green, -0.0755148492);
    }
    green = color_rgb_clamp(green, 0.0, 255.0);
    /* Calculate Blue */
    if (temperature_kelvin >= 66) {
        /* Blue values above 6600 K */
        blue = 255.0;
    } else if (temperature_kelvin <= 19) {
        /* Blue values below 1900 K */
        blue = 0.0;
    } else {
        blue = (float)(temperature_kelvin - 10);
        blue = 138.5177312231 * log(blue) - 305.0447927307;
        blue = color_rgb_clamp(blue, 0, 255);
    }
    if (r) {
        *r = (uint8_t)red;
    }
    if (g) {
        *g = (uint8_t)green;
    }
    if (b) {
        *b = (uint8_t)blue;
    }
}
