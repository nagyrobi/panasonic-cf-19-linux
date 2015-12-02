/*
 *  Panasonic Tablet Button driver
 *  (C) 2012 Heiher <admin@heiher.info>
 *
 *  derived from panasonic-laptop.c, Copyright (C) 2002-2004 John Belmonte
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  publicshed by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>


MODULE_AUTHOR("Heiher");
MODULE_DESCRIPTION("ACPI Tablet Button driver for Panasonic CF-18/19 laptops");
MODULE_LICENSE("GPL");

/* Define ACPI PATHs */
#define METHOD_HBTN_QUERY   "HINF"
#define HBTN_NOTIFY         0x80

#define ACPI_PCC_DRIVER_NAME    "Panasonic Tablet Button Support"
#define ACPI_PCC_DEVICE_NAME    "TabletButton"
#define ACPI_PCC_CLASS          "pcc"

#define ACPI_PCC_INPUT_PHYS    "panasonic/hbtn0"

static int acpi_pcc_hbtn_add(struct acpi_device *device);
static int acpi_pcc_hbtn_remove(struct acpi_device *device);
static void acpi_pcc_hbtn_notify(struct acpi_device *device, u32 event);

static const struct acpi_device_id pcc_device_ids[] = {
    { "MAT001F", 0},
    { "MAT0020", 0},
    { "", 0},
};
MODULE_DEVICE_TABLE(acpi, pcc_device_ids);

static struct acpi_driver acpi_pcc_driver = {
    .name =     ACPI_PCC_DRIVER_NAME,
    .class =    ACPI_PCC_CLASS,
    .ids =        pcc_device_ids,
    .ops =        {
                .add =       acpi_pcc_hbtn_add,
                .remove =    acpi_pcc_hbtn_remove,
                .notify =    acpi_pcc_hbtn_notify,
            },
};

static const struct key_entry panasonic_keymap[] = {
    { KE_KEY, 0x0, { KEY_RESERVED } },
    { KE_KEY, 0x4, { KEY_SCREENLOCK } }, /* Screen lock */
//    { KE_KEY, 0x6, { KEY_DIRECTION } }, /* Screen rotate */
    { KE_KEY, 0x6, { KEY_MSDOS } }, /* XF86DOS */
    { KE_KEY, 0x8, { KEY_ENTER } }, /* Enter */
//    { KE_KEY, 0xA, { KEY_KEYBOARD } }, /* Soft keyboard */
    { KE_KEY, 0xA, { KEY_MENU } }, /* XF86MenuKB */
    { KE_END, 0 }
};

struct pcc_acpi {
    acpi_handle        handle;
    struct acpi_device    *device;
    struct input_dev    *input_dev;
};

/* hbtn input device driver */

static void acpi_pcc_generate_keyinput(struct pcc_acpi *pcc)
{
    struct input_dev *hotk_input_dev = pcc->input_dev;
    int rc;
    unsigned long long result;
    struct key_entry *ke = NULL;

    rc = acpi_evaluate_integer(pcc->handle, METHOD_HBTN_QUERY,
                   NULL, &result);
    if (!ACPI_SUCCESS(rc)) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                 "error getting hbtn status\n"));
        return;
    }

    acpi_bus_generate_netlink_event(pcc->device->pnp.device_class,
				dev_name(&pcc->device->dev), HBTN_NOTIFY, result);

    ke = sparse_keymap_entry_from_scancode(hotk_input_dev, result & 0xe);
    if (!ke) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Unknown hbtn event: %d\n", result));
        return;
    }

    sparse_keymap_report_entry(hotk_input_dev, ke,
                (result & 0x1) ? 0 : 1, false);
}

static void acpi_pcc_hbtn_notify(struct acpi_device *device, u32 event)
{
    struct pcc_acpi *pcc = acpi_driver_data(device);

    switch (event) {
    case HBTN_NOTIFY:
        acpi_pcc_generate_keyinput(pcc);
        break;
    default:
        /* nothing to do */
        break;
    }
}

static int acpi_pcc_init_input(struct pcc_acpi *pcc)
{
    struct input_dev *input_dev;
    int error;

    input_dev = input_allocate_device();
    if (!input_dev) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Couldn't allocate input device for hbtn"));
        return -ENOMEM;
    }

    input_dev->name = ACPI_PCC_DRIVER_NAME;
    input_dev->phys = ACPI_PCC_INPUT_PHYS;
    input_dev->id.bustype = BUS_HOST;
    input_dev->id.vendor = 0x0001;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0100;

    error = sparse_keymap_setup(input_dev, panasonic_keymap, NULL);
    if (error) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Unable to setup input device keymap\n"));
        goto err_free_dev;
    }

    error = input_register_device(input_dev);
    if (error) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Unable to register input device\n"));
        goto err_free_keymap;
    }

    pcc->input_dev = input_dev;
    return 0;

 err_free_keymap:
    sparse_keymap_free(input_dev);
 err_free_dev:
    input_free_device(input_dev);
    return error;
}

static void acpi_pcc_destroy_input(struct pcc_acpi *pcc)
{
    sparse_keymap_free(pcc->input_dev);
    input_unregister_device(pcc->input_dev);
    /*
     * No need to input_free_device() since core input API refcounts
     * and free()s the device.
     */
}

/* kernel module interface */

static int acpi_pcc_hbtn_add(struct acpi_device *device)
{
    struct pcc_acpi *pcc;
    int result;

    if (!device)
        return -EINVAL;

    pcc = kzalloc(sizeof(struct pcc_acpi), GFP_KERNEL);
    if (!pcc) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Couldn't allocate mem for pcc"));
        return -ENOMEM;
    }

    pcc->device = device;
    pcc->handle = device->handle;
    device->driver_data = pcc;
    strcpy(acpi_device_name(device), ACPI_PCC_DEVICE_NAME);
    strcpy(acpi_device_class(device), ACPI_PCC_CLASS);

    result = acpi_pcc_init_input(pcc);
    if (result) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Error installing keyinput handler\n"));
        goto out_hbtn;
    }

    return 0;

out_hbtn:
    kfree(pcc);

    return result;
}

static int __init acpi_pcc_init(void)
{
    int result = 0;

    if (acpi_disabled)
        return -ENODEV;

    result = acpi_bus_register_driver(&acpi_pcc_driver);
    if (result < 0) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                  "Error registering hbtn driver\n"));
        return -ENODEV;
    }

    return 0;
}

static int acpi_pcc_hbtn_remove(struct acpi_device *device)
{
    struct pcc_acpi *pcc = acpi_driver_data(device);

    if (!device || !pcc)
        return -EINVAL;

    acpi_pcc_destroy_input(pcc);

    kfree(pcc);

    return 0;
}

static void __exit acpi_pcc_exit(void)
{
    acpi_bus_unregister_driver(&acpi_pcc_driver);
}

module_init(acpi_pcc_init);
module_exit(acpi_pcc_exit);
