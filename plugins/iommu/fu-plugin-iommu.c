/*
 * Copyright (C) 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <fwupdplugin.h>

struct FuPluginData {
	gboolean has_iommu;
};

static void
fu_plugin_iommu_init(FuPlugin *plugin)
{
	fu_plugin_alloc_data(plugin, sizeof(FuPluginData));
	fu_plugin_add_udev_subsystem(plugin, "iommu");
}

static void
fu_plugin_iommu_to_string(FuPlugin *plugin, guint idt, GString *str)
{
	FuPluginData *priv = fu_plugin_get_data(plugin);
	fu_string_append_kb(str, idt, "HasIommu", priv->has_iommu);
}

static gboolean
fu_plugin_iommu_backend_device_added(FuPlugin *plugin, FuDevice *device, GError **error)
{
	FuPluginData *priv = fu_plugin_get_data(plugin);

	/* interesting device? */
	if (!FU_IS_UDEV_DEVICE(device))
		return TRUE;
	if (g_strcmp0(fu_udev_device_get_subsystem(FU_UDEV_DEVICE(device)), "iommu") != 0)
		return TRUE;
	priv->has_iommu = TRUE;

	return TRUE;
}

static void
fu_plugin_iommu_add_security_attrs(FuPlugin *plugin, FuSecurityAttrs *attrs)
{
	FuPluginData *priv = fu_plugin_get_data(plugin);
	g_autoptr(FwupdSecurityAttr) attr = NULL;

	/* create attr */
	attr = fu_plugin_security_attr_new(plugin, FWUPD_SECURITY_ATTR_ID_IOMMU);
	fu_security_attrs_append(attrs, attr);

	if (priv == NULL) {
		fwupd_security_attr_add_flag(attr, FWUPD_SECURITY_ATTR_FLAG_MISSING_DATA);
		return;
	}

	fu_security_attr_add_bios_target_value(attr, "AmdVt", "enable");
	fu_security_attr_add_bios_target_value(attr, "IOMMU", "enable");
	fu_security_attr_add_bios_target_value(attr, "VtForDirectIo", "enable");

	if (!priv->has_iommu) {
		fwupd_security_attr_set_result(attr, FWUPD_SECURITY_ATTR_RESULT_NOT_FOUND);
		fwupd_security_attr_add_flag(attr, FWUPD_SECURITY_ATTR_FLAG_ACTION_CONTACT_OEM);
		fwupd_security_attr_add_flag(attr, FWUPD_SECURITY_ATTR_FLAG_ACTION_CONFIG_FW);
		return;
	}

	/* success */
	fwupd_security_attr_add_flag(attr, FWUPD_SECURITY_ATTR_FLAG_SUCCESS);
	fwupd_security_attr_set_result(attr, FWUPD_SECURITY_ATTR_RESULT_ENABLED);
}

void
fu_plugin_init_vfuncs(FuPluginVfuncs *vfuncs)
{
	vfuncs->build_hash = FU_BUILD_HASH;
	vfuncs->init = fu_plugin_iommu_init;
	vfuncs->to_string = fu_plugin_iommu_to_string;
	vfuncs->backend_device_added = fu_plugin_iommu_backend_device_added;
	vfuncs->add_security_attrs = fu_plugin_iommu_add_security_attrs;
}
