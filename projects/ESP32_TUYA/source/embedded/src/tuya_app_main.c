#include "cJSON.h"
#include "netmgr.h"
#include "tal_api.h"
#include "tal_cli.h"
#include "tal_time_service.h"
#include "tal_workq_service.h"
#include "tkl_gpio.h"
#include "tkl_output.h"
#include "tuya_authorize.h"
#include "tuya_config.h"
#include "tuya_dp_profile.h"
#include "tuya_iot.h"
#include "tuya_iot_dp.h"
#include "board_com_api.h"
#include "netcfg.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "1.0.0"
#endif

static tuya_iot_client_t app_client;
static bool app_switch_states[3] = {false, false, false};

static const TUYA_GPIO_NUM_E app_relay_pins[3] = {
    TUYA_GPIO_NUM_25,
    TUYA_GPIO_NUM_26,
    TUYA_GPIO_NUM_27,
};

static void app_fatal_error(const char *operation, OPERATE_RET error)
{
    PR_ERR("%s failed: %d", operation, error);
    for (;;) {
        tal_system_sleep(1000);
    }
}

static OPERATE_RET app_relays_init(void)
{
    const TUYA_GPIO_BASE_CFG_T config = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_LOW,
    };

    for (uint32_t i = 0; i < 3; i++) {
        OPERATE_RET ret = tkl_gpio_init(app_relay_pins[i], &config);
        if (ret != OPRT_OK) {
            return ret;
        }

        ret = tkl_gpio_write(app_relay_pins[i], TUYA_GPIO_LEVEL_LOW);
        if (ret != OPRT_OK) {
            return ret;
        }
    }

    return OPRT_OK;
}

static OPERATE_RET app_switch_set(uint32_t dpid, bool enabled)
{
    uint32_t index;

    switch (dpid) {
    case DPID_SWITCH_1:
        index = 0;
        break;
    case DPID_SWITCH_2:
        index = 1;
        break;
    case DPID_SWITCH_3:
        index = 2;
        break;
    default:
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET ret = tkl_gpio_write(
        app_relay_pins[index],
        enabled ? TUYA_GPIO_LEVEL_HIGH : TUYA_GPIO_LEVEL_LOW);
    if (ret == OPRT_OK) {
        app_switch_states[index] = enabled;
        PR_INFO("switch %u: %s", (unsigned int)(index + 1), enabled ? "ON" : "OFF");
    }

    return ret;
}

static bool app_network_check(void)
{
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    netmgr_status_e status = NETMGR_LINK_DOWN;
    OPERATE_RET ret = netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    if (ret != OPRT_OK) {
        PR_WARN("Read network status failed: %d", ret);
        return false;
    }

    return status != NETMGR_LINK_DOWN;
#else
    return false;
#endif
}

static void app_report_switch_states(tuya_iot_client_t *client)
{
    dp_obj_t dps[3] = {
        {.id = DPID_SWITCH_1, .type = PROP_BOOL, .value.dp_bool = app_switch_states[0]},
        {.id = DPID_SWITCH_2, .type = PROP_BOOL, .value.dp_bool = app_switch_states[1]},
        {.id = DPID_SWITCH_3, .type = PROP_BOOL, .value.dp_bool = app_switch_states[2]},
    };

    int ret = tuya_iot_dp_obj_report(client, client->activate.devid, dps, 3, 0);
    if (ret != OPRT_OK) {
        PR_WARN("Report initial switch states failed: %d", ret);
    }
}

static void app_event_handler(tuya_iot_client_t *client, tuya_event_msg_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->id) {
    case TUYA_EVENT_BIND_START:
        PR_INFO("Tuya device pairing started");
        break;
    case TUYA_EVENT_MQTT_CONNECTED:
        PR_INFO("Tuya cloud connected");
        app_report_switch_states(client);
        break;
    case TUYA_EVENT_MQTT_DISCONNECT:
        PR_WARN("Tuya cloud disconnected");
        break;
    case TUYA_EVENT_DP_RECEIVE_OBJ: {
        dp_obj_recv_t *received = event->value.dpobj;
        if (received == NULL) {
            PR_ERR("Received empty DP object");
            break;
        }

        dp_obj_t accepted[3] = {0};
        uint16_t accepted_count = 0;

        for (uint32_t i = 0; i < received->dpscnt; i++) {
            dp_obj_t *dp = &received->dps[i];
            if (dp->type != PROP_BOOL) {
                PR_WARN("Ignoring non-boolean DP %u (type %d)", dp->id, dp->type);
                continue;
            }

            if (dp->id != DPID_SWITCH_1 &&
                dp->id != DPID_SWITCH_2 &&
                dp->id != DPID_SWITCH_3) {
                PR_WARN("Ignoring unsupported DP %u", dp->id);
                continue;
            }

            OPERATE_RET ret = app_switch_set(dp->id, dp->value.dp_bool);
            if (ret != OPRT_OK) {
                PR_ERR("Apply DP %u failed: %d", dp->id, ret);
                continue;
            }

            accepted[accepted_count++] = *dp;
        }

        if (accepted_count > 0) {
            const char *devid = received->devid != NULL
                                    ? received->devid
                                    : tuya_iot_devid_get(client);
            int ret = tuya_iot_dp_obj_report(client, devid, accepted, accepted_count, 0);
            if (ret != OPRT_OK) {
                PR_WARN("Report switch command result failed: %d", ret);
            }
        }
        break;
    }
    case TUYA_EVENT_RESET_COMPLETE:
        PR_INFO("Tuya reset completed; rebooting");
        tal_system_reset();
        break;
    default:
        break;
    }
}

