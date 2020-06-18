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
	{ 0x659cffe2, "regulator_unregister" },
	{ 0xbf2f84ba, "regulator_register" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x97255bdf, "strlen" },
	{ 0x9d669763, "memcpy" },
	{ 0x31b5d6b6, "malloc_sizes" },
	{ 0x37a0cba, "kfree" },
	{ 0x27e1a049, "printk" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0x279fb16d, "kmem_cache_alloc" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x29cd7eec, "sw_gpio_setall_range" },
	{ 0x317857fd, "rdev_get_drvdata" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "293E1973DCBBECF3E9946E3");
