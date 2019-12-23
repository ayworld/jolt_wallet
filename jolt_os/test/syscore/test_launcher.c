#include "bipmnemonic.h"
#include "esp32/clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "jolt_helpers.h"
#include "jolt_lib.h"
#include "syscore/cli.h"
#include "syscore/filesystem.h"
#include "syscore/launcher.h"
#include "unity.h"

static const char MODULE_NAME[] = "[jolt_launcher]";

#define JELF_OFFSET_MAGIC         0
#define JELF_OFFSET_VERSION_MAJOR JELF_OFFSET_MAGIC + 6
#define JELF_OFFSET_VERSION_MINOR JELF_OFFSET_VERSION_MAJOR + 1
#define JELF_OFFSET_VERSION_PATCH JELF_OFFSET_VERSION_MAJOR + 2

/**
 * Compiled Jolt App for "Hello World"
 * https://github.com/joltwallet/japp_hello_world
 */
static uint8_t hello_world_jelf_v_0_1_0[] = {
        0x7f, 0x4a, 0x45, 0x4c, 0x46, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xa1, 0x07, 0xbf, 0xf3, 0xce,
        0x10, 0xbe, 0x1d, 0x70, 0xdd, 0x18, 0xe7, 0x4b, 0xc0, 0x99, 0x67, 0xe4, 0xd6, 0x30, 0x9b, 0xa5, 0x0d, 0x5f,
        0x1d, 0xdc, 0x86, 0x64, 0x12, 0x55, 0x31, 0xb8, 0x7a, 0x88, 0xa1, 0x53, 0xc8, 0x19, 0x56, 0x6a, 0xb7, 0xd7,
        0x7a, 0x3d, 0x40, 0x25, 0x2c, 0x64, 0xdc, 0x5a, 0xac, 0x96, 0x71, 0x42, 0x8b, 0xcb, 0xc4, 0xa1, 0x53, 0x4c,
        0x3d, 0x90, 0x2f, 0xfb, 0xd4, 0x88, 0x82, 0x36, 0x20, 0x6a, 0x71, 0xe6, 0xfa, 0x21, 0xd7, 0x38, 0xc3, 0xf3,
        0x02, 0xfa, 0xc8, 0x7d, 0xaa, 0xdb, 0xe7, 0x62, 0xb8, 0x16, 0x4d, 0x8a, 0xad, 0x37, 0xa0, 0xc3, 0x29, 0x0c,
        0x06, 0x00, 0x07, 0x00, 0x2c, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x65, 0x64, 0x32, 0x35, 0x35, 0x31,
        0x39, 0x20, 0x73, 0x65, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0xc7, 0x55, 0x8e, 0xcd, 0x0a, 0x01, 0x51, 0x18, 0x86,
        0x9f, 0x99, 0xe3, 0x67, 0xca, 0xa8, 0xe3, 0x37, 0x16, 0x34, 0xd9, 0x2a, 0x35, 0xa5, 0x61, 0xaf, 0x48, 0xc9,
        0xce, 0x05, 0xc8, 0x4f, 0x36, 0x28, 0xe5, 0x02, 0xec, 0xb8, 0x0c, 0x77, 0x62, 0xe9, 0x26, 0x94, 0x4b, 0x70,
        0x09, 0xde, 0x19, 0x49, 0xde, 0xc5, 0x73, 0xde, 0xef, 0x59, 0x9c, 0xef, 0x03, 0xb8, 0x3b, 0x69, 0xba, 0x0e,
        0x44, 0x06, 0xca, 0x2e, 0x0c, 0x8d, 0x61, 0xea, 0xa8, 0x28, 0xa2, 0x6c, 0x8a, 0x82, 0xe5, 0x61, 0xe3, 0x6e,
        0x2d, 0xdf, 0x14, 0xf9, 0x25, 0x9a, 0x13, 0xc2, 0x15, 0x6e, 0x01, 0x67, 0x23, 0x5c, 0xc2, 0x6a, 0x2b, 0x9e,
        0x4f, 0xf0, 0xf4, 0x12, 0x1b, 0x0a, 0x7d, 0xe3, 0xbb, 0xbd, 0x5a, 0xfa, 0x23, 0x1b, 0xaf, 0xc9, 0xfc, 0xb8,
        0x5b, 0x6c, 0x56, 0xcb, 0x60, 0x7d, 0xd8, 0x6f, 0x83, 0xd1, 0x6c, 0xcc, 0xbf, 0x19, 0x4c, 0xc6, 0x5a, 0x99,
        0xd5, 0xff, 0x19, 0x31, 0x45, 0x89, 0x1c, 0x75, 0xaa, 0xea, 0x9e, 0xde, 0x2c, 0x7e, 0x32, 0x41, 0x5b, 0xbe,
        0x4b, 0x47, 0xc6, 0x8a, 0xcd, 0xe4, 0xa2, 0x02, 0xf1, 0xdd, 0x79, 0xd1, 0x4b, 0xe8, 0x53, 0x11, 0x2d, 0x0d,
        0xf1, 0x0d, 0x07, 0x28, 0x1d, 0xb7};