static void user_main(void)
{
    OPERATE_RET ret;

    cJSON_InitHooks(&(cJSON_Hooks){.malloc_fn = tal_malloc, .free_fn = tal_free});
    ret = tal_time_service_init();
    if (ret != OPRT_OK) {
        app_fatal_error("time service init", ret);
    }
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    board_register_hardware();
    PR_INFO("Starting %s %s", PROJECT_NAME, PROJECT_VERSION);

    ret = app_relays_init();
    if (ret != OPRT_OK) {
        app_fatal_error("relay GPIO init", ret);
    }

    ret = tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    if (ret != OPRT_OK) {
        app_fatal_error("KV init", ret);
    }

    ret = tal_sw_timer_init();
    if (ret != OPRT_OK) {
        app_fatal_error("software timer init", ret);
    }

    ret = tal_workq_init();
    if (ret != OPRT_OK) {
        app_fatal_error("work queue init", ret);
    }

    ret = tal_cli_init();
    if (ret != OPRT_OK) {
        app_fatal_error("CLI init", ret);
    }

    ret = tuya_authorize_init();
    if (ret != OPRT_OK) {
        app_fatal_error("authorization init", ret);
    }

    tuya_iot_license_t license = {0};
    if (tuya_authorize_read(&license) != OPRT_OK) {
        license.uuid = TUYA_OPENSDK_UUID;
        license.authkey = TUYA_OPENSDK_AUTHKEY;
        PR_WARN("No device authorization stored. Add a test license before pairing.");
    }

    ret = tuya_iot_init(&app_client, &(const tuya_iot_config_t){
        .software_ver = PROJECT_VERSION,
        .productkey = TUYA_PRODUCT_ID,
        .uuid = license.uuid,
        .authkey = license.authkey,
        .event_handler = app_event_handler,
        .network_check = app_network_check,
    });
    if (ret != OPRT_OK) {
        app_fatal_error("Tuya IoT init", ret);
    }

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    ret = netmgr_init(NETCONN_WIFI);
    if (ret != OPRT_OK) {
        app_fatal_error("Wi-Fi manager init", ret);
    }

    netcfg_args_t netcfg = {.type = NETCFG_TUYA_WIFI_AP};
#if defined(ENABLE_BLUETOOTH) && (ENABLE_BLUETOOTH == 1)
    netcfg.type |= NETCFG_TUYA_BLE;
#endif
    ret = netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_NETCFG, &netcfg);
    if (ret != OPRT_OK) {
        app_fatal_error("Wi-Fi provisioning init", ret);
    }
#else
    app_fatal_error("Wi-Fi is disabled in this build", OPRT_NOT_SUPPORTED);
#endif

    ret = tuya_iot_start(&app_client);
    if (ret != OPRT_OK) {
        app_fatal_error("Tuya IoT start", ret);
    }

    PR_INFO("Three-channel switch firmware ready");
    for (;;) {
        ret = tuya_iot_yield(&app_client);
        if (ret != OPRT_OK) {
            PR_WARN("Tuya IoT yield failed: %d", ret);
            tal_system_sleep(100);
        }
    }
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    user_main();
}
#else
static THREAD_HANDLE ty_app_thread = NULL;

static void tuya_app_thread(void *arg)
{
    (void)arg;
    user_main();
    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thread_config = {0};
    thread_config.stackDepth = 1024 * 4;
    thread_config.priority = THREAD_PRIO_1;
    thread_config.thrdname = "tuya_app_main";

    OPERATE_RET ret = tal_thread_create_and_start(
        &ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thread_config);
    if (ret != OPRT_OK) {
        PR_ERR("Create app thread failed: %d", ret);
    }
}
#endif
