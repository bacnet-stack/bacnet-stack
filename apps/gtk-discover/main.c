/**
 * @file
 * @brief GTK-based BACnet Device and Object Property Discovery
 * @author Steve Karg <skarg@users.sourceforge.net
 * @date August 2025
 * @copyright SPDX-License-Identifier: MIT
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
#include "bacnet/whois.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/client/bac-discover.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

/* Global variables */
static GtkWidget *main_window;
static GtkWidget *device_tree_view;
static GtkWidget *object_tree_view;
static GtkWidget *property_tree_view;
static GtkListStore *device_store;
static GtkListStore *object_store;
static GtkListStore *property_store;

/* BACnet global variables */
static uint8_t Rx_Buf[MAX_MPDU];
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;

/* GTK interfaces */
static bool bacnet_initialized = false;
static guint bacnet_timeout_id = 0;

/* Tree store columns */
enum {
    DEVICE_COL_ID,
    DEVICE_COL_NAME,
    DEVICE_COL_MODEL,
    DEVICE_COL_ADDRESS,
    DEVICE_NUM_COLS
};

enum {
    OBJECT_COL_TYPE,
    OBJECT_COL_TYPE_NAME,
    OBJECT_COL_DEVICE_ID,
    OBJECT_COL_OBJECT_ID,
    OBJECT_COL_NAME,
    OBJECT_NUM_COLS
};

enum {
    PROPERTY_COL_ID,
    PROPERTY_COL_NAME,
    PROPERTY_COL_VALUE,
    PROPERTY_NUM_COLS
};

/**
 * @brief Make a string from the MAC address
 * @param str - Buffer to hold the string representation
 * @param str_len -
 * @param addr - MAC address to convert to a string
 * @param len - length of the MAC address
 * @return number of bytes in the string
 */
static int
bacapp_snprintf_macaddr(char *str, size_t str_len, const uint8_t *addr, int len)
{
    int ret_val = 0;
    int slen = 0;
    int j = 0;

    while (j < len) {
        if (j != 0) {
            bacapp_snprintf(str, str_len, ":");
            ret_val += bacapp_snprintf_shift(1, &str, &str_len);
        }
        slen = bacapp_snprintf(str, str_len, "%02X", addr[j]);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        j++;
    }

    return ret_val;
}

/**
 * @brief Make a string from the BACnet address
 * @param str - Buffer to hold the string representation
 * @param str_len -
 * @param address - BACnet address to convert to a string
 * @return number of bytes in the string
 */
static int
bacapp_snprintf_address(char *str, size_t str_len, BACNET_ADDRESS *address)
{
    int ret_val = 0;
    int slen = 0;
    uint8_t local_sadr = 0;

    /* MAC */
    slen =
        bacapp_snprintf_macaddr(str, str_len, address->mac, address->mac_len);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* NET */
    slen = bacapp_snprintf(str, str_len, ";%hu;", address->net);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* ADR */
    if (address->net) {
        slen =
            bacapp_snprintf_macaddr(str, str_len, address->adr, address->len);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    } else {
        slen = bacapp_snprintf_macaddr(str, str_len, &local_sadr, 1);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }

    return ret_val;
}

/**
 * @brief Add a discovered device to the GUI
 * @param device_id - BACnet Device Instance number
 * @param address - BACnet Address of the Device
 * @param device_model - BACnet Device Model
 * @param device_name - BACnet Device Name
 */
static void add_discovered_device_to_gui(
    uint32_t device_id,
    BACNET_ADDRESS *address,
    const char *device_model,
    const char *device_name)
{
    GtkTreeIter iter;
    char address_str[64] = "MAC-Address";

    /* Fill device structure */
    /* Create address string */
    if (address) {
        bacapp_snprintf_address(address_str, sizeof(address_str), address);
    }
    printf("%lu|%s|%s\n", (unsigned long)device_id, device_name, address_str);
    /* Add to GUI */
    gtk_list_store_append(device_store, &iter);
    gtk_list_store_set(
        device_store, &iter, DEVICE_COL_ID, device_id, DEVICE_COL_NAME,
        device_name, DEVICE_COL_MODEL, device_model, DEVICE_COL_ADDRESS,
        address_str, -1);
}

/**
 * @brief Add discovered objects for a device to the GUI
 * @param device_id - Device which contains objects
 */