/**
 * Screen data for "Invalid or corrupt application."
 */
static uint8_t failure_scr_data[] = {
        0x01, 0xF8, 0x83, 0x40, 0x03, 0xF8, 0x00, 0xC0, 0x83, 0xA0, 0x04, 0xC0, 0x00, 0x08, 0xF8, 0x82, 0x00, 0x02,
        0x08, 0xF8, 0x82, 0x00, 0x01, 0xC0, 0x83, 0x20, 0x01, 0xC0, 0x83, 0x00, 0x07, 0xF8, 0x00, 0xC0, 0x00, 0xF8,
        0x00, 0xC0, 0x83, 0x20, 0x04, 0xC0, 0x00, 0xE0, 0x40, 0x82, 0x20, 0x04, 0x40, 0x00, 0x08, 0xF8, 0x82, 0x00,
        0x01, 0xC0, 0x82, 0x20, 0x02, 0x40, 0xF8, 0xC9, 0x00, 0x07, 0x0B, 0x08, 0x48, 0xC8, 0x4B, 0x08, 0x09, 0x84,
        0x0A, 0x0A, 0x08, 0x0A, 0x0B, 0x0A, 0x08, 0x0A, 0x0B, 0x0A, 0x08, 0x09, 0x83, 0x0A, 0x02, 0x49, 0xC8, 0x82,
        0x08, 0x0D, 0x09, 0x4A, 0x09, 0x0A, 0x09, 0x08, 0x09, 0x0A, 0xCA, 0x0A, 0x09, 0x08, 0x0B, 0x85, 0x08, 0x05,
        0x0A, 0x0B, 0x0A, 0x08, 0x09, 0x83, 0x0A, 0x01, 0x0B, 0xA3, 0x08, 0x01, 0xC8, 0xA5, 0x08, 0x82, 0x00, 0x06,
        0x10, 0x1F, 0x10, 0x00, 0x1F, 0x02, 0x82, 0x01, 0x0A, 0x1E, 0x00, 0x07, 0x08, 0x10, 0x08, 0x07, 0x00, 0x08,
        0x15, 0x82, 0x95, 0x0B, 0x1E, 0x00, 0x10, 0x9F, 0x10, 0x00, 0x11, 0x1F, 0x10, 0x00, 0x0E, 0x82, 0x11, 0x02,
        0x12, 0x1F, 0x83, 0x00, 0x01, 0x8E, 0x83, 0x11, 0x04, 0x8E, 0x00, 0x1F, 0x02, 0x82, 0x01, 0x01, 0x02, 0x83,
        0x00, 0x01, 0x0E, 0x83, 0x11, 0x02, 0x00, 0x0E, 0x83, 0x11, 0x04, 0x0E, 0x00, 0x1F, 0x02, 0x82, 0x01, 0x04,
        0x02, 0x00, 0x1F, 0x02, 0x82, 0x01, 0x03, 0x02, 0x00, 0x0F, 0x83, 0x10, 0x03, 0x0F, 0x00, 0x3F, 0x83, 0x09,
        0x05, 0x06, 0x00, 0x01, 0x0F, 0x11, 0xA6, 0x00, 0x01, 0x10, 0x83, 0x2A, 0x03, 0x3C, 0x00, 0x7E, 0x83, 0x12,
        0x03, 0x0C, 0x00, 0x7E, 0x83, 0x12, 0x0B, 0x0C, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x22, 0x3E, 0x20, 0x00, 0x1C,
        0x83, 0x22, 0x02, 0x00, 0x10, 0x83, 0x2A, 0x0B, 0x3C, 0x00, 0x02, 0x1F, 0x22, 0x00, 0x22, 0x3E, 0x20, 0x00,
        0x1C, 0x83, 0x22, 0x04, 0x1C, 0x00, 0x3E, 0x04, 0x82, 0x02, 0x02, 0x3C, 0x00, 0x82, 0x30, 0xFF, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xC7, 0x00};

static const char test_app_name[] = "Hello World";

/**
 * @brief Tests a standard Jolt Application launch..
 */
TEST_CASE( "basic launch", MODULE_NAME )
{
    char *fn;
    FILE *f;
    int n, rc;

    vault_clear();

    fn = jolt_fs_parse( test_app_name, "jelf" );
    assert( fn );

    f = fopen( fn, "w" );
    TEST_ASSERT_NOT_NULL_MESSAGE( f, "Failed to open file" );
    n = fwrite( hello_world_jelf_v_0_1_0, 1, sizeof( hello_world_jelf_v_0_1_0 ), f );
    TEST_ASSERT_EQUAL_INT_MESSAGE( n, sizeof( hello_world_jelf_v_0_1_0 ), "Failed to write hello world app" );
    fclose( f );

    /* Need an argc > 0 in order to get a return code */
    rc = jolt_launch_file( test_app_name, 1, NULL, EMPTY_STR );
    TEST_ASSERT_EQUAL_INT_MESSAGE( 0, rc, "Failed to launch file." );

    /* Unlock to launch app */
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
    vault_set_pin_auto_enter();

    rc = jolt_cli_get_return();
    TEST_ASSERT_EQUAL_INT_MESSAGE( 0, rc, "App returned non-zero return code." );

    remove( fn );

    free( fn );
}

