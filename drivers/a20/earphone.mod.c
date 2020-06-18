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
	{ 0x8ee1026e, "class_destroy" },
	{ 0xc5df851d, "device_destroy" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x69ef5e17, "sw_gpio_irq_free" },
	{ 0x7a07a759, "device_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x216af481, "__class_create" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0x31b5d6b6, "malloc_sizes" },
	{ 0x37a0cba, "kfree" },
	{ 0xd6b80f4f, "sw_gpio_irq_request" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x279fb16d, "kmem_cache_alloc" },
	{ 0xfe990052, "gpio_free" },
	{ 0x29cd7eec, "sw_gpio_setall_range" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x6c8d5ae8, "__gpio_get_value" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x91715312, "sprintf" },
	{ 0x7861dc0f, "dev_get_drvdata" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x27e1a049, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C22DF427D0D03B93F3EE4C2");
