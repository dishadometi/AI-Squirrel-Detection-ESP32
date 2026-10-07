/* Includes ---------------------------------------------------------------- */

#include <Smart_animal_detecting_light_system_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <string.h>


/* --------------------------------------------------------------------------
   ESP32-S3 CAM V1.2 CAMERA PIN DEFINITIONS
   -------------------------------------------------------------------------- */

// Camera control
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5

// Camera data pins
#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11

// Camera synchronization
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM     13


/* --------------------------------------------------------------------------
   CONNECTION TO ARDUINO MEGA
   -------------------------------------------------------------------------- */

// ESP32-S3 GPIO1 -> level shifter -> Arduino Mega pin 7
#define SQUIRREL_SIGNAL_PIN 1

// Detection confidence threshold
#define SQUIRREL_THRESHOLD 0.50


/* Constant defines -------------------------------------------------------- */

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS   320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS   240
#define EI_CAMERA_FRAME_BYTE_SIZE         3


/* Private variables ------------------------------------------------------- */

static bool debug_nn = false;
static bool is_initialised = false;

uint8_t *snapshot_buf;


/* Camera configuration ---------------------------------------------------- */

static camera_config_t camera_config = {

    .pin_pwdn       = PWDN_GPIO_NUM,
    .pin_reset      = RESET_GPIO_NUM,
    .pin_xclk       = XCLK_GPIO_NUM,

    .pin_sscb_sda   = SIOD_GPIO_NUM,
    .pin_sscb_scl   = SIOC_GPIO_NUM,

    .pin_d7         = Y9_GPIO_NUM,
    .pin_d6         = Y8_GPIO_NUM,
    .pin_d5         = Y7_GPIO_NUM,
    .pin_d4         = Y6_GPIO_NUM,
    .pin_d3         = Y5_GPIO_NUM,
    .pin_d2         = Y4_GPIO_NUM,
    .pin_d1         = Y3_GPIO_NUM,
    .pin_d0         = Y2_GPIO_NUM,

    .pin_vsync      = VSYNC_GPIO_NUM,
    .pin_href       = HREF_GPIO_NUM,
    .pin_pclk       = PCLK_GPIO_NUM,

    .xclk_freq_hz   = 20000000,

    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,

    .pixel_format   = PIXFORMAT_JPEG,
    .frame_size     = FRAMESIZE_QVGA,

    .jpeg_quality   = 12,

    .fb_count       = 1,
    .fb_location    = CAMERA_FB_IN_PSRAM,
    .grab_mode      = CAMERA_GRAB_WHEN_EMPTY
};


/* Function definitions --------------------------------------------------- */

bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(
    uint32_t img_width,
    uint32_t img_height,
    uint8_t *out_buf
);


/* --------------------------------------------------------------------------
   SETUP
   -------------------------------------------------------------------------- */

void setup()
{
    Serial.begin(115200);

    // ESP32 GPIO1 sends detection signal to Arduino Mega
    pinMode(SQUIRREL_SIGNAL_PIN, OUTPUT);

    // Start with NO SQUIRREL
    digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32-S3 CAM V1.2");
    Serial.println("Edge Impulse Squirrel Detection");
    Serial.println("========================================");

    if (ei_camera_init() == false) {

        ei_printf("Failed to initialize Camera!\r\n");

        // Keep signal LOW if camera fails
        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        return;
    }
    else {

        ei_printf("Camera initialized\r\n");
    }

    ei_printf("\nStarting continuous inference in 2 seconds...\n");

    ei_sleep(2000);
}


/* --------------------------------------------------------------------------
   MAIN LOOP
   -------------------------------------------------------------------------- */