static void add_discovered_objects_to_gui(uint32_t device_id)
{
    GtkTreeIter iter;
    unsigned int object_index = 0;
    unsigned int object_count = 0;
    BACNET_OBJECT_ID object_id = { 0 };
    char object_name[MAX_CHARACTER_STRING_BYTES] = { 0 };

    object_count = bacnet_discover_device_object_count(device_id);
    for (object_index = 0; object_index < object_count; object_index++) {
        if (bacnet_discover_device_object_identifier(
                device_id, object_index, &object_id)) {
            gtk_list_store_append(object_store, &iter);
            bacnet_discover_property_name(
                device_id, object_id.type, object_id.instance, PROP_OBJECT_NAME,
                object_name, sizeof(object_name), "");
            gtk_list_store_set(
                object_store, &iter, OBJECT_COL_TYPE, object_id.type,
                OBJECT_COL_TYPE_NAME, bactext_object_type_name(object_id.type),
                OBJECT_COL_DEVICE_ID, device_id, OBJECT_COL_OBJECT_ID,
                object_id.instance, OBJECT_COL_NAME, object_name, -1);
        }
    }
}

/**
 * @brief Handle device selection change
 * @param selection the selection that is changed
 * @param data optional data in the callback
 */
static void
on_device_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint device_id;

    (void)data;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, DEVICE_COL_ID, &device_id, -1);
        printf("Device selected: %u\n", device_id);
        /* Clear object store and reload objects for selected device */
        gtk_list_store_clear(object_store);
        gtk_list_store_clear(property_store);
        add_discovered_objects_to_gui(device_id);
    }
}

/**
 * @brief Add discovered properties for an object to the GUI
 * @param device_id Device instance of the object properties
 * @param object_type Object type for the properties
 * @param object_instance Object instance for the properties
 */
static void add_discovered_properties_to_gui(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    GtkTreeIter iter;
    unsigned int property_count = 0;
    unsigned int index = 0;
    uint32_t property_id = 0;
    BACNET_OBJECT_PROPERTY_VALUE object_value = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    bool status = false;
    int str_len = 0;

    property_count = bacnet_discover_object_property_count(
        device_id, object_type, object_instance);
    for (index = 0; index < property_count; index++) {
        gtk_list_store_append(property_store, &iter);
        bacnet_discover_object_property_identifier(
            device_id, object_type, object_instance, index, &property_id);
        status = bacnet_discover_property_value(
            device_id, object_type, object_instance, property_id, &value);
        if (status) {
            object_value.object_type = object_type;
            object_value.object_instance = object_instance;
            object_value.object_property = property_id;
            object_value.array_index = BACNET_ARRAY_ALL;
            object_value.value = &value;
            str_len = bacapp_snprintf_value(NULL, 0, &object_value);
            if (str_len > 0) {
                char str[str_len + 1];
                bacapp_snprintf_value(str, str_len + 1, &object_value);
                gtk_list_store_set(
                    property_store, &iter, PROPERTY_COL_ID, property_id,
                    PROPERTY_COL_NAME, bactext_property_name(property_id),
                    PROPERTY_COL_VALUE, str, -1);
            } else {
                status = false;
            }
        }
        if (!status) {
            gtk_list_store_set(
                property_store, &iter, PROPERTY_COL_ID, property_id,
                PROPERTY_COL_NAME, bactext_property_name(property_id),
                PROPERTY_COL_VALUE, "-", -1);
        }
    }
}

/**
 * @brief Handle object selection change
 * @param selection selection that was changed
 * @param data optional data for the callback
 */
static void
on_object_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint object_instance, object_type, device_id;

    (void)data;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(
            model, &iter, OBJECT_COL_DEVICE_ID, &device_id,
            OBJECT_COL_OBJECT_ID, &object_instance, OBJECT_COL_TYPE,
            &object_type, -1);

        /* Clear property store and reload properties for selected object */
        gtk_list_store_clear(property_store);

        add_discovered_properties_to_gui(
            device_id, (BACNET_OBJECT_TYPE)object_type, object_instance);
    }
}

/**
 * @brief Handle discover devices button click
 * @param button button that was clicked
 * @param data optional data for the callback
 */
