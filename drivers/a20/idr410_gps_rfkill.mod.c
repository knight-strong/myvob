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
	{ 0xf9a482f9, "msleep" },
	{ 0x311b7963, "_raw_spin_unlock" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x31b5d6b6, "malloc_sizes" },
	{ 0x4cabed9, "rfkill_register" },
	{ 0x212a3d14, "rfkill_alloc" },
	{ 0x27e1a049, "printk" },
	{ 0xe4e6714c, "platform_device_unregister" },
	{ 0x6d19d005, "platform_driver_register" },
	{ 0x279fb16d, "kmem_cache_alloc" },
	{ 0x354d78a7, "platform_device_register" },
	{ 0xaad6d92f, "rfkill_init_sw_state" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0xc2161e33, "_raw_spin_lock" },
	{ 0x37a0cba, "kfree" },
	{ 0xdb68bbad, "rfkill_destroy" },
	{ 0x9d669763, "memcpy" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xd208f526, "platform_driver_unregister" },
	{ 0x83eb21c, "rfkill_unregister" },
	{ 0x29cd7eec, "sw_gpio_setall_range" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "8FBE64DCC2C425427F474DA");