void loop()
{
    if (ei_sleep(5) != EI_IMPULSE_OK) {
        return;
    }


    /* Allocate image buffer ---------------------------------------------- */

    snapshot_buf = (uint8_t*)malloc(
        EI_CAMERA_RAW_FRAME_BUFFER_COLS *
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
        EI_CAMERA_FRAME_BYTE_SIZE
    );


    if (snapshot_buf == nullptr) {

        ei_printf("ERR: Failed to allocate snapshot buffer!\n");

        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        return;
    }


    /* Create signal ------------------------------------------------------ */

    ei::signal_t signal;

    signal.total_length =
        EI_CLASSIFIER_INPUT_WIDTH *
        EI_CLASSIFIER_INPUT_HEIGHT;

    signal.get_data = &ei_camera_get_data;


    /* Capture camera image ---------------------------------------------- */

    if (
        ei_camera_capture(
            (size_t)EI_CLASSIFIER_INPUT_WIDTH,
            (size_t)EI_CLASSIFIER_INPUT_HEIGHT,
            snapshot_buf
        ) == false
    ) {

        ei_printf("Failed to capture image\r\n");

        free(snapshot_buf);

        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        return;
    }


    /* Run Edge Impulse classifier --------------------------------------- */

    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err =
        run_classifier(&signal, &result, debug_nn);


    if (err != EI_IMPULSE_OK) {

        ei_printf(
            "ERR: Failed to run classifier (%d)\n",
            err
        );

        free(snapshot_buf);

        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        return;
    }


    /* Print timing ------------------------------------------------------- */

    ei_printf(
        "Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
        result.timing.dsp,
        result.timing.classification,
        result.timing.anomaly
    );


    /* --------------------------------------------------------------------
       OBJECT DETECTION / FOMO
       -------------------------------------------------------------------- */

#if EI_CLASSIFIER_OBJECT_DETECTION == 1

    bool squirrel_detected = false;

    ei_printf("Object detection bounding boxes:\r\n");


    for (uint32_t i = 0;
         i < result.bounding_boxes_count;
         i++)
    {
        ei_impulse_result_bounding_box_t bb =
            result.bounding_boxes[i];


        if (bb.value == 0) {
            continue;
        }


        ei_printf(
            "  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
            bb.label,
            bb.value,
            bb.x,
            bb.y,
            bb.width,
            bb.height
        );


        /* Check for squirrel */

        if (
            strcmp(bb.label, "squirrel") == 0 &&
            bb.value >= SQUIRREL_THRESHOLD
        )
        {
            squirrel_detected = true;
        }
    }


    /* Send result to Arduino Mega */

    if (squirrel_detected)
    {
        digitalWrite(SQUIRREL_SIGNAL_PIN, HIGH);

        Serial.println(
            ">>> SQUIRREL DETECTED -> GPIO1 HIGH"
        );
    }
    else
    {
        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        Serial.println(
            ">>> NO SQUIRREL -> GPIO1 LOW"
        );
    }


    /* --------------------------------------------------------------------
       CLASSIFICATION
       -------------------------------------------------------------------- */

#else

    bool squirrel_detected = false;

    ei_printf("Predictions:\r\n");


    for (uint16_t i = 0;
         i < EI_CLASSIFIER_LABEL_COUNT;
         i++)
    {
        float confidence =
            result.classification[i].value;

        const char *label =
            ei_classifier_inferencing_categories[i];


        ei_printf(
            "  %s: %.5f\r\n",
            label,
            confidence
        );


        if (
            strcmp(label, "squirrel") == 0 &&
            confidence >= SQUIRREL_THRESHOLD
        )
        {
            squirrel_detected = true;
        }
    }


    /* Send result to Arduino Mega */

    if (squirrel_detected)
    {
        digitalWrite(SQUIRREL_SIGNAL_PIN, HIGH);

        Serial.println(
            ">>> SQUIRREL DETECTED -> GPIO1 HIGH"
        );
    }
    else
    {
        digitalWrite(SQUIRREL_SIGNAL_PIN, LOW);

        Serial.println(
            ">>> NO SQUIRREL -> GPIO1 LOW"
        );
    }

#endif


    /* Anomaly prediction ------------------------------------------------ */

#if EI_CLASSIFIER_HAS_ANOMALY

    ei_printf(
        "Anomaly prediction: %.3f\r\n",
        result.anomaly
    );

#endif


    /* Free image buffer -------------------------------------------------- */

    free(snapshot_buf);
}


/* --------------------------------------------------------------------------
   CAMERA INITIALIZATION
   -------------------------------------------------------------------------- */

bool ei_camera_init(void)
{
    if (is_initialised)
        return true;


    /* Initialize camera */

    esp_err_t err =
        esp_camera_init(&camera_config);


    if (err != ESP_OK)
    {
        Serial.printf(
            "Camera init failed with error 0x%x\n",
            err
        );

        return false;
    }


    /* Camera sensor settings */

    sensor_t *s =
        esp_camera_sensor_get();


    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }


    is_initialised = true;

    return true;
}


/* --------------------------------------------------------------------------
   CAMERA DEINITIALIZATION
   -------------------------------------------------------------------------- */

void ei_camera_deinit(void)
{
    esp_err_t err =
        esp_camera_deinit();


    if (err != ESP_OK)
    {
        ei_printf(
            "Camera deinit failed\n"
        );

        return;
    }


    is_initialised = false;
}


/* --------------------------------------------------------------------------
   CAMERA CAPTURE
   -------------------------------------------------------------------------- */

bool ei_camera_capture(
    uint32_t img_width,
    uint32_t img_height,
    uint8_t *out_buf
)
{
    bool do_resize = false;


    if (!is_initialised)
    {
        ei_printf(
            "ERR: Camera is not initialized\r\n"
        );

        return false;
    }


    /* Capture frame */

    camera_fb_t *fb =
        esp_camera_fb_get();


    if (!fb)
    {
        ei_printf(
            "Camera capture failed\n"
        );

        return false;
    }


    /* Convert JPEG to RGB888 */

    bool converted =
        fmt2rgb888(
            fb->buf,
            fb->len,
            PIXFORMAT_JPEG,
            out_buf
        );


    esp_camera_fb_return(fb);


    if (!converted)
    {
        ei_printf(
            "Conversion failed\n"
        );

        return false;
    }


    /* Resize if necessary */

    if (
        (img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) ||
        (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)
    )
    {
        do_resize = true;
    }


    if (do_resize)
    {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height
        );
    }


    return true;
}


/* --------------------------------------------------------------------------
   EDGE IMPULSE IMAGE DATA
   -------------------------------------------------------------------------- */

static int ei_camera_get_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    /* RGB888 buffer */

    size_t pixel_ix =
        offset * 3;

    size_t pixels_left =
        length;

    size_t out_ptr_ix =
        0;


    while (pixels_left != 0)
    {
        /* Convert BGR to RGB */

        out_ptr[out_ptr_ix] =
            (
                snapshot_buf[pixel_ix + 2] << 16
            ) +
            (
                snapshot_buf[pixel_ix + 1] << 8
            ) +
            snapshot_buf[pixel_ix];


        out_ptr_ix++;

        pixel_ix += 3;

        pixels_left--;
    }


    return 0;
}


/* --------------------------------------------------------------------------
   VERIFY CAMERA MODEL
   -------------------------------------------------------------------------- */

#if !defined(EI_CLASSIFIER_SENSOR) || \
    EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA

#error "Invalid model for current sensor"

#endif