static void on_discover_devices_clicked(GtkButton *button, gpointer data)
{
    (void)button; /* unused parameter */
    (void)data; /* unused parameter */

    if (!bacnet_initialized) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "BACnet stack not initialized. Please restart the "
            "application.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    /* discover */
    Send_WhoIs_Global(0, 4194303);
}

/**
 * @brief Process discovered devices and add them to the GUI
 */
static void process_discovered_devices(void)
{
    unsigned int device_index = 0;
    unsigned int device_count = 0;
    unsigned int max_apdu = 0;
    BACNET_ADDRESS device_address = { 0 };
    uint32_t device_id = 0;
    char model_name[MAX_CHARACTER_STRING_BYTES] = { "-" };
    char object_name[MAX_CHARACTER_STRING_BYTES] = { "-" };

    device_count = bacnet_discover_device_count();
    for (device_index = 0; device_index < device_count; device_index++) {
        device_id = bacnet_discover_device_instance(device_index);
        bacnet_discover_property_name(
            device_id, OBJECT_DEVICE, device_id, PROP_MODEL_NAME, model_name,
            sizeof(model_name), "model-name");
        bacnet_discover_property_name(
            device_id, OBJECT_DEVICE, device_id, PROP_OBJECT_NAME, object_name,
            sizeof(object_name), "object-name");
        address_get_by_device(device_id, &max_apdu, &device_address);
        add_discovered_device_to_gui(
            device_id, &device_address, model_name, object_name);
    }
}

/**
 * @brief Handle refresh button click
 * @param button button that was clicked
 * @param data optional data for the callback
 */
static void on_refresh_clicked(GtkButton *button, gpointer data)
{
    (void)button; /* unused parameter */
    (void)data; /* unused parameter */

    gtk_list_store_clear(device_store);
    process_discovered_devices();
}

/**
 * @brief Setup the device tree view object
 */
static void setup_device_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    /* Create list store */
    device_store = gtk_list_store_new(
        DEVICE_NUM_COLS, G_TYPE_UINT, /* Device ID */
        G_TYPE_STRING, /* Device Name */
        G_TYPE_STRING, /* Device Model */
        G_TYPE_STRING); /* Address */

    /* Create tree view */
    device_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(device_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(device_tree_view), TRUE);

    /* Device ID column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Device ID", renderer, "text", DEVICE_COL_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Device Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Name", renderer, "text", DEVICE_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Device Model column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Model", renderer, "text", DEVICE_COL_MODEL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Address column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Address", renderer, "text", DEVICE_COL_ADDRESS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Setup selection changed callback */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(device_tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(
        selection, "changed", G_CALLBACK(on_device_selection_changed), NULL);
}

/**
 * @brief Setup the object tree view object
 */
static void setup_object_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    /* Create list store */
    object_store = gtk_list_store_new(
        OBJECT_NUM_COLS, G_TYPE_UINT, /* OBJECT_COL_TYPE */
        G_TYPE_STRING, /* OBJECT_COL_TYPE_NAME */
        G_TYPE_UINT, /* OBJECT_COL_DEVICE_ID */
        G_TYPE_UINT, /* OBJECT_COL_OBJECT_ID */
        G_TYPE_STRING); /* OBJECT_COL_NAME */

    /* Create tree view */
    object_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(object_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(object_tree_view), TRUE);

    /* Object Type Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Object Type", renderer, "text", OBJECT_COL_TYPE_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Object Instance column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Instance", renderer, "text", OBJECT_COL_OBJECT_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Object Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Name", renderer, "text", OBJECT_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Setup selection changed callback */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(object_tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(
        selection, "changed", G_CALLBACK(on_object_selection_changed), NULL);
}

/**
 * @brief Setup the property tree view object
 */
static void setup_property_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* Create list store */
    property_store = gtk_list_store_new(
        PROPERTY_NUM_COLS, G_TYPE_UINT, /* Property ID */
        G_TYPE_STRING, /* Property Name */
        G_TYPE_STRING); /* Property Value */

    /* Create tree view */
    property_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(property_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(property_tree_view), TRUE);

    /* Property Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Property", renderer, "text", PROPERTY_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(property_tree_view), column);

    /* Property Value column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Value", renderer, "text", PROPERTY_COL_VALUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(property_tree_view), column);
}

/**
 * @brief Create the main application window
 */