/**
 * @brief Tests for when the app patch version exceeds the current version.
 *
 * Should always succeed.
 */
TEST_CASE( "greater patch launch", MODULE_NAME )
{
    char *fn;
    FILE *f;
    int n, rc;
    const uint8_t new_patch = JOLT_JELF_VERSION.patch + 1;

    vault_clear();

    fn = jolt_fs_parse( test_app_name, "jelf" );
    assert( fn );

    f = fopen( fn, "w" );
    assert( f );
    n = fwrite( hello_world_jelf_v_0_1_0, 1, sizeof( hello_world_jelf_v_0_1_0 ), f );
    TEST_ASSERT_EQUAL_INT_MESSAGE( n, sizeof( hello_world_jelf_v_0_1_0 ), "Failed to write hello world app" );
    fseek( f, JELF_OFFSET_VERSION_PATCH, SEEK_SET );
    fwrite( &new_patch, 1, 1, f );
    fclose( f );

    /* Need an argc > 0 in order to get a return code */
    rc = jolt_launch_file( test_app_name, 1, NULL, EMPTY_STR );
    TEST_ASSERT_EQUAL_INT_MESSAGE( 0, rc, "Failed to launch file." );

    /* Unlock to launch app */
    vTaskDelay( pdMS_TO_TICKS( 50 ) );
    vault_set_pin_auto_enter();

    /* Get the app's return value */
    rc = jolt_cli_get_return();
    TEST_ASSERT_EQUAL_INT_MESSAGE( 0, rc, "App returned non-zero return code." );

    remove( fn );

    free( fn );
}

/**
 * @brief Tests for when the app minor version exceeds the current version.
 *
 * Should always fail.
 */
TEST_CASE( "greater minor launch", MODULE_NAME )
{
    char *fn;
    FILE *f;
    int n, rc;
    const uint8_t new_minor = JOLT_JELF_VERSION.minor + 1;

    vault_clear();

    fn = jolt_fs_parse( test_app_name, "jelf" );
    assert( fn );

    f = fopen( fn, "w" );
    assert( f );
    n = fwrite( hello_world_jelf_v_0_1_0, 1, sizeof( hello_world_jelf_v_0_1_0 ), f );
    TEST_ASSERT_EQUAL_INT_MESSAGE( n, sizeof( hello_world_jelf_v_0_1_0 ), "Failed to write hello world app" );
    fseek( f, JELF_OFFSET_VERSION_MINOR, SEEK_SET );
    fwrite( &new_minor, 1, 1, f );
    fclose( f );

    /* Need an argc > 0 in order to get a return code */
    rc = jolt_launch_file( test_app_name, 1, NULL, EMPTY_STR );
    TEST_ASSERT_EQUAL_INT( -1, rc );

    vTaskDelay( pdMS_TO_TICKS( 50 ) );

    jolt_display_t expected_scr = {
            .type     = JOLT_DISPLAY_TYPE_SSD1306,
            .encoding = JOLT_ENCODING_JRLE,
            .len      = sizeof( failure_scr_data ),
            .data     = failure_scr_data,
    };

    TEST_ASSERT_EQUAL_DISPLAY( &expected_scr, NULL );
    JOLT_BACK;  // Exit the error screen.

    remove( fn );

    free( fn );
}

/**
 * @brief Tests for when the app minor version exceeds the current version.
 *
 * Should always fail.
 */
TEST_CASE( "greater major launch", MODULE_NAME )
{
    char *fn;
    FILE *f;
    int n, rc;
    const uint8_t new_minor = JOLT_JELF_VERSION.minor + 1;

    vault_clear();

    fn = jolt_fs_parse( test_app_name, "jelf" );
    assert( fn );

    f = fopen( fn, "w" );
    assert( f );
    n = fwrite( hello_world_jelf_v_0_1_0, 1, sizeof( hello_world_jelf_v_0_1_0 ), f );
    TEST_ASSERT_EQUAL_INT_MESSAGE( n, sizeof( hello_world_jelf_v_0_1_0 ), "Failed to write hello world app" );
    fseek( f, JELF_OFFSET_VERSION_MAJOR, SEEK_SET );
    fwrite( &new_minor, 1, 1, f );
    fclose( f );

    /* Need an argc > 0 in order to get a return code */
    rc = jolt_launch_file( test_app_name, 1, NULL, EMPTY_STR );
    TEST_ASSERT_EQUAL_INT( -1, rc );

    vTaskDelay( pdMS_TO_TICKS( 50 ) );

    jolt_display_t expected_scr = {
            .type     = JOLT_DISPLAY_TYPE_SSD1306,
            .encoding = JOLT_ENCODING_JRLE,
            .len      = sizeof( failure_scr_data ),
            .data     = failure_scr_data,
    };

    TEST_ASSERT_EQUAL_DISPLAY( &expected_scr, NULL );
    JOLT_BACK;  // Exit the error screen.

    remove( fn );

    free( fn );
}
