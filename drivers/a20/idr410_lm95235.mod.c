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
	{ 0xfe990052, "gpio_free" },
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
	{ 0xe5a1f964, "i2c_new_device" },
	{ 0xe1267af1, "i2c_get_adapter" },
	{ 0x279fb16d, "kmem_cache_alloc" },
	{ 0x8949858b, "schedule_work" },
	{ 0xf184d189, "kernel_power_off" },
	{ 0x91715312, "sprintf" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x57a36d41, "i2c_smbus_write_byte_data" },
	{ 0xab2efdb4, "i2c_smbus_read_byte_data" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x27e1a049, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6296515A3F4CE654F3B6A2D");