static void create_main_window(void)
{
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *hpaned, *vpaned;
    GtkWidget *scrolled_window;
    GtkWidget *discover_button, *refresh_button;
    GtkToolItem *tool_item;

    /* Create main window */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "BACnet Device Discovery");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 5);

    /* Connect destroy signal */
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Create main vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    /* Create toolbar */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* Add discover button */
    discover_button = gtk_button_new_with_label("Discover Devices");
    g_signal_connect(
        discover_button, "clicked", G_CALLBACK(on_discover_devices_clicked),
        NULL);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), discover_button);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Add refresh button */
    refresh_button = gtk_button_new_with_label("Refresh");
    g_signal_connect(
        refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), NULL);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), refresh_button);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Create horizontal paned widget */
    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

    /* Create vertical paned widget for right side */
    vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack2(GTK_PANED(hpaned), vpaned, TRUE, FALSE);

    /* Setup device tree view (left side) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 400, -1);
    setup_device_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), device_tree_view);
    gtk_paned_pack1(GTK_PANED(hpaned), scrolled_window, FALSE, FALSE);

    /* Setup object tree view (top right) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 200);
    setup_object_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), object_tree_view);
    gtk_paned_pack1(GTK_PANED(vpaned), scrolled_window, TRUE, FALSE);

    /* Setup property tree view (bottom right) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    setup_property_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), property_tree_view);
    gtk_paned_pack2(GTK_PANED(vpaned), scrolled_window, TRUE, FALSE);

    /* Set paned positions */
    gtk_paned_set_position(GTK_PANED(hpaned), 400);
    gtk_paned_set_position(GTK_PANED(vpaned), 200);
}

/**
 * @brief Non-blocking task for running BACnet server tasks
 */
static void bacnet_server_task(void)
{
    static bool initialized = false;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    const unsigned timeout_ms = 5;

    if (!initialized) {
        initialized = true;
        /* broadcast an I-Am on startup */
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* input */
    /* returns 0 bytes on timeout */
    pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
    /* process */
    if (pdu_len) {
        npdu_handler(&src, &Rx_Buf[0], pdu_len);
    }
    /* 1 second tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        mstimer_reset(&BACnet_Task_Timer);
        dcc_timer_seconds(1);
        datalink_maintenance_timer(1);
        dlenv_maintenance_timer(1);
    }
    if (mstimer_expired(&BACnet_TSM_Timer)) {
        mstimer_reset(&BACnet_TSM_Timer);
        tsm_timer_milliseconds(mstimer_interval(&BACnet_TSM_Timer));
    }
}

/* GTK timeout callback for BACnet processing */
static gboolean bacnet_task_timeout(gpointer data)
{
    (void)data; /* unused parameter */

    if (bacnet_initialized) {
        bacnet_server_task();
        bacnet_discover_task();
    }

    return TRUE; /* Continue calling this function */
}

/**
 * @brief Initialize the handlers for this server device
 */
static void bacnet_server_init(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* we need to handle who-has to support dynamic object binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    mstimer_set(&BACnet_Task_Timer, 1000);
    mstimer_set(&BACnet_TSM_Timer, 50);

    /* Start BACnet background processing */
    bacnet_timeout_id = g_timeout_add(100, bacnet_task_timeout, NULL);

    bacnet_initialized = true;
    printf("BACnet Stack initialized\n");
}

/* Cleanup BACnet stack */
static void bacnet_cleanup(void)
{
    if (bacnet_timeout_id > 0) {
        g_source_remove(bacnet_timeout_id);
        bacnet_timeout_id = 0;
    }

    if (bacnet_initialized) {
        datalink_cleanup();
        bacnet_initialized = false;
        printf("BACnet Stack cleanup completed\n");
    }
    bacnet_discover_cleanup();
}

/* Main function */
int main(int argc, char *argv[])
{
    BACNET_ADDRESS dest = { 0 };
    unsigned long discover_seconds = 60;

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Initialize BACnet */
    dlenv_init();
    bacnet_server_init();
    /* configure the discovery module */
    bacnet_discover_dest_set(&dest);
    bacnet_discover_seconds_set(discover_seconds);
    bacnet_discover_init();

    /* Create the main window */
    create_main_window();

    /* Show the window */
    gtk_widget_show_all(main_window);

    /* Start the GTK main loop */
    gtk_main();

    /* Cleanup */
    bacnet_cleanup();

    return 0;
}
