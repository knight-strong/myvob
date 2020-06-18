#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xde22caa5, "module_layout" },
	{ 0x35b6b772, "param_ops_charp" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xc5df851d, "device_destroy" },
	{ 0xe4e6714c, "platform_device_unregister" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xde8418e3, "cdev_del" },
	{ 0x8ee1026e, "class_destroy" },
	{ 0x7a07a759, "device_create" },
	{ 0x216af481, "__class_create" },
	{ 0xcd57e7b9, "sysfs_create_group" },
	{ 0x4255b734, "platform_device_register_full" },
	{ 0xf20064c9, "cdev_add" },
	{ 0xab7f24ec, "cdev_init" },
	{ 0x924260f3, "cdev_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x6069c16e, "script_get_pio_list" },
	{ 0xcbbb3698, "nonseekable_open" },
	{ 0x6c8d5ae8, "__gpio_get_value" },
	{ 0x353e3fa5, "__get_user_4" },
	{ 0xd9ce8f0c, "strnlen" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x7861dc0f, "dev_get_drvdata" },
	{ 0xfe990052, "gpio_free" },
	{ 0x29cd7eec, "sw_gpio_setall_range" },
	{ 0x27e1a049, "printk" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0x91715312, "sprintf" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "50222743E69349BCB1698F3");
