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
	{ 0x77891f6c, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xeae3dfd6, __VMLINUX_SYMBOL_STR(__const_udelay) },
	{ 0x26948d96, __VMLINUX_SYMBOL_STR(copy_user_enhanced_fast_string) },
	{ 0xafb8c6ff, __VMLINUX_SYMBOL_STR(copy_user_generic_string) },
	{ 0x72a98fdb, __VMLINUX_SYMBOL_STR(copy_user_generic_unrolled) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x4c4fef19, __VMLINUX_SYMBOL_STR(kernel_stack) },
	{ 0x6aff7790, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x86232093, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x504120d8, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x8dcfdac, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x552d644d, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xdb7bc396, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x8b2718b, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0xb7397693, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x99269f5f, __VMLINUX_SYMBOL_STR(filp_open) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x79ecded2, __VMLINUX_SYMBOL_STR(filp_close) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1CD333251870E40CDDEF8E9");
