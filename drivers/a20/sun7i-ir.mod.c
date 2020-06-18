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
	{ 0x15692c87, "param_ops_int" },
	{ 0x2e1ca751, "clk_put" },
	{ 0x9133f6ab, "input_unregister_device" },
	{ 0xb227ae83, "unregister_early_suspend" },
	{ 0x31b5d6b6, "malloc_sizes" },
	{ 0x9aaf7385, "input_free_device" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x37a0cba, "kfree" },
	{ 0xb5eeb329, "register_early_suspend" },
	{ 0x3be29b6b, "input_register_device" },
	{ 0xb3844f21, "input_free_platform_resource" },
	{ 0x5808d99e, "input_init_platform_resource" },
	{ 0xc5893a54, "input_fetch_sysconfig_para" },
	{ 0xd6b8e852, "request_threaded_irq" },
	{ 0x676bbc0f, "_set_bit" },
	{ 0x132a7a5b, "init_timer_key" },
	{ 0x279fb16d, "kmem_cache_alloc" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0x49e85269, "input_allocate_device" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x7d11c268, "jiffies" },
	{ 0xd9605d4c, "add_timer" },
	{ 0xc8fd727e, "mod_timer" },
	{ 0x165e7f89, "clk_disable" },
	{ 0x71c2a71a, "input_event" },
	{ 0x82f11a24, "clk_enable" },
	{ 0x27e1a049, "printk" },
	{ 0x21c601c3, "clk_set_rate" },
	{ 0x336cabdf, "clk_get" },
	{ 0x29cd7eec, "sw_gpio_setall_range" },
	{ 0xcca27eeb, "del_timer" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "DDD2BB40B4A09607B7DE78E");
